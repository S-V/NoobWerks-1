#pragma once

#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
//
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
//
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
//
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Scene/MeshInstance.h>

//
#include <Developer/ThirdPartyGames/Doom3/Doom3_MD5_Model_Parser.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_md5_Anim_Parser.h>
//



ERet convert_Doom3_md5_to_ozz_raw_skeleton(
	const Doom3::md5Model& md5_model
	, ozz::animation::offline::RawSkeleton &raw_skeleton_
	);

ERet convert_md5_model_to_ozz_skeleton(
	const Doom3::md5Model& md5_model
	, ozz::animation::Skeleton &skeleton_
	);

ERet convert_animation_from_md5_to_raw_ozz(
	const Doom3::md5Animation& md5_animation
	, ozz::animation::offline::RawAnimation &raw_animation_
	, const ozz::animation::Skeleton& ozz_skeleton
	);

ERet convert_animation_from_md5_to_ozz(
	ozz::animation::Animation &ozz_animation_
	, const Doom3::md5Animation& md5_animation_
	, const ozz::animation::Skeleton& ozz_skeleton
	);

///
struct TbSkinnedMeshData: CStruct
{
	/// part of model that is associated with a single material
	struct Submesh
	{
		AssetID						material;
		bool						can_skip_rendering;

		TArray< Vertex_Skinned >	vertices;
		TArray< U16 >				indices;
	};

	DynamicArray< Submesh >	submeshes;

	TArray< M44f >	inverseBindPoseMatrices;	// inverse bind-pose matrix array

public:
	TbSkinnedMeshData( AllocatorI & allocator = MemoryHeaps::temporary() )
		: submeshes(allocator)
	{
	}
};

ERet buildSkinnedMeshData( const Doom3::md5Model& md5_model
						  , TbSkinnedMeshData &skinned_mesh_data_
						  , const ozz::animation::Skeleton& ozz_skeleton	// for determining joint indices
						  );

void dbg_drawSkinnedMeshData_inBindPose( const TbSkinnedMeshData& skinned_mesh_data
							 , TbPrimitiveBatcher & debugRenderer
							 , const RGBAf& color );

void dbg_drawSkinnedMeshData_Animated( const TbSkinnedMeshData& skinned_mesh_data
									  , const ozz::animation::Skeleton& skeleton
									  , ozz::Range<const ozz::math::Float4x4> bone_matrices	// object-space joint transforms
									  , TbPrimitiveBatcher & debugRenderer
									  , const M44f& world_transform
									  , const RGBAf& color );

