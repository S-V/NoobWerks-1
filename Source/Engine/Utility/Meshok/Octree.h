// Octrees.
#pragma once

#include <Base/Template/Algorithm/Search.h>	// LowerBoundDescending()
#include <Utility/Meshok/Meshok.h>
#include <Utility/Meshok/Morton.h>	// for linear octrees
#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/Volumes.h>


namespace VX
{

// Octree Bounds
template< typename TYPE >
struct TOctreeBounds
{
	Tuple3< TYPE >	corner;	//!< cell's minimal corner, (0,0,0) if root
	TYPE			length;	//!< cube's side length, always > 1u

public:
	TOctreeBounds GetChildBounds( int _octant ) const
	{
		const TYPE childSize = this->length / 2;
		mxASSERT( childSize > 0 );
		const TOctreeBounds childBounds = {
			Tuple3< TYPE >(
				this->corner.x + ((_octant & MASK_POS_X) ? childSize : 0),
				this->corner.y + ((_octant & MASK_POS_Y) ? childSize : 0),
				this->corner.z + ((_octant & MASK_POS_Z) ? childSize : 0)
			),
			childSize
		};
		return childBounds;
	}
	const AABBf ToAABB() const
	{
		AABBf result;
		result.min_corner = corner;
		result.max_corner = result.min_corner + Tuple3< TYPE >(length);
		return result;
	}
};
typedef TOctreeBounds< U32 >	OctreeBoundsU;
typedef TOctreeBounds< float >	OctreeBoundsF;









enum { NUM_OCTANTS = 8 };


//// integer position of the cell's minimal corner, (0,0,0) if root
struct CellBounds
{
	UInt3	corner;	//!< cube's corner
	U32		size;	//!< cube's side length, always >= 1
};

// returns:
// x,y,z - cube's corner
// radius - cube's side length
//
inline CellBounds GetChildCorner( const CellBounds& _parent, int _octant )
{
	const U32 childSize = _parent.size / 2;
	mxASSERT( childSize > 0 );
	const CellBounds childBounds = {
		UInt3(
			_parent.corner.x + ((_octant & MASK_POS_X) ? childSize : 0),
			_parent.corner.y + ((_octant & MASK_POS_Y) ? childSize : 0),
			_parent.corner.z + ((_octant & MASK_POS_Z) ? childSize : 0)
		),
		childSize
	};
	return childBounds;
}


inline const AABBf GetChildAABB( const AABBf& _parentAABB, int _octant )
{
	const V3f halfSize = AABBf_Extent( _parentAABB );	// child size = half parent size

	AABBf	childAABB;

	childAABB.min_corner = V3f::set(
		_parentAABB.min_corner.x + ((_octant & MASK_POS_X) ? halfSize.x : 0),
		_parentAABB.min_corner.y + ((_octant & MASK_POS_Y) ? halfSize.y : 0),
		_parentAABB.min_corner.z + ((_octant & MASK_POS_Z) ? halfSize.z : 0)
		);

	childAABB.max_corner = childAABB.min_corner + halfSize;

	return childAABB;
}




mxREMOVE_THIS

template< typename N >
struct TOctBounds
{
	N x, y, z;	// center or corner (depends on chosen conventions)
	N radius;	// extent (cube's half size)

public:
	const V3f XYZ() const { return V3_Set(x,y,z); }
};

typedef TOctBounds< int > OctCubeI;
typedef TOctBounds< float > OctCubeF;
typedef TOctBounds< double > OctCubeD;

#if 0
template< class BOUNDS >
inline ATextStream & operator << ( ATextStream & log, const BOUNDS& bounds ) {
	log << "{x=" << bounds.x << ", y=" << bounds.y << ", z=" << bounds.z << ", r=" << bounds.radius << "}";
	return log;
}
#endif

template< class BOUNDS >
inline void OctBoundsToAABB( const BOUNDS& _node, AABBf &_aabb )
{
	const V3f center = { _node.x, _node.y, _node.z };
	const V3f extent = V3_SetAll( _node.radius );
	_aabb.min_corner = center - extent;
	_aabb.max_corner = center + extent;
}

// calculates the center and radius of the child octant
// returns:
// x,y,z - cube's center
// radius - cube's half size
// NOTE: this is not recommended to use with floating-point bounds - precision is lost!
//
template< typename N >
inline TOctBounds<N> GetChildOctant( const TOctBounds<N>& _parent, int _octant )
{
	const N	childRadius = _parent.radius / 2;
	const TOctBounds<N>	childBounds = {
		_parent.x + ((_octant & MASK_POS_X) ? childRadius : -childRadius),
		_parent.y + ((_octant & MASK_POS_Y) ? childRadius : -childRadius),
		_parent.z + ((_octant & MASK_POS_Z) ? childRadius : -childRadius),
		childRadius
	};
	return childBounds;
}

// determines the center and halfSize of each child octant
template< typename N >
inline void GetChildOctants( const TOctBounds<N>& _parent, TOctBounds<N> (&_children)[8] )
{
	for( int i = 0; i < 8; i++ ) {
		_children[i] = GetChildOctant( _parent, i );
	}
}

#if 0
// returns:
// x,y,z - cube's corner
// radius - cube's side length
//
template< typename N >
inline TOctBounds<N> GetChildCorner( const TOctBounds<N>& _parent, int _octant )
{
	const TOctBounds<N>	childBounds = {
		_parent.x + ((_octant & MASK_POS_X) ? _parent.radius : 0),
		_parent.y + ((_octant & MASK_POS_Y) ? _parent.radius : 0),
		_parent.z + ((_octant & MASK_POS_Z) ? _parent.radius : 0),
		_parent.radius / 2
	};
	return childBounds;
}
#endif

struct OctaCube
{
	U32	x,y,z;	// cube's corner
	U32	size;	// cube's side length
public:
	const V3f XYZ() const { return V3_Set(x,y,z); }
};
inline ATextStream & operator << ( ATextStream & log, const OctaCube& bounds ) {
	log << "{x=" << bounds.x << ", y=" << bounds.y << ", z=" << bounds.z << ", size=" << bounds.size << "}";
	return log;
}

inline OctaCube GetChildCorner( const OctaCube& _parent, int _octant )
{
	const U32 childSize = _parent.size / 2;
	const OctaCube	childBounds = {
		_parent.x + ((_octant & MASK_POS_X) ? childSize : 0),
		_parent.y + ((_octant & MASK_POS_Y) ? childSize : 0),
		_parent.z + ((_octant & MASK_POS_Z) ? childSize : 0),
		childSize
	};
	return childBounds;
}
// splits the parent's bounds into 8 uniform cubes
inline void GetChildCorners( const OctaCube& _parent, OctaCube (&_children)[8] )
{
	for( int i = 0; i < 8; i++ ) {
		_children[i] = GetChildCorner( _parent, i );
	}
}

template< typename N >
void OCT_GetCorners( const TOctBounds<N>& _bounds, V3f points[8] )
{
	for( int i = 0; i < 8; i++ )
	{
		points[i].x = _bounds.x + ((i & MASK_POS_X) ? _bounds.radius : -_bounds.radius);
		points[i].y = _bounds.y + ((i & MASK_POS_Y) ? _bounds.radius : -_bounds.radius);
		points[i].z = _bounds.z + ((i & MASK_POS_Z) ? _bounds.radius : -_bounds.radius);
	}
}

inline
V3f from_Child_To_Parent_Space( const V3f& positionInChildSpace, UINT iChildOctant )
{
	mxASSERT(positionInChildSpace.allInRangeInc( 0, 1 ));
	const CV3f vertexOffset(
		(iChildOctant & MASK_POS_X) ? 0.5f : 0.0f,
		(iChildOctant & MASK_POS_Y) ? 0.5f : 0.0f,
		(iChildOctant & MASK_POS_Z) ? 0.5f : 0.0f
	);
	// scale from [0..1] to [0..0.5] and add octant offset
	return positionInChildSpace * 0.5f + vertexOffset;
}
inline
V3f from_Parent_To_Child_Space( const V3f& positionInParentSpace, UINT iChildOctant )
{
	mxASSERT(positionInParentSpace.allInRangeInc( 0, 1 ));
	const CV3f vertexOffset(
		(iChildOctant & MASK_POS_X) ? 0.5f : 0.0f,
		(iChildOctant & MASK_POS_Y) ? 0.5f : 0.0f,
		(iChildOctant & MASK_POS_Z) ? 0.5f : 0.0f
	);
	// scale from [0..1] to [0..0.5] and add octant offset
	return (positionInParentSpace - vertexOffset) * 2.0f;
}

//enum ENodeType {
//	empty,		// empty space
//	Solid,		// solid volume
//	Boundary,	// surface, 'grey' leaf
//	Interior,	// internal node with 8 children
//};

struct FeatureVertex: CStruct
{
	V3f xyz;
	V3f N;
public:
	mxDECLARE_CLASS(FeatureVertex, CStruct);
	mxDECLARE_REFLECTION;
	FeatureVertex();
};

struct MeshOctreeNode: CStruct
{
	TArray< FeatureVertex >	features;
	UINT16	kids[8];	// eight children of this node (empty if leaf)
public:
	mxDECLARE_CLASS(MeshOctreeNode, CStruct);
	mxDECLARE_REFLECTION;
	MeshOctreeNode();
	NO_ASSIGNMENT(MeshOctreeNode);
};
struct MeshOctree: CStruct
{
	TArray< MeshOctreeNode >	m_nodes;
public:
	mxDECLARE_CLASS(MeshOctree, CStruct);
	mxDECLARE_REFLECTION;
	MeshOctree();
	PREVENT_COPY(MeshOctree);

	ERet Build( ATriangleMeshInterface* triangleMesh );
};

//struct MeshSDF : Volume
//{
//	TPtr< MeshOctree >	tree;
//public:
//	virtual float DistanceTo( const V3f& _position ) const override;
//};

V3f ClosestPointOnTriangle( const V3f triangle[3], const V3f& sourcePosition );

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
