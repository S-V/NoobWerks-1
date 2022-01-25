#pragma once

#include <Physics/Bullet_Wrapper.h>
#include <Physics/Collision/VoxelTerrainCollisions.h>

struct NwRigidBody;


///
class NwPhysicsWorld
{
public_internal:
	btDbvtBroadphase					m_broadphase;
	btDefaultCollisionConfiguration		m_collisionConfiguration;
	btCollisionDispatcher				m_dispatcher;
	btSequentialImpulseConstraintSolver	m_constraintSolver;
	btDiscreteDynamicsWorld				m_dynamicsWorld;
	float								m_fixedTimeStep;

	NwVoxelTerrainCollisionSystem		voxel_terrain_collision_system;

	TPtr< NwClump >		_storage_clump;

public:
	NwPhysicsWorld();

	ERet Initialize( NwClump * storage_clump );
	void Shutdown( NwClump & _scene );


	void Update(
		float delta_seconds
		);

public:
	void DeleteRigidBody( NwRigidBody* old_rigid_body );

	mxDEPRECATED
	void destroyRigidBody( TbRigidBody* old_rigid_body );

	void removePhysicsObjectsFromClump( NwClump & _scene );

public:

	ERet CreateStaticObject(
		U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
		const TSpan< Triangle32 >& _triangles,
		const V3f& _initialPosition,	// in world space
		const AABBf& _localBounds,	// object-space bounding box
		const float uniform_scale_local_to_world,
		NwClump & _scene,
		CollisionObject *&_newColObject
		);

	ERet DestroyObject(
		CollisionObject * _colObj
		, NwClump & _scene
	);

public:

	ERet createRigidSphere(
		TbRigidBody **new_rigid_body_
		, const V3f& _initialPosition
		, const float _radius
		, NwClump * _scene
		, const float _mass = 1.0f
		, const V3f& _initialVelocity = V3f::zero()
		);

	ERet spawnRigidBox(
		const V3f& initial_position_in_world_space
		, const V3f& collision_box_shape_half_size
		, RrTransform * graphics_engine_transform
		, const int culling_proxy
		, NwClump & scene_data_storage
		, const float mass
		, const V3f& initial_velocity_in_world_space = V3f::zero()
		, const M44f& graphics_vertices_transform = g_MM_Identity
		);
};


//
class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
	btCollisionObject* me;

public:
	ClosestNotMeRayResultCallback(
		const btVector3& rayFromWorld, const btVector3& rayToWorld,
		btCollisionObject* me
		)
		: btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld)
		, me(me)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == me) {
			return 1.0f;
		}
		if (rayResult.m_collisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
			return 1.0f;
		}
		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}
};
