#pragma once


//
#include <Base/Memory/Pool/TIndexedPool.h>

#include <Core/EntitySystem.h>

#include <Physics/TbPhysicsWorld.h>

#include <Renderer/Core/MeshInstance.h>
#include <Renderer/Modules/Animation/SkinnedMesh.h>

#include <Utility/Animation/AnimationUtil.h>

#include "TowerDefenseGame-AI.h"

class BT_Manager;

struct GameStats
{
	int	max_joints_in_md5_models;

	static GameStats	shared;
};


struct AI_Viz_Flags
{
	enum Enum
	{
		dbg_viz_rigid_bodies = BIT(0),
	};
};
mxDECLARE_FLAGS( AI_Viz_Flags::Enum, U32, AI_Viz_FlagsT );

/// AI
struct AI_Settings: CStruct
{
	AI_Viz_FlagsT		flags;

public:
	mxDECLARE_CLASS(AI_Settings, CStruct);
	mxDECLARE_REFLECTION;
	AI_Settings();
};







///
class NPC_Actor: public Entity
{
public:
	EntityID			_entity_id;

	Physics::TbRigidBody	_rigidBody;

public:	/// Animation & Rendering


	RrAnimatedModel		_animModel;

	MovementAnimationFSM	anim_fsm;

public:	/// AI

	///AI_Bot_State_Machine	_fsm;

	/// This indicates where we are moving to
	//V3f		_goalPos;

	///// current direction the character is facing (2D only) - CCW, in radians
	//float	_currentYaw;

	///// for turning
	//float	_targetYaw;

	//float _movementSpeed;            // character's movement speed -- in units/second
	//float _turningVelocity;        // character's turning speed -- in radians/second


	// Based on Unity, Coupling Animation and Navigation:
	// https://docs.unity3d.com/Manual/nav-CouplingAnimationAndNavigation.html

	V3f _previous_position;
    V3f _smooth_delta_position;
    //V3f _average_velocity;

public:
	mxDECLARE_CLASS(NPC_Actor, Entity);
	mxDECLARE_REFLECTION;

	NPC_Actor();

	ERet load(
		NwSkinnedMesh* skinned_model
		, NwClump & storage
		, Physics::TbPhysicsWorld & physicsWorld
		, const V3f& initial_position
		);

	ERet load_obsolete(
		const RrAnimatedModel::CInfo& cInfo
		, NwClump & storage
		, Physics::TbPhysicsWorld & physicsWorld
		, const V3f& initial_position
		);

	/// must be called to remove collision object from Bullet (otherwise, it crashes)
	void unload(
		Physics::TbPhysicsWorld & physicsWorld
		);


	void dbg_draw(
		ADebugDraw* dbgDrawer
		, const AI_Settings& settings
		);

	//void goToState( AI_STATE newState );ZZZ

public:
	const V3f getOriginWS() const
	{
		return fromBulletVec( _rigidBody.bt_rb().getCenterOfMassPosition() );
	}

public:
	void tick(
		double delta_seconds
		, U64 frameNumber
		, const V3f& target_pos_ws
		, const RrAnimationSettings& animation_settings
		, NwSoundSystemI& sound_engine
		, ALineRenderer & dbgLineDrawer
		);

	bool turnTowards( const V3f& target_pos_ws );

	void applyImpulseToMoveTowards(
		const V3f& position_to_move_to
		, const BT_UpdateContext& context
		);

public:	//=== AI

	static bool is_dead(
		const BT_UpdateContext& context
		);

protected:
private:

};

/// monster
class NPC_Enemy: public NPC_Actor
{
public:
	EntityID	_enemy_entity_id;

	TPtr< const NwAnimClip >	attack_anim;

public:
	mxDECLARE_CLASS(NPC_Enemy, NPC_Actor);
	mxDECLARE_REFLECTION;

	NPC_Enemy();

public:

	void attackChosenEnemy();

	void stopAttacking();

public:	//=== AI

	static BT_Status play_death_animation(
		const BT_UpdateContext& context
		);

	// Combat

	static BT_Status choose_nearby_enemy(
		const BT_UpdateContext& context
		);
	static BT_Status rotate_to_face_chosen_enemy(
		const BT_UpdateContext& context
		);
	static BT_Status move_to_chosen_enemy(
		const BT_UpdateContext& context
		);
	static BT_Status attack_chosen_enemy(
		const BT_UpdateContext& context
		);
};



/// Firearm
class Weapon
{
public:
	/// rate of fire
	int		_shots_per_minute;

	///
	int		_muzzle_velocity_meters_per_second;

	bool	_is_firing;

public:
	Weapon();

	void startFiring();
	void stopFiring();

	bool isFiring() const;
};

/// ally
class NPC_Marine: public NPC_Actor
{
public:
	bool	_has_target_point_to_shoot_at;
	V3f		_target_point_to_shoot_at;

	bool	_need_to_move_to_designated_position;
	V3f		_destination_position_to_move_to;

	//
	EntityID	_enemy_entity_id;
	Weapon		_weapon;

	TPtr< const NwAnimClip >	attack_anim;

public:
	mxDECLARE_CLASS(NPC_Marine, NPC_Actor);
	mxDECLARE_REFLECTION;

	NPC_Marine();

public:
	void shoot();

	void stopFiringWeapon();

public:	//=== AI

	static bool has_target(
		const BT_UpdateContext& context
		);
	static BT_Status rotate_to_face_target(
		const BT_UpdateContext& context
		);
	static BT_Status shoot_at_target(
		const BT_UpdateContext& context
		);

	// Combat

	static BT_Status choose_nearby_enemy(
		const BT_UpdateContext& context
		);
	static BT_Status rotate_to_face_chosen_enemy(
		const BT_UpdateContext& context
		);
	static BT_Status shoot_at_chosen_enemy(
		const BT_UpdateContext& context
		);

	//
	static bool need_to_move_to_designated_position(
		const BT_UpdateContext& context
		);
	static BT_Status rotate_to_face_designated_position(
		const BT_UpdateContext& context
		);
	static BT_Status move_to_designated_position(
		const BT_UpdateContext& context
		);

	//
	static BT_Status idle(
		const BT_UpdateContext& context
		);
};


/// for safely addressing entities by handle/id
class EntityLUT: NonCopyable
{
	struct Entry
	{
		void *	ptr;
		U16		tag;
		U16		next_free_index;
	};
	Entry	entries[MAX_ENTITIES];

	U16		_first_free_index;

	int		_max_used;          // highest index ever alloced

	static const U16 NIL_INDEX = ~0;

public:
	EntityLUT();
	~EntityLUT();

	void initialize();
	void shutdown();

	ERet add(
		void* ptr
		, EntityID *new_id_
		);

	ERet remove(
		EntityID entity_id
		);

	void* get( EntityID entity_id );

	/// validates id, then returns item, returns null if invalid.
	/// for cases like AI references and others where 'the thing might have been deleted out from under me'
	void* tryGet( EntityID entity_id );

	bool isValidId( EntityID entity_id ) const;
};

struct TbGameProjectile: CStruct
{
	EntityID				id;

	Physics::TbRigidBody *	rigid_body;

	//V3f	position;
	//F32	mass;
	//V3f	direction;
	//F32	speed;
public:
	mxDECLARE_CLASS(TbGameProjectile, CStruct);
	mxDECLARE_REFLECTION;
};

///
class TDG_WorldState
	: public TbGameEventHandlerI
	, public TGlobal< TDG_WorldState >
	//, NonCopyable
{
public:
	TDG_WorldState( AllocatorI & allocator );
	~TDG_WorldState();

public:
	ERet initialize(
		NwClump *	storage_clump
		, Physics::TbPhysicsWorld* physics_world
		);

	void shutdown(
		);

public:	// Friendly

	ERet spawnNPC_Marine(
		Physics::TbPhysicsWorld & physicsWorld
		//, BT_Manager & bt_manager
		);


	void test_set_destination_point_to_move_friendly_NPCs(
		const V3f& target_pos_WS
		);

	void test_set_point_to_attack_for_friendly_NPCs(
		const V3f& target_pos_WS
		);

public:	// Enemies

	ERet spawn_NPC_Spider(
		const V3f& position
		, Physics::TbPhysicsWorld & physicsWorld
		);

	NPC_Actor* findEnemyClosestTo(
		const V3f& world_pos
		);

	NPC_Actor* findPlayerAllyClosestTo(
		const V3f& world_pos
		);

public:

	ERet create_Weapon_Plasmagun(
		ARenderWorld * render_world
		mxREFACTOR("remove this")
		, Physics::TbPhysicsWorld & physicsWorld
		);

	ERet createTurret_Plasma(
		ARenderWorld * render_world
		, Physics::TbPhysicsWorld & physicsWorld
		);

public:	// TbGameEventHandlerI

	virtual void handleGameEvent(
		const TbGameEvent& game_event
		) override;

	ERet createProjectile(
		const TbGameEvent_ShotFired& event_data
		);

	void postEventOnCrojectileCollision( TbGameProjectile* projectile );

public:
	void update(
		 const InputState& input_state
		 , const RrRuntimeSettings& renderer_settings
		 , ALineRenderer & dbgLineDrawer
		);

	void render(
		const CameraMatrices& camera_matrices
		, GL::GfxRenderContext & render_context
		);

	void dbg_draw(
		ADebugDraw* dbgDrawer
		, const AI_Settings& settings
		);

public:
	bool	isPaused;

	// free-listed memory storage for objects
	TPtr< NwClump >	_storage_clump;

	//
	BT_Manager		_bt_manager;

	//
	V3f		_targetPosWS;

#if DRAW_SOLDIER
	NPC_Marine	_npc_marine;
#endif

	TPtr< Physics::TbPhysicsWorld >	_physics_world;


	DynamicArray< NPC_Actor* >	_alive_enemies;


	DynamicArray< TbGameProjectile >	_flying_projectiles;

	/// for safely addressing entities by handle/id
	EntityLUT	_entity_lut;

protected:
private:
	enum {
		NPC_ACTOR_ALLOC_GRANULARITY = 512
	};
};
