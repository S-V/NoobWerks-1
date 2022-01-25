/*
=============================================================================
	File:	PhysicsSimulator.h
	Desc:	Physics world.
=============================================================================
*/

#ifndef __PX_PHYSICS_WORLD_H__
#define __PX_PHYSICS_WORLD_H__

#if 0
#include <Core/Memory.h>
#include <Physics/Physics_Config.h>
#include <Physics/Collision_Detection.h>

// size of fixed time step - ~60 Hz
#define PX_DEFAULT_FIXED_STEP_SIZE (1.0f/60.0f)

//// NOTE: pxCollideable structs are not moved in memory (they're usually accessed randomly);
//// but pointers will invalidated if the array reallocates memory so use handles/indices.
//typedef
//	TList< pxCollideable >
//CollideableList;
//
//// NOTE: pxRigidBody structs can be reordered in memory (they're usually accessed linearly)
//typedef
//	TIndirectHandleManager< pxRigidBody, PX_MAX_RIGID_BODIES >
//RigidBodyList;
//
//
////
//// pxSolverInput
////
//class pxSolverInput {
//public:
//	// Time step information.
//	pxReal			deltaTime;
//
//	pxRigidBody *	bodies;
//	pxUInt			numBodies;
////	pxConstraint *	constraints;
////	pxUInt			numConstraints;
//	pxContactCache*	contacts;
//
//	pxF4	gravity;	// for faking friction
//
//public:
//	pxSolverInput()
//	{
//		setDefaults();
//	}
//	void setDefaults()
//	{
//		deltaTime = 0.0f;
//
//		bodies = nil;
//		numBodies = 0;
//
//		contacts = nil;
//
//		gravity = 10.0f;
//	}
//	bool isOk() const
//	{
//		return 1
//			&& (deltaTime > 0.0f)
//
//		//	&& CHK((bodies && numBodies)||(!bodies && !numBodies))
//			&& CHK(contacts)
//			;
//	}
//};
//

/// A spherical particle
struct Atom
{
	V3f		position;	//!< current position
	F32		radius;				//!< size
	V3f		next_position;		//!< predicted position
	F32		inverse_mass;		//!< 0 == static object
	V3f		velocity;	//!< current velocity
	U32		num_links;			//!< the number of constraints affecting the particle
	V3f		pad1;
	void *	user_data;
};

struct DerivedState
{
	//
};

struct ContactContraint
{
	U16	iAtomA;
	U16	iAtomB;
};


struct RigidBodyState
{
	V3f		current_position;	//!< center of mass position
	F32		inverse_mass;		//!< 0 == static object
	V3f		next_position;		//!< predicted position
	U32		num_links;			//!< the number of constraints affecting the particle
	V3f		current_velocity;
	F32		pad2;
	V3f		delta_x;	//!< for pre-stabilization
	U32		pad3;
//	void *	user_data;
public:
	void clear()
	{
		mxZERO_OUT(*this);
	}
};
mxSTATIC_ASSERT_ISPOW2(sizeof(RigidBodyState));




/*
-----------------------------------------------------------------------------
	PhysicsSimulator

	- is a simulation environment.
-----------------------------------------------------------------------------
*/
class PhysicsSimulator: NonCopyable
{
public:
	PhysicsSimulator();
	~PhysicsSimulator();

	struct CreationInfo {
		//pxBroadphase *			broadphase;
		//CollisionDispatcher *	dispatcher;
		//pxConstraintSolver *	constraintSolver;
		V3f		gravity;	// gravity acceleration
		UINT	pre_stabilization_iterations;	//!< contact pre-stabilization
		UINT	position_solver_iterations;	//!< number of position solver iterations per timestep
		UINT	velocity_solver_iterations;	//!< number of velocity solver iterations per timestep
	public:
		CreationInfo();
	};

	ERet Initialize( const CreationInfo& worldDesc );
	void Shutdown();

public:	// Rigid body simulation

	//pxRigidBody::Handle AddRigidBody( const pxRigidBodyInfo& cInfo );
	//void FreeRigidBody( pxRigidBody::Handle hRigidBody );
	//pxRigidBody& GetRigidBody( pxRigidBody::Handle hRigidBody );
	//UINT NumRigidBodies() const;

	/// removes all bodies from this world
	void clear();

	void Tick(
		pxReal deltaTime,
		pxReal fixedStepSize = PX_DEFAULT_FIXED_STEP_SIZE,
		int maxSubSteps = 1
	);

	void SetGravity( const V3f& newGravity );
	V3f& GetGravity();


	// Collision detection

	//pxCollideable::Handle AddCollisionObject( const pxCollideableInfo& cInfo );
	//void FreeCollisionObject( pxCollideable::Handle hCollisionObject );
	//pxCollideable& GetCollisionObject( pxCollideable::Handle hCollisionObject );

	//void CastRay( const WorldRayCastInput& input, WorldRayCastOutput &output );


	//void LinearCast( const ConvexCastInput& input, ConvexCastOutput &output );

	//void TraceBox( const TraceBoxInput& input, TraceBoxOutput &output );



	//pxConstraintSolver* GetSolver() {return m_constraintSolver;}

	//void DebugDraw( pxDebugDrawer* renderer );

	struct FrameStats
	{
		UINT	num_fixed_timesteps;
		UINT	num_contacts;
	};

public_internal:
	void Serialize( mxArchive& archive );

private:
	/// advances simulation forward by the given amount of time
	void TickInternal( const pxReal _deltaTime );

	/// predict unconstrained motion: apply external forces, advance positions
	void PredictPositions( const pxReal _deltaTime );

	/// perform collision detection and generate contacts
	void DetectCollisions( const pxReal _deltaTime );

	void StabilizeContacts( const pxReal _deltaTime );

	/// Here position constraints should be satisfied
	void CorrectPositions( const pxReal _deltaTime );
	/// Get velocities from position
	void UpdateVelocities( const pxReal _deltaTime );
	/// Solve velocity constraints
	void CorrectVelocities( const pxReal _deltaTime );

//private:
public:
	// settings
	V3f	m_gravityAcceleration;	//!< All bodies are influenced by gravity.
	float	m_collisionMargin;
	float	m_contactThreshold;
	UINT	m_preStabilizationIterations;
	UINT	m_positionSolverIterations;
	UINT	m_velocitySolverIterations;

	pxReal	m_timeAccumulator;	// delta time accumulated across previous frames


	TArray< Atom >	m_particles;


	/// collision objects
	//TArray< Collidable >		m_collideables;

	/// non-sleeping bodies
	//TArray< RigidBodyState >	m_rigidBodyStates;

	//TPtr< pxBroadphase >			m_collisionBroadphase;
	CollisionDispatcher		m_collisionDispatcher;
	//pxContactCache					m_contactCache;
	//TPtr< pxConstraintSolver >		m_constraintSolver;
	ContactsArray			m_contacts;

	TArray< ContactContraint >	m_particleContacts;

	FrameStats	m_stats;	//!< last frame stats
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//pxRigidBody::Handle pxUtil_AddStaticPlane( PhysicsSimulator* physicsWorld, const Plane3D& plane );
//pxRigidBody::Handle pxUtil_AddSphere( PhysicsSimulator* physicsWorld, FLOAT radius, FLOAT density = 1.0f );
#endif
#endif // !__PX_PHYSICS_WORLD_H__

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
