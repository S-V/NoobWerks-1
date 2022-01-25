#include <Base/Base.h>
#pragma hdrstop
#include <Base/Template/TRange.h>

#include <Core/CoreDebugUtil.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>
#include <Rendering/Private/Modules/Animation/_AnimationSystem.h>


#define DEBUG_ANIMS	(1)


//=====================================================================

namespace Rendering
{
mxDEFINE_CLASS(NwSkinnedMesh);
mxBEGIN_REFLECTION(NwSkinnedMesh)
	mxMEMBER_FIELD( inverse_bind_pose_matrices ),
	mxMEMBER_FIELD( anim_clips ),
	mxMEMBER_FIELD( bind_pose_aabb ),
	mxMEMBER_FIELD( view_offset ),
	mxMEMBER_FIELD( render_mesh ),
mxEND_REFLECTION;

NwSkinnedMesh::NwSkinnedMesh()
	: anim_clips( MemoryHeaps::renderer() )
{
	bind_pose_aabb.clear();
	view_offset = CV3f(0);
}

ERet NwSkinnedMesh::loadAssets()
{
	mxDO(render_mesh.Load());

	nwFOR_LOOP( UINT, i, anim_clips._count )
	{
		mxDO(anim_clips._values[i].Load());
	}

	return ALL_OK;
}

const NwAnimClip* NwSkinnedMesh::findAnimByNameHash( const NameHash32 name_hash ) const
{
	mxOPTIMIZE("FindRef");
	if( const TResPtr< NwAnimClip >* anim_clip = this->anim_clips.find( name_hash ) )
	{
		return *anim_clip;
	}

	return nil;
}

const NwAnimClip* NwSkinnedMesh::getAnimByNameHash( const NameHash32 name_hash ) const
{
	const NwAnimClip* anim = this->findAnimByNameHash( name_hash );
	mxASSERT_PTR(anim);
	return anim;
}

ERet NwSkinnedMesh::GetAnimByNameHash(
	const NwAnimClip *&anim_clip_
	, const NameHash32 name_hash
	) const
{
	anim_clip_ = this->findAnimByNameHash( name_hash );
	return anim_clip_ ? ALL_OK : ERR_OBJECT_NOT_FOUND;
}

#if MX_DEVELOPER

void NwSkinnedMesh::DbgPrintAnims() const
{
	mxASSERT(anim_clips.CheckKeysAreSorted());

	DBGOUT("%d anims:", anim_clips.Num());

	nwFOR_LOOP(UINT, i, anim_clips._count)
	{
		const TResPtr< NwAnimClip >& anim_clip = anim_clips._values[i];

		DBGOUT("Anim[%d]: '%s' (0x%X)",
			i,
			anim_clip._id.c_str(),
			anim_clip._ptr
			);
	}
}

#endif

//=====================================================================

///
NwSkinnedMeshLoader::NwSkinnedMeshLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( NwSkinnedMesh::metaClass(), parent_allocator )
{
}

ERet NwSkinnedMeshLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	NwSkinnedMesh *	new_instance;
	mxDO(Serialization::LoadMemoryImage(
		new_instance
		, stream
		, context.raw_allocator
		));

	// avoid the crash when ozz::Skeleton tries to deallocate previous data
	new (&new_instance->ozz_skeleton) ozz::animation::Skeleton();

	//
	ozzAssetReader			reader_stream( stream );
	ozz::io::IArchive		input_archive( &reader_stream );
	new_instance->ozz_skeleton.Load( input_archive, OZZ_SKELETON_VERSION );

	mxENSURE( new_instance->ozz_skeleton.num_joints() && new_instance->ozz_skeleton.num_soa_joints()
		, ERR_FAILED_TO_PARSE_DATA
		, "failed to create skeleton - did you pass correct version?"
		);

	//
	*new_instance_ = new_instance;

	return ALL_OK;
}

ERet NwSkinnedMeshLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	NwSkinnedMesh *skinned_model_ = static_cast< NwSkinnedMesh* >( instance );
	return skinned_model_->loadAssets();
}

void NwSkinnedMeshLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	UNDONE;
}

ERet NwSkinnedMeshLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

}//namespace Rendering
