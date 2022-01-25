// Collision Detection for Voxel Terrain.
#pragma once

#define BT_NO_SIMD_OPERATOR_OVERLOADS	(1)
#include <Bullet/btBulletCollisionCommon.h>

#include <Physics/CollisionObject.h>

// 1 - use our own optimized BIH (recommended), 0 - use btBvhTriangleMeshShape as a reference
#define nwCFG_USE_BIH_COLLISION_SHAPE	(1)


namespace Meshok
{
	class TriMesh;
}

struct NwBIH;
struct NwCollisionObject;
struct TbRigidBody;

typedef Tuple3< U32 >	Triangle32;

///
mxALIGN_BY_CACHELINE class NwBIHCollisionShape: public btConcaveShape
{
	/// must always be >= 1
	const float	_uniform_scale_local_to_world;	//4

	const AABBf	_bounding_box_in_local_space;	//24

	const V3f	_min_corner_pos_in_world_space;	//12

	// ... followed by NwBIH.

	//64


#if !nwCFG_USE_BIH_COLLISION_SHAPE
public:
	//
	TArray<V3f>		vertices;
	TArray<UInt3>	triangles;

	btTriangleIndexVertexArray *	mesh_iface;
	btTriangleMeshShape	*			bvh_shape;

#endif // nwCFG_USE_BIH_COLLISION_SHAPE


public:
	NwBIHCollisionShape(
		const AABBf& bounding_box_in_local_space
		, const float uniform_scale_local_to_world
		, const V3f& minimum_corner_in_world_space
		);
	~NwBIHCollisionShape();

	mxFORCEINLINE const NwBIH& GetBIH() const
	{
		return *(const NwBIH*) (this + 1);
	}

	// btCollisionShape interface:
	virtual void	getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const override;
	virtual void	setLocalScaling(const btVector3& scaling)  override;
	virtual const	btVector3& getLocalScaling() const  override;
	virtual void	calculateLocalInertia(btScalar mass,btVector3& inertia) const override;
	//debugging support
	virtual const char*	getName()const override;

	// btConcaveShape interface:
	virtual void processAllTriangles(
		btTriangleCallback* callback
		,const btVector3& aabbMin,const btVector3& aabbMax
	) const override;

	// based on btBvhTriangleMeshShape::performConvexcast()
	void performConvexcast(
		btTriangleCallback* triangle_callback
		, const btVector3& boxSource, const btVector3& boxTarget
		, const btVector3& boxMin, const btVector3& boxMax
		);
};





/// saves a lot of memory for a procedurally-generated terrain that has many identical chunks
class NwVoxelTerrainCollisionSystem
{
	TPtr< NwClump >	_storage_clump;

	//@todo: move these into .cpp
	//enum { HASH_TABLE_SIZE = 1 };	// number of buckets, each bucket is a linked list

	//NwBIH *	_heads[ HASH_TABLE_SIZE ];	// heads of linked lists

public:
	NwVoxelTerrainCollisionSystem();
	~NwVoxelTerrainCollisionSystem();

	ERet Initialize( NwClump * storage_clump );
	void Shutdown();


	ERet AllocateCollisionObject(
		NwCollisionObject *& new_collision_obj_
		, const TSpan<BYTE>& bih_data
		, const V3d& minimum_corner_in_world_space
		, const float uniform_scale_local_to_world
		, const AABBf& bounding_box_in_local_space
		);
	void ReleaseCollisionObject(
		NwCollisionObject *& old_collision_obj
		);

private:
	ERet AllocateCollisionShape(
		NwBIHCollisionShape *& new_collision_shape_
		);
	void ReleaseCollisionShape(
		NwBIHCollisionShape *& new_collision_shape
		);

	///
	NwBIH* grabExistingTree( const void* internal_collision_tree_data );

	NwBIH* loadTree( const void* internal_collision_tree_data
		, const AABBf& bounding_box_in_local_space
		);

	void freeTree( NwBIH * tree );
};
