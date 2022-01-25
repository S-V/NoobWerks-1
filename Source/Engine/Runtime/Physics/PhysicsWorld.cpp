#include <Base/Base.h>
#pragma hdrstop

#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>


NwPhysicsWorld::NwPhysicsWorld()
	: m_dispatcher( &m_collisionConfiguration )
	, m_dynamicsWorld( &m_dispatcher, &m_broadphase, &m_constraintSolver, &m_collisionConfiguration )
{
	m_fixedTimeStep = 1.0f / 60.0f;	// 60 Hertz
	//m_dynamicsWorld.setGravity(btVector3(0, 0, 0.0f));
	//m_dynamicsWorld.setGravity(btVector3(0, 0, -9.8f));
	m_dynamicsWorld.setGravity(btVector3(0, 0, -10.0f));
	//m_dynamicsWorld.setGravity(btVector3(0, 0, -1.2f));
}

void NwPhysicsWorld::Update(
						  float delta_seconds
						  )
{
	//PROFILE;
	m_dynamicsWorld.stepSimulation(
		delta_seconds
		, 16 /*maxSubSteps*/
		, m_fixedTimeStep
		);
}

ERet NwPhysicsWorld::Initialize( NwClump * storage_clump )
{
	_storage_clump = storage_clump;
	mxDO(voxel_terrain_collision_system.Initialize(storage_clump));
	return ALL_OK;
}

void NwPhysicsWorld::Shutdown( NwClump & _scene )
{
	this->removePhysicsObjectsFromClump( _scene );
	voxel_terrain_collision_system.Shutdown();
	_storage_clump = nil;
}

void NwPhysicsWorld::DeleteRigidBody( NwRigidBody* old_rigid_body )
{
	m_dynamicsWorld.removeCollisionObject( &old_rigid_body->bt_rb() );

	old_rigid_body->bt_rb().~btRigidBody();
	old_rigid_body->bt_ColShape().~btCollisionShape();

	//
	_storage_clump->delete_(old_rigid_body);
}

void NwPhysicsWorld::destroyRigidBody( TbRigidBody* old_rigid_body )
{
	if( old_rigid_body->m_rb.IsValid() ) {
		m_dynamicsWorld.removeRigidBody( old_rigid_body->m_rb );
	}
	old_rigid_body->Destroy();
}

void NwPhysicsWorld::removePhysicsObjectsFromClump( NwClump & _scene )
{
	{
		NwClump::Iterator< CollisionObject >	colObjIt( _scene );
		while( colObjIt.IsValid() )
		{
			CollisionObject & colObj = colObjIt.Value();

			m_dynamicsWorld.removeCollisionObject( &colObj.m_colObj );

			colObjIt.MoveToNext();
		}
		_scene.destroyAllObjectsOfType(CollisionObject::metaClass());
	}

	{
		NwClump::Iterator< NwCollisionObject >	colObjIt( _scene );
		while( colObjIt.IsValid() )
		{
			NwCollisionObject & colObj = colObjIt.Value();

			m_dynamicsWorld.removeCollisionObject( &colObj.bt_colobj() );

			colObjIt.MoveToNext();
		}
		_scene.destroyAllObjectsOfType(NwCollisionObject::metaClass());
	}

	{
		NwClump::Iterator< TbRigidBody >	rigidBodyIt( _scene );
		while( rigidBodyIt.IsValid() )
		{
			TbRigidBody & rigidBody = rigidBodyIt.Value();

			this->destroyRigidBody( &rigidBody );

			rigidBodyIt.MoveToNext();
		}
		_scene.destroyAllObjectsOfType(TbRigidBody::metaClass());
	}
}

ERet NwPhysicsWorld::CreateStaticObject(
	U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
	const TSpan< Triangle32 >& _triangles,
	const V3f& posInWorldSpace,
	const AABBf& _localBounds,
	const float uniform_scale_local_to_world,
	NwClump & _scene,
	CollisionObject *&_newColObject
	)
{
	CollisionObject *	new_collision_object = nil;
	mxDO(_scene.New( new_collision_object, CollisionObject::ALLOC_GRANULARITY ));

	mxDO(new_collision_object->Initialize( _vertexCount, _vertices, _vertexStride, _triangles, _localBounds, uniform_scale_local_to_world ));

	//
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3( posInWorldSpace.x, posInWorldSpace.y, posInWorldSpace.z ));

	new_collision_object->m_colObj.setWorldTransform( transform );

	//
	m_dynamicsWorld.addCollisionObject( &(new_collision_object->m_colObj) );

	_newColObject = new_collision_object;

	return ALL_OK;
}

ERet NwPhysicsWorld::DestroyObject(
	CollisionObject * _colObj
	, NwClump & _scene
)
{
	m_dynamicsWorld.removeCollisionObject( &_colObj->m_colObj );
	mxDO(_scene.Free( _colObj ));
	return ALL_OK;
}

ERet NwPhysicsWorld::createRigidSphere(
									   TbRigidBody **new_rigid_body_
									   , const V3f& _initialPosition
									   , const float _radius
									   , NwClump * _scene
									   , const float _mass
									   , const V3f& _initialVelocity
									   )
{
	TbRigidBody *	newRigidBody = nil;
	mxDO(_scene->New( newRigidBody, TbRigidBody::ALLOC_GRANULARITY ));

	{
		newRigidBody->m_shape = new(newRigidBody->m_shapeBuf) btSphereShape(_radius);

		btVector3	localInertia;
		newRigidBody->m_shape->calculateLocalInertia(_mass, localInertia);

		newRigidBody->m_rb = new(newRigidBody->m_rbBuf) btRigidBody(
			_mass,
			&(newRigidBody->m_motionState),
			newRigidBody->m_shape,
			localInertia
		);

		newRigidBody->m_rb->setCollisionShape( newRigidBody->m_shape );
	}

	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(toBulletVec( _initialPosition ));

		newRigidBody->m_rb->setWorldTransform( transform );

		newRigidBody->m_rb->setLinearVelocity(toBulletVec(_initialVelocity));
	}

	//newRigidBody->m_rb->setFriction(0.5f);
	newRigidBody->m_rb->setFriction(0.9f);
	newRigidBody->m_rb->setRestitution(0.1f);

	//
	newRigidBody->m_rb->setUserPointer( newRigidBody );

	//
	m_dynamicsWorld.addRigidBody(
		newRigidBody->m_rb
	);

	*new_rigid_body_ = newRigidBody;

	return ALL_OK;
}

ERet NwPhysicsWorld::spawnRigidBox(
	const V3f& initial_position_in_world_space
	, const V3f& collision_box_shape_half_size
	, RrTransform * graphics_engine_transform
	, const int culling_proxy
	, NwClump & scene_data_storage
	, const float mass
	, const V3f& initial_velocity_in_world_space
	, const M44f& graphics_vertices_transform /*= g_MM_Identity*/
	)
{
	TbRigidBody *	newRigidBody = nil;
	mxDO(scene_data_storage.New( newRigidBody, TbRigidBody::ALLOC_GRANULARITY ));

	{
		newRigidBody->m_shape = new(newRigidBody->m_shapeBuf) btBoxShape(toBulletVec(collision_box_shape_half_size));

		btVector3	localInertia;
		newRigidBody->m_shape->calculateLocalInertia(mass, localInertia);

		newRigidBody->m_rb = new(newRigidBody->m_rbBuf) btRigidBody(
			mass,
			&(newRigidBody->m_motionState),
			newRigidBody->m_shape,
			localInertia
		);

		newRigidBody->m_rb->setCollisionShape( newRigidBody->m_shape );
	}

	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3( initial_position_in_world_space.x, initial_position_in_world_space.y, initial_position_in_world_space.z ));

		newRigidBody->m_rb->setWorldTransform( transform );

		newRigidBody->m_rb->setLinearVelocity(btVector3(initial_velocity_in_world_space.x, initial_velocity_in_world_space.y, initial_velocity_in_world_space.z));
		newRigidBody->m_rb->setAngularVelocity(btVector3(0.3f,0.0f,0.2f));
	}

	newRigidBody->m_rb->setFriction(0.9f);
	newRigidBody->m_rb->setRestitution(0.1f);
UNDONE;
//	newRigidBody->m_graphicsTransform = graphics_engine_transform;
//	newRigidBody->_graphics_vertices_transform = graphics_vertices_transform;

	//newRigidBody->m_rb->setDamping(0,0);

	m_dynamicsWorld.addRigidBody(
		newRigidBody->m_rb
	);

	return ALL_OK;
}
