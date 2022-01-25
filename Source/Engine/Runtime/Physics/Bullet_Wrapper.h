#pragma once

#define BT_NO_SIMD_OPERATOR_OVERLOADS	(1)
//#include <Bullet/BulletCollision/Gimpact/btBoxCollision.h>	// btAABB
#include <Bullet/btBulletDynamicsCommon.h>
#pragma comment( lib, "Bullet.lib" )

#include <Core/ObjectModel/Clump.h>	// NwClump
#include <Physics/Physics_Config.h>
#include <Physics/Collision/VoxelTerrainCollisions.h>

struct RrTransform;


// In our coord system, Z is UP.

static mxFORCEINLINE
btVector3 toBulletVec( const V3f& xyz )
{
	return btVector3( xyz.x, xyz.y, xyz.z );
}

static mxFORCEINLINE
V3f fromBulletVec( const btVector3& xyz )
{
	return CV3f( xyz.x(), xyz.y(), xyz.z() );
}

static mxFORCEINLINE
M44f M44f_from_btTransform( const btTransform& rigid_body_transform )
{
	M44f	result;
	rigid_body_transform.getOpenGLMatrix( (float*)&result );
	return result;
}

static mxFORCEINLINE
const btTransform Get_btTransform( const M44f& my_matrix )
{
	btTransform	result;
	result.setFromOpenGLMatrix( (float*)&my_matrix );
	return result;
}




enum PhysicsEntityType
{
	PHYS_RIGID_BODY,
	PHYS_PROJECTILE,
};

struct TbRigidBody;

typedef Tuple3< U32 >	Triangle32;

//CUSTOM_CONCAVE_SHAPE_TYPE
/// Represents a voxel terrain patch.
struct StaticMeshShape : btConcaveShape
{
	TArray< V3f >			m_positions;
	TArray< Triangle32 >	m_triangles;
	btVector3	m_localScaling;
	AABBf	m_localBounds;
	//btVector3	m_localAabbMin;
	//btVector3	m_localAabbMax;
	btScalar m_margin;

public:
	StaticMeshShape();
	~StaticMeshShape();

	ERet Initialize(
		U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
		const TSpan< Triangle32 >& _triangles,
		const AABBf& _localBounds
	);

	// btCollisionShape interface:
	virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const override;
	virtual void	setLocalScaling(const btVector3& scaling)  override;
	virtual const btVector3& getLocalScaling() const  override;
	virtual void	calculateLocalInertia(btScalar mass,btVector3& inertia) const override;
	//debugging support
	virtual const char*	getName()const override;

	// btConcaveShape interface:
	virtual void processAllTriangles(
		btTriangleCallback* callback
		,const btVector3& aabbMin,const btVector3& aabbMax
	) const override;
};

/// Represents a part of a voxel terrain.
struct CollisionObject : CStruct
{
	//TAutoPtr< btBvhTriangleMeshShape >	m_shape;
	StaticMeshShape		m_shape;

	btCollisionObject	m_colObj;

public:
	mxDECLARE_CLASS( CollisionObject, CStruct );
	mxDECLARE_REFLECTION;
	enum { ALLOC_GRANULARITY = 1024 };
	//CollisionObject();

	ERet Initialize(
		U32 _vertexCount, const V3f* _vertices, U32 _vertexStride,
		const TSpan< Triangle32 >& _triangles,
		const AABBf& _localBounds,
		const float worldScale
	);
};

const size_t COLSHAPE_UNION_SIZE =
	largest(sizeof(btCapsuleShapeZ),//32:96
		largest(
			sizeof(btCylinderShapeZ),//32:96
			largest(
				sizeof(btBoxShape),	//32:96
				sizeof(btSphereShape)	//32:80
				)
			)
		);

///The btMotionState interface class allows the dynamics world to synchronize and interpolate the updated world transforms with graphics
///For optimizations, potentially only moving objects get synchronized (using setWorldPosition/setWorldOrientation)
struct TbRigidBodyMotionState: btMotionState
{
	btTransform m_rigidBodyLocalToWorld;

public:
	TbRigidBodyMotionState()
	{
		m_rigidBodyLocalToWorld = btTransform::getIdentity();
	}

	///synchronizes world transform from user to physics
	virtual void	getWorldTransform( btTransform& centerOfMassWorldTrans ) const override;

	///synchronizes world transform from physics to user
	///Bullet only calls the update of worldtransform for active objects
	virtual void	setWorldTransform( const btTransform& centerOfMassWorldTrans ) override;
};

/*
----------------------------------------------------------
	TbRigidBody
----------------------------------------------------------
*/
mxALIGN_BY_CACHELINE struct TbRigidBody: CStruct
{
	char	m_shapeBuf[ COLSHAPE_UNION_SIZE ];
	char	m_rbBuf[ sizeof(btRigidBody) ];

	TbRigidBodyMotionState	m_motionState;

	TPtr< btRigidBody >			m_rb;
	TPtr< btCollisionShape >	m_shape;

	//
	PhysicsEntityType		type;
	void *					ptr;

	/// for updating graphics transform
	//int		_culling_proxy;

public:
	enum { ALLOC_GRANULARITY = 64 };
	mxDECLARE_CLASS( TbRigidBody, CStruct );
	mxDECLARE_REFLECTION;

	TbRigidBody();

	void Destroy();

	mxFORCEINLINE btRigidBody & bt_rb()
	{ return *(btRigidBody*)m_rbBuf; }

	mxFORCEINLINE const btRigidBody& bt_rb() const
	{ return *(btRigidBody*)m_rbBuf; }

	const AABBf getAabbWorld() const;
};

//=====================================================================

class TbPrimitiveBatcher;
///
class TbBullePhysicsDebugDrawer: public btIDebugDraw
{
	TPtr< TbPrimitiveBatcher >	_prim_renderer;

public:
	TbBullePhysicsDebugDrawer();

	void init(TbPrimitiveBatcher* renderer);

	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color) override;
	virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color) override;
	virtual void	reportErrorWarning(const char* warningString) override;
	virtual void	draw3dText(const btVector3& location,const char* textString) override;
	virtual void	setDebugMode(int debugMode) override;
	virtual int		getDebugMode() const override;
};
