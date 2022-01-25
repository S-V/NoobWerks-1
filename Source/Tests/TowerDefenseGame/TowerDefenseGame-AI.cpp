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

#include <Utility/MeshLib/TriMesh.h>

#include <Utility/VoxelEngine/VoxelTerrainChunk.h>
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

// OZZ
#include <ozz/animation/runtime/blending_job.h>


#include <Voxels/vx5_config.h>

#include "TowerDefenseGame-AI.h"

#include "TowerDefenseGame-Misc.h"

//=====================================================================

#define __

BT_Node* BT_Manager::createBehaviorTree_for_marine()
{
	return BT_Builder(_bt_node_alloc)
		.selector("root_selector")

		// attack the target if needed
		__ .if_("has_target", NPC_Marine::has_target)
		__ __ .sequence("seq0")
		__ __ __ .do_("rotate_to_face_target", NPC_Marine::rotate_to_face_target)
		__ __ __ .do_("shoot_at_target", NPC_Marine::shoot_at_target)
		__ __ .end()
		__ .end()

		//
		__ .if_("need_to_move_to_designated_position", NPC_Marine::need_to_move_to_designated_position)
		__ __ .sequence("seq1")
		__ __ __ .do_("rotate_to_face_designated_position", NPC_Marine::rotate_to_face_designated_position)
		__ __ __ .do_("move_to_designated_position", NPC_Marine::move_to_designated_position)
		__ __ .end()
		__ .end()

		// attack enemies
		__ .sequence("enemy_attack_seq")
		__ __ .do_("choose_nearby_enemy", NPC_Marine::choose_nearby_enemy)
		__ __ .do_("rotate_to_face_enemy", NPC_Marine::rotate_to_face_chosen_enemy)
		__ __ .do_("shoot_at_enemy", NPC_Marine::shoot_at_chosen_enemy)
		__ .end()

		// do nothing
		__ .do_("idle", NPC_Marine::idle)

		.build()
		;
}

BT_Node* BT_Manager::createBehaviorTree_for_enemy_spider()
{
	return BT_Builder(_bt_node_alloc)
		.selector("root_selector")

		// play death animation
		__ .if_("is_dead", NPC_Enemy::is_dead)
		__ __ .sequence("death_seq")
		__ __ __ .do_("play_death_animation", NPC_Enemy::play_death_animation)
		__ __ .end()
		__ .end()

		// attack enemies
		__ .sequence("enemy_attack_seq")
		__ __ .do_("choose_nearby_enemy", NPC_Enemy::choose_nearby_enemy)
		__ __ .do_("rotate_to_face_chosen_enemy", NPC_Enemy::rotate_to_face_chosen_enemy)
		__ __ .do_("move_to_chosen_enemy", NPC_Enemy::move_to_chosen_enemy)
		__ __ .do_("attack_chosen_enemy", NPC_Enemy::attack_chosen_enemy)
		__ .end()

		// do nothing
		__ .do_("idle", NPC_Marine::idle)

		.build()
		;
}

#undef __

//=====================================================================


mxBEGIN_REFLECT_ENUM( BT_StatusT )
	mxREFLECT_ENUM_ITEM( Invalid, BT_Status::BT_Invalid ),
	mxREFLECT_ENUM_ITEM( Success, BT_Status::BT_Success ),
	mxREFLECT_ENUM_ITEM( Failure, BT_Status::BT_Failure ),
	mxREFLECT_ENUM_ITEM( Running, BT_Status::BT_Running ),
mxEND_REFLECT_ENUM

//=====================================================================

mxDEFINE_ABSTRACT_CLASS( BT_Node );
mxBEGIN_REFLECTION( BT_Node )
mxEND_REFLECTION;

mxDEFINE_ABSTRACT_CLASS( BT_ParentNode );
mxBEGIN_REFLECTION( BT_ParentNode )
mxEND_REFLECTION;

mxDEFINE_ABSTRACT_CLASS( BT_CompositeNode );
mxBEGIN_REFLECTION( BT_CompositeNode )
mxEND_REFLECTION;

mxDEFINE_CLASS( BT_Sequence );
mxBEGIN_REFLECTION( BT_Sequence )
mxEND_REFLECTION;

mxDEFINE_CLASS( BT_Selector );
mxBEGIN_REFLECTION( BT_Selector )
mxEND_REFLECTION;

mxDEFINE_CLASS( BT_Node_Condition );
mxBEGIN_REFLECTION( BT_Node_Condition )
mxEND_REFLECTION;

//=====================================================================

BT_Builder::BT_Builder( AllocatorI & allocator )
	: _allocator( allocator )
{
	//
}

void BT_Builder::attachNewNodeToLastParent( BT_Node* new_child_node )
{
	if( !_parent_node_stack.isEmpty() )
	{
		_parent_node_stack.peek()->addChild( new_child_node );
	}
}

BT_Builder& BT_Builder::end()
{
	_parent_node_stack.pop();

	return *this;
}

BT_Builder& BT_Builder::sequence( const char* name )
{
	BT_CompositeNode* new_node = mxNEW( _allocator, BT_Sequence );

	new_node->_name = name;

	this->attachNewNodeToLastParent( new_node );

	_parent_node_stack.push( new_node );

	return *this;
}

BT_Builder& BT_Builder::selector( const char* name )
{
	BT_CompositeNode* new_node = mxNEW( _allocator, BT_Selector );

	new_node->_name = name;

	this->attachNewNodeToLastParent( new_node );

	_parent_node_stack.push( new_node );

	return *this;
}

BT_Builder& BT_Builder::if_( const char* name, BT_ConditionCallback* callback )
{
	BT_Node_Condition* new_node = mxNEW( _allocator, BT_Node_Condition );

	new_node->_name = name;
	new_node->_callback = callback;

	this->attachNewNodeToLastParent( new_node );

	_parent_node_stack.push( new_node );

	return *this;
}

// action
BT_Builder& BT_Builder::do_( const char* name, BT_ActionCallback* callback )
{
	BT_Node_Action* new_node = mxNEW( _allocator, BT_Node_Action );

	new_node->_name = name;
	new_node->_callback = callback;

	this->attachNewNodeToLastParent( new_node );

	return *this;
}

BT_Node* BT_Builder::build()
{
	// TODO:
	//
	// 1. Consolidate the tree structure so that it is one contiguous block of memory,
	// this fixes the jumping all over memory problem when evaluating your tree.
	//
	// 2. Collect a list of all the “heavy lifting” behaviour nodes and update them all after the tree has evaluated
	// (which also allows the decision making aspect of the tree to be updated at a lower rate than the behaviour aspect of the tree thats need to run every frame).
	// This post update of only certain performance critical nodes allows similar types of nodes to be updated en mass, or to be updated over a common small set of data.

	return _parent_node_stack.raw()[0];
}

//=====================================================================

BT_Manager::BT_Manager()
	: _bt_node_alloc(nil)
{

}

BT_Manager::~BT_Manager()
{

}

ERet BT_Manager::initialize()
{
	_bt_marine_ai = createBehaviorTree_for_marine();

	_bt_spider_simple_ai = createBehaviorTree_for_enemy_spider();
	_bt_spider_ai = _bt_spider_simple_ai;

	return ALL_OK;
}

void BT_Manager::shutdown()
{

}

ERet BT_Manager::startBehaviorTree( const EntityID entityId )
{
	return ALL_OK;
}

ERet BT_Manager::stopBehaviorTree( const EntityID entityId )
{
	return ALL_OK;
}

void BT_Manager::tick_NPC_Marine(
								 NPC_Marine & npc_marine
								 , float delta_seconds
								 , const RrRuntimeSettings& renderer_settings
								 , TDG_WorldState& world_state
								 , NwSoundSystemI& sound_engine
								 )
{
	BT_UpdateContext	context(
		&npc_marine
		, delta_seconds
		, renderer_settings
		, world_state
		, sound_engine
		);

	const BT_Status status = _bt_marine_ai->update( context );

//	DBGOUT("BT status: %s", BT_StatusT_Type().FindString(status) );
}

void BT_Manager::tick_NPC_Spider(
								 NPC_Enemy & npc_enemy
								 , float delta_seconds
								 , const RrRuntimeSettings& renderer_settings
								 , TDG_WorldState& world_state
								 , NwSoundSystemI& sound_engine
								 )
{
	BT_UpdateContext	context(
		&npc_enemy
		, delta_seconds
		, renderer_settings
		, world_state
		, sound_engine
		);

	const BT_Status status = _bt_spider_ai->update( context );

//	DBGOUT("BT status: %s", BT_StatusT_Type().FindString(status) );
}

//void BT_Manager::tick(
//	float delta_seconds
//	)
//{
//	BT_UpdateContext	context(
//		delta_seconds
//		);
//
//	const BT_Status status = _bt_marine_ai->update( context );
//
//	DBGOUT("BT status: %s", BT_StatusT_Type().FindString(status) );
//
//	//tick_recursive(
//	//	_bt_tree
//	//	, context
//	//	, delta_seconds
//	//	);
//}

//BT_Status BT_Manager::tick_recursive(
//								BT_Node* node
//								, const BT_UpdateContext& context
//								, float delta_seconds
//								)
//{
//	//_entity_state
//
//	//if( node->status != BT_Running ) {
//	//	node->onInitialize( context );
//	//}
//
//	node->status = node->update( context );
//
//	//if( node->status != BT_Running ) {
//	//	node->onTerminate( context );
//	//}
//
//	return node->status;
//}



/* TODO:
http://web.archive.org/web/20110203040321/http://www.neatsciences.com/html/products.html

 Behavior Tree Editor for debugging:

    Open and load your behavior trees in the editor.
    Create new behavior trees and edit them using a friendly-user interface.
    Parametrize your leaf nodes and non leaf nodes.
    Connect the editor with the behavior tree library, integrated in your game.
    Load behavior trees dynamically in your game.
    Debug and set breakpoints in your running behaviors from the editor.
    Play, step, pause and stop your loaded behaviors from the interface.
    Enable and disable tree branches dinamically from the editor.
    Review your execution in a graphical tree view, going back and forward in the log.
    Save your logs and behavior trees to review them at any time.
    Analyze the behavior tree performance, including node execution calls, returned status and time spent in each node.

Open Source C++ library of behavior trees.

    Configure your behavior trees to load from a XML file or from the Behavior Tree Editor.
    Root node, selectors, sequences and up to seven different types of decorators.
    Create your own leaf nodes within a few easy steps.
    Blackboard manager implemented to use custom blackboards in your behavior trees.
    Assertion tools for a safer way of programming.
    Two demos integrated, manuals and tutorials to understand the library and its integration.

*/
