//#include <Physics/Physics_PCH.h>
#include <Base/Base.h>
#pragma hdrstop

#include <Physics/Bullet_Wrapper.h>

// btTriangleConvexcastCallback
#include <Bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include <Physics/Collision/TbQuantizedBIH.h>
#include <Physics/Collision/VoxelTerrainCollisions.h>


namespace
{
	static AllocatorI & GetCollisionShapeAllocator() { return MemoryHeaps::global(); }
}//namespace

//tbPRINT_SIZE_OF(NwBIHCollisionShape);

NwBIHCollisionShape::NwBIHCollisionShape(
	const AABBf& bounding_box_in_local_space
	, const float uniform_scale_local_to_world
	, const V3f& minimum_corner_in_world_space
	)
	: _uniform_scale_local_to_world(uniform_scale_local_to_world)
	, _bounding_box_in_local_space(bounding_box_in_local_space)
	, _min_corner_pos_in_world_space(minimum_corner_in_world_space)
{
	// BIH_SHAPE_PROXYTYPE should be placed right after SDF_SHAPE_PROXYTYPE
	m_shapeType = BIH_SHAPE_PROXYTYPE;

	m_collisionMargin = 0.1f;
}

NwBIHCollisionShape::~NwBIHCollisionShape()
{

}

void NwBIHCollisionShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	// Copied from: btScaledBvhTriangleMeshShape::getAabb()

	const btVector3 localScaling = toBulletVec(CV3f(_uniform_scale_local_to_world));

	btVector3 localAabbMin = toBulletVec( _bounding_box_in_local_space.min_corner );
	btVector3 localAabbMax = toBulletVec( _bounding_box_in_local_space.max_corner );

	btVector3 tmpLocalAabbMin = localAabbMin * localScaling;
	btVector3 tmpLocalAabbMax = localAabbMax * localScaling;

	localAabbMin[0] = tmpLocalAabbMin[0];
	localAabbMin[1] = tmpLocalAabbMin[1];
	localAabbMin[2] = tmpLocalAabbMin[2];
	localAabbMax[0] = tmpLocalAabbMax[0];
	localAabbMax[1] = tmpLocalAabbMax[1];
	localAabbMax[2] = tmpLocalAabbMax[2];

	btVector3 localHalfExtents = btScalar(0.5)*(localAabbMax-localAabbMin);

	btScalar margin = m_collisionMargin;
	localHalfExtents += btVector3(margin,margin,margin);

	//
	btVector3 localCenter = btScalar(0.5)*(localAabbMax+localAabbMin);
	
	btMatrix3x3 abs_b = t.getBasis().absolute();

	btVector3 center = t(localCenter);

    btVector3 extent = localHalfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	aabbMin = center - extent;
	aabbMax = center + extent;
}

void NwBIHCollisionShape::setLocalScaling(const btVector3& scaling)
{
	mxUNREACHABLE;
}

const btVector3& NwBIHCollisionShape::getLocalScaling() const
{
	return toBulletVec(CV3f(1));
}

void NwBIHCollisionShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}

const char*	NwBIHCollisionShape::getName()const
{
	return "VoxelTerrainChunk";
}

void NwBIHCollisionShape::processAllTriangles(
	btTriangleCallback* callback
	,const btVector3& aabbMin,const btVector3& aabbMax
) const
{
	const float scale_world_to_local01 = 1 / _uniform_scale_local_to_world;
	const AABBf tested_box_in_world_space = AABBf::make(
		fromBulletVec(aabbMin),
		fromBulletVec(aabbMax)
		);
	const AABBf tested_box_in_bih_local_space = tested_box_in_world_space
		.scaled(scale_world_to_local01)
		;



	btVector3 invLocalScaling = toBulletVec(CV3f(1 / _uniform_scale_local_to_world));

	// scaled into mesh local space
	btVector3 scaledAabbMin,scaledAabbMax;

	///support negative scaling
	scaledAabbMin[0] = aabbMin[0] * invLocalScaling[0];
	scaledAabbMin[1] = aabbMin[1] * invLocalScaling[1];
	scaledAabbMin[2] = aabbMin[2] * invLocalScaling[2];
	scaledAabbMin[3] = 0.f;

	scaledAabbMax[0] = aabbMax[0] * invLocalScaling[0];
	scaledAabbMax[1] = aabbMax[1] * invLocalScaling[1];
	scaledAabbMax[2] = aabbMax[2] * invLocalScaling[2];
	scaledAabbMax[3] = 0.f;


	this->GetBIH().ProcessTrianglesIntersectingBox( callback
		, aabbMin, aabbMax
		, scaledAabbMin, scaledAabbMax
		, _bounding_box_in_local_space
		, _uniform_scale_local_to_world
		, _min_corner_pos_in_world_space
		);
}

void NwBIHCollisionShape::performConvexcast(
	btTriangleCallback* triangle_callback
	, const btVector3& raySource, const btVector3& rayTarget
	, const btVector3& aabbMin, const btVector3& aabbMax
	)
{
	class MyNodeOverlapCallback : public NwBIH::NodeOverlapCallbackI
	{
		const NwBIH& bih;
		const float uniform_scale_local_to_world;
		btTriangleCallback* m_callback;

	public:
		MyNodeOverlapCallback(
			btTriangleCallback* callback
			, const NwBIH& bih
			, const float uniform_scale_local_to_world
			)
			: bih(bih)
			, uniform_scale_local_to_world(uniform_scale_local_to_world)
			, m_callback(callback)
		{}

		virtual void processNode(int nodeSubPart, int nodeTriangleIndex)
		{
			btVector3	triangle_vertices[3];

			bih.GetTriangleVerticesForBullet(
				nodeTriangleIndex
				, triangle_vertices
				, uniform_scale_local_to_world
				);

			/* Perform ray vs. triangle collision here */
			m_callback->processTriangle(
				triangle_vertices
				, nodeSubPart
				, nodeTriangleIndex
				);
		}
	};

	const NwBIH& bih = this->GetBIH();

	MyNodeOverlapCallback myNodeCallback(triangle_callback, bih, _uniform_scale_local_to_world);

	bih.WalkTreeAgainstRay(
		//&myNodeCallback
		triangle_callback
		, raySource, rayTarget
		, aabbMin, aabbMax
		, _bounding_box_in_local_space
		, 1 / _uniform_scale_local_to_world
		);
}

//////////////////////////////////////////////////////////////////////////

NwVoxelTerrainCollisionSystem::NwVoxelTerrainCollisionSystem()
{
	//mxZERO_OUT(_heads);
}

NwVoxelTerrainCollisionSystem::~NwVoxelTerrainCollisionSystem()
{
}

/*
static U32 getBucket( const U64 hash )
{
	const U32 bucket = ((U32)hash6432shift( hash )) % NwVoxelTerrainCollisionSystem::HASH_TABLE_SIZE;
	return bucket;
}

NwBIH* NwVoxelTerrainCollisionSystem::grabExistingTree( const void* internal_collision_tree_data )
{
	UNDONE;
#if 0
	const NwBIH::Header_d* collision_data_header =
		(NwBIH::Header_d*) internal_collision_tree_data;

	//
	const U32 bucket = getBucket( collision_data_header->hash );

	NwBIH* bih = _heads[ bucket ];
mxASSERT(IS_16_BYTE_ALIGNED(bih));
	while( bih )
	{
		NwBIH* next = bih->next;
mxASSERT(IS_16_BYTE_ALIGNED(next));

		if( bih->_hash == collision_data_header->hash )
		{
			const size_t rawl_data_size
				= collision_data_header->num_nodes * sizeof(bih->_nodes[0])
				+ collision_data_header->num_tris * sizeof(bih->_triangles[0])
				+ collision_data_header->num_verts * sizeof(bih->_vertices[0])
				;

			if( memcmp( bih+1, collision_data_header+1, rawl_data_size ) == 0 )
			{
				bih->_reference_count++;
				return bih;
			}
		}

		bih = next;
	}
#endif
	return nil;
}

NwBIH* NwVoxelTerrainCollisionSystem::loadTree( const void* internal_collision_tree_data
															  , const AABBf& bounding_box_in_local_space
															  )
{
	UNDONE;
	return nil;
#if 0
	const NwBIH::Header_d* collision_data_header =
		(NwBIH::Header_d*) internal_collision_tree_data;
	//
	NwBIH	*new_bih = nil;

	const size_t total_data_size
		= sizeof(*new_bih)
		+ collision_data_header->num_nodes * sizeof(new_bih->_nodes[0])
		+ collision_data_header->num_tris * sizeof(new_bih->_triangles[0])
		+ collision_data_header->num_verts * sizeof(new_bih->_vertices[0])
		;

	//const size_t total_data_size_4K_page_aligned = tbALIGN( total_data_size, 4*mxKILOBYTE );

	//
	AllocatorI &	allocator = GetCollisionShapeAllocator();

	//
	void* bih_mem = allocator.Allocate( total_data_size, EFFICIENT_ALIGNMENT );
	if( !bih_mem ) {
		UNDONE;
		return nil;
	}

	new_bih = new( bih_mem ) NwBIH( allocator );

	//
	const U32 bucket = getBucket( collision_data_header->hash );
	new_bih->PrependSelfToList( &_heads[ bucket ] );

	//
	new_bih->_hash = collision_data_header->hash;
	new_bih->_bounds = bounding_box_in_local_space;
	new_bih->_reference_count = 1;

	//
	memcpy( new_bih + 1,
		collision_data_header + 1,
		total_data_size - sizeof(*new_bih)
		);

	//
	UINT	src_offset = 0;

	//
	new_bih->_nodes.initializeWithExternalStorageAndCount(
		(NwBIH::Node*) mxAddByteOffset( new_bih+1, src_offset ),
		collision_data_header->num_nodes, collision_data_header->num_nodes
		);
	src_offset += new_bih->_nodes.rawSize();

	//
	new_bih->_triangles.initializeWithExternalStorageAndCount(
		(U64*) mxAddByteOffset( new_bih+1, src_offset ),
		collision_data_header->num_tris, collision_data_header->num_tris
		);
	src_offset += new_bih->_triangles.rawSize();

	//
	new_bih->_vertices.initializeWithExternalStorageAndCount(
		(U32*) mxAddByteOffset( new_bih+1, src_offset ),
		collision_data_header->num_verts, collision_data_header->num_verts
		);
	src_offset += new_bih->_vertices.rawSize();

	return new_bih;
#endif
}

void NwVoxelTerrainCollisionSystem::freeTree( NwBIH * tree )
{
	if( --tree->_reference_count == 0 )
	{
		tree->RemoveSelfFromList();

		//
		if( tree->IsLoose() )
		{
			const U32 bucket = getBucket( tree->_hash );
			if( _heads[ bucket ] == tree ) {
				_heads[ bucket ] = nil;
			}
		}

		//
		tree->~NwBIH();
		GetCollisionShapeAllocator().deallocate( tree );
	}
}
*/

NwBIH* NwVoxelTerrainCollisionSystem::loadTree( const void* internal_collision_tree_data
	, const AABBf& bounding_box_in_local_space
	)
{
	UNDONE;
	return nil;
}

void NwVoxelTerrainCollisionSystem::freeTree( NwBIH * tree )
{
	UNDONE;
}

ERet NwVoxelTerrainCollisionSystem::Initialize( NwClump * storage_clump )
{
	_storage_clump = storage_clump;
	return ALL_OK;
}

void NwVoxelTerrainCollisionSystem::Shutdown()
{
	_storage_clump = nil;
}

ERet NwVoxelTerrainCollisionSystem::AllocateCollisionObject(
	NwCollisionObject *& new_collision_obj_
	, const TSpan<BYTE>& bih_data
	, const V3d& minimum_corner_in_world_space
	, const float uniform_scale_local_to_world
	, const AABBf& bounding_box_in_local_space
	)
{
	NwCollisionObject*	new_collision_object;
	mxDO(_storage_clump->New(new_collision_object, NwCollisionObject::ALLOC_GRANULARITY));
	//
	const NwBIH &	src_bih = *reinterpret_cast<const NwBIH*>(bih_data._data);
	const size_t	total_size_to_allocate = sizeof(NwBIHCollisionShape) + bih_data.rawSize();

	mxOPTIMIZE("pooled mem alloc");
	void* new_mem = GetCollisionShapeAllocator().Allocate(total_size_to_allocate, 16);
	mxENSURE(new_mem, ERR_OUT_OF_MEMORY, "");

	NwBIHCollisionShape* new_col_shape = new(new_mem) NwBIHCollisionShape(
		bounding_box_in_local_space
		, uniform_scale_local_to_world
		, V3f::fromXYZ(minimum_corner_in_world_space)
		);
	//
	const NwBIH& bih = new_col_shape->GetBIH();
	mxOPTIMIZE("aligned simdquad memcpy");
	memcpy(
		(void*) &bih,
		bih_data.raw(),
		bih_data.rawSize()
		);

#if !nwCFG_USE_BIH_COLLISION_SHAPE


	mxDO(src_bih.ExtractVerticesAndTriangles(
		new_col_shape->vertices, new_col_shape->triangles,
		uniform_scale_local_to_world
		));
	new_col_shape->mesh_iface = new btTriangleIndexVertexArray(
		new_col_shape->triangles.num(),
		(int*)new_col_shape->triangles.raw(),
		sizeof(new_col_shape->triangles[0]),

		new_col_shape->vertices.num(),
		(btScalar*)new_col_shape->vertices.raw(),
		sizeof(new_col_shape->vertices[0])
		);

	new_col_shape->bvh_shape = new btBvhTriangleMeshShape(
		new_col_shape->mesh_iface	//btStridingMeshInterface*
		, false //useQuantizedAabbCompression
		);
	//new_col_shape->bvh_shape = new btTriangleMeshShape(
	//	new_col_shape->mesh_iface	//btStridingMeshInterface*
	//	);
	//new_col_shape->bvh_shape->m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
#endif

	//
	btCollisionObject& bt_col_obj = new_collision_object->bt;

#if nwCFG_USE_BIH_COLLISION_SHAPE
	bt_col_obj.setCollisionShape( new_col_shape );
#else
	bt_col_obj.setCollisionShape( new_col_shape->bvh_shape );
#endif

	//
	bt_col_obj.setFriction(0.5f);
	bt_col_obj.setRollingFriction(0.5f);
	bt_col_obj.setSpinningFriction(0.5f);

	//
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(toBulletVec(CV3f::fromXYZ(minimum_corner_in_world_space)));

	bt_col_obj.setWorldTransform( transform );

	//
	new_collision_obj_ = new_collision_object;

	return ALL_OK;
}

void NwVoxelTerrainCollisionSystem::ReleaseCollisionObject(
	NwCollisionObject *& old_collision_obj
	)
{
	NwBIHCollisionShape* bih_col_shape = checked_cast< NwBIHCollisionShape* >(
		old_collision_obj->bt_colobj().getCollisionShape()
		);

#if !nwCFG_USE_BIH_COLLISION_SHAPE
	delete bih_col_shape->bvh_shape;
	delete bih_col_shape->mesh_iface;
#endif

	//
	_storage_clump->Destroy(old_collision_obj);

	//
	bih_col_shape->~NwBIHCollisionShape();

	GetCollisionShapeAllocator().Deallocate(bih_col_shape);
}

// called in btCollisionWorld::objectQuerySingleInternal()
void btExtension_objectQuerySingleInternal_BIH_Shape(
	const btConvexShape* castShape, const btTransform& convexFromTrans, const btTransform& convexToTrans,
	const btCollisionObjectWrapper* colObjWrap,
	btCollisionWorld::ConvexResultCallback& resultCallback, btScalar allowedPenetration
	)
{
	const btCollisionShape* collisionShape = colObjWrap->getCollisionShape();
	const btTransform& colObjWorldTransform = colObjWrap->getWorldTransform();

	//
	NwBIHCollisionShape* bih_shape = (NwBIHCollisionShape*)collisionShape;
	btTransform worldTocollisionObject = colObjWorldTransform.inverse();
	btVector3 convexFromLocal = worldTocollisionObject * convexFromTrans.getOrigin();
	btVector3 convexToLocal = worldTocollisionObject * convexToTrans.getOrigin();
	// rotation of box in local mesh space = MeshRotation^-1 * ConvexToRotation
	btTransform rotationXform = btTransform(worldTocollisionObject.getBasis() * convexToTrans.getBasis());

	//ConvexCast::CastResult
	struct BridgeTriangleConvexcastCallback : public btTriangleConvexcastCallback
	{
		btCollisionWorld::ConvexResultCallback* m_resultCallback;
		const btCollisionObject* m_collisionObject;
		NwBIHCollisionShape* m_BIH;

		BridgeTriangleConvexcastCallback(const btConvexShape* castShape, const btTransform& from, const btTransform& to,
			btCollisionWorld::ConvexResultCallback* resultCallback
			, const btCollisionObject* collisionObject
			, NwBIHCollisionShape* bih_shape
			, const btTransform& triangleToWorld
			)
			: btTriangleConvexcastCallback(castShape, from, to, triangleToWorld, bih_shape->getMargin()),
			m_resultCallback(resultCallback),
			m_collisionObject(collisionObject),
			m_BIH(bih_shape)
		{
		}

		virtual btScalar reportHit(const btVector3& hitNormalLocal, const btVector3& hitPointLocal, btScalar hitFraction, int partId, int triangleIndex)
		{
			btCollisionWorld::LocalShapeInfo shapeInfo;
			shapeInfo.m_shapePart = partId;
			shapeInfo.m_triangleIndex = triangleIndex;
			if (hitFraction <= m_resultCallback->m_closestHitFraction)
			{
				btCollisionWorld::LocalConvexResult convexResult(m_collisionObject,
					&shapeInfo,
					hitNormalLocal,
					hitPointLocal,
					hitFraction);

				bool normalInWorldSpace = true;

				return m_resultCallback->addSingleResult(convexResult, normalInWorldSpace);
			}
			return hitFraction;
		}
	};

	BridgeTriangleConvexcastCallback tccb(
		castShape
		, convexFromTrans, convexToTrans
		, &resultCallback
		, colObjWrap->getCollisionObject()
		, bih_shape
		, colObjWorldTransform
		);
	tccb.m_hitFraction = resultCallback.m_closestHitFraction;
	tccb.m_allowedPenetration = allowedPenetration;

	btVector3 boxMinLocal, boxMaxLocal;
	castShape->getAabb(rotationXform, boxMinLocal, boxMaxLocal);

	bih_shape->performConvexcast(&tccb, convexFromLocal, convexToLocal, boxMinLocal, boxMaxLocal);
}

//#pragma comment( lib, "Bullet.lib" )
