// Binary Space Partitioning
// TODO: optimize building: polygons with different normals cannot be coplanar, etc.
#include <Base/Base.h>
//#include <Base/Template/Containers/HashMap/THashMap.h>
//#include <Base/Template/Containers/HashMap/HashIndex.h>
#include <Core/VectorMath.h>
#include <Core/Util/ScopedTimer.h>
#include "bsp.h"
#include <Utility/Meshok/Octree.h>
#include <XNAMath/XNAMath.h>

#if 0
namespace Meshok
{
namespace BSP
{

#define UnpackUV( UV )	UV
#define PackUV( UV )	UV

mxDEFINE_CLASS(BSP_Vertex);
mxBEGIN_REFLECTION(BSP_Vertex)
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( tag ),
mxEND_REFLECTION;

mxDEFINE_CLASS(BSP_Face);
mxBEGIN_REFLECTION(BSP_Face)
	mxMEMBER_FIELD( vertices ),
	mxMEMBER_FIELD( next ),
mxEND_REFLECTION;

mxDEFINE_CLASS(BSP_Node);
mxBEGIN_REFLECTION(BSP_Node)
	mxMEMBER_FIELD( plane ),
	mxMEMBER_FIELD( front ),
	mxMEMBER_FIELD( back ),
	mxMEMBER_FIELD( faces ),
mxEND_REFLECTION;

mxDEFINE_CLASS_NO_DEFAULT_CTOR(BSP_Tree);
mxBEGIN_REFLECTION(BSP_Tree)
	mxMEMBER_FIELD( m_planes ),	
	mxMEMBER_FIELD( m_nodes ),
	mxMEMBER_FIELD( m_faces ),
//	mxMEMBER_FIELD( m_bounds ),
	mxMEMBER_FIELD( m_epsilon ),
mxEND_REFLECTION;

/*
-----------------------------------------------------------------------------
	SBspStats
-----------------------------------------------------------------------------
*/
BspStats::BspStats()
{
	this->Reset();
}
void BspStats::Reset()
{
	mxZERO_OUT( *this );
}
void BspStats::Print( U32 elapsedTimeMSec )
{
	DBGOUT( "=== BSP statistics ========"			);
	DBGOUT( "Num. Polys(Begin): %u", m_polysBefore	 );
	DBGOUT( "Num. Polys(End):   %u", m_polysAfter		 );
	DBGOUT( "Num. Splits:       %u", m_numSplits			);
	DBGOUT( "Num. Planes:       %u", m_numPlanes );
	DBGOUT( "Num. Inner Nodes:  %u", m_numInternalNodes );
	DBGOUT( "Num. Solid Leaves: %u", m_numSolidLeaves	 );
	DBGOUT( "Num. empty Leaves: %u", m_numEmptyLeaves	 );
	//DBGOUT( "Tree Depth:        %u", depth			 );
	DBGOUT( "Memory used:		%u KiB", m_bytesAllocated / 1024 );
	DBGOUT( "Time elapsed:      %u msec", elapsedTimeMSec		);
	DBGOUT( "==== End ===================="			 );
}

// relation of a polygon to some splitting plane;
// used to classify polygons when building a BSP tree
//
enum EPolyStatus
{
	Poly_Coplanar	= 0,	// The polygon is lying on the plane.
	Poly_Back		= 0x1,	// The polygon is lying in back of the plane.
	Poly_Front		= 0x2,	// The polygon is lying in front of the plane.
	Poly_Split,	// The polygon intersects with the plane.
};

#if 0
static inline
EPlaneSide CalculatePlaneSide(
							  const Vector4& plane,
							  const V3f& point,
							  const float epsilon = ON_EPSILON
							  )
{
	const float distance = Plane_PointDistance( plane, point );
	if ( distance > +epsilon ) {
		return PLANESIDE_FRONT;
	} else if ( distance < -epsilon ) {
		return PLANESIDE_BACK;
	} else {
		return PLANESIDE_ON;
	}
}

static
EPolyStatus ClassifyPolygon(
							const Vector4& plane,
							const BSP_Face& face,
							const float epsilon = ON_EPSILON
							)
{
	const UINT numPoints = face.vertices.num();
	const BSP_Vertex* p = face.vertices.raw();

	UINT	counts[3] = {0};	// for each EPlaneSide

	for( U32 iVertex = 0; iVertex < numPoints; iVertex++ )
	{
		const V3f& point = p[ iVertex ].xyz;
		const EPlaneSide side = CalculatePlaneSide( plane, point, epsilon );
		counts[ side ]++;
	}

	// if the polygon is aligned with the splitting plane
	if ( counts[PLANESIDE_ON] == numPoints ) {
		return Poly_Coplanar;
	}
	// If no negative points, return self as a positive polygon
	else if( !counts[PLANESIDE_BACK] )
	{
		// if nothing at the back of the clipping plane
		return Poly_Front;	// the polygon is completely in front of the splitting plane
	}
	// If no positive points, return self as negative polygon
	else if( !counts[PLANESIDE_FRONT] )
	{
		// if nothing at the front of the clipping plane
		return Poly_Back;	// the polygon is completely behind the splitting plane
	}
	else
		return Poly_Split;
}
#else

EPolyStatus ClassifyPolygon(
							const Vector4& plane,
							const BSP_Face& face,
							const float epsilon = ON_EPSILON
							)
{
	const UINT numPoints = face.vertices.num();
	const BSP_Vertex* p = face.vertices.raw();

	EPolyStatus	status = Poly_Coplanar;

	for( U32 iVertex = 0; iVertex < numPoints; iVertex++ )
	{
		const V3f& point = p[ iVertex ].xyz;
		const float distance = Plane_PointDistance( mxTEMP(V4f&)plane, point );

		if( distance > +epsilon )
		{
			// Is this point on the front side?
			status = (EPolyStatus)(status | Poly_Front);
		}
		else if( distance < -epsilon )
		{
			// Is this point on the back side?
			status = (EPolyStatus)(status | Poly_Back);
		}
	}
	return status;
}

#endif

/// splits the face in a back and front part, this face becomes the front part
/*
Sutherland-Hodgman clipping algorithm:
- For each clip plane:
-  Traverse each line segment in polygon:
-  Test against the plane
- add a vertex if line crosses the plane
See: Splitting a polygon with a plane
http://ezekiel.vancouver.wsu.edu/~cs442/archive/lectures/bsp-trees/poly-split.pdf
*/
static EPlaneSide ClipConvexPolygonInPlace(
	BSP_Face & face,
	BSP_Face & back,	// valid only if the polygon cross the plane
	const V4f& plane,
	const float epsilon = ON_EPSILON
	)
{
	const UINT numPoints = face.vertices.num();
	const BSP_Vertex* p = face.vertices.raw();
	mxASSERT( numPoints > 0 );

	back.vertices.RemoveAll();

	// First, classify all points. This allows us to avoid any bisection if possible

	UINT	counts[3]; // 'front', 'back' and 'on'
	counts[0] = counts[1] = counts[2] = 0;

	TInplaceArray< float, 64 >		dists;
	TInplaceArray< EPlaneSide, 64 >	sides;
	dists.setNum( numPoints + 1 );
	sides.setNum( numPoints + 1 );

	// determine sides for each point
	{
		UINT k;
		for ( k = 0; k < numPoints; k++ )
		{
			// Get the distance from the plane to the vertex
			float dist = Plane_PointDistance( plane, p[k].xyz );
			dists[k] = dist;
			// Classify the point
			if ( dist > +epsilon ) {
				sides[k] = PLANESIDE_FRONT;
			} else if ( dist < -epsilon ) {
				sides[k] = PLANESIDE_BACK;
			} else {
				sides[k] = PLANESIDE_ON;
			}
			// Keep track
			counts[sides[k]]++;
		}
		sides[k] = sides[0];
		dists[k] = dists[0];
	}

	const V3f& planeNormal = Plane_GetNormal( plane );
	const float planeDistance = -plane.w;
#if 1
	// We're a building a node-storing BSP tree, i.e. the coplanar polygons are stored in the node.
	// If we were building a leaf-storing tree, coplanar polygons would be sent to either side of the plane.
	if ( counts[PLANESIDE_ON] == numPoints ) {
		return PLANESIDE_ON;	// the polygon is aligned with the splitting plane
	}
#else
	// Special (and necessary) case:
	//
	// If the polygon rests on the plane, it could be associated with either side... so we pick one.
	// The only criteria we have for this choice is to use the normals....
	//
	// The specific case ("dot product > 0 == associate with front") was chosen for cases in the octree -- a polygon facing
	// the interior of a node, but resting on an outer-most-node boundary. We want it associated with the node, not thrown
	// out completely.
	if ( !counts[PLANESIDE_FRONT] && !counts[PLANESIDE_BACK] )
	{
		const Vector4 polygonPlane = PlaneFromPolygon( face );
		const V3f& polygonNormal = Vector4_As_V3( polygonPlane );
		// if coplanar, put on the front side if the normals match
		if ( polygonNormal * planeNormal > 0.0f ) {
			return PLANESIDE_FRONT;	// Associate with front
		} else {
			return PLANESIDE_BACK;	// Associate with back
		}
	}
#endif
	// If no negative points, return self as a positive polygon
	if( !counts[PLANESIDE_BACK] )
	{
		// if nothing at the back of the clipping plane
		return PLANESIDE_FRONT;	// the polygon is completely in front of the splitting plane
	}
	// If no positive points, return self as negative polygon
	else if( !counts[PLANESIDE_FRONT] )
	{
		// if nothing at the front of the clipping plane
		return PLANESIDE_BACK;	// the polygon is completely behind the splitting plane
	}

	// Straddles the splitting plane - we must clip.

	// Estimate the maximum number of points.
	//const int maxpts = numPoints + 4;	// cant use counts[0]+2 because of fp grouping errors

	TInplaceArray< BSP_Vertex, 64 >	frontPts;
	//TInplaceArray< BSP_Vertex, 64 >	backPts;
	TArray< BSP_Vertex > &			backPts = back.vertices;

	frontPts.reserve( numPoints + 2 );
	backPts.reserve( numPoints + 2 );

	for ( UINT i = 0; i < numPoints; i++)
	{
		const BSP_Vertex* p1 = &p[i];

		if ( sides[i] == PLANESIDE_ON ) {
			frontPts.add( *p1 );
			backPts.add( *p1 );
			continue;
		}
		if ( sides[i] == PLANESIDE_FRONT ) {
			frontPts.add( *p1 );
		}
		if ( sides[i] == PLANESIDE_BACK ) {
			backPts.add( *p1 );
		}
		// Consider the edge from this vertex to the next vertex in the list, which defines a segment
		if ( sides[i+1] == PLANESIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// If the segment crosses the plane, make a new vertex where the segment intersects the plane.

		// generate a split point
		const BSP_Vertex* p2 = &p[(i+1)%numPoints];

		// always calculate the split going from the same side or minor epsilon issues can happen
		BSP_Vertex mid;
		if ( sides[i] == PLANESIDE_FRONT ) {
			float dot = dists[i] / ( dists[i] - dists[i+1] );
			for ( int j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( planeNormal[j] == 1.0f ) {
					mid.xyz[j] = planeDistance;
				} else if ( planeNormal[j] == -1.0f ) {
					mid.xyz[j] = -planeDistance;
				} else {
					mid.xyz[j] = (*p1).xyz[j] + dot * ( (*p2).xyz[j] - (*p1).xyz[j] );
				}
			}
			//const V2f p1st = UnpackUV( p1->UV );
			//const V2f p2st = UnpackUV( p2->UV );
			//mid.UV = PackUV( p1st + ( p2st - p1st ) * dot );
		} else {
			float dot = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( int j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( planeNormal[j] == 1.0f ) {
					mid.xyz[j] = planeDistance;
				} else if ( planeNormal[j] == -1.0f ) {
					mid.xyz[j] = -planeDistance;
				} else {
					mid.xyz[j] = (*p2).xyz[j] + dot * ( (*p1).xyz[j] - (*p2).xyz[j] );
				}
			}
			//const V2f p1st = UnpackUV( p1->UV );
			//const V2f p2st = UnpackUV( p2->UV );
			//mid.UV = PackUV( p2st + ( p1st - p2st ) * dot );
		}

		frontPts.add( mid );
		backPts.add( mid );
	}

	face.vertices = frontPts;
	back.vertices = backPts;

	return PLANESIDE_CROSS;
}

static Vector4 PlaneFromPolygon( const BSP_Face& polygon )
{
	mxASSERT(polygon.vertices.num() >= 3);
	const BSP_Vertex& a = polygon.vertices[0];
	const BSP_Vertex& b = polygon.vertices[1];
	const BSP_Vertex& c = polygon.vertices[2];
	return mxTEMP(Vector4&)Plane_FromThreePoints( a.xyz, b.xyz, c.xyz );
}

BSP_Tree::BSP_Tree( AllocatorI & _allocator )
	: m_nodes( _allocator )
	, m_planes( _allocator )
	, m_faces( _allocator )
{
}

ERet BSP_Tree::Build( const TriMesh& mesh, const Settings& settings )
{
	ScopedTimer	timer;

	m_planes.RemoveAll();
	m_nodes.RemoveAll();
	m_faces.RemoveAll();

	m_epsilon = settings.planeDistanceEpsilon;

	if( settings.store_polygons ) {
		mxDO(this->Build_Traditional( mesh, settings ));
	} else {
		mxDO(this->Build_Without_Polygons( mesh, settings ));
	}

	ptPRINT("Built BSP tree: %u nodes, %u planes, %u tris -> %u faces (%u milliseconds)",
		m_nodes.num(), m_planes.num(), mesh.triangles.num(), m_faces.num(), timer.ElapsedMilliseconds());

	return ALL_OK;
}

struct BSP_Face2: CStruct
{
	TArray< BSP_Vertex >	vertices;	//!< Vertices
	HPlane					plane;	//!< index of the (unique) plane this polygon lies on
	HFace					next;		//!< Next coplanar polygon, if any (also used during building)
	BSP_Vertex				buffer[7];	//!< small embedded storage to avoid memory allocations
public:
	mxDECLARE_CLASS(BSP_Face,CStruct);
	mxDECLARE_REFLECTION;
	//BSP_Face2( AllocatorI & _allocator )
	//	: vertices( _allocator )
	BSP_Face2()
	{
		vertices.initializeWithExternalStorage( buffer, mxCOUNT_OF(buffer) );
		next = NIL_FACE;
	}
};

/// contains temporary data used for building a BSP-tree.
struct BSP_Builder
{
	//typedef U32 PlaneIndexT;

	DynamicArray< BSP_Face2 >	faces;
	//THashMap2< U32, HPlane >	planes;
	idHashIndex					planeIndex;	//!< maps plane hash -> plane index

public:
	BSP_Builder( AllocatorI & _allocator )
		: faces( _allocator )
		, planeIndex( _allocator )
	{
	}
};

static inline
bool Planes_Equal( const V4f& planeA, const V4f& planeB, const float normalEpsilon, const float distanceEpsilon ) {
	if ( mmAbs( planeA.w - planeB.w ) > distanceEpsilon ) {
		return false;
	}
	const V3f normalA = Plane_GetNormal( planeA );
	const V3f normalB = Plane_GetNormal( planeB );
	return V3_Equal( normalA, normalB, normalEpsilon );
}

static
HPlane GetOrAdd_UniquePlane(
							const V4f& _plane,
							const float normalEpsilon, const float distanceEpsilon,
							idHashIndex & _planesTable, DynamicArray< Vector4 > & _planesArray
							)
{
	mxSTOLEN("idPlaneSet from idSoftware, Doom3/Prey/Quake4 SDKs");
	mxASSERT( distanceEpsilon <= 0.125f );

	const int hashKey = (int)( mmAbs( _plane.w ) * 0.125f );
	for ( int border = -1; border <= 1; border++ ) {
		for ( int i = _planesTable.First( hashKey + border ); i >= 0; i = _planesTable.Next( i ) ) {
			if ( Planes_Equal( Vector4_As_V4(_planesArray[i]), _plane, normalEpsilon, distanceEpsilon ) ) {
				return HPlane( i );
			}
		}
	}

	const HPlane newPlaneId( _planesArray.num() );
	_planesArray.add( Vector4_Set( _plane ) );
	_planesTable.add( hashKey, newPlaneId.id );
	return newPlaneId;
}
#if 0

const HFace FindBestSplitter2(
	const HFace polygons,
	const BSP_Tree& _tree,
	const BSP_Builder& _builder,
	const SplittingCriteria& options
	)
{
	HFace	bestSplitter(polygons);	// the first polygon by default
	float	bestScore = 1e6f;	// the less value the better

	// for each EPolyStatus - must be signed!
	int		faceCounts[4] = { 0 };

	// Search through all polygons and find:
	// A. The number of splits each poly would make.
	// B. The number of front and back nodes the polygon would create.
	// C. The number of coplanars.

	// Classify each polygon in the current node with respect to the partitioning plane.
	HFace iFaceA = polygons;

	while( iFaceA != NIL_FACE )
	{
		// select potential splitter
		const BSP_Face& faceA = m_faces[ iFaceA ];

		// potential splitting plane
		const Vector4 planeA = PlaneFromPolygon( faceA );

		// test other polygons against the potential splitter
		HFace iPolyB = polygons;
		while( iPolyB != NIL_FACE )
		{
			const BSP_Face& faceB = m_faces[ iPolyB ];
			// Ignore testing against self
			if( iFaceA != iPolyB )
			{
				// evaluate heuristic cost and select the best candidate

				const EPolyStatus side = ClassifyPolygon(
					planeA,
					faceB,
					options.planeDistanceEpsilon
				);
				faceCounts[side]++;

				// diff == 0 => tree is perfectly balanced
				const int diff = Abs<int>( faceCounts[Poly_Front] - faceCounts[Poly_Back] );

				// Compute score as a weighted combination between balance and splits (lower score is better)
				float score = (diff * options.balanceVsCuts)
					+ (faceCounts[Poly_Split] * options.splitCost) * (1.0f - options.balanceVsCuts)
					;

				if( CalculatePlaneType(mxTEMP(V4f&)planeA) < PLANETYPE_TRUEAXIAL )
				{
					score *= 0.8f;	// axial is better
				}

				// A smaller score will yield a better partition
				if( score < bestScore )
				{
					bestScore = score;
					bestSplitter = iFaceA;
				}
			}
			iPolyB = faceB.next;
		}//for all tested polygons
		iFaceA = faceA.next;
	}//for all potential splitters

	mxASSERT(bestSplitter != NIL_FACE);
	return bestSplitter;
}


const HNode BuildTreeRecursive2( BSP_Tree &_tree, const HFace polygons, const BSP_Tree::Settings& settings )
{
	// Select the best partitioner
	const HFace iBestSplitter = FindBestSplitter2( polygons, settings );
	const Vector4 splittingPlane = PlaneFromPolygon( m_faces[ iBestSplitter ] );

	// Bisect the scene
	HFace	frontFaces = NIL_FACE;
	HFace	backFaces = NIL_FACE;
	HFace	coplanar = NIL_FACE;
	UINT	faceCounts[4] = {0};	// for each EPolyStatus

	PartitionPolygons(
		splittingPlane, polygons, &frontFaces, &backFaces, &coplanar, faceCounts
	);

	// allocate a new internal node
	const HNode iNewNode( m_nodes.num() );
	m_nodes.add(BSP_Node());

	m_nodes[iNewNode].plane = AddUniquePlane( splittingPlane );
	m_nodes[iNewNode].faces = coplanar;

	// Recursively split the front and back polygons.
	if( frontFaces != NIL_FACE ) {
		m_nodes[iNewNode].front = BuildTreeRecursive2( frontFaces, settings );
	} else {
		m_nodes[iNewNode].front = MAKE_LEAF_ID(0, EMPTY_LEAF);
	}
	if( backFaces != NIL_FACE ) {
		m_nodes[iNewNode].back = BuildTreeRecursive2( backFaces, settings );
	} else {
		m_nodes[iNewNode].back = MAKE_LEAF_ID(0, SOLID_LEAF);
	}

	//DBGOUT("add [%u]: back=%u, front=%u",
	//	iNewNode, m_nodes[iNewNode].back, m_nodes[iNewNode].front);

	return iNewNode;
}

ERet BSP_Tree::Build2( const TriMesh& mesh, AllocatorI & scratch, const Settings& settings )
{
	ScopedTimer	timer;

	m_planes.RemoveAll();
	m_nodes.RemoveAll();
	m_faces.RemoveAll();

	m_epsilon = settings.planeDistanceEpsilon;


	BSP_Builder	builder( scratch );

	const UINT numTriangles = mesh.triangles.num();
	mxENSURE(numTriangles > 0, ERR_INVALID_PARAMETER, "No polygons");

	// there will be at least so many nodes as coplanar polygons;
	// N*log(N) in average case and N^2 in worst case.
	mxDO(builder.faces.setNum(numTriangles));
//	mxDO(builder.planes.Initialize(numTriangles/2));

	// Convert triangles to splittable BSP polygons.
	for( UINT iFace = 0; iFace < numTriangles; iFace++ )
	{
		const TriMesh::Triangle& triangle = mesh.triangles[ iFace ];

		const V3f& vertex0 = mesh.vertices[ triangle.v[0] ].xyz;
		const V3f& vertex1 = mesh.vertices[ triangle.v[1] ].xyz;
		const V3f& vertex2 = mesh.vertices[ triangle.v[2] ].xyz;

		const V4f trianglePlane = Plane_FromThreePoints( vertex0, vertex1, vertex2 );

		BSP_Face2 & newFace = builder.faces[ iFace ];

		newFace.plane = GetOrAdd_UniquePlane(
			trianglePlane,
			settings.planeNormalEpsilon, settings.planeDistanceEpsilon,
			builder.planeIndex, m_planes
			);

		mxDO(newFace.vertices.setNum(3));
		newFace.vertices[0].xyz = mesh.vertices[ triangle.v[0] ].xyz;
		newFace.vertices[1].xyz = mesh.vertices[ triangle.v[1] ].xyz;
		newFace.vertices[2].xyz = mesh.vertices[ triangle.v[2] ].xyz;
	}

	// Recursively build the tree

	m_planes.reserve(numTriangles);
	m_nodes.reserve(numTriangles);

	BuildTreeRecursive2( HFace(0), *this, settings );

	ptPRINT("Built BSP tree: %u nodes, %u planes, %u tris -> %u faces (%u milliseconds)",
		m_nodes.num(), m_planes.num(), mesh.triangles.num(), m_faces.num(), timer.ElapsedMilliseconds());

	return ALL_OK;
}

#endif

ERet BSP_Tree::Build_Traditional( const TriMesh& mesh, const Settings& settings )
{
	const UINT numTriangles = mesh.triangles.num();
	mxENSURE(numTriangles > 0, ERR_INVALID_PARAMETER, "No polygons");

	// create BSP polygons for splitting

	// Convert triangles to BSP polygons
	mxDO(m_faces.setNum(numTriangles));
	for( UINT iTri = 0; iTri < numTriangles; iTri++ )
	{
		const TriMesh::Triangle& triangle = mesh.triangles[ iTri ];

		BSP_Face & newPoly = m_faces[ iTri ];

		newPoly.vertices.setNum(3);
		newPoly.vertices[0].xyz = mesh.vertices[ triangle.v[0] ].xyz;
		newPoly.vertices[1].xyz = mesh.vertices[ triangle.v[1] ].xyz;
		newPoly.vertices[2].xyz = mesh.vertices[ triangle.v[2] ].xyz;

		// setup a linked list of polygons
		newPoly.next = HFace( iTri + 1 );

//V3f center = (newPoly.vertices[0].xyz + newPoly.vertices[1].xyz + newPoly.vertices[2].xyz) / 3.0f;
//g_pDbgView->addPointWithNormal( center, Plane_GetNormal( PlaneFromPolygon(newPoly) ) );
	}
	m_faces.getLast().next = NIL_FACE;

	// Recursively build the tree

	// there will be at least so many nodes as coplanar polygons;
	// N*log(N) in average case and N^2 in worst case.
	m_planes.reserve(numTriangles);
	m_nodes.reserve(numTriangles);

	BuildTreeRecursive( HFace(0), settings );

	return ALL_OK;
}
mxUNDONE
ERet BSP_Tree::Build_Without_Polygons( const TriMesh& mesh, const Settings& settings )
{
	const UINT numTriangles = mesh.triangles.num();
	mxENSURE(numTriangles > 0, ERR_INVALID_PARAMETER, "No polygons");
#if 0
	typedef U32 FaceID;
	const FaceID NO_FACE = ~0;

	/// temporary polygon used for building a BVH
	struct BspFace
	{
		U32		plane;
		//AABB	bounds;
		FaceID	next;
	};

	DynamicArray< BspFace >	faces( Memory::Scratch() );
	mxDO(faces.setNum(numTriangles));

	//DynamicArray< U32 >	sortedFaceIDs( Memory::Scratch() );
	//mxDO(sortedFaceIDs.setNum(numTriangles));

	// Convert triangles to BSP polygons
	{
		for( FaceID iFace = 0; iFace < numTriangles; iFace++ )
		{
			const TriMesh::Triangle& triangle = mesh.triangles[ iFace ];

			const V3f vertex0 = mesh.vertices[ triangle.v[0] ].xyz;
			const V3f vertex1 = mesh.vertices[ triangle.v[1] ].xyz;
			const V3f vertex2 = mesh.vertices[ triangle.v[2] ].xyz;

			BspFace & face = m_faces[ iFace ];

			const Vector4 trianglePlane = Plane_FromThreePoints( vertex0, vertex1, vertex2 );
			face.plane = AddUniquePlane( trianglePlane );

			face.next = iFace + 1;	// setup a linked list of polygons
		}
		faces.getLast().next = ~0;
	}

	struct BspBuildItem {
		/// If non-zero then this is the index of the parent (used in offsets).
		U32 parent;
		U32	faceList;
		/// The range of objects in the object list covered by this node.
		U32 start, end;
	};

	BspBuildItem todo[128];
	U32 iStackTop = 0;

	// Push the root
	todo[iStackTop].start = 0;
	todo[iStackTop].end = numTriangles;
	todo[iStackTop].parent = ~0;
	iStackTop++;

	while( iStackTop > 0 )
	{
		// Pop the next item off of the stack
		const BspBuildItem& stackItem = todo[--iStackTop];
		const U32 start = stackItem.start;
		const U32 end = stackItem.end;
		const U32 nPrims = end - start;

		BSP_Node newNode;
		newNode.count = 0;
		newNode.iRightChild = Untouched;	// children will decrement this field later during building

		const U32 iThisNode = m_nodes.num();	// index of the new node
		m_nodes.add(newNode);

		// Child touches parent...
		// Special case: Don't do this for the root.
		const U32 parentIndex = stackItem.parent;
		if( parentIndex != RootNodeID )
		{
			m_nodes[parentIndex].iRightChild--;

			// When this is the second touch, this is the right child.
			// The right child sets up the offset for the flat tree.
			if( m_nodes[parentIndex].iRightChild == TouchedTwice ) {
				m_nodes[parentIndex].iRightChild = iThisNode;
			}
		}


		{
			//
		}


		// Partition the list of polygons.
		{
			UINT	faceCounts[4] = {0};	// for each EPolyStatus
			FaceID	iFace = stackItem.faceList;
			while( iFace != NO_FACE )
			{
				const EPolyStatus side = ClassifyPolygon(
					planeA,
					faceB,
					options.planeEpsilon
				);
				faceCounts[side]++;


				if( faces[m_indices[i]].center[splitAxis] < split_coord ) {
					TSwap( m_indices[i], m_indices[mid] );
					++mid;
				}
			}
		}

		// If we get a bad split, just choose the center...
		if( mid == start || mid == end ) {
			mid = start + (end - start) / 2;
		}

		// Push the right child first: when the right child is popped off the stack (after the left child),
		// it will Initialize the 'right child offset' in the parent.
		todo[iStackTop].start = mid;
		todo[iStackTop].end = end;
		todo[iStackTop].parent = iThisNode;
		iStackTop++;

		// Then push the left child.
		todo[iStackTop].start = start;
		todo[iStackTop].end = mid;
		todo[iStackTop].parent = iThisNode;
		iStackTop++;
	}
#endif

	return ALL_OK;
}

const HNode BSP_Tree::BuildTreeRecursive( const HFace polygons, const Settings& settings )
{
	// Select the best partitioner
	const HFace iBestSplitter = FindBestSplitter( polygons, settings );
	const Vector4 splittingPlane = PlaneFromPolygon( m_faces[ iBestSplitter ] );

	// Bisect the scene
	HFace	frontFaces = NIL_FACE;
	HFace	backFaces = NIL_FACE;
	HFace	coplanar = NIL_FACE;
	UINT	faceCounts[4] = {0};	// for each EPolyStatus

	PartitionPolygons(
		splittingPlane, polygons, &frontFaces, &backFaces, &coplanar, faceCounts
	);

	// allocate a new internal node
	const HNode iNewNode( m_nodes.num() );
	m_nodes.add(BSP_Node());

	m_nodes[iNewNode].plane = AddUniquePlane( splittingPlane );
	m_nodes[iNewNode].faces = coplanar;

	// Recursively split the front and back polygons.
	if( frontFaces != NIL_FACE ) {
		m_nodes[iNewNode].front = BuildTreeRecursive( frontFaces, settings );
	} else {
		m_nodes[iNewNode].front = MAKE_LEAF_ID(0, EMPTY_LEAF);
	}
	if( backFaces != NIL_FACE ) {
		m_nodes[iNewNode].back = BuildTreeRecursive( backFaces, settings );
	} else {
		m_nodes[iNewNode].back = MAKE_LEAF_ID(0, SOLID_LEAF);
	}

	//DBGOUT("add [%u]: back=%u, front=%u",
	//	iNewNode, m_nodes[iNewNode].back, m_nodes[iNewNode].front);

	return iNewNode;
}

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
// and avoid introducing new partitioning planes
//
const HFace BSP_Tree::FindBestSplitter(
	const HFace polygons,
	const SplittingCriteria& options
	)
{
	HFace	bestSplitter(polygons);	// the first polygon by default
	float	bestScore = 1e6f;	// the less value the better

	// for each EPolyStatus - must be signed!
	int		faceCounts[4] = { 0 };

	// Search through all polygons in the pool and find:
	// A. The number of splits each poly would make.
	// B. The number of front and back nodes the polygon would create.
	// C. Number of coplanars.

	// Classify each polygon in the current node with respect to the partitioning plane.
	HFace iFaceA = polygons;

	while( iFaceA != NIL_FACE )
	{
		// select potential splitter
		const BSP_Face& faceA = m_faces[ iFaceA ];

		// potential splitting plane
		const Vector4 planeA = PlaneFromPolygon( faceA );

		// test other polygons against the potential splitter
		HFace iPolyB = polygons;
		while( iPolyB != NIL_FACE )
		{
			const BSP_Face& faceB = m_faces[ iPolyB ];
			// Ignore testing against self
			if( iFaceA != iPolyB )
			{
				// evaluate heuristic cost and select the best candidate

				const EPolyStatus side = ClassifyPolygon(
					planeA,
					faceB,
					options.planeDistanceEpsilon
				);
				faceCounts[side]++;

				// diff == 0 => tree is perfectly balanced
				const int diff = Abs<int>( faceCounts[Poly_Front] - faceCounts[Poly_Back] );

				// Compute score as a weighted combination between balance and splits (lower score is better)
				float score = (diff * options.balanceVsCuts)
					+ (faceCounts[Poly_Split] * options.splitCost) * (1.0f - options.balanceVsCuts)
					;

				if( CalculatePlaneType(mxTEMP(V4f&)planeA) < PLANETYPE_TRUEAXIAL )
				{
					score *= 0.8f;	// axial is better
				}

				// A smaller score will yield a better partition
				if( score < bestScore )
				{
					bestScore = score;
					bestSplitter = iFaceA;
				}
			}
			iPolyB = faceB.next;
		}//for all tested polygons
		iFaceA = faceA.next;
	}//for all potential splitters

	mxASSERT(bestSplitter != NIL_FACE);
	return bestSplitter;
}

void BSP_Tree::PartitionPolygons(
	const Vector4& partitioner,	// splitting plane
	const HFace polygons,	// linked list
	HFace *frontFaces,	// linked list
	HFace *backFaces,	// linked list
	HFace *coplanar,	// linked list
	UINT faceCounts[4],	// stats (poly counts for each EPlaneSide)
	const float epsilon
	)
{
	BSP_Face	backFace;

	HFace iPoly = polygons;
	while( iPoly != NIL_FACE )
	{
		BSP_Face &	face = m_faces[ iPoly ];
		const HFace iNextPoly = face.next;

		const EPlaneSide side = ClipConvexPolygonInPlace(
			face,
			backFace,
			mxTEMP(V4f&)partitioner,
			epsilon
		);

		faceCounts[side]++;

		if( side == PLANESIDE_CROSS )
		{
			mxASSERT( face.vertices.num() && backFace.vertices.num() );

			// this face becomes the 'front' part
			face.next = *frontFaces;
			*frontFaces = iPoly;

			// create a new poly from the 'back' part
			const HFace iNewFace(m_faces.num());
			m_faces.add(backFace);
			m_faces.getLast().next = *backFaces;
			*backFaces = iNewFace;
		}
		else if( side == PLANESIDE_FRONT )
		{
			face.next = *frontFaces;
			*frontFaces = iPoly;
		}
		else if( side == PLANESIDE_BACK )
		{
			face.next = *backFaces;
			*backFaces = iPoly;
		}
		else
		{
			mxASSERT( side == PLANESIDE_ON );
			face.next = *coplanar;
			*coplanar = iPoly;
		}

		// continue
		iPoly = iNextPoly;
	}
}

const HPlane BSP_Tree::AddUniquePlane(
	const Vector4& plane,
	const float normalEps,
	const float distEps
	)
{
	const U32 newPlaneIndex = m_planes.num();
	m_planes.add(plane);
	return HPlane(newPlaneIndex);
}

void BSP_Tree::RemoveFaceData()
{
	for( UINT iNode = 0; iNode < m_nodes.num(); iNode++ )
	{
		m_nodes[ iNode ].faces = NIL_FACE;
	}
	m_faces.RemoveAll();
}

bool BSP_Tree::Debug_Validate() const
{
	bool bOk = true;
	for( UINT iPlane = 0; iPlane < m_planes.num(); iPlane++ )
	{
		bOk &= V3_IsNormalized( Vector4_As_V3( m_planes[ iPlane ] ) );
		mxASSERT2(bOk, "Plane %d not normalized", iPlane);
	}
	return bOk;
}

ERet BSP_Tree::SaveToBlob(
	NwBlobWriter & _stream
) const
{
	Header_d header;
	header.num_nodes = m_nodes.num();
	header.num_planes = m_planes.num();
	header.plane_epsilon = m_epsilon;
	mxDO(_stream.Put( header ));

	mxDO(_stream.Write( m_nodes.raw(), m_nodes.rawSize() ));
	mxDO(_stream.Write( m_planes.raw(), m_planes.rawSize() ));

	return ALL_OK;
}

void BSP_Tree::Translate( const V3f& _translation )
{
	for( UINT iPlane = 0; iPlane < m_nodes.num(); iPlane++ )
	{
		Vector4 & plane = m_planes[ iPlane ];
		mxTEMP(V4f&)plane = Plane_Translate( mxTEMP(V4f&)plane, _translation );	//plane.w -= Plane_GetNormal(plane) * T;
	}
}
void BSP_Tree::Scale( const float scale )
{
	for( UINT iPlane = 0; iPlane < m_nodes.num(); iPlane++ )
	{
		Vector4 & plane = m_planes[ iPlane ];
		(mxTEMP(V4f&)plane).w *= scale;
	}
}

mxREMOVE_THIS
#if 0
// Transforms a 4D point by a 4x4 matrix.
mxFORCEINLINE const V4f M44_Transform4( const M44f& m, const V4f& p )
{
	V4f	result;
	result.x = (m.r0.x * p.x) + (m.r0.y * p.y) + (m.r0.z * p.z) + (m.r0.w * p.w);
	result.y = (m.r1.x * p.x) + (m.r1.y * p.y) + (m.r1.z * p.z) + (m.r1.w * p.w);
	result.z = (m.r2.x * p.x) + (m.r2.y * p.y) + (m.r2.z * p.z) + (m.r2.w * p.w);
	result.w = (m.r3.x * p.x) + (m.r3.y * p.y) + (m.r3.z * p.z) + (m.r3.w * p.w);
	return result;
}
//
//		mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w * vec.w,
//		mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w * vec.w,
//		mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w * vec.w,
//		mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w * vec.w );

mxFORCEINLINE const V4f TransformPlaneByMatrix( const V4f& plane, const M44f& m )
{
	V3f	planeNormal = Plane_GetNormal( plane );
	V3f	pointOnPlane = planeNormal * -plane.w;

	V3f	newPlaneNormal = M44_TransformNormal( m, planeNormal );
	V3f	newPointOnPlane = M44_TransformPoint( m, pointOnPlane );

	newPlaneNormal = V3_Normalized( newPlaneNormal );

	return Plane_FromPointNormal( newPointOnPlane, newPlaneNormal );
}

/// Transforms all planes by an angle preserving transform.
void BSP_Tree::TransformBy( const M44f& _matrix )
{
	for( UINT iPlane = 0; iPlane < m_nodes.num(); iPlane++ )
	{
		Vector4 & plane = m_planes[ iPlane ];

		plane = M44_Transform( _matrix, plane );
#if 0
		//plane = M44_Transform4( _matrix, plane );
		plane = M44_Transform( _matrix, plane );

		V3f & N = *reinterpret_cast< V3f* >( &plane );
		N = V3_Normalized(N);
		//Plane_Normalize( plane );
#endif

		//plane = TransformPlaneByMatrix( plane, _matrix );
	}
}
void BSP_Tree::Transform( const V3f& _translation, const float _scaling )
{
	const M44f localToWorld = M44_BuildTRS( _translation, _scaling );
	const M44f localToWorldIT = M44_Transpose( M44_OrthoInverse( localToWorld ) );
//TODO: optimize!
	for( UINT iPlane = 0; iPlane < m_planes.num(); iPlane++ )
	{
		Vector4 & plane = m_planes[ iPlane ];

#if 0//:(
		(XMVECTOR&)plane = XMPlaneTransform((XMVECTOR&)plane, (XMMATRIX&)localToWorldIT);
#else
		const V3f	planeNormal = Plane_GetNormal( plane );
		const V3f	pointOnPlane = planeNormal * -plane.w;
		V3f	newPointOnPlane = M44_TransformPoint( localToWorld, pointOnPlane );
		V3f	newPlaneNormal = M44_TransformNormal( localToWorldIT, planeNormal );
		//newPlaneNormal = V3_Normalized( newPlaneNormal );

		//plane = Plane_FromPointNormal( newPointOnPlane, newPlaneNormal );

		V3f& N = (V3f&)plane;
		N = V3_Normalized( newPlaneNormal );
		//plane.w = -V3_Dot( newPlaneNormal, newPointOnPlane );
		plane.w = -V3_Dot( N, newPointOnPlane );
#endif
	}
}
#endif

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
	const BSP_Tree& tree,
	const HNode nodeIndex,
	const UINT currentDepth
	)
{
	if( !IS_LEAF( nodeIndex ) )
	{
		const BSP_Node& node = tree.m_nodes[ nodeIndex ];
		const Vector4& plane = tree.m_planes[ node.plane ];
		{
			LogStream	log(LL_Info);
			log.Repeat('\t', currentDepth);
			log << "Node[" << nodeIndex.id << "]: plane=" << plane << ", back: " << GetNodeName(node.back) << ", front: " << GetNodeName(node.front);
		}
		DbgPrint_R( tree, node.back, currentDepth + 1 );
		DbgPrint_R( tree, node.front, currentDepth + 1 );
	}
}
void BSP_Tree::DbgPrint()
{
	DbgPrint_R( *this, HNode(0), 0 );
}

// Returns the total amount of occupied memory in bytes.
size_t BSP_Tree::BytesAllocated() const
{
	return m_nodes.allocatedMemorySize()
		+ m_planes.allocatedMemorySize()
		+ m_faces.allocatedMemorySize()
		;
}

bool BSP_Tree::IsEmpty() const
{
	return m_nodes.IsEmpty();
}

bool BSP_Tree::PointInSolid( const V3f& point ) const
{
	const Vector4 point4 = Vector4_Set( point, 1.0f );

	UINT nodeIndex = 0;
	// If < 0, we are in a leaf node.
	while( !IS_LEAF( nodeIndex ) )
	{
		const BSP_Node& node = m_nodes[ nodeIndex ];
		// find which side of the node we are on
		const Vector4& plane = m_planes[ node.plane ];
		const float distance = Vector4_Get_X( Vector4_Dot( plane, point4 ) );
		// If it is not necessary to categorize points as lying on the boundary
		// and they can instead be considered inside the solid,
		// this test can be simplified to visit the back side
		// when the point is categorized as on the dividing plane.
		const UINT nearIndex = (distance > 0/*m_epsilon*/);	// Child of Node for half-space containing the point: 1 - 'front', 0 - 'back' and 'on'.
		const UINT sides[2] = { node.back, node.front };
		// Go down the appropriate side.
		nodeIndex = sides[ nearIndex ];
	}
	// Now at a leaf, inside/outside status determined by the leaf type.
	return IS_SOLID_LEAF( nodeIndex );
}

#if 0
	// returns > 0, if outside
	// negative distance - the point is inside solid region
	float DistanceToPointR( const V3f& point, float epsilon = ON_EPSILON, UINT nodeIndex = 0 ) const
	{
		if( IS_LEAF( nodeIndex ) )
		{
			mxFIXME("")
			return IS_SOLID_LEAF(nodeIndex) ? -1.0f : +1.0f;
		}
		// find which side of the node we are on
		const BSP_Node& node = m_nodes[ nodeIndex ];
		const Vector4& plane = m_planes[ node.plane ];
		const float distance = Plane_PointDistance( plane, point );
		// Go down the appropriate side
		if( distance > +epsilon ) {
			return DistanceToPointR( point, epsilon, node.front );
		} else if( distance < -epsilon ) {
			return DistanceToPointR( point, epsilon, node.back );
		} else {
			// Point on dividing plane; must traverse both sides
			float dist1 = DistanceToPointR( point, epsilon, node.front );
			float dist2 = DistanceToPointR( point, epsilon, node.back );
			// return the minimum (closest) distance
			return fabs(dist1) < fabs(dist2) ? dist1 : dist2;
		}
	}
#endif

mxBIBREF("'Real-Time Collision Detection' by Christer Ericson (2005), 8.4.2 Intersecting a Ray Against a Solid-leaf BSP Tree, PP.376-377");
	// Intersect ray/segment R(t)=p+t*d, tmin <= t <= tmax, against bsp tree ’node’,
	// returning time thit of first intersection with a solid leaf, if any.
#if 0
	// Intersect ray/segment R(t)=p+t*d, tmin <= t <= tmax, against bsp tree
	// ’node’, returning time thit of first intersection with a solid leaf, if any
	int RayIntersect(BSPNode *node, Point p, Vector d, float tmin, float tmax, float *thit)
	{
		std::stack<BSPNode *> nodeStack;
		std::stack<float> timeStack;
		assert(node != NULL);
		while (1) {
			if (!node->IsLeaf()) {
				float denom = Dot(node- >plane.n, d);
				float dist = node->plane.d - Dot(node->plane.n, p);
				int nearIndex = dist > 0.0f;
				// If denom is zero, ray runs parallel to plane. In this case,
				// just fall through to visit the near side (the one p lies on)
				if (denom != 0.0f) {
					float t = dist / denom;
					if (0.0f <= t && t <= tmax) {
						if (t >= tmin) {
							// Straddling, push far side onto stack, then visit near side
							nodeStack.push(node->child[1^nearIndex]);
							timeStack.push(tmax);
							tmax = t;
						} else nearIndex = 1^nearIndex;// 0 <= t < tmin, visit far side
					}
				}
				node = node->child[nearIndex];
			} else {
				// Now at a leaf. If it is solid, there’s a hit at time tmin, so exit
				if (node->IsSolid()) {
					*thit = tmin;
					return 1;
				}
				// Exit if no more subtrees to visit, else pop off a node and continue
				if (nodeStack.empty()) break;
				tmin = tmax;
				node = nodeStack.top(); nodeStack.pop();
				tmax = timeStack.top(); timeStack.pop();
			}
		}
		// No hit
		return 0;
	}
#endif

/// ray stabbing query, recursive version.
static bool CastRay_R(
	const V3f& start, const V3f& direction,
	const BSP_Tree& tree,
	const HNode nodeIndex,
	float tmin, float tmax,
	const HNode parentIndex,
	float *thit, V3f *hitNormal,
	const bool startInSolid
	)
{
	if( IS_INTERNAL( nodeIndex ) )
	{
		const BSP_Node& node = tree.m_nodes[ nodeIndex ];
		const V4f& plane = mxTEMP(V4f&) tree.m_planes[ node.plane ];
		const float distance = Plane_PointDistance( plane, start );
		const V3f planeNormal = Plane_GetNormal( plane );
		const float denom = V3_Dot( planeNormal, direction );
		const int nearIndex = (distance > 0.f);	// child index for half-space containing the ray origin: 0 - back, 1 - front
		const HNode sides[2] = { node.back, node.front };
		int nearSide = nearIndex;

		// If denom is zero, ray runs parallel to plane. In this case,
		// just fall through to visit the near side (the one 'start' lies on).
		if( denom != 0.0f )
		{
			const float t = -distance / denom;	// distance along the ray until the intersection with the plane

			// Determine which side(s) of the plane the ray should be sent down.
			if( 0.0f <= t && t <= tmax )
			{
				if( t >= tmin ) {
					// visit the near side with the ray interval [tmin, t]
					if( CastRay_R( start, direction, tree, sides[ nearSide ], tmin, t, parentIndex, thit, hitNormal, startInSolid ) ) {
						return true;
					}
					// visit the far side with the ray interval [t, tmax]
					return CastRay_R( start, direction, tree, sides[ nearSide^1 ], t, tmax, nodeIndex, thit, hitNormal, startInSolid );
				} else {
					// 0 <= t < tmin, i.e. the ray crosses the plane before 'tmin' => only the far side needs to be traversed
					nearSide = 1^nearSide;
				}
			}
		}
		return CastRay_R( start, direction, tree, sides[ nearSide ], tmin, tmax, parentIndex, thit, hitNormal, startInSolid );
	}
	else
	{
		// Now at a leaf. If there's a hit at time tmin, then exit.
		const bool endInSolid = IS_SOLID_LEAF( nodeIndex );
		if( startInSolid != endInSolid ) {
			*thit = tmin;
			const BSP_Node& parentNode = tree.m_nodes[ parentIndex.id ];
			const Vector4& parentNodePlane = tree.m_planes[ parentNode.plane.id ];
			*hitNormal = Plane_GetNormal(mxTEMP(V4f&) parentNodePlane);
			return true;
		}
	}
	// No hit
	return false;
}

bool BSP_Tree::CastRay(
	const V3f& start, const V3f& direction,
	float tmin, float tmax,
	float *thit, V3f *hitNormal,
	const bool startInSolid /*= false*/
	) const
{
	const bool bHit = CastRay_R( start, direction, *this, HNode(0), tmin, tmax, HNode(0), thit, hitNormal, startInSolid );
	return bHit;
}

BSP_Tree::Sampler::Sampler( const BSP_Tree& _tree, const AABBf& _bounds, const Int3& _resolution )
	: m_tree( _tree )
{
	m_cellSize = V3_Divide( _bounds.size(), V3f::fromXYZ(_resolution) );
	m_offset = _bounds.min_corner;
}

bool BSP_Tree::Sampler::IsInside(
	int iCellX, int iCellY, int iCellZ
) const
{
	const V3f cornerXYZ = V3f::set( iCellX, iCellY, iCellZ );
	const V3f cellCorner = m_offset + V3_Multiply( cornerXYZ, m_cellSize );
	return m_tree.PointInSolid( cellCorner );
}

bool BSP_Tree::Sampler::CastRay(
	const V3f& start, const V3f& direction,
	float tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
	float *thit, V3f *hitNormal,
	const bool startInSolid /*= false*/
) const
{
	const bool intersects = m_tree.CastRay(
		m_offset + start, direction,
		0.0f, tmax,
		thit, hitNormal,
		startInSolid
	);
	return intersects;
}

U32 BSP_Tree::Sampler::SampleHermiteData(
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
	if( vxIS_ZERO_OR_0xFF(signMask) )
	{
		return signMask;
	}

	int numWritten = 0;

	U32 edgeMask = CUBE_ACTIVE_EDGES[ signMask ];
	while( edgeMask )
	{
		U32	iEdge;
		edgeMask = TakeNextTrailingBit32( edgeMask, &iEdge );

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
	}

	_sample.numPoints = numWritten;

	return signMask;
}

}//namespace BSP
}//namespace Meshok

/*
some ideas on how to reduce memory consumption:
- store polygons in 2D (plane is known)
- don't store UVs per each vertex - store polygon's basis
- don't store polygons at all - derive b-rep (or, at least, some vertex attributes)
*/

/*
Useful references:
BSP Tips
October 30, 2000
by Charles Bloom, cbloom@cbloom.com
http://www.cbloom.com/3d/techdocs/bsp_tips.txt

source code:
CSGTOOL is a library, Ruby Gem and command line tool for performing Constructive Solid Geometry operations on STL Files using 3D BSP Trees.
https://github.com/sshirokov/csgtool

https://github.com/apache/commons-math/blob/master/src/main/java/org/apache/commons/math4/geometry/partitioning/BSPTree.java

https://github.com/spiked3/CnCgo7/blob/master/CnCgo7/Math/Bsp2.cs

Papers:
Efficient Boundary Extraction of BSP Solids Based on Clipping Operations

http://www.me.mtu.edu/~rmdsouza/CSG_BSP.html
*/
#endif
