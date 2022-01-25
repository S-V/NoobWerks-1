//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2019 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/runtime/sampling_job.h"

#include <cassert>

#include "ozz/animation/runtime/animation.h"
#include "ozz/base/maths/math_constant.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/allocator.h"

// Internal include file
#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "animation/runtime/animation_keyframe.h"

namespace ozz {
namespace animation {

namespace internal {
struct InterpSoaTranslation {
  math::SimdFloat4 ratio[2];
  math::SoaFloat3 value[2];
};
struct InterpSoaRotation {
  math::SimdFloat4 ratio[2];
  math::SoaQuaternion value[2];
};
struct InterpSoaScale {
  math::SimdFloat4 ratio[2];
  math::SoaFloat3 value[2];
};
}  // namespace internal

bool SamplingJob::Validate() const {
  // Don't need any early out, as jobs are valid in most of the performance
  // critical cases.
  // Tests are written in multiple lines in order to avoid branches.
  bool valid = true;

  // Test for NULL pointers.
  if (!animation || !cache) {
    return false;
  }
  valid &= output.begin != NULL;

  // Tests output range, implicitly tests output.end != NULL.
  const ptrdiff_t num_soa_tracks = animation->num_soa_tracks();
  valid &= output.end - output.begin >= num_soa_tracks;

  // Tests cache size.
  valid &= cache->max_soa_tracks() >= num_soa_tracks;

  return valid;
}

namespace {
// Loops through the sorted key frames and update cache structure.
template <typename _Key>
void UpdateKeys(float _ratio, int _num_soa_tracks, ozz::Range<const _Key> _keys,
                int* _cursor, int* _cache, unsigned char* _outdated) {
  assert(_num_soa_tracks >= 1);
  const int num_tracks = _num_soa_tracks * 4;
  assert(_keys.begin + num_tracks * 2 <= _keys.end);

  const _Key* cursor = &_keys.begin[*_cursor];
  if (!*_cursor) {
    // Initializes interpolated entries with the first 2 sets of key frames.
    // The sorting algorithm ensures that the first 2 key frames of a track
    // are consecutive.
    for (int i = 0; i < _num_soa_tracks; ++i) {
      const int in_index0 = i * 4;                   // * soa size
      const int in_index1 = in_index0 + num_tracks;  // 2nd row.
      const int out_index = i * 4 * 2;
      _cache[out_index + 0] = in_index0 + 0;
      _cache[out_index + 1] = in_index1 + 0;
      _cache[out_index + 2] = in_index0 + 1;
      _cache[out_index + 3] = in_index1 + 1;
      _cache[out_index + 4] = in_index0 + 2;
      _cache[out_index + 5] = in_index1 + 2;
      _cache[out_index + 6] = in_index0 + 3;
      _cache[out_index + 7] = in_index1 + 3;
    }
    cursor = _keys.begin + num_tracks * 2;  // New cursor position.

    // All entries are outdated. It cares to only flag valid soa entries as
    // this is the exit condition of other algorithms.
    const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
    for (int i = 0; i < num_outdated_flags - 1; ++i) {
      _outdated[i] = 0xff;
    }
    _outdated[num_outdated_flags - 1] =
        0xff >> (num_outdated_flags * 8 - _num_soa_tracks);
  } else {
    assert(cursor >= _keys.begin + num_tracks * 2 && cursor <= _keys.end);
  }

  // Search for the keys that matches _ratio.
  // Iterates while the cache is not updated with left and right keys required
  // for interpolation at time ratio _ratio, for all tracks. Thanks to the
  // keyframe sorting, the loop can end as soon as it finds a key greater that
  // _ratio. It will mean that all the keys lower than _ratio have been
  // processed, meaning all cache entries are up to date.
  while (cursor < _keys.end &&
         _keys.begin[_cache[cursor->track * 2 + 1]].ratio <= _ratio) {
    // Flag this soa entry as outdated.
    _outdated[cursor->track / 32] |= (1 << ((cursor->track & 0x1f) / 4));
    // Updates cache.
    const int base = cursor->track * 2;
    _cache[base] = _cache[base + 1];
    _cache[base + 1] = static_cast<int>(cursor - _keys.begin);
    // Process next key.
    ++cursor;
  }
  assert(cursor <= _keys.end);

  // Updates cursor output.
  *_cursor = static_cast<int>(cursor - _keys.begin);
}

void UpdateSoaTranslations(int _num_soa_tracks,
                           ozz::Range<const TranslationKey> _keys,
                           const int* _interp, uint8_t* _outdated,
                           internal::InterpSoaTranslation* soa_translations_) {
  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    uint8_t outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }
      const int base = i * 4 * 2;  // * soa size * 2 keys

      // Decompress left side keyframes and store them in soa structures.
      const TranslationKey& k00 = _keys.begin[_interp[base + 0]];
      const TranslationKey& k10 = _keys.begin[_interp[base + 2]];
      const TranslationKey& k20 = _keys.begin[_interp[base + 4]];
      const TranslationKey& k30 = _keys.begin[_interp[base + 6]];
      soa_translations_[i].ratio[0] =
          math::simd_float4::Load(k00.ratio, k10.ratio, k20.ratio, k30.ratio);
      soa_translations_[i].value[0].x = math::HalfToFloat(math::simd_int4::Load(
          k00.value[0], k10.value[0], k20.value[0], k30.value[0]));
      soa_translations_[i].value[0].y = math::HalfToFloat(math::simd_int4::Load(
          k00.value[1], k10.value[1], k20.value[1], k30.value[1]));
      soa_translations_[i].value[0].z = math::HalfToFloat(math::simd_int4::Load(
          k00.value[2], k10.value[2], k20.value[2], k30.value[2]));

      // Decompress right side keyframes and store them in soa structures.
      const TranslationKey& k01 = _keys.begin[_interp[base + 1]];
      const TranslationKey& k11 = _keys.begin[_interp[base + 3]];
      const TranslationKey& k21 = _keys.begin[_interp[base + 5]];
      const TranslationKey& k31 = _keys.begin[_interp[base + 7]];
      soa_translations_[i].ratio[1] =
          math::simd_float4::Load(k01.ratio, k11.ratio, k21.ratio, k31.ratio);
      soa_translations_[i].value[1].x = math::HalfToFloat(math::simd_int4::Load(
          k01.value[0], k11.value[0], k21.value[0], k31.value[0]));
      soa_translations_[i].value[1].y = math::HalfToFloat(math::simd_int4::Load(
          k01.value[1], k11.value[1], k21.value[1], k31.value[1]));
      soa_translations_[i].value[1].z = math::HalfToFloat(math::simd_int4::Load(
          k01.value[2], k11.value[2], k21.value[2], k31.value[2]));
    }
  }
}

#define DECOMPRESS_SOA_QUAT(_k0, _k1, _k2, _k3, _quat)                         \
  do {                                                                         \
    /* Selects proper mapping for each key.*/                                  \
    const int* m0 = kCpntMapping[_k0.largest];                                 \
    const int* m1 = kCpntMapping[_k1.largest];                                 \
    const int* m2 = kCpntMapping[_k2.largest];                                 \
    const int* m3 = kCpntMapping[_k3.largest];                                 \
                                                                               \
    /* Prepares an array of input values, according to the mapping required */ \
    /* to restore quaternion largest component.*/                              \
    OZZ_ALIGN(16)                                                              \
    int cmp_keys[4][4] = {                                                     \
        {_k0.value[m0[0]], _k1.value[m1[0]], _k2.value[m2[0]],                 \
         _k3.value[m3[0]]},                                                    \
        {_k0.value[m0[1]], _k1.value[m1[1]], _k2.value[m2[1]],                 \
         _k3.value[m3[1]]},                                                    \
        {_k0.value[m0[2]], _k1.value[m1[2]], _k2.value[m2[2]],                 \
         _k3.value[m3[2]]},                                                    \
        {_k0.value[m0[3]], _k1.value[m1[3]], _k2.value[m2[3]],                 \
         _k3.value[m3[3]]},                                                    \
    };                                                                         \
                                                                               \
    /* Resets largest component to 0.*/                                        \
    cmp_keys[_k0.largest][0] = 0;                                              \
    cmp_keys[_k1.largest][1] = 0;                                              \
    cmp_keys[_k2.largest][2] = 0;                                              \
    cmp_keys[_k3.largest][3] = 0;                                              \
                                                                               \
    /* Rebuilds quaternion from quantized values.*/                            \
    math::SimdFloat4 cpnt[4] = {                                               \
        kInt2Float *                                                           \
            math::simd_float4::FromInt(math::simd_int4::LoadPtr(cmp_keys[0])), \
        kInt2Float *                                                           \
            math::simd_float4::FromInt(math::simd_int4::LoadPtr(cmp_keys[1])), \
        kInt2Float *                                                           \
            math::simd_float4::FromInt(math::simd_int4::LoadPtr(cmp_keys[2])), \
        kInt2Float *                                                           \
            math::simd_float4::FromInt(math::simd_int4::LoadPtr(cmp_keys[3])), \
    };                                                                         \
                                                                               \
    /* Get back length of 4th component. Favors performance over accuracy by*/ \
    /* using x * RSqrtEst(x) instead of Sqrt(x).*/                             \
    /* ww0 cannot be 0 because we 're recomputing the largest component.*/     \
    const math::SimdFloat4 dot = cpnt[0] * cpnt[0] + cpnt[1] * cpnt[1] +       \
                                 cpnt[2] * cpnt[2] + cpnt[3] * cpnt[3];        \
    const math::SimdFloat4 ww0 = math::Max(eps, one - dot);                    \
    const math::SimdFloat4 w0 = ww0 * math::RSqrtEst(ww0);                     \
    /* Re-applies 4th component' s sign.*/                                     \
    const math::SimdInt4 sign = math::ShiftL(                                  \
        math::simd_int4::Load(_k0.sign, _k1.sign, _k2.sign, _k3.sign), 31);    \
    const math::SimdFloat4 restored = math::Or(w0, sign);                      \
                                                                               \
    /* Re-injects the largest component inside the SoA structure.*/            \
    cpnt[_k0.largest] =                                                        \
        math::Or(cpnt[_k0.largest], math::And(restored, mf000));               \
    cpnt[_k1.largest] =                                                        \
        math::Or(cpnt[_k1.largest], math::And(restored, m0f00));               \
    cpnt[_k2.largest] =                                                        \
        math::Or(cpnt[_k2.largest], math::And(restored, m00f0));               \
    cpnt[_k3.largest] =                                                        \
        math::Or(cpnt[_k3.largest], math::And(restored, m000f));               \
                                                                               \
    /* Stores result.*/                                                        \
    _quat.x = cpnt[0];                                                         \
    _quat.y = cpnt[1];                                                         \
    _quat.z = cpnt[2];                                                         \
    _quat.w = cpnt[3];                                                         \
  } while (void(0), 0)

void UpdateSoaRotations(int _num_soa_tracks,
                        ozz::Range<const RotationKey> _keys, const int* _interp,
                        uint8_t* _outdated,
                        internal::InterpSoaRotation* _soa_rotations) {
  // Prepares constants.
  const math::SimdFloat4 one = math::simd_float4::one();
  const math::SimdFloat4 eps = math::simd_float4::Load1(1e-16f);
  const math::SimdFloat4 kInt2Float =
      math::simd_float4::Load1(1.f / (32767.f * math::kSqrt2));
  const math::SimdInt4 mf000 = math::simd_int4::mask_f000();
  const math::SimdInt4 m0f00 = math::simd_int4::mask_0f00();
  const math::SimdInt4 m00f0 = math::simd_int4::mask_00f0();
  const math::SimdInt4 m000f = math::simd_int4::mask_000f();

  // Defines a mapping table that defines components assignation in the output
  // quaternion.
  const int kCpntMapping[4][4] = {
      {0, 0, 1, 2}, {0, 0, 1, 2}, {0, 1, 0, 2}, {0, 1, 2, 0}};

  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    uint8_t outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }

      const int base = i * 4 * 2;  // * soa size * 2 keys per track

      // Decompress left side keyframes and store them in soa structures.
      {
        const RotationKey& k0 = _keys.begin[_interp[base + 0]];
        const RotationKey& k1 = _keys.begin[_interp[base + 2]];
        const RotationKey& k2 = _keys.begin[_interp[base + 4]];
        const RotationKey& k3 = _keys.begin[_interp[base + 6]];

        _soa_rotations[i].ratio[0] =
            math::simd_float4::Load(k0.ratio, k1.ratio, k2.ratio, k3.ratio);
        math::SoaQuaternion& quat = _soa_rotations[i].value[0];
        DECOMPRESS_SOA_QUAT(k0, k1, k2, k3, quat);
      }

      // Decompress right side keyframes and store them in soa structures.
      {
        const RotationKey& k0 = _keys.begin[_interp[base + 1]];
        const RotationKey& k1 = _keys.begin[_interp[base + 3]];
        const RotationKey& k2 = _keys.begin[_interp[base + 5]];
        const RotationKey& k3 = _keys.begin[_interp[base + 7]];

        _soa_rotations[i].ratio[1] =
            math::simd_float4::Load(k0.ratio, k1.ratio, k2.ratio, k3.ratio);
        math::SoaQuaternion& quat = _soa_rotations[i].value[1];
        DECOMPRESS_SOA_QUAT(k0, k1, k2, k3, quat);
      }
    }
  }
}

#undef DECOMPRESS_SOA_QUAT

void UpdateSoaScales(int _num_soa_tracks, ozz::Range<const ScaleKey> _keys,
                     const int* _interp, uint8_t* _outdated,
                     internal::InterpSoaScale* soa_scales_) {
  const int num_outdated_flags = (_num_soa_tracks + 7) / 8;
  for (int j = 0; j < num_outdated_flags; ++j) {
    uint8_t outdated = _outdated[j];
    _outdated[j] = 0;  // Reset outdated entries as all will be processed.
    for (int i = j * 8; outdated; ++i, outdated >>= 1) {
      if (!(outdated & 1)) {
        continue;
      }
      const int base = i * 4 * 2;  // * soa size * 2 keys

      // Decompress left side keyframes and store them in soa structures.
      const ScaleKey& k00 = _keys.begin[_interp[base + 0]];
      const ScaleKey& k10 = _keys.begin[_interp[base + 2]];
      const ScaleKey& k20 = _keys.begin[_interp[base + 4]];
      const ScaleKey& k30 = _keys.begin[_interp[base + 6]];
      soa_scales_[i].ratio[0] =
          math::simd_float4::Load(k00.ratio, k10.ratio, k20.ratio, k30.ratio);
      soa_scales_[i].value[0].x = math::HalfToFloat(math::simd_int4::Load(
          k00.value[0], k10.value[0], k20.value[0], k30.value[0]));
      soa_scales_[i].value[0].y = math::HalfToFloat(math::simd_int4::Load(
          k00.value[1], k10.value[1], k20.value[1], k30.value[1]));
      soa_scales_[i].value[0].z = math::HalfToFloat(math::simd_int4::Load(
          k00.value[2], k10.value[2], k20.value[2], k30.value[2]));

      // Decompress right side keyframes and store them in soa structures.
      const ScaleKey& k01 = _keys.begin[_interp[base + 1]];
      const ScaleKey& k11 = _keys.begin[_interp[base + 3]];
      const ScaleKey& k21 = _keys.begin[_interp[base + 5]];
      const ScaleKey& k31 = _keys.begin[_interp[base + 7]];
      soa_scales_[i].ratio[1] =
          math::simd_float4::Load(k01.ratio, k11.ratio, k21.ratio, k31.ratio);
      soa_scales_[i].value[1].x = math::HalfToFloat(math::simd_int4::Load(
          k01.value[0], k11.value[0], k21.value[0], k31.value[0]));
      soa_scales_[i].value[1].y = math::HalfToFloat(math::simd_int4::Load(
          k01.value[1], k11.value[1], k21.value[1], k31.value[1]));
      soa_scales_[i].value[1].z = math::HalfToFloat(math::simd_int4::Load(
          k01.value[2], k11.value[2], k21.value[2], k31.value[2]));
    }
  }
}

void Interpolates(float _anim_ratio, int _num_soa_tracks,
                  const internal::InterpSoaTranslation* _translations,
                  const internal::InterpSoaRotation* _rotations,
                  const internal::InterpSoaScale* _scales,
                  math::SoaTransform* _output) {
  const math::SimdFloat4 anim_ratio = math::simd_float4::Load1(_anim_ratio);
  for (int i = 0; i < _num_soa_tracks; ++i) {
    // Prepares interpolation coefficients.
    const math::SimdFloat4 interp_t_ratio =
        (anim_ratio - _translations[i].ratio[0]) *
        math::RcpEst(_translations[i].ratio[1] - _translations[i].ratio[0]);
    const math::SimdFloat4 interp_r_ratio =
        (anim_ratio - _rotations[i].ratio[0]) *
        math::RcpEst(_rotations[i].ratio[1] - _rotations[i].ratio[0]);
    const math::SimdFloat4 interp_s_ratio =
        (anim_ratio - _scales[i].ratio[0]) *
        math::RcpEst(_scales[i].ratio[1] - _scales[i].ratio[0]);

    // Processes interpolations.
    // The lerp of the rotation uses the shortest path, because opposed
    // quaternions were negated during animation build stage (AnimationBuilder).
    _output[i].translation = Lerp(_translations[i].value[0],
                                  _translations[i].value[1], interp_t_ratio);
    _output[i].rotation = NLerpEst(_rotations[i].value[0],
                                   _rotations[i].value[1], interp_r_ratio);
    _output[i].scale =
        Lerp(_scales[i].value[0], _scales[i].value[1], interp_s_ratio);
  }
}
}  // namespace

SamplingJob::SamplingJob() : ratio(0.f), animation(NULL), cache(NULL) {}

bool SamplingJob::Run() const {
  if (!Validate()) {
    return false;
  }

  const int num_soa_tracks = animation->num_soa_tracks();
  if (num_soa_tracks == 0) {  // Early out if animation contains no joint.
    return true;
  }

  // Clamps ratio in range [0,duration].
  const float anim_ratio = math::Clamp(0.f, ratio, 1.f);

  // Step the cache to this potentially new animation and ratio.
  assert(cache->max_soa_tracks() >= num_soa_tracks);
  cache->Step(*animation, anim_ratio);

  // Fetch key frames from the animation to the cache a r = anim_ratio.
  // Then updates outdated soa hot values.
  UpdateKeys(anim_ratio, num_soa_tracks, animation->translations(),
             &cache->translation_cursor_, cache->translation_keys_,
             cache->outdated_translations_);
  UpdateSoaTranslations(num_soa_tracks, animation->translations(),
                        cache->translation_keys_, cache->outdated_translations_,
                        cache->soa_translations_);

  UpdateKeys(anim_ratio, num_soa_tracks, animation->rotations(),
             &cache->rotation_cursor_, cache->rotation_keys_,
             cache->outdated_rotations_);
  UpdateSoaRotations(num_soa_tracks, animation->rotations(),
                     cache->rotation_keys_, cache->outdated_rotations_,
                     cache->soa_rotations_);

  UpdateKeys(anim_ratio, num_soa_tracks, animation->scales(),
             &cache->scale_cursor_, cache->scale_keys_,
             cache->outdated_scales_);
  UpdateSoaScales(num_soa_tracks, animation->scales(), cache->scale_keys_,
                  cache->outdated_scales_, cache->soa_scales_);

  // Interpolates soa hot data.
  Interpolates(anim_ratio, num_soa_tracks, cache->soa_translations_,
               cache->soa_rotations_, cache->soa_scales_, output.begin);

  return true;
}

SamplingCache::SamplingCache()
    : max_soa_tracks_(0),
      soa_translations_(NULL) {  // soa_translations_ is the allocation pointer.
  Invalidate();
}

SamplingCache::SamplingCache(int _max_tracks)
    : max_soa_tracks_(0),
      soa_translations_(NULL) {  // soa_translations_ is the allocation pointer.
  Resize(_max_tracks);
}

SamplingCache::~SamplingCache() {
  // Deallocates everything at once.
  memory::default_allocator()->Deallocate(soa_translations_);
}

void SamplingCache::Resize(int _max_tracks) {
  using internal::InterpSoaRotation;
  using internal::InterpSoaScale;
  using internal::InterpSoaTranslation;

  // Reset existing data.
  Invalidate();
  memory::default_allocator()->Deallocate(soa_translations_);

  // Updates maximum supported soa tracks.
  max_soa_tracks_ = (_max_tracks + 3) / 4;

  // Allocate all cache data at once in a single allocation.
  // Alignment is guaranteed because memory is dispatch from the highest
  // alignment requirement (Soa data: SimdFloat4) to the lowest (outdated
  // flag: unsigned char).

  // Computes allocation size.
  const size_t max_tracks = max_soa_tracks_ * 4;
  const size_t num_outdated = (max_soa_tracks_ + 7) / 8;
  const size_t size =
      sizeof(InterpSoaTranslation) * max_soa_tracks_ +
      sizeof(InterpSoaRotation) * max_soa_tracks_ +
      sizeof(InterpSoaScale) * max_soa_tracks_ +
      sizeof(int) * max_tracks * 2 * 3 +  // 2 keys * (trans + rot + scale).
      sizeof(uint8_t) * 3 * num_outdated;

  // Allocates all at once.
  memory::Allocator* allocator = memory::default_allocator();
  char* alloc_begin = reinterpret_cast<char*>(
      allocator->Allocate(size, OZZ_ALIGN_OF(InterpSoaTranslation)));
  char* alloc_cursor = alloc_begin;

  // Distributes buffer memory while ensuring proper alignment (serves larger
  // alignment values first).
  OZZ_STATIC_ASSERT(
      OZZ_ALIGN_OF(InterpSoaTranslation) >= OZZ_ALIGN_OF(InterpSoaRotation) &&
      OZZ_ALIGN_OF(InterpSoaRotation) >= OZZ_ALIGN_OF(InterpSoaScale) &&
      OZZ_ALIGN_OF(InterpSoaScale) >= OZZ_ALIGN_OF(int) &&
      OZZ_ALIGN_OF(int) >= OZZ_ALIGN_OF(uint8_t));

  soa_translations_ = reinterpret_cast<InterpSoaTranslation*>(alloc_cursor);
  assert(
      math::IsAligned(soa_translations_, OZZ_ALIGN_OF(InterpSoaTranslation)));
  alloc_cursor += sizeof(InterpSoaTranslation) * max_soa_tracks_;
  soa_rotations_ = reinterpret_cast<InterpSoaRotation*>(alloc_cursor);
  assert(math::IsAligned(soa_rotations_, OZZ_ALIGN_OF(InterpSoaRotation)));
  alloc_cursor += sizeof(InterpSoaRotation) * max_soa_tracks_;
  soa_scales_ = reinterpret_cast<InterpSoaScale*>(alloc_cursor);
  assert(math::IsAligned(soa_scales_, OZZ_ALIGN_OF(InterpSoaScale)));
  alloc_cursor += sizeof(InterpSoaScale) * max_soa_tracks_;

  translation_keys_ = reinterpret_cast<int*>(alloc_cursor);
  assert(math::IsAligned(translation_keys_, OZZ_ALIGN_OF(int)));
  alloc_cursor += sizeof(int) * max_tracks * 2;
  rotation_keys_ = reinterpret_cast<int*>(alloc_cursor);
  alloc_cursor += sizeof(int) * max_tracks * 2;
  scale_keys_ = reinterpret_cast<int*>(alloc_cursor);
  alloc_cursor += sizeof(int) * max_tracks * 2;

  outdated_translations_ = reinterpret_cast<uint8_t*>(alloc_cursor);
  assert(math::IsAligned(outdated_translations_, OZZ_ALIGN_OF(uint8_t)));
  alloc_cursor += sizeof(uint8_t) * num_outdated;
  outdated_rotations_ = reinterpret_cast<uint8_t*>(alloc_cursor);
  alloc_cursor += sizeof(uint8_t) * num_outdated;
  outdated_scales_ = reinterpret_cast<uint8_t*>(alloc_cursor);
  alloc_cursor += sizeof(uint8_t) * num_outdated;

  assert(alloc_cursor == alloc_begin + size);
}

void SamplingCache::Step(const Animation& _animation, float _ratio) {
  // The cache is invalidated if animation has changed or if it is being rewind.
  if (animation_ != &_animation || _ratio < ratio_) {
    animation_ = &_animation;
    translation_cursor_ = 0;
    rotation_cursor_ = 0;
    scale_cursor_ = 0;
  }
  ratio_ = _ratio;
}

void SamplingCache::Invalidate() {
  animation_ = NULL;
  ratio_ = 0.f;
  translation_cursor_ = 0;
  rotation_cursor_ = 0;
  scale_cursor_ = 0;
}
}  // namespace animation
}  // namespace ozz
