#include "D:/research/_RF1/rf-reversed/rfa_format.h"

#include <Base/Base.h>

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_AnimationLoader.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Experimental.h>


#define DBG_PRINT_ANIM_TRACK_DATA	(1)

namespace RF1
{

static char	gs_temp_buffer[mxKiB(4)];	// used for skipping - allows to inspect skipped data


/*

Header:
start time and end time are in milliseconds

*/


struct RFA_quaternion16
{
	// NOTE: signed 16-bit integers!
    I16	x, y, z, w;
};
ASSERT_SIZEOF(RFA_quaternion16, 8);

const float QUAT_DEQUANT_COEFF = 0.000061038882f;
//const float QUAT_DEQUANT_COEFF = 1.0f / 16383.0f;


struct RFA_RotationKey // size = 4+4*2+4=16
{
    uint32_t Time;

	// [0..1] floats quantized to unsigned 16-bit ints
	RFA_quaternion16 qRot;

    int8_t NextInterp; // some interpolation factors
    int8_t PrevInterp; // some interpolation factors
    int8_t unk3[2]; // always 0?
};
ASSERT_SIZEOF(RFA_RotationKey, 16);


struct RFA_PositionKey // size = 4+9*4=40
{
    uint32_t Time;
    V3f vPos; // position in t=Time
    V3f vPrevInterp; // used for interpolation before Time
    V3f vNextInterp; // used for interpolation after Time
};
ASSERT_SIZEOF(RFA_PositionKey, 40);





U32 CompressUnitFloatRL(F32 unitFloat, U32 nBits)
{
	// Determine the number of intervals based on the    
	// number of output bits we’ve been asked to produce.
	U32  nIntervals = 1u << nBits;
	// Scale the input value from the range [0, 1] into    
	// the range [0, nIntervals – 1]. We subtract one    
	//  interval because we want the largest output value  
	//  to fit into nBits bits.
	F32  scaled = unitFloat * (F32)(nIntervals - 1u);
	// Finally, round to the nearest interval center. We   
	// do this by adding 0.5f, and then truncating to the  
	// next-lowest interval index (by casting to U32).
	U32  rounded = (U32)(scaled * 0.5f);
	// Guard against invalid input values.
	if (rounded > nIntervals - 1u)
		rounded = nIntervals - 1u;
	return  rounded;
}
F32 DecompressUnitFloatRL(U32 quantized, U32 nBits)
{
	// Determine the number of intervals based on the    
	// number of bits we used when we encoded the value.
	U32  nIntervals = 1u << nBits;
	// Decode by simply converting the U32 to an F32, and
	// scaling by the interval size.
	F32  intervalSize = 1.0f / (F32)(nIntervals - 1u);
	F32  approxUnitFloat = (F32) quantized *  intervalSize;
	return  approxUnitFloat;
}

U32 CompressFloatRL(F32 value, F32 min , F32 max ,
					U32  nBits)
{
	F32 unitFloat = (value - min) / (max - min);
	U32 quantized = CompressUnitFloatRL(unitFloat,      
		nBits);
	return quantized;
}
F32 DecompressFloatRL(U32 quantized, F32 min , F32 max ,
					  U32 nBits)
{
	F32 unitFloat = DecompressUnitFloatRL(quantized,     
		nBits);
	F32 value = min + (unitFloat * (max - min));
	return value;
}



inline U16 CompressRotationChannel (F32 qx)
{
  return (U16)CompressFloatRL(qx, -1.0f, 1.0f, 16u);
}
inline F32 DecompressRotationChannel(U16 qx)
{
 return  DecompressFloatRL((U32)qx, -1.0f, 1.0f, 16u);
}




ERet load_RFA_from_file(
						//TcModel *model_
						const char* filename
						, Anim &anim_
						, const TestSkinnedModel& test_model
						//, ObjectAllocatorI * storage
						)
{
	FileReader	file;
	mxDO(file.Open(filename));

	//
	rfa_header_t	header;	// 72 bytes
	mxDO(file.Get(header));

	//
	const UINT num_bones = header.cBones;

	//
	DBGOUT("Parsing '%s': version=%d, delta=%f, epsilon=%f, bones: %u, start_time: %u, end_time: %u, RampInTime: %u, RampOutTime: %u",
		filename,
		(int)header.version,
		header.unk, header.unk2, num_bones, header.StartTime, header.EndTime, header.RampInTime, header.RampOutTime
		);

	LogStream(LL_Trace)
		,", Quat: ", RF1::toMyQuat( Q4f::fromRawFloatPtr(&header.unk3.x) )
		,", Pos: ", RF1::toMyVec3( CV3f(header.unk4.x, header.unk4.y, header.unk4.z) )
		;

	//mxASSERT(header.unk == 0.0f);
	//mxASSERT(header.unk2 == 0.0f);
	mxASSERT(header.cMorphVertices == 0);
	mxASSERT(header.cMorphKeyframes == 0);

	anim_.start_time = header.StartTime;
	anim_.end_time = header.EndTime;

	//
    // Offsets are from file begin
    uint32_t MorphVerticesOffset; // offset to rfa_morph_vertices_t
	mxDO(file.Get(MorphVerticesOffset));

    uint32_t MorphKeyframesOffset; // offset to rfa_morph_keyframes8_t or rfa_morph_keyframes7_t
	mxDO(file.Get(MorphKeyframesOffset));

	// 
	uint32_t BoneOffsets[ MAX_BONES ]; // offsets to rfa_bone_t
	mxENSURE(header.cBones <= mxCOUNT_OF(BoneOffsets), ERR_UNSUPPORTED_FEATURE, "");
	mxDO(file.Read( BoneOffsets, sizeof(BoneOffsets[0]) * num_bones ));

	////
	//DBGOUT("Bone offsets:");
	//for( UINT iBone = 0; iBone < num_bones; iBone++ )
	//{
	//	const uint32_t bone_offset = BoneOffsets[ iBone ];
	//	DEVOUT("Bone[%u]: offset = %u", iBone, bone_offset);
	//}//For each bone.

	//
	mxDO(anim_.bone_tracks.setNum( num_bones ));

	//
	for( UINT iBone = 0; iBone < num_bones; iBone++ )
	{
		//
		//const TestSkinnedModel::AnimatedJoint& joint = test_model._animated_joints[ iBone ];
		//const TestSkinnedModel::Bone& bone = test_model._bones[ joint.bone_index ];
		const TestSkinnedModel::Bone& bone = test_model._bones[ iBone ];

		// can be 10.0, 8.0
		float	f_unknown;
		mxDO(file.Get( f_unknown ));

		//
		uint16_t cRotKeys;
		mxDO(file.Get( cRotKeys ));

		//
		uint16_t cPosKeys;
		mxDO(file.Get( cPosKeys ));

		DEVOUT("\n! Bone[%u]: '%s', f = %f, rot keys = %u, pos keys = %u",
			iBone, bone.name.c_str(), f_unknown, cRotKeys, cPosKeys
			);

		//
		BoneTrack &dst_bone_track = anim_.bone_tracks._data[ iBone ];

		mxDO(dst_bone_track.rot_keys.setNum( cRotKeys ));
		mxDO(dst_bone_track.pos_keys.setNum( cPosKeys ));


		//
		for( UINT iRotKey = 0; iRotKey < cRotKeys; iRotKey++ )
		{
			RFA_RotationKey	rot_key;
			mxDO(file.Get( rot_key ));

			//
			const Q4f	raw_quat = {
				rot_key.qRot.x * QUAT_DEQUANT_COEFF,
				rot_key.qRot.y * QUAT_DEQUANT_COEFF,
				rot_key.qRot.z * QUAT_DEQUANT_COEFF,
				rot_key.qRot.w * QUAT_DEQUANT_COEFF,
			};

			////
			//mxASSERT2(
			//	raw_quat.w >= 0,
			//	"\t\t Raw Quaternion's real part must be >= 0, but got w: %f", raw_quat.w
			//	);

			//
			const float	raw_quat_len = raw_quat.Length();
			const bool is_normalized = mmAbs( raw_quat_len - 1.0f ) < 1e-3f;
			mxASSERT2(
				raw_quat.IsNormalized(1e-3f),
				"\t\t Raw Quaternion not normalized! Length: %f", raw_quat_len
				);

			const Q4f	quat =
				RF1::removeFlip( RF1::toMyQuat( raw_quat ) )
				.normalized()
				//.inverse()
				;

			//
#if DBG_PRINT_ANIM_TRACK_DATA

			//ptPRINT("\t rot_key[%u]: time = %u, Raw Q = ( %u, %u, %u, %u )",
			//	iRotKey,
			//	rot_key.Time,
			//	(UINT)rot_key.qRot.x, (UINT)rot_key.qRot.y, (UINT)rot_key.qRot.z, (UINT)rot_key.qRot.w
			//	);

			//ptPRINT("\t rot_key[%u]: time=%u, q=(%f, %f, %f, %f), next_interp=%u, prev_interp=%u, unkwn0=%u, unkwn1=%u",
			//	iRotKey,
			//	rot_key.Time,
			//	quat.x, quat.y, quat.z, quat.w,
			//	(UINT)rot_key.NextInterp, (UINT)rot_key.PrevInterp,
			//	rot_key.unk3[0], rot_key.unk3[1]
			//	);

			ptPRINT("\t rot_key[%u]: time=%u, q=(%f, %f, %f, %f)",
				iRotKey,
				rot_key.Time,
				quat.x, quat.y, quat.z, quat.w
				);

#endif // DBG_PRINT_ANIM_TRACK_DATA

			mxASSERT(rot_key.NextInterp == 0);
			mxASSERT(rot_key.PrevInterp == 0);
			mxASSERT(rot_key.unk3[0] == 0);
			mxASSERT(rot_key.unk3[1] == 0);

			//
			RotKey &dst_rot_key = dst_bone_track.rot_keys[ iRotKey ];
			dst_rot_key.q = quat;
			dst_rot_key.time = rot_key.Time;

			//if( raw_quat.w < 0 )
			//{
			//	dst_rot_key.q = quat.negated();
			//}

			//
#if DBG_PRINT_ANIM_TRACK_DATA

			//LogStream(LL_Trace),"\t\t Raw Quat: ", raw_quat;
			ptPRINT("\t\t Orientation: %s.", dst_rot_key.q.humanReadableString().c_str() );

#endif // DBG_PRINT_ANIM_TRACK_DATA

		}//For each rotation key frame.

		//
		ptPRINT("\t\t ---");

		//
		for( UINT iPosKey = 0; iPosKey < cPosKeys; iPosKey++ )
		{
			RFA_PositionKey	pos_key;
			mxDO(file.Get( pos_key ));

			//
			pos_key.vPos = RF1::toMyVec3( pos_key.vPos );
			pos_key.vPrevInterp = RF1::toMyVec3( pos_key.vPrevInterp );
			pos_key.vNextInterp = RF1::toMyVec3( pos_key.vNextInterp );

			//
#if DBG_PRINT_ANIM_TRACK_DATA
			ptPRINT("\t pos_key[%u]: time=%u, pos=(%.4f, %.4f, %.4f), next_interp=(%.4f, %.4f, %.4f), prev_interp=(%.4f, %.4f, %.4f)",
				iPosKey,
				pos_key.Time,
				pos_key.vPos.x, pos_key.vPos.y, pos_key.vPos.z,
				pos_key.vNextInterp.x, pos_key.vNextInterp.y, pos_key.vNextInterp.z,
				pos_key.vPrevInterp.x, pos_key.vPrevInterp.y, pos_key.vPrevInterp.z
				);
#endif // DBG_PRINT_ANIM_TRACK_DATA
			//
			PosKey &dst_pos_key = dst_bone_track.pos_keys[ iPosKey ];
			dst_pos_key.t = pos_key.vPos;
			dst_pos_key.time = pos_key.Time;
			dst_pos_key.prev_interp = pos_key.vPrevInterp;
			dst_pos_key.next_interp = pos_key.vNextInterp;
		}
	}//For each bone.

	return ALL_OK;
}

float Anim::durationInSeconds() const
{
	return MS2SEC( end_time );
}

}//namespace RF1
