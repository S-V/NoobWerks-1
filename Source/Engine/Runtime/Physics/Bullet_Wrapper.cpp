#include <Base/Base.h>
#pragma hdrstop

#include <algorithm>

#include <Graphics/Public/graphics_utilities.h>

#include <Rendering/Public/Core/Mesh.h>

#include <Physics/Bullet_Wrapper.h>

enum {
	PHYSICS_GROUP_WORLD = BIT(0),
	PHYSICS_GROUP_ACTOR = BIT(1),
	PHYSICS_GROUP_PLAYER = BIT(2),

	PHYSICS_FILTER_ALL = PHYSICS_GROUP_WORLD | PHYSICS_GROUP_ACTOR | PHYSICS_GROUP_PLAYER,
	PHYSICS_FILTER_NOT_PLAYER = PHYSICS_FILTER_ALL & ~PHYSICS_GROUP_PLAYER,
};

StaticMeshShape::StaticMeshShape()
	: m_localScaling(btScalar(1.),btScalar(1.),btScalar(1.))
{
	m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
	m_margin = 0;
}

StaticMeshShape::~StaticMeshShape()
{

}

ERet StaticMeshShape::Initialize(
	U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
	const TSpan< Triangle32 >& _triangles,
	const AABBf& _localBounds
)
{
	mxDO(m_positions.setNum( _vertexCount ));
	for( UINT iVertex = 0; iVertex < _vertexCount; iVertex++ )
	{
		m_positions[ iVertex ] = *(V3f*) mxAddByteOffset( _vertices, iVertex * _vertexStride );
	}

	mxDO(m_triangles.setNum( _triangles.num() ));
	m_triangles.CopyFromArray( _triangles.raw(), _triangles.num() );

//	m_shape = new btBvhTriangleMeshShape();

	m_localBounds = _localBounds;

	return ALL_OK;
}

void StaticMeshShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	//const btVector3& center = t.getOrigin();
	//aabbMin = center + btVector3( m_localBounds.min_corner.x, m_localBounds.min_corner.y, m_localBounds.min_corner.z );
	//aabbMax = center + btVector3( m_localBounds.max_corner.x, m_localBounds.max_corner.y, m_localBounds.max_corner.z );

#if 0
	M44f m4x4;
	t.getOpenGLMatrix(m4x4.a);

	AABBf bb2 = m_localBounds.transformed(m4x4);

	aabbMin = toBulletVec( bb2.mins );
	aabbMax = toBulletVec( bb2.maxs );
#else

	// Copied from: btScaledBvhTriangleMeshShape::getAabb()

	btVector3 localAabbMin = toBulletVec( m_localBounds.min_corner );
	btVector3 localAabbMax = toBulletVec( m_localBounds.max_corner );

	btVector3 tmpLocalAabbMin = localAabbMin * m_localScaling;
	btVector3 tmpLocalAabbMax = localAabbMax * m_localScaling;

	localAabbMin[0] = (m_localScaling.getX() >= 0.) ? tmpLocalAabbMin[0] : tmpLocalAabbMax[0];
	localAabbMin[1] = (m_localScaling.getY() >= 0.) ? tmpLocalAabbMin[1] : tmpLocalAabbMax[1];
	localAabbMin[2] = (m_localScaling.getZ() >= 0.) ? tmpLocalAabbMin[2] : tmpLocalAabbMax[2];
	localAabbMax[0] = (m_localScaling.getX() <= 0.) ? tmpLocalAabbMin[0] : tmpLocalAabbMax[0];
	localAabbMax[1] = (m_localScaling.getY() <= 0.) ? tmpLocalAabbMin[1] : tmpLocalAabbMax[1];
	localAabbMax[2] = (m_localScaling.getZ() <= 0.) ? tmpLocalAabbMin[2] : tmpLocalAabbMax[2];

	btVector3 localHalfExtents = btScalar(0.5)*(localAabbMax-localAabbMin);
	btScalar margin = m_margin;
	localHalfExtents += btVector3(margin,margin,margin);
	btVector3 localCenter = btScalar(0.5)*(localAabbMax+localAabbMin);
	
	btMatrix3x3 abs_b = t.getBasis().absolute();

	btVector3 center = t(localCenter);

    btVector3 extent = localHalfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	aabbMin = center - extent;
	aabbMax = center + extent;
#endif
}

void StaticMeshShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& StaticMeshShape::getLocalScaling() const
{
	return m_localScaling;
}
void StaticMeshShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}
const char*	StaticMeshShape::getName()const
{
	return "Terrain";
}

void StaticMeshShape::processAllTriangles(
	btTriangleCallback* callback
	,const btVector3& aabbMin,const btVector3& aabbMax
) const
{
	btVector3 invLocalScaling(1.f/m_localScaling.getX(),1.f/m_localScaling.getY(),1.f/m_localScaling.getZ());

	btVector3 scaledAabbMin,scaledAabbMax;

	///support negative scaling
	scaledAabbMin[0] = m_localScaling.getX() >= 0. ? aabbMin[0] * invLocalScaling[0] : aabbMax[0] * invLocalScaling[0];
	scaledAabbMin[1] = m_localScaling.getY() >= 0. ? aabbMin[1] * invLocalScaling[1] : aabbMax[1] * invLocalScaling[1];
	scaledAabbMin[2] = m_localScaling.getZ() >= 0. ? aabbMin[2] * invLocalScaling[2] : aabbMax[2] * invLocalScaling[2];
	scaledAabbMin[3] = 0.f;
	
	scaledAabbMax[0] = m_localScaling.getX() <= 0. ? aabbMin[0] * invLocalScaling[0] : aabbMax[0] * invLocalScaling[0];
	scaledAabbMax[1] = m_localScaling.getY() <= 0. ? aabbMin[1] * invLocalScaling[1] : aabbMax[1] * invLocalScaling[1];
	scaledAabbMax[2] = m_localScaling.getZ() <= 0. ? aabbMin[2] * invLocalScaling[2] : aabbMax[2] * invLocalScaling[2];
	scaledAabbMax[3] = 0.f;

	//m_bvhTriMeshShape->processAllTriangles(&scaledCallback,scaledAabbMin,scaledAabbMax);

	for( UINT iTriangle = 0; iTriangle < m_triangles.num(); iTriangle++ )
	{
		const Triangle32& triangle = m_triangles[ iTriangle ];

		const V3f& a = m_positions[ triangle.a[0] ];
		const V3f& b = m_positions[ triangle.a[1] ];
		const V3f& c = m_positions[ triangle.a[2] ];

		/*const*/ btVector3	faceverts[3] = {
			btVector3( a.x, a.y, a.z ),
			btVector3( b.x, b.y, b.z ),
			btVector3( c.x, c.y, c.z ),
		};

		btVector3 newTriangle[3];
		newTriangle[0] = faceverts[0]*m_localScaling;
		newTriangle[1] = faceverts[1]*m_localScaling;
		newTriangle[2] = faceverts[2]*m_localScaling;

		callback->processTriangle( newTriangle, 0/*partId*/, iTriangle/*triangleIndex*/ );
	}
}

//=====================================================================

mxDEFINE_CLASS(CollisionObject);
mxBEGIN_REFLECTION(CollisionObject)
	//mxMEMBER_FIELD( flatShading ),
	//mxMEMBER_FIELD( showWireframe ),
mxEND_REFLECTION;

ERet CollisionObject::Initialize(
	U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
	const TSpan< Triangle32 >& _triangles,
	const AABBf& _localBounds,
	const float worldScale
)
{
//	m_shape = new btBvhTriangleMeshShape();
	mxDO(m_shape.Initialize( _vertexCount, _vertices, _vertexStride, _triangles, _localBounds ));

	m_shape.m_localScaling = toBulletVec(CV3f(worldScale));

	m_colObj.setCollisionShape( &m_shape );

	return ALL_OK;
}

///synchronizes world transform from user to physics
void TbRigidBodyMotionState::getWorldTransform( btTransform& centerOfMassWorldTrans_ ) const
{
	centerOfMassWorldTrans_ = m_rigidBodyLocalToWorld;
}

///synchronizes world transform from physics to user
///Bullet only calls the update of worldtransform for active objects
void TbRigidBodyMotionState::setWorldTransform( const btTransform& centerOfMassWorldTrans )
{
	m_rigidBodyLocalToWorld = centerOfMassWorldTrans;
}

mxDEFINE_CLASS(TbRigidBody);
mxBEGIN_REFLECTION(TbRigidBody)
	//mxMEMBER_FIELD( flatShading ),
	//mxMEMBER_FIELD( showWireframe ),
mxEND_REFLECTION;

TbRigidBody::TbRigidBody()
{
	type = PHYS_RIGID_BODY;
	ptr = nil;
}

void TbRigidBody::Destroy()
{
	if( m_shape.IsValid() )
	{
		m_shape->~btCollisionShape();
		m_shape = nil;
	}

	if( m_rb.IsValid() )
	{
		m_rb->~btRigidBody();
		m_rb = nil;
	}
}

const AABBf TbRigidBody::getAabbWorld() const
{
	btVector3	aabb_min;
	btVector3	aabb_max;
	this->bt_rb().getAabb( aabb_min, aabb_max );

	return AABBf::make(
		fromBulletVec(aabb_min)
		, fromBulletVec(aabb_max)
		);
}

//=====================================================================

TbBullePhysicsDebugDrawer::TbBullePhysicsDebugDrawer()
{
}

void TbBullePhysicsDebugDrawer::init(TbPrimitiveBatcher* renderer)
{
	_prim_renderer = renderer;
}

void TbBullePhysicsDebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
{
	const RGBAf rgba = RGBAf::make(color.x(), color.y(), color.z());
	//_dbgview->addLine(fromBulletVec(from), fromBulletVec(to), rgba);
	_prim_renderer->DrawLine(fromBulletVec(from), fromBulletVec(to), rgba, rgba);
}

void TbBullePhysicsDebugDrawer::drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
{
	//
}

void TbBullePhysicsDebugDrawer::reportErrorWarning(const char* warningString)
{
	//
}

void TbBullePhysicsDebugDrawer::draw3dText(const btVector3& location,const char* textString)
{
	//
}

void TbBullePhysicsDebugDrawer::setDebugMode(int debugMode)
{
	//
}

int TbBullePhysicsDebugDrawer::getDebugMode() const
{
	//return ~0;	// VERY SLOW!!!
	return DBG_DrawAabb|DBG_DrawFrames
		|DBG_DrawWireframe
		;
}
