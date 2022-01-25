// Animation
#pragma once

//
#include <Base/Template/Containers/Dictionary/TSortedMap.h>
//
#include <Core/Assets/AssetReference.h>	// TResPtr<>
#include <Core/Tasking/TaskSchedulerInterface.h>
//
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Private/Modules/Animation/PlaybackController.h>
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/Modules/Animation/AnimClip.h>


namespace Rendering
{
/// immutable data, shared by all skinned model instances
mxPREALIGN(16) struct NwSkinnedMesh: NwSharedResource
{
	// Runtime skeleton.
	ozz::animation::Skeleton	ozz_skeleton;

	/// For rendering, we need to pre-multiply model-space bone matrices by inverse bind-pose matrices.
	TArray< M44f >	inverse_bind_pose_matrices;



	/// Anim set: holds all the animation data which belongs to one target object
	/// (for instance, all the animation data for a character)

	/// maps a name hash to the animation's index
	TSortedMap< NameHash32, TResPtr< NwAnimClip > >	anim_clips;


	/// local-space bounding box in bind pose
	AABBf		bind_pose_aabb;

	/// used by weapon models for first-person view (so that player hands don't go through the weapon mesh)
	V3f			view_offset;


	///
	TResPtr< NwMesh >	render_mesh;

public:
	mxDECLARE_CLASS( NwSkinnedMesh, NwResource );
	mxDECLARE_REFLECTION;
	NwSkinnedMesh();

	ERet loadAssets();

	const NwAnimClip* findAnimByNameHash( const NameHash32 name_hash ) const;

	mxDEPRECATED
	const NwAnimClip* getAnimByNameHash( const NameHash32 name_hash ) const;

	ERet GetAnimByNameHash(
		const NwAnimClip *&anim_clip_
		, const NameHash32 name_hash
		) const;

public:

	#if MX_DEVELOPER

	void DbgPrintAnims() const;

	#endif

} mxPOSTALIGN(16);


///
class NwSkinnedMeshLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	NwSkinnedMeshLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;
};

}//namespace Rendering
