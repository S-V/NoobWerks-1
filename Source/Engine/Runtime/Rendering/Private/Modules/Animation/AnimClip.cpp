#include <Base/Base.h>
#pragma hdrstop

#include <Core/Client.h>	// NwTime
#include <Core/Serialization/Serialization.h>
#include <Rendering/Private/Modules/Animation/AnimClip.h>


namespace Rendering
{
mxDEFINE_CLASS(NwAnimClip);
mxBEGIN_REFLECTION(NwAnimClip)
	mxMEMBER_FIELD(events),
	mxMEMBER_FIELD(root_joint_pos_in_last_frame),
	mxMEMBER_FIELD(root_joint_average_speed),
	//mxMEMBER_FIELD(frame_count),
	mxMEMBER_FIELD(id),
mxEND_REFLECTION;
NwAnimClip::NwAnimClip()
{
	root_joint_pos_in_last_frame = CV3f(0);
	root_joint_average_speed = CV3f(0);
	//frame_count = 0;
}

//U32 NwAnimClip::timeRatioToFrameIndex( float time_ratio ) const
//{
//	return (U32) mmFloorToInt( time_ratio * frame_count );
//}



NwAnimClipLoader::NwAnimClipLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( NwAnimClip::metaClass(), parent_allocator )
{
}

ERet NwAnimClipLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	//
	AssetReader	reader_stream;
	mxDO(Resources::OpenFile( context.key, &reader_stream ));

	//
	NwAnimClip *	new_anim_clip;
	mxDO(Serialization::LoadMemoryImage(
		new_anim_clip
		, reader_stream
		, context.raw_allocator
		));

	//
	new(&new_anim_clip->ozz_animation) ozz::animation::Animation();

	//
	mxDO(Reader_Align_Forward(reader_stream, OZZ_DATA_ALIGNMENT));

	//
	ozzAssetReader			ozz_reader_stream( reader_stream );
	ozz::io::IArchive		input_archive( &ozz_reader_stream );
	new_anim_clip->ozz_animation.Load( input_archive, OZZ_ANIMATION_VERSION );

	mxENSURE( new_anim_clip->ozz_animation.num_tracks() > 0
		, ERR_FAILED_TO_PARSE_DATA
		, "failed to parse animation - did you pass correct version?"
		);

	*new_instance_ = new_anim_clip;

	return ALL_OK;
}

ERet NwAnimClipLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	//NwAnimClip *animation_ = static_cast< NwAnimClip* >( instance );
	return ALL_OK;
}

void NwAnimClipLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	UNDONE;
}

ERet NwAnimClipLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

}//namespace Rendering
