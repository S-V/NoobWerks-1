// Bounding Interval Hierarchy
#pragma once

#include <Base/Template/Containers/Blob.h>
#include <Base/Template/LoadInPlace/LoadInPlaceTypes.h>


/// Use 32-bit quantized vertex positions?
/// NOTE: (11;11;10) positions are not enough - collision geom is too far from visual geom!
#define BIT_USE_32BIT_VERTICES	(1)


#if BIT_USE_32BIT_VERTICES

	typedef U32 BIHVertexQ;

	#define BIH_ENCODE_NORMALIZED_POS( normalized_pos )	(Encode_XYZ01_to_R11G11B10( normalized_pos ))
	#define BIH_DECODE_NORMALIZED_POS( quantized_pos )	(Decode_XYZ01_from_R11G11B10( quantized_pos ))

#else

	/// (21;21;22) - quantized vertex positions
	typedef U64 BIHVertexQ;

	#define BIH_ENCODE_NORMALIZED_POS( normalized_pos )	(Encode_XYZ01_to_R21G21B22_64( normalized_pos ))
	#define BIH_DECODE_NORMALIZED_POS( quantized_pos )	(Decode_XYZ01_from_R21G21B22_64( quantized_pos ))

#endif



class btTriangleCallback;

namespace Meshok
{
	class TriMesh;
}

template< typename T >
struct TPowOf2StridedData
{
	const T *	start;
	U32			stride_log2;
	U32			count;

public:
	template< class ARRAY >
	static TPowOf2StridedData FromArray(const ARRAY& a)
	{
		mxSTATIC_ASSERT(TIsPowerOfTwo<sizeof(a[0])>::value);

		TPowOf2StridedData	result;
		result.start = reinterpret_cast< const T* >(a.raw());
		result.stride_log2 = TLog2< sizeof(a[0]) >::value;
		result.count = a.num();
		return result;
	}
	mxFORCEINLINE const T& GetAt(UINT index) const
	{
		return *(T*) ( (char*)start + (index << stride_log2) );
	}
};

typedef TPowOf2StridedData<UInt3> StridedTrianglesT;
typedef TPowOf2StridedData<V3f> StridedPositionsT;





/*
-----------------------------------------------------------------------------
	NwBIH
-----------------------------------------------------------------------------
*/
struct NwBIH
{
#pragma pack (push,1)
	/// Each leaf nodes stores an index to a triangle.
	/// If a triangle straddles the splitting plane, it's added to either the left or right child
	/// (unlike a Kd-tree where the object has to be referenced in both children).
	union Node
	{
		U32	type_and_type_specific_data;

		struct InternalNodeData
		{
			/// upper two bits: 0 if leaf, othewise, split axis: 1 = x, 2 = y, 3 = z-split;
			/// 30 bits: index of the second (right).
			U32	axis_and_right_child_index;	//!<
			U16	split[2];	//!< conservative quantized splitting planes
		} inner;

		struct LeafNodeData
		{
			U32	first_triangle_index;	//!< 2 upper bits must be zero if this is a leaf
			U16	triangle_count;
			U16	material_index;
		} leaf;

	public:
		static const U32 SPLIT_AXIS_MASK		= (3 << 30);	// extracts the two most significant bits
		static const U32 RIGHT_CHILD_INDEX_MASK	= ~SPLIT_AXIS_MASK;
	};
#pragma pack (pop)
	mxSTATIC_ASSERT(sizeof(Node) == 8);

	/// vertex indices: (21,21,22) bits
	typedef U64 QTri;
	
	/// (21;21;22) - quantized vertex positions
	/// NOTE: (11;11;10) positions are not enough - collision geom is too far from visual geom!
	typedef BIHVertexQ QVert;


	TRelArray< Node >	_nodes;		//8 'flat' tree
	TRelArray< QTri >	_triangles;	//8 vertex indices: (21,21,22) bits
	TRelArray< QVert >	_vertices;	//8 quantized vertex positions
	U32					unused_padding[2];

	//32

public:
	NwBIH();

	/// 
	void ProcessTrianglesIntersectingBox( btTriangleCallback* callback
		, const btVector3& world_aabb_min
		, const btVector3& world_aabb_max

		, const btVector3& local_aabb_min
		, const btVector3& local_aabb_max
		//, const AABBf& tested_box_in_bih_local_space	// [0..1]

		, const AABBf& bih_bounds_local_space	//
		, float uniform_scale_local_to_world	// must be >= 1
		, const V3f& minimum_corner_in_world_space	// for debugging only
	) const;

	mxFORCEINLINE
	void GetTriangleVerticesForBullet(
		const int triangle_index
		, btVector3 triangle_vertices[3]
		, const float uniform_scale_local_to_world
		) const
	{
		const QTri tri = _triangles[ triangle_index ];

		const UINT iV0 = (tri >> 43) & ((1<<21) - 1);
		const UINT iV1 = (tri >> 22) & ((1<<21) - 1);
		const UINT iV2 = (tri >>  0) & ((1<<22) - 1);

		const QVert* qverts = _vertices.begin();
		const V3f a = BIH_DECODE_NORMALIZED_POS( qverts[ iV0 ] );
		const V3f b = BIH_DECODE_NORMALIZED_POS( qverts[ iV1 ] );
		const V3f c = BIH_DECODE_NORMALIZED_POS( qverts[ iV2 ] );

		triangle_vertices[0] = toBulletVec( a ) * uniform_scale_local_to_world;
		triangle_vertices[1] = toBulletVec( b ) * uniform_scale_local_to_world;
		triangle_vertices[2] = toBulletVec( c ) * uniform_scale_local_to_world;
	}

	///
	struct NodeOverlapCallbackI
	{
		virtual void processNode(int subPart, int triangleIndex) = 0;
	};

	/// Based on Bullet's btQuantizedBvh::walkStacklessQuantizedTreeAgainstRay()
	void WalkTreeAgainstRay(
		//NodeOverlapCallbackI * node_callback
		btTriangleCallback* triangle_callback
		, const btVector3& raySource, const btVector3& rayTarget
		, const btVector3& aabbMin, const btVector3& aabbMax
		, const AABBf& bih_bounds_local_space	//
		, const float uniform_scale_world_to_local
		) const;

	ERet ExtractVerticesAndTriangles(
		TArray<V3f>		&vertices_,
		TArray<UInt3>	&triangles_,
		const float uniform_scale_local_to_world	// must be >= 1
		) const;

public:	// Debugging:
	//static const RGBAf getDebugColorForTreeLevel( UINT depth );
};
ASSERT_SIZEOF(NwBIH, 32);

/*
-----------------------------------------------------------------------------
	NwBIHBuilder
-----------------------------------------------------------------------------
*/
class NwBIHBuilder: NonCopyable
{
	AABBf					_bounds;	//!< 12

	DynamicArray< NwBIH::Node >	_nodes;		//!< 'flat' tree

	DynamicArray< U64 >		_triangles;	//!< vertex indices: (21,21,22) bits

	DynamicArray< U32 >		_vertices;	//!< (11;11;10) - quantized vertex positions

public:
	NwBIHBuilder( AllocatorI & allocator );

public:	// Building.

	struct TriangleMeshI
	{
		virtual const StridedTrianglesT GetTriangles() const = 0;
		virtual const StridedPositionsT GetVertexPositions() const = 0;

	protected:
		virtual ~TriangleMeshI() {}
	};

	///
	struct Options
	{
		UINT	maxTrisPerLeaf;
		UINT	maxTreeDepth;
	public:
		Options()
		{
			maxTrisPerLeaf = 4;
			maxTreeDepth = 128;
		}
	};

	ERet Build( const TriangleMeshI& trimesh
		, const Options& build_options
		, AllocatorI & scratch
		);

public:	// Serialization:

	ERet SaveToBlob( NwBlob &blob_ );
};
