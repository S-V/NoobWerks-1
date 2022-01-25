// Skeletal animation system
#include <Base/Base.h>
#pragma hdrstop

// ozz::memory::SetDefaulAllocator()
#include <ozz/base/memory/allocator.h>

#include <Base/Template/TRange.h>

#include <Core/CoreDebugUtil.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>
#include <Rendering/Private/Modules/Animation/_AnimationSystem.h>


namespace Rendering
{

namespace
{
void _set_Custom_Allocator_for_Ozz_Animation_Library()
{
	class MyOzzAllocator: public ozz::memory::Allocator
	{
		mxFORCEINLINE AllocatorI& getAllocator() { return MemoryHeaps::animation(); }

	public:
		// Allocates _size bytes on the specified _alignment boundaries.
		// Allocate function conforms with standard malloc function specifications.
		virtual void* Allocate(size_t _size, size_t _alignment) override
		{
			return getAllocator().Allocate( _size, _alignment );
		}

		// Frees a block that was allocated with allocate or Reallocate.
		// Argument _block can be NULL.
		// Deallocate function conforms with standard free function specifications.
		virtual void Deallocate(void* _block) override
		{
			return getAllocator().Deallocate( _block );
		}

		// Changes the size of a block that was allocated with allocate.
		// Argument _block can be NULL.
		// Reallocate function conforms with standard realloc function specifications.
		virtual void* Reallocate(void* _block, size_t _size, size_t _alignment) override
		{
			return getAllocator().reallocate( _block, _size, _alignment );
		}
	};

	static MyOzzAllocator	s_myOzzAllocator;

	ozz::memory::SetDefaulAllocator( &s_myOzzAllocator );
}
}//namespace


NwAnimationSystem::NwAnimationSystem()
{
}

NwAnimationSystem::~NwAnimationSystem()
{
	mxASSERT(sampling_cache_pool.NumValidItems() == 0);
}

ERet NwAnimationSystem::SetUp()
{
	sampling_cache_pool.Initialize(
		sizeof(NwAnimSamplingCache)
		, NwAnimSamplingCache::GRANULARITY
		, &MemoryHeaps::animation()
		);

	_set_Custom_Allocator_for_Ozz_Animation_Library();

	return ALL_OK;
}

void NwAnimationSystem::ShutDown()
{
	mxASSERT(sampling_cache_pool.NumValidItems() == 0);
}

NwAnimSamplingCache* NwAnimationSystem::AllocateSamplingCache(
	int num_joints_in_skeleton
	)
{
	NwAnimSamplingCache * new_sampling_cache = sampling_cache_pool.New<NwAnimSamplingCache>();
	chkRET_NIL_IF_NIL(new_sampling_cache);
	new_sampling_cache->ozz_sampling_cache.Resize(nwRENDERER_MAX_BONES);
	return new_sampling_cache;
}

void NwAnimationSystem::ReleaseSamplingCache(
	NwAnimSamplingCache* sampling_cache
	)
{
	sampling_cache_pool.Delete( sampling_cache );
}

}//namespace Rendering
