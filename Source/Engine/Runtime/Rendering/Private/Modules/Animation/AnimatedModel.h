#pragma once

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
#include <Rendering/Private/Modules/idTech4/idRenderModel.h>
#include <Rendering/Private/Modules/Animation/AnimPlayback.h>


class EnemyNPC;

namespace Rendering
{
struct idEntityDef;
mxALIGN_BY_CACHELINE struct NwAnimationJobResult
{
	/// current offset of the root joint, in model space;
	/// used for root motion
//	V3f		root_joint_offset;
//	U32		padding;

	V3f		blended_root_joint_pos_in_model_space;

	/// valid only for weapon models
	M44f	barrel_joint_mat_local;
};

///
struct NwAnimatedModel: CStruct, NonCopyable
{
	///
	NwAnimPlayer	anim_player;

	bool			is_paused;

	///
	TResPtr< NwSkinnedMesh >	_skinned_mesh;	// shared

	// for playing anim events
	NwAnimEventList::AnimEventCallback *	anim_events_callback;
	void *									anim_events_callback_data;

	// maps sound id to sound source (optional)
	// TODO: REFACTOR, should NOT be here!
	TPtr< idEntityDef >			entity_def;
	TPtr< EnemyNPC >			npc;


	/// for rendering
	TPtr< idRenderModel >		render_model;	// owned / 1:1

	//
	int		barrel_joint_index;

	// written by jobs!
	NwAnimationJobResult	job_result;

public:
	mxDECLARE_CLASS(NwAnimatedModel, CStruct);
	mxDECLARE_REFLECTION;
	NwAnimatedModel();

	ERet PlayAnim(
		const NameHash32 anim_name_hash
		, const NwPlayAnimParams& params = NwPlayAnimParams()
		, const PlayAnimFlags::Raw play_anim_flags = PlayAnimFlags::Defaults
		, const NwStartAnimParams& start_params = NwStartAnimParams()
		);

	ERet PlayAnim_Additive(
		const NameHash32 anim_name_hash
		, const NwPlayAnimParams& params = NwPlayAnimParams()
		);

	bool IsPlayingAnim(const NameHash32 anim_name_hash) const;

	void RemoveAnimationsWithID(const NameHash32 anim_name_hash);

	///
	bool IsAnimDone(
		const NameHash32 anim_name_hash,
		const SecondsF remaining_time_seconds
		) const;

public:
};


namespace NwAnimatedModel_
{
	ERet Create(
		NwAnimatedModel *& new_anim_model_
		, NwClump & clump
		, const TResPtr<NwSkinnedMesh>& skinned_mesh
		, NwAnimEventList::AnimEventCallback* anim_events_callback
		, idEntityDef* id_entity_def = nil
		);

	void Destroy(
		NwAnimatedModel *& anim_model
		, NwClump & clump
		);

	//
	void AddToWorld(
		NwAnimatedModel* anim_model
		, SpatialDatabaseI* spatial_database
		);
	void RemoveFromWorld(
		NwAnimatedModel* anim_model
		, SpatialDatabaseI* spatial_database
		);

	//
	JobID UpdateInstances(
		NwClump & clump
		, const NwTime& args
		, NwJobSchedulerI& task_scheduler
		);
}//namespace

}//namespace Rendering
