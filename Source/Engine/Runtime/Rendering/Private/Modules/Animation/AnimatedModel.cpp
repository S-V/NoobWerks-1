#include <Base/Base.h>
#pragma hdrstop

#include <Core/Client.h>	// NwTime
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>

namespace Rendering
{
mxDEFINE_CLASS(NwAnimatedModel);
mxBEGIN_REFLECTION(NwAnimatedModel)
	mxMEMBER_FIELD( _skinned_mesh ),
mxEND_REFLECTION;
NwAnimatedModel::NwAnimatedModel()
{
	is_paused = false;

	barrel_joint_index = INDEX_NONE;
//	job_result.root_joint_offset = CV3f(0);
	job_result.blended_root_joint_pos_in_model_space = CV3f(0);
	job_result.barrel_joint_mat_local = g_MM_Identity;
//	TSetArray<const NwAnimClip*>(_known_animations, nil);
}

ERet NwAnimatedModel::PlayAnim(
							   const NameHash32 anim_name_hash
							   , const NwPlayAnimParams& params
							   , const PlayAnimFlags::Raw play_anim_flags
							   , const NwStartAnimParams& start_params
							   )
{
	const NwAnimClip* anim = _skinned_mesh->findAnimByNameHash( anim_name_hash );

	mxENSURE(mxIsValidHeapPointer(anim), ERR_OBJECT_NOT_FOUND, "");

	anim_player.PlayAnim(
		anim
		, params
		, play_anim_flags
		, start_params
		);

	return ALL_OK;
}

ERet NwAnimatedModel::PlayAnim_Additive(
							   const NameHash32 anim_name_hash
							   , const NwPlayAnimParams& params
							   )
{
	const NwAnimClip* anim = _skinned_mesh->findAnimByNameHash( anim_name_hash );

	mxENSURE(mxIsValidHeapPointer(anim), ERR_OBJECT_NOT_FOUND, "");
UNDONE;
	//anim_player.PlayAnim_Additive(
	//	anim
	//	, params.transition_duration
	//	, params.target_blend_weight
	//	, params.flags
	//	);

	return ALL_OK;
}

bool NwAnimatedModel::IsPlayingAnim(const NameHash32 anim_name_hash) const
{
	const NwAnimClip* anim = _skinned_mesh->findAnimByNameHash( anim_name_hash );
	mxASSERT_PTR(anim);
	return anim_player.IsPlayingAnim(anim);
}

void NwAnimatedModel::RemoveAnimationsWithID(const NameHash32 anim_name_hash)
{
	const NwAnimClip* anim_clip = _skinned_mesh->findAnimByNameHash( anim_name_hash );
	mxASSERT_PTR(anim_clip);

	anim_player.RemoveAnimationsPlayingClip(anim_clip);
}

bool NwAnimatedModel::IsAnimDone(
	const NameHash32 anim_name_hash,
	const SecondsF remaining_time_seconds
	) const
{
	const NwAnimClip* anim_clip = _skinned_mesh->findAnimByNameHash( anim_name_hash );
	if(anim_clip) {
		return anim_player.RemainingPlayTime(anim_clip) <= remaining_time_seconds;
	}
	return true;
}


namespace NwAnimatedModel_
{

ERet Create(
	NwAnimatedModel *& new_anim_model_
	, NwClump & clump
	, const TResPtr<NwSkinnedMesh>& skinned_mesh
	, NwAnimEventList::AnimEventCallback* anim_events_callback
	, idEntityDef* id_entity_def /*= nil*/
	)
{
	TResPtr<NwSkinnedMesh> skinned_model(skinned_mesh);
	mxDO(skinned_model.Load(&clump));

	//
	idRenderModel *	new_render_model;
	mxDO(idRenderModel_::CreateFromMesh(
		new_render_model
		, skinned_model->render_mesh
		, clump
		));

	//
	NwAnimatedModel *	new_anim_model;
	mxDO(clump.New(new_anim_model));


	mxOPTIMIZE("reduce mem allocs");

	//
	const ozz::animation::Skeleton&	ozz_skeleton = skinned_model->ozz_skeleton;

	const int num_joints = ozz_skeleton.num_joints();

	mxASSERT2(ozz_skeleton.num_soa_joints() <= nwRENDERER_MAX_BONES,
		"NwAnimSamplingCache is too small!");

	//
	mxDO(new_render_model->joint_matrices.setNum( num_joints ));
	Arrays::setAll( new_render_model->joint_matrices, g_MM_Identity );

	//
	new_anim_model->_skinned_mesh = skinned_model;

	new_anim_model->anim_events_callback = anim_events_callback;
	new_anim_model->anim_events_callback_data = new_anim_model;

	new_anim_model->entity_def = id_entity_def;

	new_anim_model->render_model = new_render_model;

	new_anim_model_ = new_anim_model;

	return ALL_OK;
}

void Destroy(
	NwAnimatedModel *& anim_model
	, NwClump & clump
	)
{
	mxASSERT2(anim_model->render_model->world_proxy_handle.IsNil(),
		"Did you forget to call RemoveFromWorld()?"
		);

	//
	clump.delete_(anim_model->render_model._ptr);

	clump.delete_(anim_model);

	anim_model = nil;
}

void AddToWorld(
	NwAnimatedModel* anim_model
	, SpatialDatabaseI* spatial_database
	)
{
	UNDONE;
	//SpatialDatabaseI_::Add_idRenderModel(
	//	spatial_database
	//	, anim_model->render_model
	//	);
}

void RemoveFromWorld(
	NwAnimatedModel* anim_model
	, SpatialDatabaseI* spatial_database
	)
{
	UNDONE;
	/*SpatialDatabaseI_::Remove_idRenderModel(
		spatial_database
		, anim_model->render_model
		);*/
}

/// Updates current animation time and skeleton pose.
static
bool RunAndBlendAnimations(
	NwAnimatedModel & anim_model
	, float delta_seconds
	, NwClump & clump
	, NwJobSchedulerI& task_scheduler
)
{
	// Update and sample all animations to their respective local space transform buffers.

	NwAnimPlayer& anim_player = anim_model.anim_player;

	//
	NwAnimPlayer::UpdateOutput anim_update_output;

	anim_player.UpdateOncePerFrame(
		delta_seconds
		, anim_update_output
		);

	if(mxLIKELY(anim_update_output.anims_count))
	{
		// Launch parallel jobs.
		NwJobData	anim_job_data;

		task_scheduler.AddJob(
			new(&anim_job_data) AnimJobs::ComputeJointMatricesJob(
			anim_model
			, anim_update_output
			)
			, JobPriority_High
			);

		// Emit anim events.
		{
			NwAnimEventList::AnimEventCallback* anim_events_callback = anim_model.anim_events_callback;
			void * anim_events_callback_data = anim_model.anim_events_callback_data;

			mxASSERT_PTR(anim_events_callback);
			mxASSERT_PTR(anim_events_callback_data);

			NwAnimEventList_::ProcessAnimEventsWithCallback(
				anim_update_output.playing_anims
				, anim_update_output.anims_count
				, anim_events_callback
				, anim_events_callback_data
				);
		}
	}

	return true;
}

static
void UpdateInstance(
					NwAnimatedModel & anim_model
					, const NwTime& args
					, NwClump & clump
					, NwJobSchedulerI& task_scheduler
					)
{
	RunAndBlendAnimations(
		anim_model
		, args.real.delta_seconds
		, clump
		, task_scheduler
		);
}

JobID UpdateInstances(
					  NwClump & clump
					  , const NwTime& args
					  , NwJobSchedulerI& task_scheduler
					  )
{
	const JobID anim_jobs_group = task_scheduler.beginGroup();

	NwClump::Iterator< NwAnimatedModel >	it( clump );
	while( it.IsValid() )
	{
		NwAnimatedModel & anim_model = it.Value();

		if(!anim_model.is_paused)
		{
			UpdateInstance(
				anim_model
				, args
				, clump
				, task_scheduler
				);
		}

		it.MoveToNext();
	}

	task_scheduler.endGroup();

	return anim_jobs_group;
}

}//namespace

}//namespace Rendering
