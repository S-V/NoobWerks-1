#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/IO/StreamIO.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/private/shader_uniforms.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Physics/Bullet_Wrapper.h>

#include <Sound/SoundSystem.h>


#include <Utility/MeshLib/TriMesh.h>

#include <Utility/VoxelEngine/VoxelTerrainChunk.h>
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>
#include <Utility/ThirdPartyGames/Doom3/Doom3_Constants.h>

// OZZ
#include <ozz/animation/runtime/blending_job.h>


#include <Voxels/vx5_config.h>

#include "TowerDefenseGame.h"
#include "TowerDefenseGame-Misc.h"



GameStats	GameStats::shared;


mxBEGIN_FLAGS( AI_Viz_FlagsT )
	mxREFLECT_BIT( dbg_viz_rigid_bodies, AI_Viz_Flags::dbg_viz_rigid_bodies ),
mxEND_FLAGS

mxDEFINE_CLASS(AI_Settings);
mxBEGIN_REFLECTION(AI_Settings)
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION;

AI_Settings::AI_Settings()
{
	flags.raw_value = 0;
}


//=====================================================================


//=====================================================================

mxDEFINE_CLASS(NPC_Actor);
mxBEGIN_REFLECTION(NPC_Actor)
mxEND_REFLECTION;

NPC_Actor::NPC_Actor()
{
	_previous_position = CV3f(0);
	_smooth_delta_position = CV3f(0);
	//_average_velocity = CV3f(0);
	//_currentYaw = 0;
	//_targetYaw = 0;

	//_turningVelocity = DEG2RAD(10);	// 10 deg per second
}

/// updates the graphics transform used for rendering
static void updateGraphicsTransform( NPC_Actor& _zombieNPC )
{
	const ozz::math::Float4x4 rootJointTransform = _zombieNPC._animModel.animator.models.getFirst();
	const V3f rootJointTranslation = Vector4_As_V3( rootJointTransform.cols[3] );

	const M44f graphics_pretransform = _zombieNPC._animModel.orig_graphics_pretransform
			* M44_buildTranslationMatrix( -rootJointTranslation * DOOM3_TO_METERS );

	const M44f	rigid_body_local_to_world_matrix = M44f_from_btTransform( _zombieNPC._rigidBody.bt_rb().getCenterOfMassTransform() );

	const M44f	visual_representation_local_to_world_matrix = graphics_pretransform * rigid_body_local_to_world_matrix;

	_zombieNPC._animModel.render_model.transform->setLocalToWorldMatrix( visual_representation_local_to_world_matrix );
}

ERet NPC_Actor::load(
	NwSkinnedMesh* skinned_model
	, NwClump & storage
	, Physics::TbPhysicsWorld & physicsWorld
	, const V3f& initial_position
	)
{
	mxDO(_animModel.load(
		skinned_model
		, storage
		, NwMaterial::getFallbackMaterial()
		));

	//
	const V3f bindPoseLocalAabbSize = skinned_model->bind_pose_aabb.size();
	const V3f bindPoseLocalAabbHalfSize = bindPoseLocalAabbSize * 0.5f;

	RrTransform* graphicsTransform = _animModel.render_model.transform;
	graphicsTransform->setLocalToWorldMatrix(M44_Identity());


	// Create rigid body (physics).

	const float collision_cylinder_height_ws = bindPoseLocalAabbHalfSize.z * DOOM3_TO_METERS;
	const float collision_cylinder_radius_ws = Min3(
		bindPoseLocalAabbHalfSize.x * DOOM3_TO_METERS,
		bindPoseLocalAabbHalfSize.y * DOOM3_TO_METERS,
		bindPoseLocalAabbHalfSize.z * DOOM3_TO_METERS
		);

	//float height = NPC_HEIGHT * 0.5f;
	//float _radius = height * 0.7f;
	float _mass = (bindPoseLocalAabbSize * DOOM3_TO_METERS).boxVolume() * 80.0f;

	V3f _initialPosition = initial_position;	// could be stuck in the ground, if 'initial_position' is zero vector
	_initialPosition += CV3f(0,0, collision_cylinder_height_ws * 0.5f);	// standing on the ground

	V3f _initialVelocity = CV3f(0);

	{
		// the pivot of the monster's model is around its origin, and not around the bounding
		// box's origin, so we have to compensate for this when the model is offset so that
		// the monster still appears to rotate around it's origin.

		const M44f graphics_pretransform
			= M44_buildTranslationMatrix(-bindPoseLocalAabbHalfSize)	// center the model around the origin for scaling
			* M44_Scaling(DOOM3_TO_METERS, DOOM3_TO_METERS, DOOM3_TO_METERS)	// scale from md5 into our world units
			* M44_buildTranslationMatrix(CV3f( bindPoseLocalAabbHalfSize.x * DOOM3_TO_METERS, bindPoseLocalAabbHalfSize.y * DOOM3_TO_METERS, 0 ))	// shift to the center
			;

		_animModel.orig_graphics_pretransform = graphics_pretransform;


		//
		_rigidBody.m_shape = new(_rigidBody.m_shapeBuf) btCylinderShapeZ(toBulletVec(CV3f(collision_cylinder_radius_ws, collision_cylinder_radius_ws, collision_cylinder_height_ws)));
		//_rigidBody.m_shape = new(_rigidBody.m_shapeBuf) btCapsuleShapeZ( NPC_HEIGHT*0.4f, NPC_HEIGHT );

		//
		btVector3	localInertia;
		_rigidBody.m_shape->calculateLocalInertia(_mass, localInertia);

		_rigidBody.m_rb = new(_rigidBody.m_rbBuf) btRigidBody(
			_mass,
			&(_rigidBody.m_motionState),
			_rigidBody.m_shape,
			localInertia
		);

		_rigidBody.m_rb->setCollisionShape( _rigidBody.m_shape );
	}

	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3( _initialPosition.x, _initialPosition.y, _initialPosition.z ));

		_rigidBody.m_rb->setWorldTransform( transform );

		_rigidBody.m_rb->setLinearVelocity(btVector3(_initialVelocity.x, _initialVelocity.y, _initialVelocity.z));
	}

	_rigidBody.m_rb->setFriction(0.5f);
	_rigidBody.m_rb->setRestitution(0.5f);

	// objects fall too slowly with high linear damping
	_rigidBody.m_rb->setDamping( 1e-2f, 1e-4f );


	// prevent the character from toppling over - restrict the rigid body to rotate only about the UP axes
	_rigidBody.m_rb->setAngularFactor(btVector3(0.0f, 0.0f, 1.0f));


	// never sleep!
	_rigidBody.m_rb->setActivationState(DISABLE_DEACTIVATION);

//	_rigidBody._culling_proxy = culling_proxy;

	physicsWorld.m_dynamicsWorld.addRigidBody(
		_rigidBody.m_rb
	);

	return ALL_OK;
}

ERet NPC_Actor::load_obsolete(
	const RrAnimatedModel::CInfo& cInfo
	, NwClump & storage
	, Physics::TbPhysicsWorld & physicsWorld
	, const V3f& initial_position
	)
{
	UNDONE;
#if 0
	NwMaterial* fallback_material;
	mxDO(Resources::Load( fallback_material, MakeAssetID("material_default") ));

	//

	mxDO(RrAnimatedModel::loadFromMd5(
		_animModel
		, cInfo
		, storage
		, fallback_material
		));

	//
	const V3f bindPoseLocalAabbSize = _animModel.boundsInBindPose.size();
	const V3f bindPoseLocalAabbHalfSize = bindPoseLocalAabbSize * 0.5f;

RrTransform* graphicsTransform = _animModel.render_model._transform;
graphicsTransform->setLocalToWorldMatrix(M44_Identity());


	// Create rigid body (physics).

const float collision_cylinder_height_ws = bindPoseLocalAabbHalfSize.z * DOOM3_TO_METERS;
const float collision_cylinder_radius_ws = Min3(
							 bindPoseLocalAabbHalfSize.x * DOOM3_TO_METERS,
							 bindPoseLocalAabbHalfSize.y * DOOM3_TO_METERS,
							 bindPoseLocalAabbHalfSize.z * DOOM3_TO_METERS
							 );

//float height = NPC_HEIGHT * 0.5f;
//float _radius = height * 0.7f;
float _mass = (bindPoseLocalAabbSize * DOOM3_TO_METERS).boxVolume() * 80.0f;

V3f _initialPosition = initial_position;	// could be stuck in the ground, if 'initial_position' is zero vector
_initialPosition += CV3f(0,0, collision_cylinder_height_ws * 0.5f);	// standing on the ground

V3f _initialVelocity = CV3f(0);

	{
		// the pivot of the monster's model is around its origin, and not around the bounding
		// box's origin, so we have to compensate for this when the model is offset so that
		// the monster still appears to rotate around it's origin.

		const M44f graphics_pretransform
			= M44_buildTranslationMatrix(-bindPoseLocalAabbHalfSize)	// center the model around the origin for scaling
			* M44_Scaling(DOOM3_TO_METERS, DOOM3_TO_METERS, DOOM3_TO_METERS)	// scale from md5 into our world units
			* M44_buildTranslationMatrix(CV3f( bindPoseLocalAabbHalfSize.x * DOOM3_TO_METERS, bindPoseLocalAabbHalfSize.y * DOOM3_TO_METERS, 0 ))	// shift to the center
			;

		_animModel.orig_graphics_pretransform = graphics_pretransform;


		//
		_rigidBody.m_shape = new(_rigidBody.m_shapeBuf) btCylinderShapeZ(toBulletVec(CV3f(collision_cylinder_radius_ws, collision_cylinder_radius_ws, collision_cylinder_height_ws)));
		//_rigidBody.m_shape = new(_rigidBody.m_shapeBuf) btCapsuleShapeZ( NPC_HEIGHT*0.4f, NPC_HEIGHT );

		//
		btVector3	localInertia;
		_rigidBody.m_shape->calculateLocalInertia(_mass, localInertia);

		_rigidBody.m_rb = new(_rigidBody.m_rbBuf) btRigidBody(
			_mass,
			&(_rigidBody.m_motionState),
			_rigidBody.m_shape,
			localInertia
		);

		_rigidBody.m_rb->setCollisionShape( _rigidBody.m_shape );
	}

	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3( _initialPosition.x, _initialPosition.y, _initialPosition.z ));

		_rigidBody.m_rb->setWorldTransform( transform );

		_rigidBody.m_rb->setLinearVelocity(btVector3(_initialVelocity.x, _initialVelocity.y, _initialVelocity.z));
	}

	_rigidBody.m_rb->setFriction(0.5f);
	_rigidBody.m_rb->setRestitution(0.5f);

	// objects fall too slowly with high linear damping
	_rigidBody.m_rb->setDamping( 1e-2f, 1e-4f );


	// prevent the character from toppling over - restrict the rigid body to rotate only about the UP axes
	_rigidBody.m_rb->setAngularFactor(btVector3(0.0f, 0.0f, 1.0f));


	// never sleep!
	_rigidBody.m_rb->setActivationState(DISABLE_DEACTIVATION);

//	_rigidBody._culling_proxy = culling_proxy;

	physicsWorld.m_dynamicsWorld.addRigidBody(
		_rigidBody.m_rb
	);
#endif

	return ALL_OK;
}

void NPC_Actor::unload(
					   Physics::TbPhysicsWorld & physicsWorld
					   )
{
	physicsWorld.m_dynamicsWorld.removeRigidBody(
		&_rigidBody.bt_rb()
		);
}

void setLookDirection(btRigidBody & rb, const btVector3& newLook)
{
    // assume that "forward" for the player in local-frame is +zAxis
    // and that the player is constrained to rotate about yAxis (+yAxis is "up")
    btVector3 localLook(1.0f, 0.0f, 0.0f); // X Axis
    btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis

    // compute currentLook and angle
    btTransform transform = rb.getCenterOfMassTransform();
    btQuaternion rotation = transform.getRotation();
    btVector3 currentLook = transform.getBasis() * localLook;
    btScalar angle = currentLook.angle(newLook);

    // compute new rotation
    btQuaternion deltaRotation(rotationAxis, angle);
    btQuaternion newRotation = deltaRotation * rotation;

	if( angle < DEG2RAD(1) ) {
		return;
	}

    // apply new rotation
    transform.setRotation(newRotation);
    rb.setCenterOfMassTransform(transform);
}

static
void ProcessAnimEvents(
							const NwPlayingAnim* playing_animations
							, const int num_playing_animations
							, NwAnimEventList::AnimEventCallback* callback
							, void * user_data
							)
{
	TFixedArray< const NwAnimClip*, MAX_BLENDED_ANIMS >	already_processed_layers;

	for( UINT i = 0; i < num_playing_animations; i++ )
	{
		const NwPlayingAnim& playing_anim = playing_animations[ i ];

		// so that commands aren't called twice
		mxASSERT( !already_processed_layers.contains( playing_anim.animation ) );

		already_processed_layers.add( playing_anim.animation );

		//
		const NwAnimEventList& events = playing_anim.animation->events;
		
		const NwAnimClip* animation = playing_anim.animation->animation.ptr();

		//const U32 start_frame = animation->timeRatioToFrameIndex( playing_anim.prev_time_ratio );
		//const U32 end_frame = animation->timeRatioToFrameIndex( playing_anim.curr_time_ratio );

		const U32 start_frame = mmFloorToInt( playing_anim.prev_time_ratio * 65535.0f );
		const U32 end_frame = mmFloorToInt( playing_anim.curr_time_ratio * 65535.0f );



		events.ProcessEventsWithCallback(
			start_frame, end_frame
			, callback
			, user_data
			);

		//if( num_emitted_events )
		//{
		//	ptPRINT("\nAnim: %s, prev_time_ratio: %.3f, curr_time_ratio: %.3f, start_frame = %d, end_frame = %d"
		//		, playing_anim.animation->animation._id.d.c_str()
		//		, playing_anim.prev_time_ratio, playing_anim.curr_time_ratio
		//		, start_frame, end_frame
		//		);
		//}
	}
}


static
void _playSound_Weapon(
					   const AssetID& sound_shader_id
					   , NwSoundSystemI* sound_engine
					   , const NPC_Actor* npc_actor
					   )
{
	const btRigidBody& rb = npc_actor->_rigidBody.bt_rb();

	sound_engine->PlaySound3D(
		sound_shader_id
		, fromBulletVec( rb.getCenterOfMassPosition() )
		, fromBulletVec( rb.getLinearVelocity() )
		);
}

struct ProcessAnimationEventParams
{
	const NPC_Actor* npc_actor;
	NwSoundSystemI *	sound_engine;
	double delta_seconds;
	U64 frameNumber;
};

static
void ProcessAnimEvent(
						   const NwAnimEvent& animation_event
						   , void * user_data
									)
{
	//NPC_Actor* actor = (NPC_Actor*) user_data;
//DBGOUT("");
	//NwSoundSystemI& sound_engine = *(NwSoundSystemI*) user_data;
	ProcessAnimationEventParams& params = *(ProcessAnimationEventParams*) user_data;

	switch( animation_event.type )
	{
	case FC_SOUND:
		break;

	case FC_SOUND_VOICE:
			//ptWARN("dt = %.3f, frame = %llu"
			//	, params.delta_seconds, params.frameNumber
			//	);
			//params.sound_engine->PlaySound3D(AssetID());
			break;

	case FC_SOUND_VOICE2:
	case FC_SOUND_BODY:
	case FC_SOUND_BODY2:
	case FC_SOUND_BODY3:
		break;

	case FC_SOUND_WEAPON:
		_playSound_Weapon(
			animation_event.parameter
			, params.sound_engine
			, params.npc_actor
			);
		break;

	case FC_LAUNCH_MISSILE:
		//sound_engine.PlaySound3D(AssetID());
//		if( 0==strcmp(AssetId_ToChars(animation_event.parameter),"monster_zombie_commando_breath_inhale") )
		//{
		//	ptWARN("dt = %.3f, frame = %llu"
		//		, params.delta_seconds, params.frameNumber
		//		);
		//	params.sound_engine->PlaySound3D(AssetID());
		//}
		break;

	default:
		break;
	}
}

void NPC_Actor::tick(
					  double delta_seconds
					  , U64 frameNumber
					  , const V3f& target_pos_ws
					  , const RrAnimationSettings& animation_settings
					  , NwSoundSystemI& sound_engine
					  , ALineRenderer & dbgLineDrawer
					  )
{
	NwPlayingAnim playing_animations[MAX_PLAYED_ANIMATIONS];
	int num_playing_animations;

	_animModel.animator.AdvanceAnimations(
		delta_seconds * animation_settings.animation_speed_multiplier
		, _animModel._skinned_model
		, playing_animations
		, &num_playing_animations
		);

	//
	ProcessAnimationEventParams	params;
	params.npc_actor = this;
	params.sound_engine = &sound_engine;
	params.delta_seconds = delta_seconds;
	params.frameNumber = frameNumber;

//ptWARN("== \n New frame: %llu", frameNumber);

	ProcessAnimEvents(
		playing_animations
		, num_playing_animations
		, ProcessAnimEvent
		, &params
		);

	//
	const btRigidBody& bt_rigid_body = _rigidBody.bt_rb();
	const V3f current_position_ws = fromBulletVec( bt_rigid_body.getCenterOfMassPosition() );
	const V3f delta_position = current_position_ws - _previous_position;

	// Low-pass filter the deltaMove
	const float smooth = minf(1.0f, delta_seconds/0.15f);
	_smooth_delta_position = V3_Lerp( _smooth_delta_position, delta_position, smooth );

	// Update velocity if time advances
	V3f velocity = CV3f(0);
	if( delta_seconds > 1e-5f ) {
		velocity = V3f::div( _smooth_delta_position, delta_seconds );
	}

	const bool should_move = velocity.lengthSquared() > 0.2f;// && agent.remainingDistance > agent.radius;

	// Update animation parameters

	const V3f full_velocity = fromBulletVec( bt_rigid_body.getLinearVelocity() );
	const V3f forward_velocity = fromBulletVec( bt_rigid_body.getCenterOfMassTransform().getBasis().getColumn(0) * bt_rigid_body.getLinearVelocity() );
	const V3f angular_velocity = fromBulletVec( bt_rigid_body.getAngularVelocity() );

	anim_fsm.updateMovementAnimations(
		delta_seconds
		, forward_velocity
		, angular_velocity
		, _animModel.animator
		);

	//_animModel.animator.blendMovementAnimations(
	//	delta_seconds
	//	, should_move
	//	, full_velocity
	//	, forward_velocity
	//	, angular_velocity
	//	);

    //GetComponent<LookAt>().lookAtTargetPosition = agent.steeringTarget + transform.forward;

	_previous_position = current_position_ws;
}

bool NPC_Actor::turnTowards( const V3f& target_pos_ws )
{
	const V3f currPos = fromBulletVec( _rigidBody.bt_rb().getCenterOfMassPosition() );

	V3f targetDir = ( target_pos_ws - currPos );
	const float distToTargetPos = targetDir.normalize();

	//

	btVector3 localLook(1.0f, 0.0f, 0.0f); // X Axis
	btVector3 rotationAxis(0.0f, 0.0f, 1.0f); // Z Axis

	V3f targetDirAlongHorizPlane = targetDir;
	targetDirAlongHorizPlane.z = 0;
	targetDirAlongHorizPlane.normalize();

	//
	btTransform transform = _rigidBody.m_rb->getCenterOfMassTransform();
	btQuaternion currentOrientation = transform.getRotation();
	btVector3 currentLook = transform.getBasis() * localLook;

	////
	//dbgLineDrawer.DrawLine(
	//					   fromBulletVec(transform.getOrigin()),
	//					   fromBulletVec(transform.getOrigin() + currentLook * distToTargetPos)
	//					   );

	F32 minimumCorrectionAngle = DEG2RAD(1);

	btScalar angle = currentLook.angle(toBulletVec(targetDirAlongHorizPlane));


	btQuaternion desiredOrientation( rotationAxis, angle );

	btQuaternion inverseCurrentOrientation = currentOrientation.inverse();

	btScalar turningSpeed = 5.0f; // 

	if( mmAbs(angle) > minimumCorrectionAngle )
	{
		// https://stackoverflow.com/a/11022610
		V3f crossProd = V3f::cross( fromBulletVec(currentLook), targetDirAlongHorizPlane );	// right-hand rule

		// shortest arc angle
		F32 signedAngle = angle * signf(crossProd.z);

		btVector3 angularVelocity = (signedAngle * turningSpeed) * rotationAxis;
		_rigidBody.m_rb->setAngularVelocity(angularVelocity);

//LogStream(LL_Warn),"Delta Angle: ",angle,", DEGS: ",RAD2DEG(angle),", signedAngle=",signedAngle;

		return false;

	} else {
//		LogStream(LL_Warn),"Delta Angle == 0";
		_rigidBody.m_rb->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));

		return true;
	}
}

void NPC_Actor::dbg_draw(
						  ADebugDraw* dbgDrawer
						  , const AI_Settings& settings
						  )
{
	//_npc_marine.dbg_draw( dbg_draw, settings );
}


//=====================================================================

void NPC_Actor::applyImpulseToMoveTowards(
	const V3f& position_to_move_to
	, const BT_UpdateContext& context
	)
{
	const RrAnimationSettings& animation_settings = context.renderer_settings.animation;

	const V3f currPos = fromBulletVec( this->_rigidBody.bt_rb().getCenterOfMassPosition() );

	V3f targetDir = ( position_to_move_to - currPos );
	const float distToTargetPos = targetDir.normalize();


	//_rigidBody.m_rb->setLinearVelocity(toBulletVec(targetDir) * 3.0f);
	float invMass = this->_rigidBody.bt_rb().getInvMass();
	float impulseScale = 1.0f / invMass;

	// take speed from the animation
	//const V3f running_speed_from_animation = this->_animModel.animator.getCurrentAnimationSpeed();
	const V3f walking_speed_from_animation = this->anim_fsm.walk_anim->animation->speed * 0.9f;
	const V3f running_speed_from_animation = this->anim_fsm.run_anim->animation->speed;

	//
	btVector3	bsphere_center;
	btScalar	bsphere_radius;
	this->_rigidBody.bt_rb().getCollisionShape()->getBoundingSphere(
		bsphere_center
		, bsphere_radius
		);

	const bool is_close_to_dest = (distToTargetPos < bsphere_radius * 1.5f);


	const F32 speed = is_close_to_dest
		? mmSqrt( walking_speed_from_animation.lengthSquared() ) * animation_settings.animation_speed_multiplier
		: mmSqrt( running_speed_from_animation.lengthSquared() ) * animation_settings.animation_speed_multiplier
		;

	this->_rigidBody.m_rb->setLinearVelocity(toBulletVec(targetDir * speed));
}


bool NPC_Actor::is_dead(
	const BT_UpdateContext& context
	)
{
	NPC_Actor & npc_actor = *static_cast< NPC_Actor* >( context.npc_actor );

	if( HealthComponent* healthComponent = npc_actor.findComponentOfType<HealthComponent>() )
	{
		return healthComponent->isDead();
	}

	return true;
}

//=====================================================================

mxDEFINE_CLASS(NPC_Enemy);
mxBEGIN_REFLECTION(NPC_Enemy)
mxEND_REFLECTION;

NPC_Enemy::NPC_Enemy()
{
}

void NPC_Enemy::attackChosenEnemy()
{
	_animModel.animator
		.animation_controller
		.FadeOut()
		;

	_animModel.animator
		.animation_controller
		.PlayAnim_Exclusive( attack_anim )
		;

	//_weapon.startFiring();
//	context.sound_engine._fmod.PlaySound_Shoot();
}

void NPC_Enemy::stopAttacking()
{
	_animModel.animator.animation_controller
		.FadeOut();

	//_weapon.stopFiring();
}

BT_Status NPC_Enemy::play_death_animation(
							   const BT_UpdateContext& context
							   )
{
	NPC_Enemy & npc_enemy = *static_cast< NPC_Enemy* >( context.npc_actor );

	//
	npc_enemy.stopAttacking();

	//
	npc_enemy._animModel.animator
		.animation_controller
		.FadeOut()
		;

	//npc_enemy._animModel.animator
	//	.animation_controller
	//	.blendTo( ANIM_BELLY_PAIN )
	//	;

	npc_enemy._enemy_entity_id.setNil();

	return BT_Success;
}

//

BT_Status NPC_Enemy::choose_nearby_enemy( const BT_UpdateContext& context )
{
	NPC_Enemy & npc_enemy = *static_cast< NPC_Enemy* >( context.npc_actor );

	//
	if( NPC_Actor* npc_actor = context.world_state
		.findPlayerAllyClosestTo(
			fromBulletVec( npc_enemy._rigidBody.bt_rb().getCenterOfMassPosition() )
		))
	{
		npc_enemy._enemy_entity_id = npc_actor->_entity_id;
	}
	else
	{
		npc_enemy._enemy_entity_id.setNil();
	}

	return npc_enemy._enemy_entity_id.isValid() ? BT_Success : BT_Failure;
}

BT_Status NPC_Enemy::rotate_to_face_chosen_enemy( const BT_UpdateContext& context )
{
	NPC_Enemy & npc_enemy = *static_cast< NPC_Enemy* >( context.npc_actor );

	if( NPC_Actor* enemy = (NPC_Actor*) context.world_state._entity_lut.tryGet( npc_enemy._enemy_entity_id ) )
	{
		npc_enemy.stopAttacking();

		return npc_enemy.turnTowards( enemy->getOriginWS() ) ? BT_Success : BT_Running;
	}

	return BT_Failure;
}

BT_Status NPC_Enemy::move_to_chosen_enemy( const BT_UpdateContext& context )
{
	NPC_Enemy & npc_enemy = *static_cast< NPC_Enemy* >( context.npc_actor );

	if( NPC_Actor* enemy = (NPC_Actor*) context.world_state._entity_lut.tryGet( npc_enemy._enemy_entity_id ) )
	{
		npc_enemy.stopAttacking();

		//
		const AABBf my_aabb_ws = npc_enemy._rigidBody.getAabbWorld();
		const AABBf enemys_aabb_ws = enemy->_rigidBody.getAabbWorld();

		if( my_aabb_ws.intersects( enemys_aabb_ws ) )
		{
			return BT_Success;
		}

		npc_enemy.applyImpulseToMoveTowards(
			enemy->getOriginWS()
			, context
			);

		return BT_Running;
	}

	return BT_Failure;
}

BT_Status NPC_Enemy::attack_chosen_enemy( const BT_UpdateContext& context )
{
	NPC_Enemy & npc_enemy = *static_cast< NPC_Enemy* >( context.npc_actor );

	npc_enemy.attackChosenEnemy();

	return BT_Success;
}

//=====================================================================

Weapon::Weapon()
{
	_shots_per_minute = 600;
	_muzzle_velocity_meters_per_second = 100;
	_is_firing = false;
}

void Weapon::startFiring()
{
	_is_firing = true;
}

void Weapon::stopFiring()
{
	_is_firing = false;
}

bool Weapon::isFiring() const
{
	return _is_firing;
}

//=====================================================================

mxDEFINE_CLASS(NPC_Marine);
mxBEGIN_REFLECTION(NPC_Marine)
mxEND_REFLECTION;

NPC_Marine::NPC_Marine()
{
	_has_target_point_to_shoot_at = false;
	_target_point_to_shoot_at = CV3f(0);

	_need_to_move_to_designated_position = false;
	_destination_position_to_move_to = CV3f(0);

	//
	_weapon.stopFiring();
}

void NPC_Marine::shoot()
{
	_animModel.animator
		.animation_controller
		.FadeOut()
		;

	_animModel.animator
		.animation_controller
		.PlayAnim_Exclusive( attack_anim )
		;

	_weapon.startFiring();
//	context.sound_engine._fmod.PlaySound_Shoot();
}

void NPC_Marine::stopFiringWeapon()
{
	_animModel.animator.animation_controller
		.FadeOut();

	_weapon.stopFiring();
}

bool NPC_Marine::has_target( const BT_UpdateContext& context )
{
	NPC_Marine* npc_marine = static_cast< NPC_Marine* >( context.npc_actor );
	return npc_marine->_has_target_point_to_shoot_at;
}

BT_Status NPC_Marine::rotate_to_face_target( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	npc_marine.stopFiringWeapon();

	return npc_marine.turnTowards( npc_marine._target_point_to_shoot_at ) ? BT_Success : BT_Running;
}

BT_Status NPC_Marine::shoot_at_target( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	npc_marine.shoot();

	return BT_Success;
}

//

BT_Status NPC_Marine::choose_nearby_enemy( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	//
	if( NPC_Actor* npc_actor = context.world_state
		.findEnemyClosestTo(
			fromBulletVec( npc_marine._rigidBody.bt_rb().getCenterOfMassPosition() )
		))
	{
		HealthComponent* healthComponent = npc_actor->findComponentOfType<HealthComponent>();
		mxASSERT(healthComponent);
		if( healthComponent->isAlive() )
		{
			npc_marine._enemy_entity_id = npc_actor->_entity_id;
		}
		else
		{
			npc_marine._enemy_entity_id.setNil();
		}
	}
	else
	{
		npc_marine._enemy_entity_id.setNil();
	}

	return npc_marine._enemy_entity_id.isValid() ? BT_Success : BT_Failure;
}

BT_Status NPC_Marine::rotate_to_face_chosen_enemy( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	if( NPC_Actor* enemy = (NPC_Actor*) context.world_state._entity_lut.tryGet( npc_marine._enemy_entity_id ) )
	{
		npc_marine.stopFiringWeapon();

		return npc_marine.turnTowards( enemy->getOriginWS() ) ? BT_Success : BT_Running;
	}

	return BT_Failure;
}

BT_Status NPC_Marine::shoot_at_chosen_enemy( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	npc_marine.shoot();

	//if( NPC_Actor* enemy = context.world_state._entity_lut.tryGet( npc_marine._enemy_entity_id ) )
	//{
	//	HealthComponent* healthComponent = enemy->findComponentOfType<HealthComponent>();
	//	mxASSERT(healthComponent);

	//	healthComponent->applyDamage(10);

	//	if( healthComponent->isDead() )
	//	{
	//		npc_marine._enemy_entity_id.setNil();
	//	}
	//}

	return BT_Success;
}

//

bool NPC_Marine::need_to_move_to_designated_position( const BT_UpdateContext& context )
{
	const NPC_Marine& npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	//
	V3f currPos = fromBulletVec( npc_marine._rigidBody.bt_rb().getCenterOfMassPosition() );

	btVector3	worldAabbMin, worldAabbMax;
	npc_marine._rigidBody.bt_rb().getAabb( worldAabbMin, worldAabbMax );

	// on the floor
	currPos.z = worldAabbMin.z();

	//
	V3f targetDir = ( npc_marine._destination_position_to_move_to - currPos );
	const float distToTargetPos = targetDir.normalize();


	btVector3	bsphere_center;
	btScalar	bsphere_radius;
	npc_marine._rigidBody.bt_rb().getCollisionShape()->getBoundingSphere(
		bsphere_center
		, bsphere_radius
		);

	return npc_marine._need_to_move_to_designated_position
		&& distToTargetPos > bsphere_radius
		;
}

BT_Status NPC_Marine::rotate_to_face_designated_position( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	npc_marine._animModel.animator.animation_controller
		.FadeOut();

	npc_marine._weapon.stopFiring();

	return npc_marine.turnTowards( npc_marine._destination_position_to_move_to ) ? BT_Success : BT_Running;
}

BT_Status NPC_Marine::move_to_designated_position( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	npc_marine.applyImpulseToMoveTowards(
		npc_marine._destination_position_to_move_to
		, context
		);

	return BT_Success;
}

BT_Status NPC_Marine::idle( const BT_UpdateContext& context )
{
	NPC_Marine & npc_marine = *static_cast< NPC_Marine* >( context.npc_actor );

	//
	npc_marine._animModel.animator.animation_controller
		.FadeOut();

	npc_marine._has_target_point_to_shoot_at = false;
	npc_marine._need_to_move_to_designated_position = false;

	npc_marine._weapon.stopFiring();

	//
	npc_marine._rigidBody.m_rb->setLinearVelocity(toBulletVec(CV3f(0)));
	npc_marine._rigidBody.m_rb->setAngularVelocity(toBulletVec(CV3f(0)));

	return BT_Success;
}

//=====================================================================

EntityLUT::EntityLUT()
{
	_first_free_index = NIL_INDEX;
	_max_used = 0;
}

EntityLUT::~EntityLUT()
{
	//
}

void EntityLUT::initialize()
{
	_first_free_index = 0;

	//
	for( UINT i = 0; i < mxCOUNT_OF(entries); i++ )
	{
		entries[ i ].ptr = nil;
		entries[ i ].next_free_index = i + 1;
	}
	entries[ mxCOUNT_OF(entries) - 1 ].ptr = nil;
	entries[ mxCOUNT_OF(entries) - 1 ].next_free_index = NIL_INDEX;
}

void EntityLUT::shutdown()
{

}

ERet EntityLUT::add(
	void* ptr
	, EntityID *new_id_
	)
{
	const U16 free_index = _first_free_index;
	if( free_index != NIL_INDEX )
	{
		mxASSERT(_first_free_index < MAX_ENTITIES);

		Entry & entry = entries[ free_index ];

		//
		mxASSERT(entry.next_free_index < MAX_ENTITIES);
		_first_free_index = entry.next_free_index;

		//
		entry.ptr = ptr;

		//
		EntityID	new_id;
		{
			new_id.slot = free_index;
			new_id.version = ++entry.tag;
		}
		*new_id_ = new_id;

		return ALL_OK;
	}

	UNDONE;
	return ERR_TOO_MANY_OBJECTS;
}

ERet EntityLUT::remove(
	EntityID entity_id
	)
{
	if( isValidId( entity_id ) )
	{
		Entry & entry = entries[ entity_id.slot ];

		entry.ptr = nil;
		entry.tag++;

		mxASSERT(_first_free_index < MAX_ENTITIES);
		entry.next_free_index = _first_free_index;

		_first_free_index = entity_id.slot;

		return ALL_OK;
	}

	UNDONE;
	return ERR_OBJECT_NOT_FOUND;
}

void* EntityLUT::get( EntityID entity_id )
{
	mxASSERT( this->isValidId( entity_id ) );
	return entries[ entity_id.slot ].ptr;
}

void* EntityLUT::tryGet( EntityID entity_id )
{
	if( this->isValidId( entity_id ) )
	{
		return entries[ entity_id.slot ].ptr;
	}

	return nil;
}

bool EntityLUT::isValidId( EntityID entity_id ) const
{
	const U16 safe_index = entity_id.slot & ( MAX_ENTITIES - 1 );

	const Entry& entry = entries[ safe_index ];

	return ( entry.ptr != nil )
		&& ( entity_id.slot < MAX_ENTITIES )
		&& ( entry.tag == entity_id.version )
		;
}



mxDEFINE_CLASS(TbGameProjectile);
mxBEGIN_REFLECTION(TbGameProjectile)
mxEND_REFLECTION;


//=====================================================================

TDG_WorldState::TDG_WorldState( AllocatorI & allocator )
	: _alive_enemies( allocator )
	, _flying_projectiles( allocator )
{
	_targetPosWS = CV3f(0);

	isPaused = 0;
}

TDG_WorldState::~TDG_WorldState()
{
}

//
// Iterate over all manifolds and generate audio feedback for colliding objects.
//
static void Physics_internalTickCallback( btDynamicsWorld *world, btScalar timeStep )
{
	btDispatcher* dispatcher = world->getDispatcher();

	btPersistentManifold ** ppManifold = dispatcher->getInternalManifoldPointer();

	const int numManifolds = dispatcher->getNumManifolds();

	for ( int i = 0; i < numManifolds; i++ )
	{
		btPersistentManifold * pManifold = dispatcher->getManifoldByIndexInternal( i );

		const int numContacts = pManifold->getNumContacts();
		if( !numContacts ) {
			continue;
		}

		const void* pBody0 = pManifold->getBody0();
		const void* pBody1 = pManifold->getBody1();

		const btCollisionObject * pObj0 = static_cast< const btCollisionObject* >( pBody0 );
		const btCollisionObject * pObj1 = static_cast< const btCollisionObject* >( pBody1 );

		void * pUserPtr0 = pObj0->getUserPointer();
		void * pUserPtr1 = pObj1->getUserPointer();

		const Physics::TbRigidBody * pRigidBody0 = static_cast< const Physics::TbRigidBody* >( pUserPtr0 );
		const Physics::TbRigidBody * pRigidBody1 = static_cast< const Physics::TbRigidBody* >( pUserPtr1 );

		//
		if( pRigidBody0 && pRigidBody0->type == Physics::PHYS_PROJECTILE ) {
			TbGameProjectile* projectile0 = static_cast< TbGameProjectile* >( pRigidBody0->ptr );
			TDG_WorldState::Get().postEventOnCrojectileCollision( projectile0 );
		}
		if( pRigidBody1 && pRigidBody1->type == Physics::PHYS_PROJECTILE ) {
			TbGameProjectile* projectile1 = static_cast< TbGameProjectile* >( pRigidBody1->ptr );
			TDG_WorldState::Get().postEventOnCrojectileCollision( projectile1 );
		}

#if 0
		//TODO: avoid extra checks for NULL pointers:
		if ( (pEntity0 && pEntity0->IsGamePlayer()) || (pEntity1 && pEntity1->IsGamePlayer()) ) {
			continue;
		}

		for ( int iPointIdx = 0; iPointIdx < pManifold->getNumContacts(); iPointIdx++ )
		{
			btScalar  impulse = pManifold->getContactPoint( iPointIdx ).getAppliedImpulse();
			int  lifeTime = pManifold->getContactPoint( iPointIdx ).getLifeTime();

			const btVector3  position( pManifold->getContactPoint( iPointIdx ).getPositionWorldOnB() );

			// Generate impact sound.
			if ( lifeTime < 2 && impulse > 50 )
			{
				ContactInfo_s   contactInfo;
				contactInfo.impulse = impulse;

				mxSoundSource * pSndSource = GPhysicsWorld->GetMaterialSystem().GetSoundForMaterials( NULL,NULL, contactInfo );//pEntity0->GetMaterial(), pEntity1->GetMaterial() );
				if ( pSndSource ) {
					Vec3D  pos;
					Assign( pos, position );

					GSoundWorld->Play3D( pSndSource, pos );
				}
			}
		}
#endif
	}
}

ERet TDG_WorldState::initialize(
								NwClump *	storage_clump
								, Physics::TbPhysicsWorld* physics_world
								)
{
	_storage_clump = storage_clump;

	_entity_lut.initialize();

	_physics_world = physics_world;

	_physics_world->m_dynamicsWorld.setInternalTickCallback( &Physics_internalTickCallback );

	// AI
	_bt_manager.initialize();

	mxDO(_flying_projectiles.reserve(1024));

	EventSystem::addEventHandler( this );

	return ALL_OK;
}

void TDG_WorldState::shutdown(
							  )
{
	EventSystem::removeEventHandler( this );

#if DRAW_SOLDIER
	_npc_marine.unload( *_physics_world );
#endif
	//
	{
		NwClump::Iterator< NPC_Actor >	npc_actor_iterator( *_storage_clump );
		while( npc_actor_iterator.isValid() )
		{
			NPC_Actor & npc_actor = npc_actor_iterator.Value();

			npc_actor.unload(
				*_physics_world
				);

			npc_actor_iterator.MoveToNext();
		}
	}

	_physics_world = nil;

	_entity_lut.shutdown();

	_storage_clump = nil;
}

ERet TDG_WorldState::spawnNPC_Marine(
								 Physics::TbPhysicsWorld & physicsWorld
								 //, BT_Manager & bt_manager
								 )
{

#if DRAW_SOLDIER

	NwSkinnedMesh* skinned_model;
	mxDO(Resources::Load(
		skinned_model
		, MakeAssetID("monster_zombie_commando_cgun")
		, &_storage_clump
		));

	//
	mxDO(_npc_marine.load(
		skinned_model
		, _storage_clump
		, physicsWorld
		, CV3f(0)
		));

	RrModelAnimator &	animator = _npc_marine._animModel.animator;

	animator._known_animations[ANIM_IDLE] = skinned_model->getAnimByNameHash(mxHASH_STR("idle"));
	animator._known_animations[ANIM_WALK] = skinned_model->getAnimByNameHash(mxHASH_STR("walk"));
	animator._known_animations[ANIM_RUN] = skinned_model->getAnimByNameHash(mxHASH_STR("run"));
	animator._known_animations[ANIM_ATTACK] = skinned_model->getAnimByNameHash(mxHASH_STR("range_attack_loop1"));
//animator._known_animations[ANIM_ATTACK] = animator._known_animations[ANIM_IDLE];

	//animator._blend_layers[0].animation = animator._known_animations[ANIM_IDLE];
	//animator._blend_layers[1].animation = animator._known_animations[ANIM_IDLE];
	//animator._blend_layers[2].animation = animator._known_animations[ANIM_IDLE];
	//animator._blend_layers[3].animation = animator._known_animations[ANIM_IDLE];

	//
	_npc_marine.anim_fsm.idle_anim = animator._known_animations[ANIM_IDLE];
	_npc_marine.anim_fsm.walk_anim = animator._known_animations[ANIM_WALK];
	_npc_marine.anim_fsm.run_anim = animator._known_animations[ANIM_RUN];
	_npc_marine.attack_anim = animator._known_animations[ANIM_ATTACK];

	//
	mxDO(_entity_lut.add(
		&_npc_marine
		, &_npc_marine._entity_id
		));

	return ALL_OK;

#else

	return ERR_NOT_IMPLEMENTED;

#endif
}

ERet TDG_WorldState::spawn_NPC_Spider(
									  const V3f& position
									  , Physics::TbPhysicsWorld & physicsWorld
									  )
{
	NwSkinnedMesh* skinned_model;
	mxDO(Resources::Load(
		skinned_model
		, MakeAssetID("monster_demon_trite")
		, _storage_clump
		));

	//
	NPC_Enemy* new_NPC;
	mxDO(_storage_clump->New< NPC_Enemy >(
		new_NPC,
		NPC_ACTOR_ALLOC_GRANULARITY
		));

	//
	mxDO(new_NPC->load(
		skinned_model
		, *_storage_clump
		, physicsWorld
		, position
		));


	RrModelAnimator &	animator = new_NPC->_animModel.animator;

	animator._known_animations[ANIM_IDLE] = skinned_model->getAnimByNameHash(mxHASH_STR("idle"));
	animator._known_animations[ANIM_WALK] = skinned_model->getAnimByNameHash(mxHASH_STR("walk1"));
	animator._known_animations[ANIM_RUN] = skinned_model->getAnimByNameHash(mxHASH_STR("run"));
	animator._known_animations[ANIM_ATTACK] = skinned_model->getAnimByNameHash(mxHASH_STR("melee_attack1"));

	//
	new_NPC->anim_fsm.idle_anim = animator._known_animations[ANIM_IDLE];
	new_NPC->anim_fsm.walk_anim = animator._known_animations[ANIM_WALK];
	new_NPC->anim_fsm.run_anim = animator._known_animations[ANIM_RUN];
	new_NPC->attack_anim = animator._known_animations[ANIM_ATTACK];

	new_NPC->_animModel.animator
		.animation_controller.PlayAnim_Exclusive(animator._known_animations[ANIM_IDLE]);

	//
	//RrAnimatedModel::CInfo	cInfo;

	//cInfo.model_filename = "D:/dev/__source_assets/md5/monsters/trite/trite.md5mesh";

	//// "initial" is not an anim!
	////cInfo.anims[ANIM_IDLE] = "D:/dev/__source_assets/md5/monsters/trite/initial.md5anim";
	//cInfo.anims[ANIM_IDLE] = "D:/dev/__source_assets/md5/monsters/trite/alert_idle.md5anim";

	//cInfo.anims[ANIM_WALK] = "D:/dev/__source_assets/md5/monsters/trite/walk1.md5anim";

	//cInfo.anims[ANIM_RUN] = "D:/dev/__source_assets/md5/monsters/trite/walk3.md5anim";


	//cInfo.anims[ANIM_EVADE_LEFT] = "D:/dev/__source_assets/md5/monsters/trite/evade_left.md5anim";
	//cInfo.anims[ANIM_EVADE_RIGHT] = "D:/dev/__source_assets/md5/monsters/trite/evade_right.md5anim";

	//cInfo.anims[ANIM_BELLY_PAIN] = "D:/dev/__source_assets/md5/monsters/trite/pain_head.md5anim";

	//cInfo.anims[ANIM_ATTACK] = "D:/dev/__source_assets/md5/monsters/trite/melee1.md5anim";


	//
	HealthComponent* health_component;
	mxDO(_storage_clump->New< HealthComponent >(
		health_component
		));

	mxDO(new_NPC->AddComponent( health_component ));


	//
	mxDO(_entity_lut.add(
		new_NPC
		, &new_NPC->_entity_id
		));

	mxDO(_alive_enemies.add( new_NPC ));

	return ALL_OK;
}

NPC_Actor* TDG_WorldState::findEnemyClosestTo(
	const V3f& world_pos
	)
{
	F32	closest_dist_sq = BIG_NUMBER;
	NPC_Actor* closest_enemy = nil;

	for( UINT i = 0; i < _alive_enemies.num(); i++ )
	{
		NPC_Actor* npc_actor = _alive_enemies[i];

		const F32 dist_sq = ( npc_actor->getOriginWS() - world_pos ).lengthSquared();
		if( dist_sq < closest_dist_sq )
		{
			closest_dist_sq = dist_sq;
			closest_enemy = npc_actor;
		}
	}

	return closest_enemy;
}

NPC_Actor* TDG_WorldState::findPlayerAllyClosestTo(
	const V3f& world_pos
	)
{
#if DRAW_SOLDIER
	return &_npc_marine;
#else
	return nil;
#endif
}

void TDG_WorldState::test_set_destination_point_to_move_friendly_NPCs( const V3f& target_pos_WS )
{
	_targetPosWS = target_pos_WS;
#if DRAW_SOLDIER
	//
	_npc_marine._need_to_move_to_designated_position = true;
	_npc_marine._destination_position_to_move_to = target_pos_WS;

	_npc_marine._has_target_point_to_shoot_at = false;
#endif
}

void TDG_WorldState::test_set_point_to_attack_for_friendly_NPCs( const V3f& target_pos_WS )
{
	_targetPosWS = target_pos_WS;
#if DRAW_SOLDIER
	//
	_npc_marine._has_target_point_to_shoot_at = true;
	_npc_marine._target_point_to_shoot_at = target_pos_WS;

	_npc_marine._need_to_move_to_designated_position = false;
#endif
}


ERet TDG_WorldState::createTurret_Plasma(
	ARenderWorld * render_world
	, Physics::TbPhysicsWorld & physicsWorld
	)
{
	NwMesh *	mesh;
	mxDO(Resources::Load( mesh, MakeAssetID("plasma_turret") ));

	const M44f	initial_graphics_transform = M44_BuildTS( CV3f(0), CV3f(1) );

	RrMeshInstance *	render_model;
	mxDO(RrMeshInstance::ñreateFromMesh(
		render_model,
		mesh
		, *_storage_clump
		, &initial_graphics_transform
		));

	const AABBf mesh_aabb_in_world_space = AABBf::fromSphere(CV3f(0), 99999); //mesh->;

	//
	render_model->m_proxy = render_world->addEntity(
		render_model
		, RE_RigidModel
		, mesh_aabb_in_world_space
		);

	return ALL_OK;
}

void TDG_WorldState::update(
	 const InputState& input_state
	 , const RrRuntimeSettings& renderer_settings
	 , ALineRenderer & dbgLineDrawer
	)
{
	if( !isPaused )
	{

#if DRAW_SOLDIER
		_bt_manager.tick_NPC_Marine(
			_npc_marine
			, input_state.delta_time_seconds
			, renderer_settings
			, *this
			, sound_engine
			);

		_npc_marine.tick(
			input_state.delta_time_seconds
			, input_state.frameNumber
			, _targetPosWS
			, renderer_settings.animation
			, sound_engine
			, dbgLineDrawer
			);

#endif

		////
		//{
		//	NwClump::Iterator< NPC_Actor >	npc_actor_iterator( _storage_clump );
		//	while( npc_actor_iterator.isValid() )
		//	{
		//		NPC_Actor & npc_actor = npc_actor_iterator.Value();

		//		npc_actor.tick(
		//			input_state.delta_time_seconds
		//			, _targetPosWS
		//			, renderer_settings.animation
		//			, dbgLineDrawer
		//			);

		//		npc_actor_iterator.MoveToNext();
		//	}
		//}

		//
		{
			NwClump::Iterator< NPC_Enemy >	npc_enemy_iterator( *_storage_clump );
			while( npc_enemy_iterator.isValid() )
			{
				NPC_Enemy & npc_enemy = npc_enemy_iterator.Value();

#if TB_TEST_USE_SOUND_ENGINE

				_bt_manager.tick_NPC_Spider(
					npc_enemy
					, input_state.delta_time_seconds
					, renderer_settings
					, *this
					, sound_engine
					);

				npc_enemy.tick(
					input_state.delta_time_seconds
					, input_state.frameNumber
					, _targetPosWS
					, renderer_settings.animation
					, sound_engine
					, dbgLineDrawer
					);

#else

				UNDONE;

#endif

				npc_enemy_iterator.MoveToNext();
			}
		}
	}
}

static
void _render_NPC_Actor(
					   NPC_Actor & npc_actor
					   , const CameraMatrices& camera_matrices
					   , GL::GfxRenderContext & render_context
					   )
{
	RrAnimatedModel::updateJointMatrices( npc_actor._animModel );

	updateGraphicsTransform( npc_actor );

	//
	RrAnimatedModel::submitToRenderQueue(
		npc_actor._animModel
		, camera_matrices
		, render_context
		);
}


static
ERet _render_NPC_Marine( NPC_Marine & npc_marine
						, const CameraMatrices& camera_matrices
						, GL::GfxRenderContext & render_context
						)
{
	RrAnimatedModel::updateJointMatrices( npc_marine._animModel );

	updateGraphicsTransform( npc_marine );

	//
	const RrAnimatedModel& animated_model = npc_marine._animModel;

	const RrMeshInstance& model = animated_model.render_model;

	mxDO(Rendering::submitModel(
		model
		, camera_matrices
		, render_context
		));

	return ALL_OK;
}

void TDG_WorldState::render(
							const CameraMatrices& camera_matrices
							, GL::GfxRenderContext & render_context
							)
{
#if DRAW_SOLDIER

	_render_NPC_Marine(
		_npc_marine
		, camera_matrices
		, render_context
		);

#endif

	//_render_NPC_Marine(
	//	_plasmagun
	//	, camera_matrices
	//	, render_context
	//	);

	//
	{
		NwClump::Iterator< NPC_Actor >	npc_actor_iterator( *_storage_clump );
		while( npc_actor_iterator.isValid() )
		{
			NPC_Actor & npc_actor = npc_actor_iterator.Value();

			//
			_render_NPC_Actor(
				npc_actor
				, camera_matrices
				, render_context
				);

			//
			npc_actor_iterator.MoveToNext();
		}
	}

#if 0
	if(SHOW_MD5_ANIM)
	{
		//m_md5_anim_model.drawBaseFrameSkeleton();
		//md5_model.drawMeshInBindPose( _render_system->m_debugRenderer, M44_Identity(), RGBAf::YELLOW );
		m_md5_anim_model.Dbg_Draw(_render_system->m_debugRenderer, M44_buildTranslationMatrix(CV3f(100,0,0)), RGBAf::GREEN);
	}
	if(SHOW_OZZ_ANIM)
	{
		//dbg_drawSkeletonInBindPose(_anim_data._skeleton, _render_system->m_debugRenderer);
		//dbg_drawSkeletalAnim(_anim_data, _render_system->m_debugRenderer);
	}
	if(SHOW_SKINNED_MESH)
	{
		//dbg_drawSkinnedMeshData_inBindPose( skinned_mesh_data
		//	, _render_system->m_debugRenderer
		//	, RGBAf::WHITE );

		//dbg_drawSkinnedMeshData_Animated( skinned_mesh_data
		//	, _anim_data._skeleton
		//	, _anim_data.models
		//	, _render_system->m_debugRenderer
		//	, M44_buildTranslationMatrix(CV3f(50,0,0))
		//	, RGBAf::RED );

		//
		Rendering::updateSkinningMatrices( _gfx_skinned_model, _skinned_mesh_data, _anim_data.models_ );

		const CameraMatrices	camera_matrices( camera_matrices );
		drawModel( *_gfx_skinned_model, camera_matrices, *_scene_renderer.m_renderPath, render_context );

		//Rendering::dbg_drawTangents( *_gfx_skinned_model, camera_matrices, *_scene_renderer.m_renderPath, render_context, &m_runtime_clump );
	}
#endif

}

static
void _dbg_draw_NPC_Marine(
						  const NPC_Marine& npc_marine
						  , ADebugDraw* dbgDrawer
						  , const AI_Settings& settings
						  )
{
	if( npc_marine._weapon.isFiring() )
	{
		const RrModelAnimator& animator = npc_marine._animModel.animator;
		const ozz::animation::Skeleton& skeleton = npc_marine._animModel._skinned_model->skeleton;

		const ozz::Range<const char* const> joint_names = skeleton.joint_names();

		int barrel_joint_index = getJointIndexByName(
			"barrelconnector"//"barrel"
			, skeleton
			);

		mxASSERT( barrel_joint_index != -1 );

		const M44f& muzzle_joint_matrix_model_space = ozzMatricesToM44f( animator.models[ barrel_joint_index ] );

		const V3f muzzle_joint_pos_model_space
			= M44_getTranslation( muzzle_joint_matrix_model_space )
			//+ CV3f( -0.1f, 0, 0 )	// -20 cm
			;

		//
		const M44f local_to_world_matrix = npc_marine._animModel.render_model.transform->getLocalToWorld4x4Matrix();

		const V3f muzzle_joint_pos_world_space
			= M44_TransformPoint( local_to_world_matrix, muzzle_joint_pos_model_space )
			;

		const V3f muzzle_joint_axis_model_space = V3_Normalized( Vector4_As_V3( muzzle_joint_matrix_model_space.r[2] ) );
		const V3f muzzle_joint_axis_world_space = M44_TransformNormal( local_to_world_matrix, muzzle_joint_axis_model_space );


		const V3f barrel_direction_world_space = V3_Normalized( muzzle_joint_axis_world_space );//V3_Normalized( muzzle_joint_pos_world_space - muzzle_parent_joint_pos_world_space );

		//
		dbgDrawer->DrawLine(
			muzzle_joint_pos_world_space, muzzle_joint_pos_world_space + barrel_direction_world_space * 10.0f
			, RGBAf::RED, RGBAf::WHITE
			);

		//
	}
}

void TDG_WorldState::dbg_draw(
						  ADebugDraw* dbgDrawer
						  , const AI_Settings& settings
						  )
{

#if DRAW_SOLDIER

	_npc_marine.dbg_draw( dbgDrawer, settings );

	_dbg_draw_NPC_Marine(
		_npc_marine
		, dbgDrawer
		, settings
		);

#endif

}

void TDG_WorldState::handleGameEvent(
									 const TbGameEvent& game_event
									 )
{
	switch( game_event.type )
	{
	case EV_PROJECTILE_FIRED:
		this->createProjectile( game_event.shot_fired );
		break;

	case EV_PROJECTILE_COLLIDED:
		{
			EntityID	projectile_id = game_event.projectile_collided.projectile_id;

			TbGameProjectile* projectile = (TbGameProjectile*) _entity_lut.tryGet( projectile_id );

			if( projectile )	// prevent
			{
DBGOUT("EV_PROJECTILE_COLLIDED: %d (slot=%d, tag=%d)", projectile_id.id, projectile_id.slot, projectile_id.version);

				TbGameEvent	new_event;
				{
					new_event.type = EV_PROJECTILE_EXPLODED;
					new_event.timestamp = Testbed::gameTime();

					new_event.projectile_exploded.projectile_id = projectile_id;
					new_event.projectile_exploded.position = game_event.projectile_collided.position;
				}
				EventSystem::PostEvent( new_event );
			}
		}
		break;

	case EV_PROJECTILE_EXPLODED:
		{
			EntityID	projectile_id = game_event.projectile_exploded.projectile_id;

			TbGameProjectile* projectile = (TbGameProjectile*) _entity_lut.tryGet( projectile_id );

			if( projectile )
			{
DBGOUT("EV_PROJECTILE_EXPLODED: %d (slot=%d, tag=%d)", projectile_id.id, projectile_id.slot, projectile_id.version);

				Physics::TbRigidBody* rigid_body = projectile->rigid_body;

//TowerDefenseGame::Get().m_dbgView.addSphere(
//	rigid_body->getAabbWorld().center(), 0.5f, RGBAf::WHITE
//	);
				//

				//
				_physics_world->destroyRigidBody( rigid_body );

				//
				_storage_clump->Destroy( projectile );

				//
				_entity_lut.remove( projectile_id );

				//
			}
			else
			{
				ptWARN("NULL Projectile!");
			}
		}
		break;

	default:
		break;
	}
}

ERet TDG_WorldState::createProjectile(
									  const TbGameEvent_ShotFired& event_data
									  )
{
	//
	TbGameProjectile *	game_projectile;
	mxDO(_storage_clump->New( game_projectile ));

	mxDO(_entity_lut.add( game_projectile, &game_projectile->id ));
	
	//
	Physics::TbRigidBody *	new_rigid_body;

	_physics_world->createRigidSphere(
		&new_rigid_body
		, event_data.position
		, event_data.col_radius
		, _storage_clump
		, event_data.projectile_mass
		, event_data.direction * 50.0f//event_data.lin_velocity
		);

	//
	new_rigid_body->type = Physics::PHYS_PROJECTILE;
	new_rigid_body->ptr = game_projectile;

	//
	game_projectile->rigid_body = new_rigid_body;

	////
	//new_rigid_body->bt_rb().setCollisionFlags(
	//	new_rigid_body->bt_rb().getCollisionFlags() |
	//	btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK
	//	);

	//gContactProcessedCallback

	return ALL_OK;
}

void TDG_WorldState::postEventOnCrojectileCollision( TbGameProjectile* projectile )
{
	TbGameEvent	new_event;
	{
		new_event.type = EV_PROJECTILE_COLLIDED;
		new_event.timestamp = Testbed::gameTime();

		new_event.projectile_collided.projectile_id = projectile->id;

		Physics::TbRigidBody* rigid_body = projectile->rigid_body;
		new_event.projectile_collided.position = fromBulletVec( rigid_body->bt_rb().getCenterOfMassPosition() );
	}
	EventSystem::PostEvent( new_event );
}
