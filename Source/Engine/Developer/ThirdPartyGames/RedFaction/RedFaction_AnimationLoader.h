//
// Loader for Volition's motion/Red Faction Animation files (*.rfa)
// used in Red Faction 1.
// 
// Based on work done by Rafal Harabien:
// https://github.com/rafalh/rf-reversed
//
#pragma once

#include <Base/Math/Quaternion.h>


class TestSkinnedModel;


namespace RF1
{

struct RotKey
{
	Q4f	q;
	U32	time;
};

struct PosKey
{
	V3f	t;
	U32	time;
	V3f	prev_interp;
	V3f	next_interp;
};

struct BoneTrack
{
	TArray< RotKey >	rot_keys;
	TArray< PosKey >	pos_keys;
};

struct Anim: NonCopyable
{
	DynamicArray< BoneTrack >	bone_tracks;

	U32	start_time;
	U32	end_time;

public:
	Anim( AllocatorI & allocator )
		: bone_tracks( allocator )
		, start_time( 0 )
		, end_time( 0 )
	{
	}

	float durationInSeconds() const;
};

ERet load_RFA_from_file(
						//TcModel *model_
						const char* filename
						, Anim &anim_
						, const TestSkinnedModel& test_model
						//, ObjectAllocatorI * storage
						);
}//namespace RF1
