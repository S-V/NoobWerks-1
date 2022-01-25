// for placing meshes in a voxel terrain
#include "stdafx.h"
#pragma hdrstop

#include <algorithm>	// std:min/max
#include <Core/Serialization/Serialization.h>
#include <VoxelEngine/VoxelizedMesh.h>

#define DBG_BSP	(0)

namespace BSP2
{

#define IS_LEAF( index )				(index & (~0u<<30u))
#define IS_INTERNAL( index )			(!(index & (~0u<<30u)))
#define SET_NODE_ID( type, payload )	(HNode((type<<30u) | payload))
#define GET_TYPE( index )				(ENodeType((index >> 30u) & 3))
#define GET_PAYLOAD( index )			(index & ~(~0u<<30u))
#define IS_SOLID_LEAF( index )			(GET_TYPE(index) == SolidLeaf)
#define IS_EMPTY_LEAF( index )			(GET_TYPE(index) == EmptyLeaf)

mxDEFINE_CLASS(Plane);
mxBEGIN_REFLECTION(Plane)
	mxMEMBER_FIELD( x ),
	mxMEMBER_FIELD( y ),
	mxMEMBER_FIELD( z ),
	mxMEMBER_FIELD( w ),
mxEND_REFLECTION;

mxDEFINE_CLASS(Tree::Node);
mxBEGIN_REFLECTION(Tree::Node)
	mxMEMBER_FIELD( plane ),
	mxMEMBER_FIELD( kids ),
	mxMEMBER_FIELD( faces ),
mxEND_REFLECTION;
Tree::Node::Node() {
	plane = NIL_PLANE;
	// by default, the back child is 'solid', the front child is 'empty'
	kids[BackChild] = SET_NODE_ID(SolidLeaf,0);
	kids[FrontChild] = SET_NODE_ID(EmptyLeaf,0);
	faces = NIL_FACE;
}

mxDEFINE_CLASS_NO_DEFAULT_CTOR(Tree);
mxBEGIN_REFLECTION(Tree)
	mxMEMBER_FIELD( m_nodes ),
	mxMEMBER_FIELD( m_planes ),
	mxMEMBER_FIELD( m_worldToLocal ),
	mxMEMBER_FIELD( m_localToWorld ),
mxEND_REFLECTION;
Tree::Tree( AllocatorI & _allocator )
	: m_nodes( _allocator )
	, m_planes( _allocator )
{
//	m_epsilon = 0;
}

mxOBSOLETE
mxFORCEINLINE int	/// Returns 0 if the point is on the plane (within tolerance), otherwise +/-1 if the point is above/below the plane
cmpPointToPlane( const Vec3T& pos, const Plane& plane, Real tol )
{
	const Real dist = plane.distance( pos );
	return dist < -tol ? -1 : (dist < tol ? 0 : 1);
}

/// Denotes a relation of a polygon/surface to some splitting plane;
/// used when classifying polygons/surfaces when building BSP trees.
/// Don't change the values - they are used as bit-masks / 'OR' ops.
enum EPlaneSide
{
	On		= 0x0,	//!< The polygon is lying on the plane.
	Back	= 0x1,	//!< The polygon is lying in back of the plane ('below', 'behind').
	Front	= 0x2,	//!< The polygon is lying in front of the plane ('above', 'over').
	Split	= 0x3,	//!< The polygon intersects with the plane ('spanning' the plane).
};

///
mxFORCEINLINE static
EPlaneSide ClassifyPoint( const Vec3T& point, const Plane& plane, const Real epsilon )
{
	const Real d = plane.distance( point );
	return ( d > +epsilon ) ? Front : ( d < -epsilon ) ? Back : On;
	//return d < -thickness ? Back : (d < thickness ? On : Front);
}

static mxFORCEINLINE
bool TriangleLiesOnPlane(
						 const Vec3T& _p0, const Vec3T& _p1, const Vec3T& _p2,
						 const Vec3T& _faceNormal,
						 const Plane& _plane,
						 const Tolerances& _tolerances
						 )
{
	return
		( _faceNormal ^ _plane.normal() ).lengthSquared() < Square(_tolerances.angular) &&
		( _faceNormal | _plane.normal() ) > 0 &&
		0 == cmpPointToPlane(_p0, _plane, _tolerances.linear) &&
		0 == cmpPointToPlane(_p1, _plane, _tolerances.linear) &&
		0 == cmpPointToPlane(_p2, _plane, _tolerances.linear);
}

/// for debugging
static
Plane planeFromTriangle_Naive( const Vec3T& v0, const Vec3T& v1, const Vec3T& v2 )
{
	const Vec3T r1 = v1 - v0;
	const Vec3T r2 = v2 - v0;
	Vec3T N = r1 ^ r2;
	N.normalize();

	Plane	plane;
	plane.fromPointNormal( v0, N );	//<= we could use any one of the three points
	return plane;
}

static
Plane planeFromPolygon( const Face& face, const TempMesh& _mesh )
{
	CVec3T	center(0);
	for( UINT i = face.vertices.first; i < face.vertices.Bound(); i++ ) {
		const Vertex& v = _mesh.m_V[ i ];
		center += v.xyz;
	}
	center *= Real(1) / face.vertices.count;

	CVec3T	normal(0);

	Vec3T	prev = _mesh.m_V[ face.vertices.Bound() - 1 ].xyz;	// last
	for( UINT i = face.vertices.first; i < face.vertices.Bound(); i++ ) {
		const Vec3T& curr = _mesh.m_V[ i ].xyz;
		const Vec3T	relV0 = prev - center;
		const Vec3T	relV1 = curr - center;
		normal += relV0 ^ relV1;
		prev = curr;
	}
	normal.normalize();

	Plane	plane;
	plane.fromPointNormal( center, normal );
	return plane;
}

static
Real computeFaceArea( const Vertex* verts, const UINT numVerts )
{
#if 1	// Fast, but sloppy

	CVec3T N( 0 );
	const Vec3T	v0 = verts[ 0 ].xyz;
	for( UINT i = 2; i < numVerts; i++ ) {
		const Vec3T	prev = verts[ i - 1 ].xyz;
		const Vec3T	curr = verts[ i     ].xyz;
		N += (prev - v0) ^ (curr - v0);
	}
	return mmSqrt( N.lengthSquared() ) * (Real)0.5;

#else	// Slower, but more precise

	CVec3T	center(0);
	for( UINT i = 0; i < numVerts; i++ ) {
		center += verts[ i ].xyz;
	}
	center *= Real(1) / (int)numVerts;

	CVec3T N( 0 );
	Vec3T prev = verts[ numVerts - 1 ].xyz;
	for( UINT i = 0; i < numVerts; i++ ) {
		const Vec3T& vi = verts[ i ].xyz;
		N += (prev - center) ^ (vi - center);
		prev = vi;
	}
	return mmSqrt( N.lengthSquared() ) * (Real)0.5;

#endif
}

/// Classify the polygon with respect to the plane.
static
EPlaneSide ClassifyPolygon(
						   const Face& face,
						   const Plane& plane,
						   const Real epsilon,
						   const TempMesh& _mesh
						   )
{
	//mxASSERT(!face.vertices.IsEmpty());
	unsigned status = On;	// EPlaneSide
	const Vertex* faceVerts = _mesh.m_V.raw() + face.vertices.first;
	for( UINT i = 0; i < face.vertices.count; i++ )
	{
		const Vertex& v = faceVerts[ i ];
		const Real distance = plane.distance( v.xyz );
		if( distance > +epsilon ) {
			status |= Front;	// this point is on the front side
		} else if( distance < -epsilon ) {
			status |= Back;		// this point is on the back side
		}
	}
	return (EPlaneSide) status;
}

/// NOTE: for consistency (to prevent minor epsilon issues)
/// the point 'a' must always be in front of the plane, the point 'a' - behind the plane,
/// i.e. 'distA' must always be > 0, and 'distB' must always be < 0.
static mxFORCEINLINE
Vertex GetIntersection(
					   const Vertex& a, const Vertex& b,
					   const Plane& plane,
					   const Real distA, const Real distB
					   )
{
	// always calculate the split going from the same side or minor epsilon issues can happen
	//mxASSERT(distA > 0 && distB < 0);

	Vertex	interpolatedVertex;

	const Real fraction = distA / ( distA - distB );
	interpolatedVertex.xyz = a.xyz + (b.xyz - a.xyz) * fraction;

	return interpolatedVertex;
}

/// Splits the face in a back and front part.
/// May create new BSP vertices.
static
EPlaneSide SplitPolygonByPlane(
							  const Face& _face,
							  const Plane& _plane,
							  Face &_back,
							  Face &_front,
							  const Real epsilon,
							  TempMesh & _mesh
						   )
{
	const UINT	numPoints = _face.vertices.count;
	Real *		dists = (Real*)			mxSTACK_ALLOC( (numPoints + 1) * sizeof(dists[0]) );
	EPlaneSide *sides = (EPlaneSide*)	mxSTACK_ALLOC( (numPoints + 1) * sizeof(sides[0]) );

	// First, classify all points. This allows us to avoid any bisection if possible
	UINT	counts[3] = {0}; // 'on', 'back' and 'front'
	UINT	faceStatus = On;	// EPlaneSide
	// determine sides for each point
	for( UINT i = 0; i < numPoints; i++ )
	{
		const Vertex& v = _mesh.m_V[ _face.vertices.first + i ];
		const Real d = _plane.distance( v.xyz );
		dists[ i ] = d;
		const EPlaneSide side = ( d > +epsilon ) ? Front : ( d < -epsilon ) ? Back : On;
		sides[ i ] = side;
		faceStatus |= side;
		counts[ side ]++;
	}
	sides[numPoints] = sides[0];
	dists[numPoints] = dists[0];

	if( faceStatus != Split ) {
		_back = _face;
		_front = _face;
		return (EPlaneSide) faceStatus;
	}

	// Straddles the splitting plane - we must clip.

	mxBIBREF("'Real-Time Collision Detection' by Christer Ericson (2005), 8.3.4 Splitting Polygons Against a Plane, PP.369-373");

	Vertex	backVerts[32], frontVerts[32];
	UINT	numFront = 0, numBack = 0;

	// Test all edges (a, b) starting with edge from last to first vertex
	Vertex vA = _mesh.m_V[ _face.vertices.first + _face.vertices.count - 1 ];
	Real distA = dists[ _face.vertices.count - 1 ];
	EPlaneSide sideA = sides[ _face.vertices.count - 1 ];

	// Loop over all edges given by vertex pair (n - 1, n)
	for ( UINT i = 0; i < numPoints; i++ )
	{
		const Vertex vB = _mesh.m_V[ _face.vertices.first + i ];
		const Real distB = dists[ i ];
		const EPlaneSide sideB = sides[ i ];
		if (sideB == Front) {
			if (sideA == Back) {
				// Edge (a, b) straddles, output intersection point to both sides
				// always calculate the split going from the same side or minor epsilon issues can happen
				const Vertex v = GetIntersection( vB, vA, _plane, distB, distA );
				mxASSERT(ClassifyPoint(v.xyz, _plane, epsilon) == On);
				frontVerts[numFront++] = backVerts[numBack++] = v;
			}
			// In all three cases, output b to the front side
			frontVerts[numFront++] = vB;
		} else if (sideB == Back) {
			if (sideA == Front) {
				// Edge (a, b) straddles plane, output intersection point
				const Vertex v = GetIntersection( vA, vB, _plane, distA, distB );
				mxASSERT(ClassifyPoint(v.xyz, _plane, epsilon) == On);
				frontVerts[numFront++] = backVerts[numBack++] = v;
			} else if (sideA == On) {
				// Output a when edge (a, b) goes from ‘on’ to ‘behind’ plane
				backVerts[numBack++] = vA;
			}
			// In all three cases, output b to the back side
			backVerts[numBack++] = vB;
		} else {
			// b is on the plane. In all three cases output b to the front side
			frontVerts[numFront++] = vB;
			// In one case, also output b to back side
			if (sideA == Back)
				backVerts[numBack++] = vB;
		}
		// Keep b as the starting point of the next edge
		vA = vB;
		distA = distB;
		sideA = sideB;
	}

	// Create (and return) two new polygons from the two vertex lists.

	// Allocate new vertices (contiguous).
UNDONE;
#if 0
	const UINT iNewVertex0 = _mesh.m_V.num();
	Vertex *newVerts = _mesh.m_V.AddManyUninitialized( numBack + numFront );
	TCopyArray( newVerts,           backVerts,  numBack );
	TCopyArray( newVerts + numBack, frontVerts, numFront );

	_back.vertices.first = iNewVertex0;
	_back.vertices.count = numBack;
	_back.normal = _face.normal;
	_back.area = computeFaceArea( backVerts, numBack );

	_front.vertices.first = iNewVertex0 + numBack;
	_front.vertices.count = numFront;
	_front.normal = _face.normal;
	_front.area = computeFaceArea( frontVerts, numFront );

	mxASSERT(_back.area > 0 && _front.area > 0);
#endif
	return (EPlaneSide) faceStatus;
}

/// May create new BSP faces.
void SplitSurfaceByPlane(
						 const Surface& _surface,
						 const Plane& _splitPlane,
						 Surface &_backSurface,
						 Surface &_frontSurface,
						 const Tolerances& _tolerances,
						 TempMesh & _mesh
						 )
{
	// inherit the original surface's plane
	_backSurface.plane = _surface.plane;
	_frontSurface.plane = _surface.plane;

	_backSurface.area = 0;
	_frontSurface.area = 0;

	_backSurface.faces.first = _mesh.m_F.num();

	// front polys will be copied to output later
	DynamicArray< Face >	frontFaces(MemoryHeaps::temporary());

	for( UINT iFace = _surface.faces.first; iFace < _surface.faces.bound; iFace++ )
	{
		const Face& face = _mesh.m_F[ iFace ];

		Face	back, front;
		const EPlaneSide side = SplitPolygonByPlane(
			face,
			_splitPlane,
			back, front,
			_tolerances.base,
			_mesh
		);

		mxASSERT(side != On);
		if( side & Back ) {
			_mesh.m_F.add( back );
			_backSurface.area += back.area;
		}
		if( side & Front ) {
			frontFaces.add( front );
			_frontSurface.area += front.area;
		}
	}//for each polygon on the surface

	_backSurface.faces.bound = _mesh.m_F.num();

	// add front polys
	_frontSurface.faces.first = _mesh.m_F.num();
	Arrays::append( _mesh.m_F, frontFaces.raw(), frontFaces.num() );
	_frontSurface.faces.bound =  _mesh.m_F.num();
}

#if 0
/// for caching results of Surface vs Plane tests
class PlaneTestsCache {
	/// these store packed EPlaneSide for each face
	DynamicArray< U32 >	m_testResults;
	DynamicArray< U32 >	m_bestResults;
public:
	PlaneTestsCache( AllocatorI & _allocator )
		: m_testResults( _allocator )
		, m_bestResults( _allocator )
	{
		mxSTATIC_ASSERT(BYTES_TO_BITS(sizeof(m_testResults[0]))==32);
	}
	void setup( UINT numSurfaces )
	{
		const UINT neededBits = numSurfaces * 2;	// 2 bits for storing 4 values from EPlaneSide
		const UINT neededInts = tbALIGN( neededBits, 32 ) / 32;
		m_testResults.setNum( neededInts );
		m_bestResults.setNum( neededInts );
	}
	mxFORCEINLINE void resetResults()
	{
		m_testResults.zeroOut();
	}
	mxFORCEINLINE void set( UINT surfaceIndex, EPlaneSide surfaceStatus )
	{
		// each 32-bit int can store up to 16 EPlaneSide'es
		m_testResults[ surfaceIndex / 16 ] |= (surfaceStatus << (surfaceIndex % 16));
	}
	/// caches test results to avoid re-testing surfaces against the best splitting plane
	mxFORCEINLINE void saveBestResults()
	{
		TCopyArray( m_bestResults.raw(), m_testResults.raw(), m_testResults.num() );
	}
	mxFORCEINLINE EPlaneSide get( UINT surfaceIndex ) const
	{
		return (EPlaneSide) ( (m_bestResults[ surfaceIndex / 16 ] >> (surfaceIndex % 16)) & 0x3 );
	}
};
#endif

/// for caching results of Surface vs Plane tests
class PlaneTestsCache {
	DynamicArray< U8 >	m_testResults;
	DynamicArray< U8 >	m_bestResults;
public:
	PlaneTestsCache( AllocatorI & _allocator )
		: m_testResults( _allocator )
		, m_bestResults( _allocator )
	{
	}
	void setup( UINT numSurfaces )
	{
		m_testResults.setNum( numSurfaces );
		m_bestResults.setNum( numSurfaces );
	}
	mxFORCEINLINE void resetResults()
	{
		Arrays::zeroOut( m_testResults );
	}
	mxFORCEINLINE void set( UINT surfaceIndex, EPlaneSide surfaceStatus )
	{
		m_testResults[ surfaceIndex ] = surfaceStatus;
	}
	/// caches test results to avoid re-testing surfaces against the best splitting plane
	mxFORCEINLINE void saveBestResults()
	{
		TCopyArray( m_bestResults.raw(), m_testResults.raw(), m_testResults.num() );
	}
	mxFORCEINLINE EPlaneSide get( UINT surfaceIndex ) const
	{
		return (EPlaneSide) m_bestResults[ surfaceIndex ];
	}
};

/*
The selection of the base polygon and the partitioning plane
is the crucial part of the BSP-tree construction. Depend-
ing on criteria for the base polygon selection different BSP-
trees can be obtained. In our work we use two different ap-
proaches: "naive" selection, where the polygon is randomly
selected from the polygon list, and "optimized" selection.
The optimization means using selection criteria that allow
for obtaining a tree with the following properties:
- Minimization of polygon splitting operations to reduce
the total number of nodes and the number of operations
in the function evaluation
- Minimization of computational errors during the function
evaluation and BSP-tree construction;
- Balancing the BSP tree, i.e., minimization of difference
between positive and negative list for the minimization of
the depth of the tree.
*/
// function for picking an optimal partitioning plane.
// we have two conflicting goals:
// 1) keep the tree balanced
// 2) avoid splitting the polygons
void selectBestSplitterAndBisectSurfaces(
	Tree::Node &_newNode,
	IdxRange2 &_backSurfaces,
	IdxRange2 &_frontSurfaces,
	DynamicArray< Surface > & _surfaceStack,
	const IdxRange2& _surfaceStackRange,
	const BuildParameters& _options,
	const Tolerances& _tolerances,
	PlaneTestsCache	& _cache,
	TempMesh & _mesh,
	Tree & _tree
	)
{
	const U32 numSurfaces = _surfaceStackRange.bound - _surfaceStackRange.first;
	const Surface* surfaceList = _surfaceStack.begin() + _surfaceStackRange.first;

	_cache.setup( numSurfaces );

	U32		bestSplitter = 0;		// the first surface by default
	int		bestCount[4] = { 0 };	// number of surfaces for each EPlaneSide
	bool	splittingCalculated = false;

	if( numSurfaces > 1 )
	{
		const bool selectBestSurfaceBasedOnArea = true;

		F32 maxLogArea = -MAX_FLOAT32;
		F32 meanLogArea = 0.0f;
		F32 sigma2LogArea = 0.0f;

		if( selectBestSurfaceBasedOnArea )
		{
			if( _options.logAreaSigmaThreshold > 0 )
			{
				U32 positiveAreaCount = 0;
				for (U32 i = 0; i < numSurfaces; ++i)
				{
					// Test surface
					const Surface& testSurface = surfaceList[i];
					mxASSERT(testSurface.area > 0);
					if (testSurface.area <= 0.0f)
					{
						continue;
					}
					++positiveAreaCount;
					const F32 logArea = mmLn(testSurface.area);
					if (logArea > maxLogArea)
					{
						maxLogArea = logArea;
						bestSplitter = i;	// Candidate
					}
					meanLogArea += logArea;
				}
				if (positiveAreaCount > 1)
				{
					meanLogArea /= (F32)positiveAreaCount;
					for (U32 i = 0; i < numSurfaces; ++i)
					{
						// Test surface
						const Surface& testSurface = surfaceList[i];
						mxASSERT(testSurface.area > 0);
						if (testSurface.area <= 0.0f)
						{
							continue;
						}
						const F32 logArea = mmLn(testSurface.area);
						sigma2LogArea += squaref(logArea - meanLogArea);
					}
					sigma2LogArea /= (F32)(positiveAreaCount-1);
				}
			}//if( _options.logAreaSigmaThreshold > 0 )
		}

		if( !selectBestSurfaceBasedOnArea || (maxLogArea > meanLogArea && squaref(maxLogArea - meanLogArea) < squaref(_options.logAreaSigmaThreshold)*sigma2LogArea) )
		{
			// branchSurface chosen by max area does not have an area that is outside of one standard deviation from the mean surface area.
			// Use another method to determine branchSurface.

			// Pick test surfaces
			const U32 testSetSize = _options.testSetSize > 0 ? Min(numSurfaces, _options.testSetSize) : numSurfaces;

			// Search through all polygons in the pool and find:
			// A. The number of splits each poly would make.
			// B. The number of front and back nodes the polygon would create.
			// C. The number of coplanars.

			float	bestScore = FLT_MAX;	// the lowest score wins

			// Test each surface against each other (N^2).
			for( UINT i = 0; i < numSurfaces; i++ )
			{
				// select potential splitter
				const Surface& splitter = surfaceList[ i ];
				// potential splitting plane
				const Plane& splitPlane = _tree.m_planes[ splitter.plane ];

				int	surfaceCount[4] = { 0 };// for each EPlaneSide
				int	trianglesTested = 0;	// for normalizing the score

				_cache.resetResults();	// clear cache

				// test other polygons against the potential splitter
				for( UINT j = 0; j < numSurfaces; j++ )
				{
					if( i != j )	// Ignore testing against self
					{
						// Surface to contribute to the score
						const Surface& surface = surfaceList[ j ];
						unsigned surfaceStatus = EPlaneSide::On;

						const Plane& plane = _tree.m_planes[ surface.plane ];

						// Run through all faces on the surface
						for( UINT k = surface.faces.first; k < surface.faces.bound; k++ )
						{
							const Face& face = _mesh.m_F[ k ];
							const EPlaneSide side = ClassifyPolygon(
								face,
								splitPlane,
								_tolerances.base,
								_mesh
							);
							surfaceStatus |= side;
						}//for each tested polygon

						surfaceCount[ surfaceStatus ]++;

						_cache.set( j, (EPlaneSide)surfaceStatus );

						trianglesTested += surface.faces.count();
					}
				}//for all tested surfaces

				// diff == 0 => tree is perfectly balanced
				const int diff = Abs<int>( surfaceCount[Front] - surfaceCount[Back] );

				// Compute score = (surface area)/(max area) + (split weight)*(# splits)/(# triangles) + (imbalance weight)*|(# above) - (# below)|/(# triangles)
				const F32 score = splitter.area * _options.recipMaxArea +
					(
						_options.splitWeight * surfaceCount[Split] +
						_options.imbalanceWeight * diff
					) / trianglesTested
					;
				// A smaller score will yield a better partition
				if( score < bestScore )
				{
					bestScore = score;
					bestSplitter = i;
					_cache.saveBestResults();	// so that won't need to test these surfaces again
					TCopyStaticArray( bestCount, surfaceCount );	// remember stats
				}
			}//for all potential splitters
			splittingCalculated = true;
		}//If couldn't find optimal surface based on area only.
	}//if( numSurfaces > 1 )

	const U32 bestPlaneIndex = surfaceList[ bestSplitter ].plane;
	const Plane& splitPlane = _tree.m_planes[ bestPlaneIndex ];

	if( !splittingCalculated )
	{
		_cache.resetResults();	// clear cache

		// test other polygons against the potential splitter
		for( UINT j = 0; j < numSurfaces; j++ )
		{
			if( bestSplitter != j )	// Ignore testing against self
			{
				// Surface to contribute to the score
				const Surface& surface = surfaceList[ j ];
				unsigned surfaceStatus = EPlaneSide::On;

				const Plane& plane = _tree.m_planes[ surface.plane ];

				// Run through all faces on the surface
				for( UINT k = surface.faces.first; k < surface.faces.bound; k++ )
				{
					const Face& face = _mesh.m_F[ k ];
					const EPlaneSide side = ClassifyPolygon(
						face,
						splitPlane,
						_tolerances.base,
						_mesh
					);
					surfaceStatus |= side;
				}//for each tested polygon

				bestCount[ surfaceStatus ]++;

				_cache.set( j, (EPlaneSide)surfaceStatus );
			}
		}//for all tested surfaces

		_cache.saveBestResults();	// so that won't need to test these surfaces again
	}//if( !splittingCalculated )

	const UINT numBackSurfaces = bestCount[ Back ];
	const UINT numFrontSurfaces = bestCount[ Front ];
	const UINT numSplitSurfaces = bestCount[ Split ];

#if DBG_BSP
	LogStream	log(LL_Trace);
	log << "Plane[" << bestPlaneIndex << "]=" << splitPlane
		<< "(" << numBackSurfaces << " back, " << numFrontSurfaces << " front, " << numSplitSurfaces << " split, " << bestCount[On] << " on)";
#endif

	// Allocate space on the stack.
	const UINT oldStackSize = _surfaceStack.num();
	UNDONE;
//	_surfaceStack.AddManyUninitialized( numBackSurfaces + numFrontSurfaces + numSplitSurfaces * 2 );
	// the array might have been resized
	surfaceList = _surfaceStack.begin() + _surfaceStackRange.first;

	// Run through the surfaces and create below/above arrays on the stack.
	// These arrays will be contiguous with child[0] surfaces first.
	IdxRange2	backSurfs;
	IdxRange2	frontSurfs;

	backSurfs.first = oldStackSize;
	backSurfs.bound = backSurfs.first;	// will be incremented at least 'numBackSurfaces' times

	frontSurfs.first = backSurfs.first + numBackSurfaces + numSplitSurfaces;	// reserve max space
	frontSurfs.bound = frontSurfs.first;	// will be incremented at least 'numFrontSurfaces' times

	for( UINT j = 0; j < numSurfaces; j++ )
	{
		if( j != bestSplitter )
		{
			const Surface& surface = surfaceList[ j ];
			const EPlaneSide surfaceSide = _cache.get( j );
			switch( surfaceSide ) {
				case On:
					continue;
				case Back:
					_surfaceStack[ backSurfs.bound++ ] = surface;
					continue;
				case Front:
					_surfaceStack[ frontSurfs.bound++ ] = surface;
					continue;
				case Split:
					Surface	backSurface, frontSurface;
					SplitSurfaceByPlane( surface, splitPlane, backSurface, frontSurface, _tolerances, _mesh );
					_surfaceStack[ backSurfs.bound++ ] = backSurface;
					_surfaceStack[ frontSurfs.bound++ ] = frontSurface;
					continue;
				mxDEFAULT_UNREACHABLE(;);
			}
		}
	}

	_backSurfaces = backSurfs;
	_frontSurfaces = frontSurfs;

	_newNode.plane.id = surfaceList[ bestSplitter ].plane;
	_newNode.faces.id = ~0;
}

struct BuildTreeFrame
{
	HNode		parent;
	int			side;	//!< BackChild or FrontChild
	IdxRange2	surfaces;	//!< surface stack range
	U32			surfaceStackSize;
};

ERet Tree::buildTree(
			   DynamicArray< Surface > & _surfaceStack,
			   const BuildParameters& _buildOptions,
			   const Tolerances& _tolerances,
			   TempMesh & _mesh,
			   AllocatorI & scratchpad
			   )
{
	PlaneTestsCache	cache( scratchpad );	// to avoid re-testing surfaces against the best splitting plane

	DynamicArray< BuildTreeFrame >	frameStack( scratchpad );
	frameStack.reserve( _surfaceStack.num() );	// To avoid reallocations

	BuildTreeFrame localFrame;	//!< the top of the simulated frame stack
	localFrame.parent = NIL_NODE;
	localFrame.side = ~0;
	localFrame.surfaces.first = 0;
	localFrame.surfaces.bound = _surfaceStack.num();
	localFrame.surfaceStackSize = _surfaceStack.num();

	for (;;)
	{
		// If there are no surfaces remaining...
		if( localFrame.surfaces.IsEmpty() )
		{
			// ...create a leaf node.
			if( frameStack.IsEmpty() ) {
				break;
			}
			localFrame = frameStack.PopLastValue();
			_surfaceStack.setNum( localFrame.surfaceStackSize );
		}
		else
		{
			// Select the best partitioner and Bisect the scene
			Node		newNode;
			IdxRange2	backSurfaces;	//!< surfaces behind the splitting plane
			IdxRange2	frontSurfaces;	//!< surfaces in front of the plane
			selectBestSplitterAndBisectSurfaces(
				newNode, backSurfaces, frontSurfaces, _surfaceStack, localFrame.surfaces,
				_buildOptions, _tolerances, cache, _mesh, *this
			);
			// add the new internal node
			const HNode hNewNode( m_nodes.num() );
			m_nodes.add( newNode );

			if( localFrame.parent != NIL_NODE ) {
				m_nodes[ localFrame.parent ].kids[ localFrame.side ] = hNewNode;
			}

			// Push child 1 ('Front')
			BuildTreeFrame	callFrame;
			callFrame.parent = hNewNode;
			callFrame.side = FrontChild;
			callFrame.surfaces = frontSurfaces;
			callFrame.surfaceStackSize = _surfaceStack.num();
			frameStack.add( callFrame );

			// Process child 0 ('Back')
			localFrame.parent = hNewNode;
			localFrame.side = BackChild;
			localFrame.surfaces = backSurfaces;
			localFrame.surfaceStackSize = _surfaceStack.num();
		}
	}

	return ALL_OK;
}

ERet Tree::buildFromMesh(
	const Meshok::TriMesh& _mesh,
	AllocatorI & scratchpad,
	const Settings& _settings
)
{
	BuildParameters	buildOptions;
	Tolerances	tolerances;
//tolerances.linear = 1e-3;
//tolerances.angular = 1e-3;
//tolerances.base = 1e-4;

//const Real multiplier = 16;
//tolerances.linear *= multiplier;
//tolerances.angular *= multiplier;
//tolerances.base *= multiplier;
#if !BSP_USE_DBL
tolerances.linear = 0.01f;
tolerances.angular = 0.001f;
tolerances.base = 0.01f;

tolerances.linear = 0.0001f;
tolerances.angular = 0.00001f;
tolerances.base = 0.0001f;
#endif
	const UINT origNumTriangles = _mesh.triangles.num();
	mxENSURE(origNumTriangles > 0, ERR_INVALID_PARAMETER, "No polygons");

	this->clear();

	const U64 startTimeMSec = mxGetTimeInMilliseconds();

	// Collect mesh triangles and find mesh bounds.
	AabbT	meshBounds;
	meshBounds.clear();

	// Convert triangles to BSP polygons
	TempMesh	tempMesh( scratchpad );
	mxDO(tempMesh.m_F.setNum( origNumTriangles ));

	const UINT origNumVertices = origNumTriangles * 3;
	mxDO(tempMesh.m_V.setNum( origNumVertices ));

	for( UINT iTri = 0; iTri < origNumTriangles; iTri++ )
	{
		const UInt3& triangle = _mesh.triangles[ iTri ];

		const Meshok::Vertex& srcVertex0 = _mesh.vertices[ triangle.a[0] ];
		const Meshok::Vertex& srcVertex1 = _mesh.vertices[ triangle.a[1] ];
		const Meshok::Vertex& srcVertex2 = _mesh.vertices[ triangle.a[2] ];
		const Vec3T srcPos0 = Vec3T::fromXYZ( srcVertex0.xyz );
		const Vec3T srcPos1 = Vec3T::fromXYZ( srcVertex1.xyz );
		const Vec3T srcPos2 = Vec3T::fromXYZ( srcVertex2.xyz );

		const U32 iDstVertex0 = iTri * 3;
		Vertex &dstVertex0 = tempMesh.m_V[ iDstVertex0 + 0 ];
		Vertex &dstVertex1 = tempMesh.m_V[ iDstVertex0 + 1 ];
		Vertex &dstVertex2 = tempMesh.m_V[ iDstVertex0 + 2 ];
		dstVertex0.xyz = srcPos0;
		dstVertex1.xyz = srcPos1;
		dstVertex2.xyz = srcPos2;

		Face &newPoly = tempMesh.m_F[ iTri ];
		newPoly.vertices.first = iDstVertex0;
		newPoly.vertices.count = 3;

		meshBounds.addPoint( srcPos0 );
		meshBounds.addPoint( srcPos1 );
		meshBounds.addPoint( srcPos2 );
	}//For each source mesh triangle.

	// Size scales
	const Vec3T halfSize = meshBounds.halfSize();
	const Real meshRadius = Max3( halfSize.x, halfSize.y, halfSize.z );

	// Scale to unit size [-1..+1] and zero offset for BSP building.
	Vec3T recipScale;
	if( _settings.preserveRelativeScale ) {
		recipScale = CVec3T( (meshRadius > tolerances.linear) ? meshRadius : (Real)1 );
	} else {
		recipScale = CVec3T(
			(halfSize.x > tolerances.linear) ? halfSize.x : (Real)1,
			(halfSize.y > tolerances.linear) ? halfSize.y : (Real)1,
			(halfSize.z > tolerances.linear) ? halfSize.z : (Real)1
		);
	}

	const Vec3T scale ={
		(Real)1 / recipScale[0],
		(Real)1 / recipScale[1],
		(Real)1 / recipScale[2]
	};

	const Vec3T center = meshBounds.center();

	const Real gridSize = (Real) buildOptions.snapGridSize;
	const Real recipGridSize = buildOptions.snapGridSize > 0 ? (Real)1/buildOptions.snapGridSize : (Real)0;

#if DBG_BSP
DEVOUT("Building a BSP tree from mesh: %d triangles, %d vertices", origNumTriangles, _mesh.vertices.num());
#endif

	// We need to transform all vertices into [-1..+1] range.
//@optimize: we are transforming about 3x more vertices than needed
	for( UINT iVertex = 0; iVertex < origNumVertices; ++iVertex )
	{
		Vertex & v = tempMesh.m_V[ iVertex ];
		v.xyz = Vec3T::mul( v.xyz - center, scale );
		if( buildOptions.snapGridSize )
		{
			for( UINT i = 0; i < 3; i++ ) {
				v.xyz[i] = recipGridSize * floor( gridSize * v.xyz[i] + (Real)0.5 );
			}
		}
	}//For each vertex.

	m_worldToLocal = M44_BuildTS( V3f::fromXYZ( -center ), V3f::fromXYZ( scale ) );
	m_localToWorld = M44_BuildTS( V3f::fromXYZ(  center ), V3f::fromXYZ( recipScale ) );

	// Now all vertices lie inside [-1..+1].

	for( UINT iFace = 0; iFace < origNumTriangles; iFace++ )
	{
		const Vertex& vertex0 = tempMesh.m_V[ iFace * 3 + 0 ];
		const Vertex& vertex1 = tempMesh.m_V[ iFace * 3 + 1 ];
		const Vertex& vertex2 = tempMesh.m_V[ iFace * 3 + 2 ];

		// calculate normal and area
		const Vec3T center = (vertex0.xyz + vertex1.xyz + vertex2.xyz) / (Real)3;
		const Vec3T relV0 = vertex0.xyz - center;
		const Vec3T relV1 = vertex1.xyz - center;
		const Vec3T relV2 = vertex2.xyz - center;

		Face & face = tempMesh.m_F[ iFace ];
		face.normal = Vec3T::cross(relV0, relV1) + Vec3T::cross(relV1, relV2) + Vec3T::cross(relV2, relV0);
		face.area = (Real)0.5 * face.normal.normalize();
#if DBG_BSP
	LogStream	log(LL_Trace);
	log << "Face[" << iFace << "]: " << _mesh.triangles[iFace] << ": " << V3f::fromXYZ(vertex0.xyz) << ", " << V3f::fromXYZ(vertex1.xyz) << ", " << V3f::fromXYZ(vertex2.xyz) << ", area=" << face.area << ", N=" << face.normal;
#endif
		mxASSERT(face.area > (Real)0);
	}//For each face.

	// Initialize surface stack with surfaces formed from mesh triangles.
	DynamicArray< Surface >	surfaceStack( scratchpad );
	// Crude estimate, hopefully will reduce re-allocations
	const int reservedSurfCount = origNumTriangles * ((int)mmLn( (F32)origNumTriangles ) + 1);
	mxDO(surfaceStack.reserve( reservedSurfCount ));
	mxDO(m_planes.reserve( reservedSurfCount ));	//  #surfaces == #planes

	// Track maximum and total surface triangle area.
	F32 maxArea = 0;
	Real totalArea = 0;

	// There will be at least so many nodes as coplanar polygons;
	// N*log(N) in average case and N^2 in worst case.

	// add mesh triangles and planes.
	{
		U32 triangleIndex = 0;
		while( triangleIndex < origNumTriangles )
		{
			// Create a surface for the next triangle
			const Face& tri = tempMesh.m_F[ triangleIndex ];
UNDONE;
			Surface &newSurface = *(Surface*)nil;
				//surfaceStack.AddNew();
			newSurface.plane = m_planes.num();
			newSurface.faces.first = triangleIndex++;

			Real surfaceTotalTriangleArea = tri.area;
UNDONE;
			Plane &newPlane = *(Plane*)nil;
				//m_planes.AddNew();
			{
				const Vertex& v0 = tempMesh.m_V[ tri.vertices.first + 0 ];
				const Vertex& v1 = tempMesh.m_V[ tri.vertices.first + 1 ];
				const Vertex& v2 = tempMesh.m_V[ tri.vertices.first + 2 ];
				newPlane.fromPointNormal(
					(v0.xyz + v1.xyz + v2.xyz) / (Real)3,
					tri.normal
				);
			}

			// See if any of the remaining triangles can fit on this surface.
			for( U32 testTriangleIndex = triangleIndex; testTriangleIndex < origNumTriangles; ++testTriangleIndex )
			{
				const Face testTri = tempMesh.m_F[ testTriangleIndex ];	//NOTE: copy by value, because the triangle can be swapped
				const Vertex& testVertex0 = tempMesh.m_V[ testTri.vertices.first + 0 ];
				const Vertex& testVertex1 = tempMesh.m_V[ testTri.vertices.first + 1 ];
				const Vertex& testVertex2 = tempMesh.m_V[ testTri.vertices.first + 2 ];
				if( TriangleLiesOnPlane( testVertex0.xyz, testVertex1.xyz, testVertex2.xyz, testTri.normal, newPlane, tolerances ) )
				{
					// This triangle fits.  Move it next to others in the surface.
					if( triangleIndex != testTriangleIndex )
						TSwap( tempMesh.m_F[ triangleIndex ], tempMesh.m_F[ testTriangleIndex ] );

					// add in the new normal, properly weighted
					Vec3T averageNormal = newPlane.normal() * surfaceTotalTriangleArea + testTri.normal * testTri.area;
					averageNormal.normalize();
					surfaceTotalTriangleArea += testTri.area;
					++triangleIndex;
					// Calculate the average projection
					Real averageProjection = 0;
					for( U32 iFace = newSurface.faces.first; iFace < triangleIndex; iFace++ )
					{
						const Face& face = tempMesh.m_F[ iFace ];
						averageProjection += Vec3T::dot( averageNormal, tempMesh.m_V[ face.vertices.first + 0 ].xyz );
						averageProjection += Vec3T::dot( averageNormal, tempMesh.m_V[ face.vertices.first + 1 ].xyz );
						averageProjection += Vec3T::dot( averageNormal, tempMesh.m_V[ face.vertices.first + 2 ].xyz );
					}
					averageProjection /= (Real)3 * (triangleIndex - newSurface.faces.first);
					newPlane.set( averageNormal, -averageProjection );
				}//If the test triangle lies on the current surface.
			}//For each test triangle.

			newSurface.faces.bound = triangleIndex;
			newSurface.area = (F32)surfaceTotalTriangleArea;

			maxArea = Max(maxArea, newSurface.area);
			totalArea += surfaceTotalTriangleArea;

			// Ensure triangles lie on or below surface
			Real maxProjection = -MAX_REAL;
			for( U32 iFace = newSurface.faces.first; iFace < newSurface.faces.bound; iFace++ )
			{
				const Face& face = tempMesh.m_F[ iFace ];
				maxProjection = Max( Vec3T::dot( newPlane.normal(), tempMesh.m_V[ face.vertices.first + 0 ].xyz ), maxProjection );
				maxProjection = Max( Vec3T::dot( newPlane.normal(), tempMesh.m_V[ face.vertices.first + 1 ].xyz ), maxProjection );
				maxProjection = Max( Vec3T::dot( newPlane.normal(), tempMesh.m_V[ face.vertices.first + 2 ].xyz ), maxProjection );
			}
			newPlane.w = -maxProjection;
#if DBG_BSP
{
	{
		LogStream	log(LL_Warn);
		log << "Surface [" << (surfaceStack.num()-1) << "]: plane [" << newSurface.plane << "]=" << newPlane << ", triangles: " << (newSurface.faces.count());
	}
	for( U32 iFace = newSurface.faces.first; iFace < newSurface.faces.bound; iFace++ )
	{
		const Face& face = tempMesh.m_F[ iFace ];
		const Vertex& vertex0 = tempMesh.m_V[ face.vertices.first + 0 ];
		const Vertex& vertex1 = tempMesh.m_V[ face.vertices.first + 1 ];
		const Vertex& vertex2 = tempMesh.m_V[ face.vertices.first + 2 ];
		LogStream	log(LL_Trace);
		log << "\tFace[" << iFace << "]: " << _mesh.triangles[iFace] << ": " << V3f::fromXYZ(vertex0.xyz) << ", " << V3f::fromXYZ(vertex1.xyz) << ", " << V3f::fromXYZ(vertex2.xyz) << ", N=" << face.normal;

		EPlaneSide side = ClassifyPolygon( face, newPlane, tolerances.base, *this );
		mxASSERT(side == On);
	}
}
#endif
		}//For each triangle in the mesh.

		buildOptions.recipMaxArea = (maxArea > 0) ? (1.0f / maxArea) : (F32)0;
	}//Create planes and add surfaces.

DEVOUT("%d triangles -> %d planes/surfaces (%d msec)", origNumTriangles, m_planes.num(), mxGetTimeInMilliseconds() - startTimeMSec);
#if DBG_BSP
for( UINT iPlane = 0; iPlane < m_planes.num(); iPlane++ ) {
	LogStream	log(LL_Info);
	log << "Plane[" << iPlane << "]=" << m_planes[iPlane];
}
#endif

	m_nodes.reserve( surfaceStack.num() );	// To avoid reallocations
	mxDO(this->buildTree( surfaceStack, buildOptions, tolerances, tempMesh, scratchpad ));

DEVOUT("Built tree: %d nodes (%d msec), %d KiB", m_nodes.num(), mxGetTimeInMilliseconds() - startTimeMSec, this->memoryUsed()/1024);

	return ALL_OK;
}

void Tree::clear()
{
	m_nodes.RemoveAll();
	m_planes.RemoveAll();
}

U32 Tree::memoryUsed() const
{
	return m_nodes.usedMemorySize() + m_planes.usedMemorySize();
}

bool Tree::PointInSolid( const V3f& _point ) const
{
	//// Transform the world-space point to local space of this BSP tree.
	//const V3f p = M44_TransformPoint( m_worldToLocal, _point );
	//const Vec3T pointLocal = Vec3T::fromXYZ( p );
	const Vec3T pointLocal = Vec3T::fromXYZ( _point );

	U32 nodeIndex = 0;	// start from the root node
	// If < 0, we are in a leaf node.
	while( !IS_LEAF( nodeIndex ) )
	{
		const Node& node = m_nodes[ nodeIndex ];
		// find which side of the node we are on
		const Plane& plane = m_planes[ node.plane ];
		const Real distance = plane.distance( pointLocal );
		// If it is not necessary to categorize points as lying on the boundary
		// and they can instead be considered inside the solid,
		// this test can be simplified to visit the back side
		// when the point is categorized as on the dividing plane.
		const int nearIndex = (distance > 0);	// Child of Node for half-space containing the point: 1 = 'front', 0 = 'back' and 'on'.
		// Go down the appropriate side.
		nodeIndex = node.kids[ nearIndex ];
	}
	// Now at a leaf, inside/outside status determined by the leaf type.
	return IS_SOLID_LEAF( nodeIndex );
}

/// ray stabbing query, recursive version.
mxBIBREF("'Real-Time Collision Detection' by Christer Ericson (2005), 8.4.2 Intersecting a Ray Against a Solid-leaf BSP Tree, PP.376-377");
/// Intersect ray/segment R(t)=p+t*d, tmin <= t <= tmax, against bsp tree ’node’,
/// returning time thit of first intersection with a solid leaf, if any.
static bool CastRay_R(
	const Vec3T& start, const Vec3T& direction,
	const Real tmin, const Real tmax,
	const Tree& tree,
	const HNode iNodeIndex,
	const HNode iLastHitNode,	//!< index of the last intersected node
	Real *thit, Vec3T *hitNormal,
	const bool startInSolid
	)
{
	if( IS_INTERNAL( iNodeIndex ) )
	{
		const Tree::Node node = tree.m_nodes[ iNodeIndex ];	// make a copy on stack, because Node struct is small
		const Plane& plane = tree.m_planes[ node.plane ];
		const Real distance = plane.distance( start );
		const Real denom = Vec3T::dot( plane.normal(), direction );
		int nearSide = (distance > Real(0));	// child index for half-space containing the ray origin: 0 - back, 1 - front
		// If denom is zero, ray runs parallel to plane. In this case,
		// just fall through to visit the near side (the one 'start' lies on).
		if( denom != Real(0) ) {
			const Real t = -distance / denom;	// distance along the ray until the intersection with the plane
			// Determine which side(s) of the plane the ray should be sent down.
			if( Real(0) <= t && t <= tmax ) {
				if( t >= tmin ) {
					// visit the near side with the ray interval [tmin, t]
					if( CastRay_R( start, direction, tmin, t, tree, node.kids[ nearSide ], iLastHitNode, thit, hitNormal, startInSolid ) ) {
						return true;
					}
					// visit the far side with the ray interval [t, tmax]
					// we hit the plane of this node, so use this node's index as the index of the last intersected node
					return CastRay_R( start, direction, t, tmax, tree, node.kids[ nearSide^1 ], iNodeIndex, thit, hitNormal, startInSolid );
				} else {
					// 0 <= t < tmin, i.e. the ray crosses the plane before 'tmin' => only the far side needs to be traversed
					nearSide = 1^nearSide;
				}
			}
		}//if( denom != 0 )
		// the ray doesn't intersect this node's plane, so we pass the parent's node index
		return CastRay_R( start, direction, tmin, tmax, tree, node.kids[ nearSide ], iLastHitNode, thit, hitNormal, startInSolid );
	}
	else
	{
		// Now at a leaf. If there's a hit at time tmin, then exit.
		const bool endInSolid = IS_SOLID_LEAF( iNodeIndex );
		if( startInSolid != endInSolid ) {
			*thit = tmin;
			const Tree::Node& parentNode = tree.m_nodes[ iLastHitNode.id ];
			const Plane& parentNodePlane = tree.m_planes[ parentNode.plane.id ];
			*hitNormal = parentNodePlane.normal();
			return true;
		}
	}
	// No hit
	return false;
}

bool Tree::CastRay(
	const V3f& _start, const V3f& _direction,
	float _tmin, float _tmax,
	float *_thit, V3f *_hitNormal,
	const bool _startInSolid /*= false*/
	) const
{
	mxASSERT(V3_IsNormalized(_direction));
	//const Vec3T originLocal = Vec3T::fromXYZ( M44_TransformPoint( m_worldToLocal, _start ) );
	//Vec3T directionLocal = Vec3T::fromXYZ( M44_TransformNormal( m_worldToLocal, _direction ) );
	const Vec3T originLocal = Vec3T::fromXYZ( _start );
	Vec3T directionLocal = Vec3T::fromXYZ( _direction );
	//directionLocal.normalize();

	Real	thit( _tmin );
	CVec3T	hitNormal( 0 );

#if 0	// Recursive version.

	const bool bHit = CastRay_R( originLocal, directionLocal, _tmin, _tmax, *this, HNode(0), HNode(0), &thit, &hitNormal, _startInSolid );

	*_thit = thit;
//	*_hitNormal = M44_TransformNormal( m_localToWorld, V3f::fromXYZ( hitNormal ) );
	_hitNormal->fromXYZ(hitNormal);

	return bHit;

#else	// Faster iterative (stackless) version.

	enum { StackSize = 128 };
	struct StackEntry {
		HNode	node;		//!< index of the current node (can be leaf ID)
		HNode	lastHit;	//!< index of the last intersected node (never a leaf)
		Real	time;
	};
	StackEntry stack[ StackSize ];
	int stackTop = 0;

	HNode	iNode(0);	// start with the root
	HNode	iLastHitNode(0);	//!< always an internal node ID (i.e. cannot be a leaf ID)
	Real	tmin = _tmin, tmax = _tmax;

	const Node* nodes = m_nodes.raw();
	const Plane* planes = m_planes.raw();

	for(;;)
	{
		if( IS_INTERNAL( iNode ) )
		{
			const Node& node = nodes[ iNode ];
			const Plane& plane = planes[ node.plane ];
			const Real distance = plane.distance( originLocal );
			const Real denom = ( plane.normal() | directionLocal );
			int nearSide = (distance > Real(0));
			// If denom is zero, ray runs parallel to plane. In this case,
			// just fall through to visit the near side (the one 'start' lies on).
			if( denom != Real(0) ) {
				const Real t = -distance / denom;	// distance along the ray until the intersection with the plane
				// Determine which side(s) of the plane the ray should be sent down.
				if( Real(0) <= t && t <= tmax ) {
					if( t >= tmin ) {
						// Straddling, push far side onto stack, then visit near side
						// visit the near side with the ray interval [tmin, t]
						stack[stackTop].node = node.kids[ 1-nearSide ];
						stack[stackTop].lastHit = iNode;
						stack[stackTop].time = tmax;
						stackTop++;
						tmax = t;
					} else {
						// 0 <= t < tmin, visit far side
						nearSide = 1 - nearSide;
					}
				}
			}//if( denom != 0 )
			iNode = node.kids[ nearSide ];
		}
		else
		{
			// Now at a leaf. If it is solid, there’s a hit at time tmin, so exit
			const bool endInSolid = IS_SOLID_LEAF( iNode );
			if( _startInSolid != endInSolid ) {
				*_thit = tmin;
				const Node& parentNode = nodes[ iLastHitNode.id ];
				const Plane& parentNodePlane = planes[ parentNode.plane.id ];
				hitNormal = parentNodePlane.normal();
//	*_hitNormal = M44_TransformNormal( m_localToWorld, V3f::fromXYZ( hitNormal ) );
				_hitNormal->setFrom(hitNormal);
				return true;
			}
			// Exit if no more subtrees to visit, else pop off a node and continue
			if( !stackTop ) {
				break;
			}
			tmin = tmax;
			stackTop--;
			iNode = stack[stackTop].node;
			tmax = stack[stackTop].time;
			iLastHitNode = stack[stackTop].lastHit;
		}//if at a leaf
	}//for

	// No hit
	return false;
#endif
}

Tree::Sampler::Sampler( const Tree& _tree, const AABBf& _bounds, const Int3& _resolution )
	: m_tree( _tree )
{
	m_cellSize = V3_Divide( _bounds.size(), V3f::fromXYZ(_resolution) );
	m_offset = _bounds.min_corner;
}

bool Tree::Sampler::IsInside(
	int iCellX, int iCellY, int iCellZ
) const
{
	const V3f cellCorner = m_offset + V3f::mul( CV3f( iCellX, iCellY, iCellZ ), m_cellSize );
	return m_tree.PointInSolid( cellCorner );
}

bool Tree::Sampler::isPointInside( const V3f& localPosition ) const
{
	return m_tree.PointInSolid( m_offset + localPosition );
}

bool Tree::Sampler::CastRay(
	const V3f& start, const V3f& direction,
	float tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
	float *thit, V3f *hitNormal,
	const bool startInSolid /*= false*/
) const
{
	const bool intersects = m_tree.CastRay(
		m_offset + start, direction,
		0, tmax,
		thit, hitNormal,
		startInSolid
	);
	return intersects;
}

U32 Tree::Sampler::SampleHermiteData(
	int iCellX, int iCellY, int iCellZ,
	VX::HermiteDataSample & _sample
) const
{
	V3f corners[8];	// cell corners in world space
	UINT signMask = 0;
	for( UINT iCorner = 0; iCorner < 8; iCorner++ )
	{
		const int iCornerX = ( iCorner & MASK_POS_X ) ? iCellX+1 : iCellX;
		const int iCornerY = ( iCorner & MASK_POS_Y ) ? iCellY+1 : iCellY;
		const int iCornerZ = ( iCorner & MASK_POS_Z ) ? iCellZ+1 : iCellZ;
		const V3f cornerXYZ = V3f::set( iCornerX, iCornerY, iCornerZ );
		const V3f cellCorner = m_offset + V3_Multiply( cornerXYZ, m_cellSize );

		const bool cornerIsInside = m_tree.PointInSolid( cellCorner );
		signMask |= (cornerIsInside << iCorner);

		corners[iCorner] = cellCorner;
	}
	if( vxIS_ZERO_OR_0xFF(signMask) ) {
		return signMask;
	}

	int numWritten = 0;

	U32 edgeMask = 0;//CUBE_ACTIVE_EDGES[ signMask ];
	while( edgeMask )
	{
//		U32	iEdge;
		UNDONE;
	//		edgeMask = TakeNextTrailingBit32( edgeMask, &iEdge );

#if 0
		const EAxis edgeAxis = (EAxis)( iEdge / 4 );
		const V3f& axisDir = (V3f&) g_CardinalAxes[ edgeAxis ];

		const UINT iCornerA = CUBE_EDGE[iEdge][0];
		//const UINT iCornerB = CUBE_EDGE[iEdge][1];

		const V3f& cornerA = corners[iCornerA];
		//const V3f& cornerB = corners[iCornerB];

		F32 intersectionTime;
		V3f intersectionNormal;
		const bool intersects = m_tree.CastRay(
			cornerA, axisDir,
			0.0f, m_cellSize[ edgeAxis ],
			&intersectionTime, &intersectionNormal,
			(signMask & BIT(iCornerA))
		);
		mxASSERT(intersects);

		V3f intersectionPoint = cornerA;
		intersectionPoint[ edgeAxis ] += intersectionTime;
//g_pDbgView->addPointWithNormal(intersectionPoint, intersectionNormal);

		_sample.positions[ numWritten ] = intersectionPoint;
		_sample.normals[ numWritten ] = intersectionNormal;
		numWritten++;
#endif
	}

	_sample.num_points = numWritten;

	return signMask;
}

bool Tree::Sampler::getEdgeIntersection(
	int iCellX, int iCellY, int iCellZ	//!< coordinates of the cell's minimal corner (or the voxel at that corner)
	, const EAxis edgeAxis	//!< axis index [0..3)
	, float &distance_
	, V3f &normal_	//!< directed distance along the positive axis direction and unit normal to the surface
) const
{
	const V3f cellCorner = m_offset + V3f::mul( CV3f( iCellX, iCellY, iCellZ ), m_cellSize );

	const bool cornerIsInside = m_tree.PointInSolid( cellCorner );

	const V3f& axisDir = (V3f&) g_CardinalAxes[ edgeAxis ];

	const bool intersects = m_tree.CastRay(
		cellCorner, axisDir,
		0, m_cellSize[ edgeAxis ],
		&distance_, &normal_,
		cornerIsInside
	);

	return intersects;
}

ERet Tree::SaveToBlob(
	NwBlobWriter & _stream
) const
{
	Header_d header;
	header.num_nodes = m_nodes.num();
	header.num_planes = m_planes.num();
//	header.plane_epsilon = m_epsilon;
	mxDO(_stream.Put( header ));

	mxDO(_stream.Write( m_nodes.raw(), m_nodes.rawSize() ));
	mxDO(_stream.Write( m_planes.raw(), m_planes.rawSize() ));

	return ALL_OK;
}

static const String32 GetNodeName( const HNode nodeIndex )
{
	String32	result;
	if( IS_INTERNAL( nodeIndex ) ) {
		Str::Format( result, "[%d]", nodeIndex.id );
	} else if( IS_SOLID_LEAF( nodeIndex ) ) {
		Str::Format( result, "SOLID" );
	} else {
		Str::Format( result, "EMPTY" );
	}
	return result;
}
static void DbgPrint_R(
	const Tree& tree,
	const HNode nodeIndex,
	const UINT currentDepth
	)
{
	if( !IS_LEAF( nodeIndex ) )
	{
		const Tree::Node& node = tree.m_nodes[ nodeIndex ];
		const Plane& plane = tree.m_planes[ node.plane ];
		{
			LogStream	log(LL_Info);
			log.Repeat('\t', currentDepth);
			log << "Node[" << nodeIndex.id << "]: plane[" << node.plane << "]=" << plane << ", back: " << GetNodeName(node.kids[BackChild]) << ", front: " << GetNodeName(node.kids[FrontChild]);
		}
		DbgPrint_R( tree, node.kids[BackChild], currentDepth + 1 );
		DbgPrint_R( tree, node.kids[FrontChild], currentDepth + 1 );
	}
}

void Tree::Debug_Print() const
{
	{
		LogStream	log(LL_Info);
		log << m_nodes.num() << " nodes, " << m_planes.num() << " planes";
	}
	DbgPrint_R( *this, HNode(0), 0 );
}
bool Tree::Debug_Validate() const
{
	bool bOk = true;
	for( UINT iPlane = 0; iPlane < m_planes.num(); iPlane++ )
	{
		bOk &= m_planes[ iPlane ].IsValid();
		mxASSERT2(bOk, "Plane [%d] is not normalized!", iPlane);
	}
	return bOk;
}

}//namespace BSP2

namespace VX
{

ERet VoxelizedMesh::CreateFromMesh(
	const char* _filepath
	, AllocatorI & scratchpad
	)
{
	Meshok::TriMesh	mesh( scratchpad );
	mxDO(mesh.ImportFromFile( _filepath ));
	//m_aabb = mesh.localAabb;
	//mesh.FitInto( AABBf::make( CV3f(-1), CV3f(+1) ) );
	BSP2::Tree::Settings	bspSettings;
	bspSettings.preserveRelativeScale = true;
	mxDO(m_tree.buildFromMesh( mesh, scratchpad, bspSettings ));
	return ALL_OK;
}

ERet VoxelizedMesh::Load( AReader& _stream )
{
	mxDO(Serialization::LoadBinary( m_tree, _stream ));
	return ALL_OK;
}

ERet VoxelizedMesh::Save( AWriter &_stream ) const
{
	mxDO(Serialization::SaveBinary( m_tree, _stream ));
	return ALL_OK;
}

ERet VoxelizedMesh::Save( NwBlob &_blob ) const
{
	NwBlobWriter	wr( _blob );
	return m_tree.SaveToBlob( wr );
}

void VoxelizedMesh::deleteSelf()
{
	mxDELETE( this, m_storage );
}

ERet loadOrCreateModel( TRefPtr< VX::VoxelizedMesh > &model,
					   const char* _sourceMesh, AllocatorI & _storage, const char* _pathToCache, bool _forceRebuild /*= false*/ )
{
	String256	cachedFilePath;
	const char* fileNameOnly = Str::GetFileName( _sourceMesh );
	Str::ComposeFilePath( cachedFilePath, _pathToCache, fileNameOnly, "vxm" );

	model = mxNEW( _storage, VX::VoxelizedMesh, _storage );

	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	if( _forceRebuild )
	{
		mxDO(model->CreateFromMesh( _sourceMesh, scratchpad ));

		FileWriter	cacheWriter;
		if(mxSUCCEDED( cacheWriter.Open( cachedFilePath.c_str() ) )) {
			mxDO(model->Save( cacheWriter ));
		}
	}
	else
	{
		FileReader	cacheReader;
		if(mxSUCCEDED( cacheReader.Open( cachedFilePath.c_str() ) )) {
			mxDO(model->Load( cacheReader ));
		} else {
			mxDO(model->CreateFromMesh( _sourceMesh, scratchpad ));

			FileWriter	cacheWriter;
			if(mxSUCCEDED( cacheWriter.Open( cachedFilePath.c_str() ) )) {
				mxDO(model->Save( cacheWriter ));
			}
		}
	}

	return ALL_OK;
}

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
