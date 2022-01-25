/*
=============================================================================
	File:	PhysicsSimulator.cpp
	Desc:
=============================================================================
*/
//#include <Physics/Physics_PCH.h>
#include <Base/Base.h>
#pragma hdrstop
#if 0
//#include <Physics.h>
#include <Physics/Physics_Simulator.h>
//#include <Physics/Collide/Shape/pxShape_Convex.h>
//#include <Physics/Collide/Shape/pxShape_HalfSpace.h>
//#include <Physics/Collide/Shape/pxShape_Sphere.h>
//#include <Physics/Collide/Shape/pxShape_Box.h>
//#include <Physics/Collide/Shape/pxShape_StaticBSP.h>

/*
-----------------------------------------------------------------------------
	PhysicsSimulator
-----------------------------------------------------------------------------
*/
PhysicsSimulator::CreationInfo::CreationInfo()
{
	//broadphase = nil;
	//dispatcher = Physics::GetCollisionDispatcher();
	//constraintSolver = nil;
	gravity = V3f::set( 0, 0, -Physik::EARTH_GRAVITY );
	pre_stabilization_iterations = 2;
	position_solver_iterations = 3;
	velocity_solver_iterations = 2;
}

PhysicsSimulator::PhysicsSimulator()
{
	m_collisionDispatcher.Initialize();
	//m_collisionBroadphase = worldDesc.broadphase;
	//m_collisionDispatcher = worldDesc.dispatcher;
	//m_constraintSolver = worldDesc.constraintSolver;
}

PhysicsSimulator::~PhysicsSimulator()
{
}

ERet PhysicsSimulator::Initialize( const CreationInfo& worldDesc )
{
	m_gravityAcceleration = worldDesc.gravity;
	m_timeAccumulator = 0.0f;
	m_preStabilizationIterations = worldDesc.pre_stabilization_iterations;
	m_positionSolverIterations = worldDesc.position_solver_iterations;
	m_velocitySolverIterations = worldDesc.velocity_solver_iterations;
	m_collisionMargin = COLLISION_MARGIN;
	m_contactThreshold = CONTACT_THRESHOLD;
	return ALL_OK;
}
void PhysicsSimulator::Shutdown()
{

}
//
//pxCollideable::Handle PhysicsSimulator::AddCollisionObject( const pxCollideableInfo& cInfo )
//{
//	const pxCollideable::Handle hNewCollideable = m_collideables.add();
//
//	pxCollideable& rNewCollideable = m_rigidBodies.GetPtrByHandle( hNewCollideable );
//	rNewCollideable._ctor( cInfo );
//
//	m_collisionBroadphase->add( &rNewCollideable );
//
//	return hNewCollideable;
//}
//
//void PhysicsSimulator::FreeCollisionObject( pxCollideable::Handle hCollisionObject )
//{
//	UNDONE;
//}
//
//pxCollideable& PhysicsSimulator::GetCollisionObject( pxCollideable::Handle hCollisionObject )
//{
//	UNDONE;
//}
//
//void PhysicsSimulator::CastRay( const WorldRayCastInput& input, WorldRayCastOutput &output )
//{
//	Unimplemented;
//}

//void PhysicsSimulator::LinearCast( const ConvexCastInput& input, ConvexCastOutput &output )
//{
//	m_collisionBroadphase->LinearCast( input, output );
////}
//
//void PhysicsSimulator::TraceBox( const TraceBoxInput& input, TraceBoxOutput &output )
//{
//	m_collisionBroadphase->TraceBox( input, output );
//}
//
//pxRigidBody::Handle PhysicsSimulator::AddRigidBody( const pxRigidBodyInfo& cInfo )
//{
//	const pxRigidBody::Handle hNewRigidBody = m_rigidBodies.add();
//
//	pxRigidBody* pNewRigidBody = m_rigidBodies.GetPtrByHandle( hNewRigidBody );
//	pNewRigidBody->_ctor( cInfo );
//
//	m_collisionBroadphase->add( pNewRigidBody );
//
//	return hNewRigidBody;
//}
//
//void PhysicsSimulator::FreeRigidBody( pxRigidBody::Handle hRigidBody )
//{
//	UNDONE;	//see pxSimpleBroadphaseProxy::o - invalid when they are moved
//	// and collision agents, etc.
//
//	//pxRigidBody& rRigidBody = m_rigidBodies.GetPtrByHandle( hRigidBody );
//
//	//m_collisionBroadphase->Remove( &rRigidBody );
//
//	//m_rigidBodies.Remove( hRigidBody );
//}
//
//pxRigidBody& PhysicsSimulator::GetRigidBody( pxRigidBody::Handle hRigidBody )
//{
//	return *m_rigidBodies.GetPtrByHandle( hRigidBody );
//}
//
//UINT PhysicsSimulator::NumRigidBodies() const
//{
//	return m_rigidBodies.num();
//}

void PhysicsSimulator::Tick( pxReal deltaTime, pxReal fixedStepSize, int maxSubSteps )
{
	mxASSERT(deltaTime>=0.0f);
	mxASSERT(maxSubSteps > 0);

	// it's recommended to use fixed time step for better accuracy/stability
	bool bUseFixedTimeStep = 1;

	if( bUseFixedTimeStep )
	{
		int numSubSteps = 0;	// number of fixed simulation time steps

		m_timeAccumulator += deltaTime;

		if( m_timeAccumulator >= fixedStepSize )
		{
			numSubSteps = m_timeAccumulator / fixedStepSize;
			m_timeAccumulator -= numSubSteps * fixedStepSize;
		}

		if( numSubSteps > 0 )
		{
			//clamp the number of substeps, to prevent simulation grinding spiralling down to a halt
			numSubSteps = smallest( numSubSteps, maxSubSteps );

			for( int iFixedSubStep = 0; iFixedSubStep < numSubSteps; iFixedSubStep++ )
			{
				this->TickInternal( fixedStepSize );
			}

			m_stats.num_fixed_timesteps = numSubSteps;
		}
		else
		{
			// interpolate
			PredictPositions( deltaTime );//this->IntegrateTransforms( deltaTime );
		}

		//DBGOUT("%d substeps, dt = %.3f\n",numSubSteps,deltaTime);
	}
	else
	{
		// variable time step
		this->TickInternal( deltaTime );
	}
}

void PhysicsSimulator::TickInternal( const pxReal _deltaTime )
{
	PROFILE;

	// 1. Predict positions: P := X1 + V1 * dt
	// 2. Correct positions: X2 := modify( P )
	// 3. Update velocities: V := (X2 - X1) / dt
	// 4. Correct velocities: V2 := modify( V )


	// predict unconstrained motion: apply external forces, integrate velocities
	PredictPositions( _deltaTime );

	// perform collision detection and generate contacts
	DetectCollisions( _deltaTime );

	// Pre-Stabilization
	StabilizeContacts( _deltaTime );

	// project position constraints
	CorrectPositions( _deltaTime );

	// compute velocities from position deltas; apply friction and damping
	UpdateVelocities( _deltaTime );

	// solve velocity constraints
	CorrectVelocities( _deltaTime );

	m_stats.num_contacts = m_contacts.num();
	m_contacts.RemoveAll();
}

static inline M44f Local_To_World_Transform( const RigidBodyState& _state )
{
	return M44_buildTranslationMatrix( _state.next_position );
}

/// apply external forces (such as gravity), integrate velocities and predict positions
void PhysicsSimulator::PredictPositions( const pxReal _deltaTime )
{
	for( UINT iAtom = 0; iAtom < m_particles.num(); iAtom++ )
	{
		Atom & atom = m_particles[ iAtom ];

		// All bodies are influenced by gravity.
		atom.velocity += m_gravityAcceleration * _deltaTime;	// integrate velocities

		// damp velocities
		const float air_damping = 0.99f;	// a viscous force proportional and opposed to the velocity of each particle

		// integrate transforms, predict positions
		atom.next_position = atom.position + atom.velocity * (air_damping * _deltaTime);

// collision with ground, Z=0
atom.next_position.z = maxf( atom.radius, atom.next_position.z );
	}

#if 0
	const UINT numRigidBodies = m_rigidBodyStates.num();
	RigidBodyState* rigidBodies = m_rigidBodyStates.raw();

	for( UINT iBody = 0; iBody < numRigidBodies; iBody++ )
	{
		RigidBodyState & body = rigidBodies[ iBody ];

		// All bodies are influenced by gravity.
		body.current_velocity += m_gravityAcceleration * _deltaTime;	// integrate velocities

		// damp velocities
		const float air_damping = 0.99f;	// a viscous force proportional and opposed to the velocity of each particle

		// integrate transforms, predict positions
		body.next_position = body.current_position + body.current_velocity * (air_damping * _deltaTime);
	}
#endif
}

void PhysicsSimulator::DetectCollisions( const pxReal _deltaTime )
{
	for( UINT i=0; i < m_particles.num()-1; i++ )
	{
		const Atom& atomA = m_particles[ i ];

		for( int j=i+1; j < m_particles.num(); j++ )
		{
			const Atom& atomB = m_particles[ j ];

			// squared distance between the centers
			const float distanceSquared = V3_LengthSquared( atomA.position - atomB.position );
			if( distanceSquared < Square(atomA.radius + atomB.radius) )
			{
				ContactContraint newContact;
				newContact.iAtomA = i;
				newContact.iAtomB = j;
				m_particleContacts.add( newContact );
			}
		}
	}

#if 0
	const UINT totalNumObjects = m_collideables.num();
	for( UINT i=0; i < totalNumObjects-1; i++ )
	{
		const Collidable& objA = m_collideables[ i ];
		const RigidBodyState& stateA = m_rigidBodyStates[ objA.iBody ];
		const M44f transformA = M44_buildTranslationMatrix( stateA.next_position );

		for( int j=i+1; j < totalNumObjects; j++ )
		{
			const Collidable& objB = m_collideables[ j ];
			const RigidBodyState& stateB = m_rigidBodyStates[ objB.iBody ];
			const M44f transformB = M44_buildTranslationMatrix( stateB.next_position );

			m_collisionDispatcher.Collide( i, j, objA, objB, transformA, transformB, m_contacts );
		}
	}
#endif
}

const float over_relaxation = 1.5f;	// [1..2]

void PhysicsSimulator::StabilizeContacts( const pxReal _deltaTime )
{
#if 0
	DynamicArray< V3f >	deltaPositions(Memory::Scratch());
	deltaPositions.setNum(m_particles.num());

	for( UINT iteration = 0; iteration < m_preStabilizationIterations; iteration++ )
	{
		for( UINT iBody = 0; iBody < m_particles.num(); iBody++ )
		{
			RigidBodyState & body = m_particles[ iBody ];
			body.delta_x = V3_Zero();
		}

		for( UINT iContact = 0; iContact < m_particleContacts.num(); iContact++ )
		{
			const ContactContraint& contact = m_contacts[ iContact ];

			Atom & __restrict atomA = m_particles[ contact.iAtomA ];
			Atom & __restrict atomB = m_particles[ contact.iAtomB ];

			const float distanceSquared = V3_LengthSquared( atomA - atomB );
			const float summed_radii = atomA.radius + atomB.radius;
			if( distanceSquared < Square( summed_radii ) )
			{
				const float diff = mmSqrt(distanceSquared) - summed_radii;

				const float inv_w1_plus_w2 = 1.0f / (atomA.inverse_mass + atomB.inverse_mass);

				const V3f deltaPosA = -N * (atomA.inverse_mass * inv_w1_plus_w2) * diff;
				const V3f deltaPosB = +N * (atomB.inverse_mass * inv_w1_plus_w2) * diff;

				atomA.next_position += deltaPosA;
				atomB.next_position += deltaPosB;
			}
		}

		for( UINT iBody = 0; iBody < m_particles.num();; iBody++ )
		{
			Atom & atom = m_particles[ iBody ];
			body.current_position += body.delta_x;
			body.next_position += body.delta_x;
		}
	}
#endif

#if 0
struct ContactInfo
{
	//
};
	DynamicArray< ContactInfo >	deltaPositions(Memory::Scratch());
	deltaPositions.setNum(m_particles.num());

	for( UINT iteration = 0; iteration < m_preStabilizationIterations; iteration++ )
	{
		for( UINT iBody = 0; iBody < m_particles.num(); iBody++ )
		{
			RigidBodyState & body = m_particles[ iBody ];
			body.delta_x = V3_Zero();
		}

		for( UINT iContact = 0; iContact < m_contacts.num(); iContact++ )
		{
			const Contact& contact = m_contacts[ iContact ];
			// Enforce collision constraint
			// Collisions are unilateral constraints, i.e. constraints

			const Collidable& __restrict oA = m_collideables[ contact.bodyA ];
			const Collidable& __restrict oB = m_collideables[ contact.bodyB ];

			RigidBodyState & __restrict rbA = m_rigidBodyStates[ oA.iBody ];
			RigidBodyState & __restrict rbB = m_rigidBodyStates[ oB.iBody ];

			const V3f contact_on_A = contact.position_on_B + contact.normal_on_B * contact.penetration_depth;
			const V3f contact_on_B = contact.position_on_B;

			// project the position along the gradient of the constraint function
			const V3f N = contact.normal_on_B;

			// By convention, the contact normal points from B to A,
			// i.e. body A should be moved in the normal direction.

			//const F32 len = V3_Length(rbA.current_position - rbB.current_position);
			const F32 distance = contact.penetration_depth;
			const F32 rest_length = m_contactThreshold;
			const float diff = distance - rest_length;
			if( diff < 0 )
			{
				const float w1 = rbA.inverse_mass;
				const float w2 = rbB.inverse_mass;
				const float inv_w1_plus_w2 = 1.0f / (w1 + w2);

				const V3f deltaPosA = -N * (w1 * inv_w1_plus_w2) * diff;
				const V3f deltaPosB = +N * (w2 * inv_w1_plus_w2) * diff;

				const float overrel = over_relaxation / 1;

				rbA.delta_x += deltaPosA * overrel;
				rbB.delta_x += deltaPosB * overrel;
			}
		}

		for( UINT iBody = 0; iBody < numRigidBodies; iBody++ )
		{
			RigidBodyState & body = rigidBodies[ iBody ];
			body.current_position += body.delta_x;
			body.next_position += body.delta_x;
		}
	}
#endif
}

// The system is solved at each simulation time step by relaxation; that is by
// enforcing the constraints one after the other for a given number of iterations.
void PhysicsSimulator::CorrectPositions( const pxReal _deltaTime )
{
	for( UINT iteration = 0; iteration < m_preStabilizationIterations; iteration++ )
	{
		for( UINT iContact = 0; iContact < m_particleContacts.num(); iContact++ )
		{
			const ContactContraint& contact = m_particleContacts[ iContact ];

			Atom & atomA = m_particles[ contact.iAtomA ];
			Atom & atomB = m_particles[ contact.iAtomB ];

			const V3f diff = atomA.position - atomB.position;
			const float distanceSquared = V3_LengthSquared( diff );
			const float summed_radii = atomA.radius + atomB.radius;
			if( distanceSquared < Square( summed_radii ) )
			{
				const float length = mmSqrt(distanceSquared);
				const V3f normal = diff / length;

				const float scale = length - summed_radii;

				const float inv_w1_plus_w2 = 1.0f / (atomA.inverse_mass + atomB.inverse_mass);

				const V3f deltaPosA = -normal * (scale * atomA.inverse_mass * inv_w1_plus_w2);
				const V3f deltaPosB = +normal * (scale * atomB.inverse_mass * inv_w1_plus_w2);

				atomA.next_position += deltaPosA;
				atomB.next_position += deltaPosB;
			}
		}
	}

#if 0
	// For every relaxation step...
	for( UINT iteration = 0; iteration < m_positionSolverIterations; iteration++ )
	{
		// NOTE: Gauss Seidel is order dependent.
		// Every constraint that gets solved is affected by the constraints that follow,
		// while the last constraint remains unaffected. Last constraints are strongest.
		for( UINT iContact = 0; iContact < m_contacts.num(); iContact++ )
		{
			const Contact& contact = m_contacts[ iContact ];
			// Enforce collision constraint
			// Collisions are unilateral constraints, i.e. constraints

			const Collidable& oA = m_collideables[ contact.bodyA ];
			const Collidable& oB = m_collideables[ contact.bodyB ];

			RigidBodyState & rbA = m_rigidBodyStates[ oA.iBody ];
			RigidBodyState & rbB = m_rigidBodyStates[ oB.iBody ];


			const V3f contact_on_A = contact.position_on_B + contact.normal_on_B * contact.penetration_depth;
			const V3f contact_on_B = contact.position_on_B;

#if 0
			// positions of contact points with respect to body PORs
			const V3f rA( contact_on_A - rbA.current_position );
			const V3f rB( contact_on_B - rbB.current_position );

			const V3f rA_cross_N( V3_Cross( rA, contact.normal_on_B ) );	// rA x normal
			const V3f rB_cross_N( V3_Cross( rb, contact.normal_on_B ) );	// rB x normal
#endif
			const V3f relativeVelocity = rbA.current_velocity - rbB.current_velocity;	// relative to B
			// The relative velocity of the two bodies projected onto the contact normal:
			const float velocityProjection = contact.normal_on_B * relativeVelocity;
			// If the projected relative velocity is positive, the bodies are moving apart.
			// Vrel = 0 => resting contact
			// Vrel < 0 => colliding contact



			//                 C(p)
			// lambda = - ________________
			//             |grad(C(p))|^2

			// project the position along the gradient of the constraint function

			//const V3f N = V3_Normalized(rbA.current_position - rbB.current_position);
			const V3f N = contact.normal_on_B;

			// By convention, the contact normal points from B to A,
			// i.e. body A should be moved in the normal direction.

			//const F32 len = V3_Length(rbA.current_position - rbB.current_position);
			const F32 distance = contact.penetration_depth;
			const F32 rest_length = m_contactThreshold;

			const float diff = distance - rest_length;
			if( diff < 0 )
			{
				const float w1 = rbA.inverse_mass;
				const float w2 = rbB.inverse_mass;
				const float temp0 = diff / (w1 + w2);

				const V3f deltaPosA = -N * (w1 * temp0);
				const V3f deltaPosB = +N * (w2 * temp0);

				const float overrel = over_relaxation / 1;

				rbA.next_position += deltaPosA * overrel;
				rbB.next_position += deltaPosB * overrel;
			}
		}
	}
#endif
}

void PhysicsSimulator::UpdateVelocities( const pxReal _deltaTime )
{
	const pxReal inverseDeltaTime = pxReal(1) / _deltaTime;

	for( UINT iAtom = 0; iAtom < m_particles.num(); iAtom++ )
	{
		Atom & atom = m_particles[ iAtom ];
		atom.velocity = (atom.next_position - atom.position) * inverseDeltaTime;
		atom.position = atom.next_position;
	}

#if 0
	const UINT numRigidBodies = m_rigidBodyStates.num();
	RigidBodyState* rigidBodies = m_rigidBodyStates.raw();

	for( UINT iBody = 0; iBody < numRigidBodies; iBody++ )
	{
		RigidBodyState & body = rigidBodies[ iBody ];
		body.current_velocity = (body.next_position - body.current_position) * inverseDeltaTime;
		body.current_position = body.next_position;
		body.next_position = V3_Zero();
	}
#endif
}





// After all collisions are processed, only the velocities of the rigid bodies are updated
void PhysicsSimulator::CorrectVelocities( const pxReal _deltaTime )
{
#if 0
	DynamicArray< ContactInfo >	contactInfos(Memory::Scratch());
	contactInfos.setNum(m_contacts.num());


	for( UINT iContact = 0; iContact < m_contacts.num(); iContact++ )
	{
		const Contact& contact = m_contacts[ iContact ];
		ContactInfo & ci = contactInfos[ iContact ];
	}

	for( UINT iteration = 0; iteration < m_velocitySolverIterations; iteration++ )
	{
		for( UINT iContact = 0; iContact < m_contacts.num(); iContact++ )
		{
			const Contact& contact = m_contacts[ iContact ];

			const Collidable& oA = m_collideables[ contact.bodyA ];
			const Collidable& oB = m_collideables[ contact.bodyB ];

			RigidBodyState & rbA = m_rigidBodyStates[ oA.iBody ];
			RigidBodyState & rbB = m_rigidBodyStates[ oB.iBody ];

			// By convention, a contact normal is always directed from the second body to the first one.
			const V3f contact_on_B = contact.position_on_B;
			const V3f contact_on_A = contact.position_on_B + contact.normal_on_B * contact.penetration_depth;			

#if 0
			// positions of contact points with respect to body PORs
			const V3f rA( contact_on_A - rbA.current_position );
			const V3f rB( contact_on_B - rbB.current_position );

			const V3f rA_cross_N( V3_Cross( rA, contact.normal_on_B ) );	// rA x normal
			const V3f rB_cross_N( V3_Cross( rb, contact.normal_on_B ) );	// rB x normal
#endif

			// The parameter 'COLLISION_THRESHOLD' is a small numerical tolerance used for deciding if bodies are colliding.
			const float COLLISION_THRESHOLD = -1e-4f;


			// j is the magnitude of the collision impulse
			// e is the coefficient of restitution [0,1]
			// float j = max( - ( 1 + e ) * d, 0 );


			//Here M is a 3x3 matrix dependent only upon the masses and mass matrices of the
			//colliding bodies and the locations of the contact points relative to their centers of mass.
			//By our infinitesimal collision time assumption M is constant over the entire collision.



			const V3f relativeVelocity = rbA.current_velocity - rbB.current_velocity;	// relative to B
			// The relative velocity of the two contact points projected onto the contact normal:
			const float velocityProjection = contact.normal_on_B * relativeVelocity;
			// If the projected relative velocity is positive, the bodies are moving away from each other.
			// |Vrel| < epsilon => colliding contact
			// |Vrel| = epsilon => resting contact




			//coefficient of restitution governs bounciness
			const float restitution = 1;// coefficient of restitution

			// compute goal velocity in normal direction after collision
			const float targetNormalVelocity =
				( velocityProjection < COLLISION_THRESHOLD ) ? (-velocityProjection * restitution) : 0.0f;


			// For resting contact: -Vrel < sqrt( 2 * gravity_acceleration * collision_tolerance ).

			// Brian Mirtich, John Canny. Impulse-based Dynamic Simulation [1994]
			// Brian Vincent Mirtich. Impulse-based dynamic simulation of rigid body systems. PhD thesis, University of California, Berkeley, 1996.
			// Constraint-based collision and contact handling using impulses [2006]




#if 0
			if( velocityProjection < COLLISION_THRESHOLD )
			{
				// a negative normal velocity corresponds to approaching primitives
				// colliding contact
				rbA.current_velocity += contact.current_velocity * velocityProjection * rbA.inverse_mass;
				rbB.current_velocity -= contact.current_velocity * velocityProjection * rbB.inverse_mass;
			}

			float p = velocityProjection * 0.5f;
			V3f pv = -contact.normal_on_B * p;

			rbA.current_velocity += pv * rbA.inverse_mass;
			rbB.current_velocity -= pv * rbB.inverse_mass;
#endif

			// Apply impulses to colliding objects at point of collision causing instantaneous change in velocities
		}
	}
#endif
}
#endif
//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
