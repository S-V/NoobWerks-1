// Skeletal animation system
#pragma once

//
#include <Base/Template/Containers/Dictionary/TSortedMap.h>
//
#include <Core/Assets/AssetReference.h>	// TResPtr<>
#include <Core/Tasking/TaskSchedulerInterface.h>
//
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Private/Modules/Animation/AnimEvents.h>
#include <Rendering/Private/Modules/Animation/PlaybackController.h>
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/Modules/Animation/AnimPlayback.h>
#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>

namespace Rendering
{
class NwAnimationSystem: public TGlobal< NwAnimationSystem >
{
	FreeListAllocator	sampling_cache_pool;

public:
	NwAnimationSystem();
	~NwAnimationSystem();

	ERet SetUp();
	void ShutDown();

	/// Allocates a cache that matches animation requirements.
	NwAnimSamplingCache* AllocateSamplingCache(
		int num_joints_in_skeleton
		);

	void ReleaseSamplingCache(
		NwAnimSamplingCache* sampling_cache
		);
};

}//namespace Rendering
