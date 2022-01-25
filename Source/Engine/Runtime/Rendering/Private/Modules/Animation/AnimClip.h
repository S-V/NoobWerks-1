#pragma once

#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/Modules/Animation/AnimEvents.h>

namespace Rendering
{
/// An animation clip is a collection of related animation curves
/// under a common name (e.g. "walk", "run", "idle", etc.).
///
struct NwAnimClip: public NwSharedResource
{
	/// runtime animation
	ozz::animation::Animation	ozz_animation;

	/// Animation events
	NwAnimEventList				events;

	/// root motion support
	V3f							root_joint_pos_in_last_frame;
	V3f							root_joint_average_speed;

	// for debugging
	AssetID						id;

public:
	mxDECLARE_CLASS( NwAnimClip, CStruct );
	mxDECLARE_REFLECTION;
	NwAnimClip();

	const char* dbgName() const { return id.c_str(); }

	// time_ratio - [0..1]
	//U32 timeRatioToFrameIndex( float time_ratio ) const;
};

///
class NwAnimClipLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	NwAnimClipLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;
};

}//namespace Rendering
