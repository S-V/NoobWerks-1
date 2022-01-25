#pragma once

// std::aligned_storage
#include <type_traits>

#include <Core/Tasking/TinyCoroutines.h>

#include <Developer/RunTimeCompiledCpp.h>

#include "game_compile_config.h"
#include "game_forward_declarations.h"

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#endif // GAME_CFG_WITH_PHYSICS


/*
----------------------------------------------------------
	NwRigidBody
----------------------------------------------------------
*/

#if !GAME_CFG_WITH_PHYSICS

struct NwRigidBody: CStruct
{
};

#endif	// !GAME_CFG_WITH_PHYSICS


enum EMonsterType
{
	MONSTER_IMP,
	MONSTER_TRITE,
	MONSTER_MAGGOT,
	MONSTER_MANCUBUS,

	MONSTER_TYPE_COUNT
};
mxDECLARE_ENUM( EMonsterType, U8, MonsterType8 );

//extern const char* g_monster_def_file_by_type[MONSTER_TYPE_COUNT];

/*
-----------------------------------------------------------------------------
	AI blackboard
-----------------------------------------------------------------------------
*/
struct AI_Blackboard: TGlobal<AI_Blackboard>
{
	// for NPC<->player line-of-sight checks
	V3f			player_pos;
	bool		is_player_alive;

	const btCollisionObject* player_collision_object;

	// for NPC<->player overlap checks
	V3f	player_aabb_min_with_margin;
	V3f	player_aabb_max_with_margin;

	MyGamePlayer *	player;

public:
	AI_Blackboard()
	{
		player_pos = CV3f(0);
		is_player_alive = false;

		player_aabb_min_with_margin = CV3f(0);
		player_aabb_max_with_margin = CV3f(0);
	}
};




/*
-----------------------------------------------------------------------------
	HealthComponent
-----------------------------------------------------------------------------
*/
struct HealthComponent: CStruct//EntityComponent
{
public:
	int		_health;

public:
	mxDECLARE_CLASS(HealthComponent, CStruct);
	mxDECLARE_REFLECTION;
	HealthComponent();
	~HealthComponent();

	void DealDamage( int damage_points );

	bool IsAlive() const { return _health > 0; }
	bool IsDead() const { return !this->IsAlive(); }
};

struct StandardAnims
{
	TPtr< const NwAnimClip >	run;
	TPtr< const NwAnimClip >	attack;

	TPtr< const NwAnimClip >	pain;
};


/// shared by NPCs of same type
struct NPC_BehaviorI
{
	virtual ~NPC_BehaviorI() {}

	virtual ERet CreateDataForNPC(
		EnemyNPC & npc
		)
	{
		return ALL_OK;
	};
	virtual void DeleteDataForNPC(
		EnemyNPC & npc
		)
	{};

	//struct ThinkParams
	//{
	//	//
	//};
	virtual void Think(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, const NwTime& game_time
		, const CoTimestamp now
		)
	{};

	virtual void InflictDamage(
		EnemyNPC & npc
		, int damage
		, const V3f& pos_WS
		)
	{};

	virtual int GetDamageForDealingToPlayer(
		const NwAnimEventParameter& anim_event_parameter
		) = 0;
};

/*
-----------------------------------------------------------------------------
	EnemyNPC
-----------------------------------------------------------------------------
*/
class EnemyNPC
	: public CStruct
	, public TStaticCounter<EnemyNPC>
{
public:
	// Physics model
	TPtr< NwRigidBody >		rigid_body;

	// Visuals
	TPtr< NwAnimatedModel >	anim_model;

	StandardAnims	anims;

	// AI
	EMonsterType			monster_type;
	TPtr< NPC_BehaviorI >	behavior;
	TPtr< CStruct >			ai_brain;

public:	// Updated by AI behavior.

	HealthComponent	comp_health;

public_internal:	// Updated by movement code.
	// Movement mode
	bool	is_driven_by_animation;	// anim or phys?

	// enable animation/root-motion driven movement only if we're touching the ground
	bool	is_touching_ground;

	V3f	previous_root_position;

	//EntityID	_enemy_entity_id;
	//TPtr< const NwAnimClip >	attack_anim;

	//V3f _previous_position;
 //   V3f _smooth_delta_position;

public:
	enum { ALLOC_GRAN = 64 };

	mxDECLARE_CLASS(EnemyNPC, CStruct);
	mxDECLARE_REFLECTION;

	EnemyNPC();
};


///
class MyGameNPCs
	: NonCopyable
	, RunTimeCompiledCpp::ListenerI
{
	AI_Blackboard	blackboard;

	//
	TPtr< NwClump >	_clump;

public:

	ERet Init(
		NwClump* scene_clump
		);

	void Deinit(
		MyGameWorld& game_world
		);

	virtual void RTCPP_OnConstructorsAdded() override;

public:
	void UpdateGraphicsTransformsFromAnimationsAndPhysics(
		const NwTime& game_time
		, ARenderWorld* render_world
		, NwPhysicsWorld& physics_world
		);

	// NOTE: must be run after physics update!
	void TickAI(
		const NwTime& game_time
		, const V3f& player_pos
		, const CubeMLf& world_bounds
		);

	void DebugDrawAI(
		const NwTime& game_time
		) const;

	void KillNPC(EnemyNPC & npc);

	void DeleteNPC(EnemyNPC & npc);

	void DeleteAllNPCs();


public:
	ERet Spawn_Spider(
		EnemyNPC *&new_npc_
		, MyGameWorld& game_world
		, const V3f& position
		);
	ERet Spawn_Imp(
		EnemyNPC *&new_npc_
		, MyGameWorld& game_world
		, const V3f& position
		);
	ERet Spawn_Mancubus(
		EnemyNPC *&new_npc_
		, MyGameWorld& game_world
		, const V3f& position
		);
	ERet Spawn_Maggot(
		EnemyNPC *&new_npc_
		, MyGameWorld& game_world
		, const V3f& position
		);

private:
	ERet _SpawnMonster(
		EnemyNPC *&new_npc_
		, MyGameWorld& game_world
		, const V3f& position
		, const EMonsterType monster_type
		, TResPtr<NwSkinnedMesh> skinned_mesh
		);
};

namespace MyGameUtils
{
	ERet SpawnMonster(
		const V3f& position
		, const EMonsterType monster_type
		, EnemyNPC **new_npc_ = nil
		);
}
