// Behavior Trees
#pragma once

#include <Base/Memory/Allocators/FixedBufferAllocator.h>

#include <Core/EntitySystem.h>	// EntityID

///
class NPC_Actor;
class NPC_Marine;
class NPC_Enemy;

class TDG_WorldState;
class NwSoundSystemI;

///
template< typename TYPE, int MAX_COUNT >
class TStack: NonCopyable
{
	TYPE	_items[ MAX_COUNT ];
	int		_top;	// points past the valid element

public:
	mxFORCEINLINE TStack()
	{
		_top = 0;
	}

	mxFORCEINLINE bool isEmpty() const
	{
		return _top == 0;
	}

	mxFORCEINLINE bool isFull() const
	{
		return _top >= MAX_COUNT;
	}

	mxFORCEINLINE TYPE& peek()
	{
		mxASSERT(!this->isEmpty());
		return _items[ _top - 1 ];
	}

	mxFORCEINLINE void push( const TYPE& new_item )
	{
		mxASSERT(!this->isFull());
		_items[ _top++ ] = new_item;
	}

	mxFORCEINLINE void pop()
	{
		mxASSERT(!this->isEmpty());
		--_top;
	}

	mxFORCEINLINE TYPE* raw()
	{
		return _items;
	}

	mxFORCEINLINE const TYPE* raw() const
	{
		return _items;
	}
};


class BT_Node;

enum BehaviorTreeConstants
{
	BT_MAX_STACK_DEPTH = 8,
};

/// The return type when invoking behaviour tree nodes.
enum BT_Status
{
	BT_Invalid,

	/// The node has finished what it was doing and succeeded.
	BT_Success,

	/// The node has finished, but failed.
	BT_Failure,

	/// The node is still working on something.
	BT_Running
};
mxDECLARE_ENUM( BT_Status, U8, BT_StatusT );

/// The behavior tree itself is kind of stateless in that
/// it relies on the entity or the environment for storing state.
struct BT_EntityState
{
	/// remembers state when traversing a Behavior Tree
	struct BT_StackEntry
	{
		BT_Node *	current;
		BT_Node *	parent;

		//
		BT_Status	status;

	public:
		BT_StackEntry()
		{
			current = nil;
			parent = nil;
			status = BT_Invalid;
		}
	};

	BT_StackEntry	stack[8];
	int				top;
	//TStack
public:
	BT_EntityState()
	{
		top = 0;
	}
};

///
struct BT_UpdateContext
{
	BT_UpdateContext(
		NPC_Actor * npc_actor
		, const float delta_seconds
		, const RrRuntimeSettings& renderer_settings
		, TDG_WorldState& world_state
		, NwSoundSystemI& sound_engine
		)
		: npc_actor( npc_actor )
		, delta_seconds( delta_seconds )
		, renderer_settings( renderer_settings )
		, world_state( world_state )
		, sound_engine( sound_engine )
	{
	}

	/// The AI agent being simulated.
	NPC_Actor * const	npc_actor;

	/// the time elapsed since last update, in seconds
	const float	delta_seconds;

	const RrRuntimeSettings& renderer_settings;

	TDG_WorldState& world_state;

	NwSoundSystemI& sound_engine;


#if 0
	BT_UpdateContext(
		const AZ::EntityId _id
		, IEntity* _entity
		, BehaviorVariablesContext& _variables
		, TimestampCollection& _timestamps
		, Blackboard& _blackboard
#ifdef USING_BEHAVIOR_TREE_LOG
		, MessageQueue& _behaviorLog
#endif // USING_BEHAVIOR_TREE_LOG
#ifdef DEBUG_MODULAR_BEHAVIOR_TREE
		, DebugTree* _debugTree = NULL
#endif // DEBUG_MODULAR_BEHAVIOR_TREE

		)
		: entityId(_id)
		, entity(_entity)
		, runtimeData(NULL)
		, variables(_variables)
		, timestamps(_timestamps)
		, blackboard(_blackboard)
#ifdef USING_BEHAVIOR_TREE_LOG
		, behaviorLog(_behaviorLog)
#endif // USING_BEHAVIOR_TREE_LOG
#ifdef DEBUG_MODULAR_BEHAVIOR_TREE
		, debugTree(_debugTree)
#endif // DEBUG_MODULAR_BEHAVIOR_TREE
	{
	}

	AZ::EntityId entityId;
	IEntity* entity;
	void* runtimeData;
	BehaviorVariablesContext& variables;
	TimestampCollection& timestamps;
	Blackboard& blackboard;

#ifdef USING_BEHAVIOR_TREE_LOG
	MessageQueue& behaviorLog;
#endif // USING_BEHAVIOR_TREE_LOG

#ifdef DEBUG_MODULAR_BEHAVIOR_TREE
	DebugTree* debugTree;
#endif // DEBUG_MODULAR_BEHAVIOR_TREE
#endif
};

///
typedef bool BT_ConditionCallback( const BT_UpdateContext& context );

///
typedef BT_Status BT_ActionCallback( const BT_UpdateContext& context );

///
class BT_Node: public AObject
{
public://ZZZ - move to bitflags in entity state
	//BT_Status	status;

	/// This name is purely for testing and debugging purposes.
	const char *	_name;

public:
	mxDECLARE_ABSTRACT_CLASS( BT_Node, AObject );
	mxDECLARE_REFLECTION;

	BT_Node()
	{
		//status = BT_Invalid;
		_name = "";
	}

	virtual ~BT_Node() {}


	//// When you "tick" a node the following rules apply:
	//// 1. If the node was not previously running the node will first be
	////    initialized and then immediately updated.
	//// 2. If the node was previously running, but no longer is, it will be
	////    terminated.
	//virtual BT_Status Tick(const BT_UpdateContext& context) = 0;

	//// Call this to explicitly terminate a node. The node will itself take
	//// care of cleaning things up.
	//// It's safe to call Terminate on an already terminated node,
	//// although it's of course redundant.
	//virtual void Terminate(const BT_UpdateContext& context) = 0;



    //// Send an event to the node.
    //// The event will be dispatched to the correct HandleEvent method
    //// with validated runtime data.
    //// Never override!
    //virtual void SendEvent(const EventContext& context, const Event& event) = 0;




	//// Called before the first call to Update.
	//virtual void onInitialize(const BT_UpdateContext& context) {}

	//// Called when a node is being terminated. Clean up after yourself here!
	//// Not to be confused with Terminate which you call when you want to
	//// terminate a node. :) Confused? Read it again.
	//// OnTerminate is called in one of the following cases:
	//// a) The node itself returned Success/Failure in Update.
	//// b) Another node told this node to Terminate while this node was running.
	//virtual void onTerminate(const BT_UpdateContext& context) {}

	// Do you node's work here.
	// - Note that OnInitialize will have been automatically called for you
	// before you get your first update.
	// - If you return Success or Failure the node will automatically
	// get OnTerminate called on itself.
	virtual BT_Status update( const BT_UpdateContext& context ) { return BT_Running; }
};

///
class BT_ParentNode: public BT_Node
{
public:
	mxDECLARE_ABSTRACT_CLASS( BT_ParentNode, BT_Node );
	mxDECLARE_REFLECTION;

	virtual void addChild( BT_Node* new_child ) = 0;
};


/// Composites have at least 1 child and are used to filter their child's process result.
/// Composites are a form of flow control and determine how the child branches that are connected to them execute.
class BT_CompositeNode: public BT_ParentNode
{
public:
	mxDECLARE_ABSTRACT_CLASS( BT_CompositeNode, BT_ParentNode );
	mxDECLARE_REFLECTION;

	enum { MAX_KIDS = 4 };

	TFixedArray< BT_Node*, MAX_KIDS >	_kids;

public:
	virtual void addChild( BT_Node* new_child ) override
	{
		_kids.add( new_child );
	}
};

//  Processes child nodes in order. Logical AND of the child nodes.
//
//    Returns SUCCESS if all children returned success.
//    Stops processing and returns RUNNING if a child returned RUNNING.
//    Stops processing and returns FAILURE if a child returned FAILURE.
//    
// The Sequence composite ticks each child node in order.
// If a child fails or runs, the sequence returns the same status.
// In the next tick, it will try to run each child in order again.
// If all children succeeds, only then does the sequence succeed.
class BT_Sequence: public BT_CompositeNode
{
public:
	mxDECLARE_ABSTRACT_CLASS( BT_Sequence, BT_CompositeNode );
	mxDECLARE_REFLECTION;

	/// Executes branches from left-to-right and are more commonly used to execute a series of children in order.
	/// Unlike Selectors, the Sequence continues to execute its children until it reaches a node that fails.
	/// For example, if we had a Sequence to move to the Player, check if they are in range, then rotate and attack.
	/// If the check if they are in range portion failed, the rotate and attack actions would not be performed.
	///
	virtual BT_Status update( const BT_UpdateContext& context ) override
	{
		const UINT numKids = _kids.num();
		mxASSERT(numKids);

		BT_Node** kids = _kids.raw();

		for( UINT i = 0; i < numKids; i++ )
		{
			const BT_Status child_status = kids[i]->update( context );
			if( child_status != BT_Success )
			{
				return child_status;
			}
		}

		return BT_Success;
	}
};

///
/// Selects the first node that succeeds. Tries successive nodes until it finds one that doesn't fail.
/// 
//  Processes child nodes in order. Logical OR of the child nodes.
//
//    Stops processing and returns SUCCESS if any child returned SUCCESS.
//    Stops processing and returns RUNNING if any child returned RUNNING.
//    Returns FAILURE if no child returned SUCCESS or RUNNING.
///
class BT_Selector: public BT_CompositeNode
{
public:
	mxDECLARE_ABSTRACT_CLASS( BT_Selector, BT_CompositeNode );
	mxDECLARE_REFLECTION;

	/// Executes branches from left-to-right and are typically used to select between subtrees.
	/// Selectors stop moving between subtrees when they find a subtree they successfully execute.
	/// For example, if the AI is successfully chasing the Player, it will stay in that branch until its execution is finished,
	/// then go up to selector's parent composite to continue the decision flow.
	///
	virtual BT_Status update( const BT_UpdateContext& context ) override
	{
		const UINT numKids = _kids.num();
		mxASSERT(numKids);

		BT_Node** kids = _kids.raw();

		for( UINT i = 0; i < numKids; i++ )
		{
			const BT_Status child_status = kids[i]->update( context );
			if( child_status != BT_Failure )
			{
				return child_status;
			}
		}

		return BT_Failure;
	}
};

///// 
//class BT_Parallel: public BT_CompositeNode
//{
//public:
//	virtual BT_Status update( const BT_UpdateContext& context ) override
//	{
//		const UINT numKids = _kids.num();
//		mxASSERT(numKids);
//
//		BT_Node** kids = _kids.raw();
//
//		for( UINT i = 0; i < numKids; i++ )
//		{
//			const BT_Status child_status = kids[i]->update( context );
//			if( child_status != BT_Success )
//			{
//				return child_status;
//			}
//		}
//
//		return BT_Success;
//	}
//};





/*
Simple Parallel
Simple Parallel has two "connections". The first one is the Main Task, and it can only be assigned a Task node (meaning no Composites). The second connection (the Background Branch) is the activity that's supposed to be executed while the main Task is still running. Depending on the properties, the Simple Parallel may finish as soon as the Main Task finishes, or wait for the Background Branch to finish as well.
*/





///
/// Like an action node... but the function can return true/false and is mapped to success/failure.
///
class BT_Node_Condition: public BT_ParentNode
{
public:
	mxDECLARE_ABSTRACT_CLASS( BT_Node_Condition, BT_ParentNode );
	mxDECLARE_REFLECTION;

public:
	BT_ConditionCallback *	_callback;
	BT_Node *				_child;

public:
	BT_Node_Condition()
	{
		_callback = nil;
		_child = nil;
	}

	virtual BT_Status update( const BT_UpdateContext& context ) override
	{
		if( (*_callback)( context ) )
		{
			return _child->update( context );
		}

		return BT_Failure;
	}

	virtual void addChild( BT_Node* new_child ) override
	{
		mxASSERT2(nil == _child, "only one child is supported");
		_child = new_child;
	}
};



///
/// A behaviour tree leaf node for running an action.
///
class BT_Node_Action: public BT_Node
{
public:
	/// Function to invoke for the action.
	BT_ActionCallback *	_callback;

public:
	virtual BT_Status update( const BT_UpdateContext& context ) override
	{
		return (*_callback)( context );
	}
};




///
/// Fluent API for building a behaviour tree.
///
class BT_Builder: NonCopyable
{
	/// Stack node nodes that are used to build the tree via the fluent API.
	TStack< BT_ParentNode*, BT_MAX_STACK_DEPTH >	_parent_node_stack;

	AllocatorI &	_allocator;

public:
	BT_Builder( AllocatorI & allocator );

	BT_Node* build();


	//
	// Internal nodes:
	//

	BT_Builder& sequence( const char* name );

	BT_Builder& selector( const char* name );

	BT_Builder& if_( const char* name, BT_ConditionCallback* callback );

	//
	BT_Builder& end();

	//
	// Leaf nodes:
	//

	// action
	BT_Builder& do_( const char* name, BT_ActionCallback* callback );

private:
	void attachNewNodeToLastParent( BT_Node* new_child_node );
};

///
class BT_Manager: NonCopyable
{
public:
	BT_Manager();
	~BT_Manager();

	ERet initialize();
	void shutdown();

	ERet startBehaviorTree( const EntityID entityId );
	ERet stopBehaviorTree( const EntityID entityId );

public:

	void tick_NPC_Marine(
		NPC_Marine & npc_marine
		, float delta_seconds
		, const RrRuntimeSettings& renderer_settings
		, TDG_WorldState& world_state
		, NwSoundSystemI& sound_engine
		);

	void tick_NPC_Spider(
		NPC_Enemy & npc_enemy
		, float delta_seconds
		, const RrRuntimeSettings& renderer_settings
		, TDG_WorldState& world_state
		, NwSoundSystemI& sound_engine
		);

	//void tick(
	//	float delta_seconds
	//	);

private:
	BT_Node* createBehaviorTree_for_marine();
	BT_Node* createBehaviorTree_for_enemy_spider();

	//BT_Status tick_recursive(
	//	BT_Node* node
	//	, const BT_UpdateContext& context
	//	, float delta_seconds
	//	);

public:
	BT_EntityState	_entity_state;


	BT_Node *	_bt_spider_simple_ai;	// when far away and outside the view
	BT_Node *	_bt_spider_ai;

	BT_Node *	_bt_marine_ai;

	TempAllocator1024	_bt_node_alloc;
};

namespace BehaviorTree
{



}//namespace BehaviorTree


// References:
// 
// General AI programming:
// https://developer.valvesoftware.com/wiki/AI_Programming_Overview
// https://developer.valvesoftware.com/wiki/State 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// Behavior Trees:
// https://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php
// 
// Introduction to Behavior Trees by Bjoern Knafla [2011]:
// http://web.archive.org/web/20110429053741/http://altdevblogaday.org/2011/02/24/introduction-to-behavior-trees/
// 
// GDC 2005 Proceeding: Handling Complexity in the Halo 2 AI:
// https://www.gamasutra.com/view/feature/130663/gdc_2005_proceeding_handling_.php
// 
// Utility AI vs Behavior Trees:
// https://www.gamasutra.com/blogs/JakobRasmussen/20160427/271188/Are_Behavior_Trees_a_Thing_of_the_Past.php
// 
// Advanced Behavior Tree Structures:
// https://medium.com/game-dev-daily/advanced-behavior-tree-structures-4b9dc0516f92
// 
// Synchronized Behavior Trees:
// https://takinginitiative.wordpress.com/2014/02/17/synchronized-behavior-trees/
// 
// This C++ library provides a framework to create BehaviorTrees. It was designed to be flexible, easy to use, reactive and fast.
// https://github.com/BehaviorTree/BehaviorTree.CPP
// 
// Behavior Trees:
// https://github.com/libgdx/gdx-ai/wiki/Behavior-Trees
//
// Fluent behavior trees for AI and game-logic:
// http://www.what-could-possibly-go-wrong.com/fluent-behavior-trees-for-ai-and-game-logic/
//
// http://web.archive.org/web/20110429025655/http://altdevblogaday.org/2011/04/24/data-oriented-streams-spring-behavior-trees/
// 
// https://behaviortree.github.io/BehaviorTree.CPP/