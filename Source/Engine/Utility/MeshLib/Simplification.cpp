// Mesh Simplification
#include <Base/Base.h>
#pragma hdrstop
#include <algorithm>	// std::sort
#include <Meshok/Meshok.h>
#include <MeshLib/Simplification.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Util/ScopedTimer.h>

namespace Meshok
{

#if 0
//NOTE: not normalized to [0..1], because edge collapse cost depends on edge length
//#define MAX_COST	(1e5f)
#define MAX_COST	(1.0f)

struct FullEdgeInfo
{
	HEdge	halfEdge[2];	//!< half-edge and its twin
	EdgeTag	edgeTags[2];
	HFace	adjacent[2];	//!< left and right sides
	HVertex	endPoint[2];	//!< startVertex, endVertex
	UINT	relative[2];	//!< face-relative vertex indices, [0..3)
};

const FullEdgeInfo GetEdgeInfo( const HEdge iEdge, const ProgressiveMesh& m )
{
	const Adjacency& adj = m.adjacency;

	const HEdge iOpposite = adj.TwinEdge( iEdge );

	const EdgeTag edgeTag = adj.halfEdges[ iEdge ];
	const EdgeTag oppositeTag = adj.halfEdges[ iOpposite ];

	const HFace iFace = GetFaceIndex( edgeTag );
	const Face& face = m.m_faces[ iFace ];
	const UINT faceCorner = GetFaceCorner( edgeTag );

	const HFace iOppositeFace = (oppositeTag != INVALID_TAG) ? GetFaceIndex( oppositeTag ) : HFace(~0);
	const UINT oppositeCorner = (iOppositeFace != ~0) ? GetFaceCorner( oppositeTag ) : ~0;
//	mxASSERT(!face.deleted);
//		mxASSERT(!m.m_faces[GetFaceIndex( oppositeTag )].deleted);

	FullEdgeInfo result;
	result.halfEdge[0] = iEdge;
	result.halfEdge[1] = iOpposite;
	result.edgeTags[0] = edgeTag;
	result.edgeTags[1] = oppositeTag;
	result.adjacent[0] = iFace;
	result.adjacent[1] = iOppositeFace;
	result.endPoint[0] = face.v[ faceCorner ];
	result.endPoint[1] = face.v[ NextFaceVertex(faceCorner) ];
	result.relative[0] = faceCorner;
	result.relative[1] = oppositeCorner;
	mxASSERT(m.m_faces.isValidIndex(result.adjacent[0]));
//		mxASSERT(m.m_faces.isValidIndex(result.adjacent[1]));
	return result;
}

// Tells whether triangles become inverted if origin point is moved at given location.
// Pair contractions do not necessarily preserve the orientation of the faces in the area of the contraction.
// For instance, it is possible to contract an edge and cause some neighboring faces to fold over on each other.
// When considering a possible contraction, we compare the normal of each neighboring face before and after the contraction.
// If the normal flips, that contraction can be either heavily penalized or disallowed.
bool Becomes_Flipped_Or_Degenerated(
									const ProgressiveMesh& m,
									const Face& face,
									UINT faceVertex,
									HVertex iNewVertex
									)
{
	HVertex	v[3];
	v[0] = face.v[0];
	v[1] = face.v[1];
	v[2] = face.v[2];

	v[faceVertex] = iNewVertex;

	const V3f a = m.m_vertices[v[0]].position;
	const V3f b = m.m_vertices[v[1]].position;
	const V3f c = m.m_vertices[v[2]].position;
	const V3f newNormal = Triangle_Normal( a, b, c );
	return V3_Dot( newNormal, face.normal ) <= 0.0f;
}

mxBIBREF("A simple, fast, and effective polygon reduction algorithm, 1998");
mxBIBREF("A GPU-based Approach for Massive Model Rendering with Frame-to-Frame Coherence, 2012");
float ComputeEdgeCollapseCost( const ProgressiveMesh& m,
							  const Vertex& vertex, const HEdge iEdge )
{
	const FullEdgeInfo FEI = GetEdgeInfo(iEdge,m);

	// Check whether this edge can be contracted into the end vertex.

	// Half-edge collapse is possible only if the start vertex is a 'star' vertex.
	const HVertex iStartVertex = FEI.endPoint[0];
	const HVertex iEndVertex = FEI.endPoint[1];

//NOTE: we skip visiting adjacent half-edges
	const HEdge iStartEdge = m.adjacency.NextEdgeAroundVertex( iEdge );
	const HEdge iEndEdge = m.adjacency.PreviousEdgeAroundVertex( iEdge );

#if 0
	{
		// find neighbor vertices adjacent to the start vertex
		TInplaceArray< HVertex, 16 >	verticesA;
		for(
			HEdge current = iStartEdge;
			current != iEndEdge;
			current = m.adjacency.NextEdgeAroundVertex( current )
			)
		{
			if( current == INVALID_EDGE ) {
				goto L_StartVertexLiesOnBoundary;
			}

			HVertex vert = m.GetEndVertex( current );
//			DBGOUT("[%d->%d] %d is adj to %d", iStartVertex, iEndVertex, vert, m.GetStartVertex(current));
			verticesA.add( vert );
		}

		{
			const HEdge start2 = m.adjacency.NextEdgeAroundVertex( FEI.halfEdge[1] );
			const HEdge end2 = m.adjacency.PreviousEdgeAroundVertex( FEI.halfEdge[1] );
			for(
				HEdge current = start2;
				current != end2;
				current = m.adjacency.NextEdgeAroundVertex( current )
				)
			{
				if( current == INVALID_EDGE ) {
					ptWARN("[%d->%d]: end has NIL edge", iEndVertex, iStartVertex);
					break;
				}
				HVertex vert = m.GetEndVertex( current );
//				DBGOUT("[%d->%d] %d is adj to %d", iEndVertex, iStartVertex, vert, iEndVertex);
				if( verticesA.contains(vert)) {
					return MAX_COST;
				}
				//verticesA.add( m.GetStartVertex( current ) );
			}
		}
	}
#endif
	const Face& sideA = m.m_faces[ FEI.adjacent[0] ];
	const Face& sideB = m.m_faces[ FEI.adjacent[1] ];

	const float edgeLength =
		DistanceBetween( m.m_vertices[iStartVertex].position, m.m_vertices[iEndVertex].position );

	// use the triangle facing most away from the sides 
	// to determine our curvature term
	float curvatureAtVertex = 0.0f;	// [0..1]

	for(
		HEdge current = iStartEdge;
		current != iEndEdge;
		current = m.adjacency.NextEdgeAroundVertex( current )
		)
	{
		if( current == INVALID_EDGE ) {
			goto L_StartVertexLiesOnBoundary;
		}

		const EdgeTag tag = m.adjacency.halfEdges[ current ];
		const HFace iAdjacentFace = GetFaceIndex( tag );
		const Face& adjacentFace = m.m_faces[ iAdjacentFace ];

#if 1
		if( iAdjacentFace != FEI.adjacent[0] && iAdjacentFace != FEI.adjacent[1] )
		{
			if( Becomes_Flipped_Or_Degenerated( m, adjacentFace, GetFaceCorner(tag), iEndVertex ) ) {
				return MAX_COST;
			}
		}
#endif

		//The curvature term for collapsing an edge AB is determined
		// by comparing dot products of face normals in order to find
		// the triangle adjacent to A that faces furthest away
		// from the other triangles that are along AB.
		float minimumCurvature = MAX_COST;

// use a threshold value for the maximum dihedral angle of edges

		// use dot product of face normals. 
		float dotprod;

		// (1-dot(f.normal,n.normal)) calculates how similar the two normals are
		// dot product is the cosine of the angle between the two normals
		// and it approaches 1 when the angle approaches 0.
		dotprod = V3_Dot(adjacentFace.normal, sideA.normal);
		minimumCurvature = std::min(minimumCurvature,(1-dotprod)/2.0f);

		dotprod = V3_Dot(adjacentFace.normal, sideB.normal);
		minimumCurvature = std::min(minimumCurvature,(1-dotprod)/2.0f);

		curvatureAtVertex = std::max( curvatureAtVertex, minimumCurvature );
	}
	// the more coplanar the lower the curvature term
	const float edgeCost = edgeLength * curvatureAtVertex;
	return edgeCost;

L_StartVertexLiesOnBoundary:
	return MAX_COST;
}

void ComputeEdgeCostAtVertex( ProgressiveMesh& m, Vertex & v )
{
	// compute the edge collapse cost for all edges that start
	// from vertex v.  Since we are only interested in reducing
	// the object by selecting the min cost edge at each step, we
	// only cache the cost of the least cost edge at this vertex
	// (in member variable collapse) as well as the value of the 
	// cost (in member variable objdist).
	if( v.valence )
	{
		v.cost = MAX_COST;
		v.edgeToCollapse = -1;
		// we cannot move boundary vertices
		if( !v.onBoundary )
		{
			// search all neighboring edges for "least cost" edge
			for( UINT i = 0; i < v.valence; i++ )
			{
				const HEdge iEdge = m.incidentEdges[ v.firstIncidentEdge + i ];
				float cost = ComputeEdgeCollapseCost( m, v, iEdge );
				if( cost < v.cost )
				{
					v.edgeToCollapse = i;	// candidate for edge collapse
					v.cost = cost;          // cost of the collapse
				}
			}
		}
	}
	else
	{
		// v doesn't have neighbors so it costs nothing to collapse
		//v.collapse=NULL;
		//v.objdist=-0.01f;
	}
}
void ComputeAllEdgeCollapseCosts( ProgressiveMesh& m )
{
	// For all the edges, compute the difference it would make
	// to the model if it was collapsed.  The least of these
	// per vertex is cached in each vertex object.
	for( HVertex iVertex = HVertex(0); iVertex < m.m_vertices.num(); iVertex++ )
	{
		ComputeEdgeCostAtVertex(m, m.m_vertices[iVertex]);
	}
}

const Vertex* FindVertexWithMinimumCostEdge( const ProgressiveMesh& m, float error_threshold )
{
	// find the edge that when collapsed will affect model the least.
	// This function actually returns a Vertex, the second vertex
	// of the edge (collapse candidate) is stored in the vertex data.
	const Vertex* bestVertex = nil;
	float minimumCost = MAX_COST;
	for( HVertex iVertex = HVertex(0); iVertex < m.m_vertices.num(); iVertex++ )
	{
		const Vertex& v = m.m_vertices[iVertex];
		if( v.cost < minimumCost ) {
			minimumCost = v.cost;
			bestVertex = &v;
		}
	}
	return minimumCost < error_threshold ? bestVertex : nil;
}

ERet ProgressiveMesh::SelectNextEdgeToCollapse( float error_threshold )
{
	mxPROFILE_FUNCTION;

	AllocatorI& scratch = MemoryHeaps::temporary();

	nextVertexToRemove = nil;

	// recompute face normals (they will be needed for shading, anyway)
	this->UpdateFaceNormals();

	for( HVertex iVertex = HVertex(0); iVertex < m_vertices.num(); iVertex++ )
	{
		m_vertices[iVertex].cost = MAX_COST;
		m_vertices[iVertex].valence = 0;
		m_vertices[iVertex].firstIncidentEdge = 0;
		m_vertices[iVertex].edgeToCollapse = -1;
		m_vertices[iVertex].onBoundary = false;
	}

	mxDO(adjacency.Build( m_faces, m_vertices, scratch ));

	// build the tables for quickly finding incident edges of each vertex
	for( HEdge iEdge = HEdge(0); iEdge < adjacency.halfEdges.num(); iEdge++ )
	{
		const FullEdgeInfo FEI = GetEdgeInfo( iEdge, *this );
		const HVertex iVertex = FEI.endPoint[0];
		m_vertices[ iVertex ].valence++;
	}
	// calculate the size of the vertex-edge table
	{
		UINT incidentEdgeCounter = 0;	// head of list of incident edges
		for( HVertex iVertex = HVertex(0); iVertex < m_vertices.num(); iVertex++ )
		{
			m_vertices[ iVertex ].firstIncidentEdge = incidentEdgeCounter;
			incidentEdgeCounter += m_vertices[ iVertex ].valence;
		}
		mxDO(incidentEdges.setNum(incidentEdgeCounter));
	}
	// valences will be reused to track how many incident edges were encountered for each vertex
	for( HVertex iVertex = HVertex(0); iVertex < m_vertices.num(); iVertex++ )
	{
		m_vertices[iVertex].valence = 0;
	}
	// build edge list for each vertex
	for( HEdge iEdge = HEdge(0); iEdge < adjacency.halfEdges.num(); iEdge++ )
	{
		const FullEdgeInfo FEI = GetEdgeInfo( iEdge, *this );
		const HVertex iVertex = FEI.endPoint[0];
		Vertex & vertex = m_vertices[ iVertex ];
		const UINT incidentEdgeIndex = vertex.valence++;
		incidentEdges[ vertex.firstIncidentEdge + incidentEdgeIndex ] = iEdge;
	}

	// iterate over each internal edge and cache all edge collapse costs
	ComputeAllEdgeCollapseCosts(*this);

	// get the next vertex to remove
	const Vertex* v = FindVertexWithMinimumCostEdge(*this, error_threshold);

	nextVertexToRemove = v;

	if( v ) {
		iteration++;
	}

	return ALL_OK;
}

/// Collapses the given edge for simplification.
/// This is a Half-Edge collapse operation (aka Subset vertex placement)
/// because the vertex 'V(Previous(c))' is moved to 'V(Next(c))',
/// i.e. we have no freedom in choosing a new vertex.
mxBIBREF("A General Framework for Mesh Decimation, 1998, Topological operators");
void HalfEdgeCollapse( const HEdge edgeToCollapse, ProgressiveMesh & m )
{
	// if we collapse the edge From-To by moving 'from' to 'to' then
	// how much different will the model change, i.e. how much "error".
	// Texture, vertex normal, and border vertex code was removed
	// to keep this demo as simple as possible.
	// The method of determining cost was designed in order 
	// to exploit small and coplanar regions for
	// effective polygon reduction.
	// Is is possible to add some checks here to see if "folds"
	// would be generated.  i.e. normal of a remaining face gets
	// flipped.  I never seemed to run into this problem and
	// therefore never added code to detect this case.
	const FullEdgeInfo FEI = GetEdgeInfo( edgeToCollapse, m );

	// Collapse the edge uv by moving vertex u onto v.
	// move the vertex with minimal error

	// Delete the edge by moving its source vertex to its destination vertex.

	const HVertex iVertexOld = FEI.endPoint[0];
	const HVertex iVertexNew = FEI.endPoint[1];

	// Fixup vertex references: find all faces/edges referencing vertex VOld and change them to use VNew instead.
	//ptWARN("{%d} Move Vertex %d -> %d (adj faces %d & %d)",
	//	m.iteration, iVertexOld, iVertexNew, FEI.adjacent[0], FEI.adjacent[1]);

//		const HEdge startHalfEdge = vertexOld.temp;
//		mxASSERT(startHalfEdge == FEI.halfEdge[0] || startHalfEdge == FEI.halfEdge[1]);
	{
		int c = 0;
		HEdge current = edgeToCollapse;
		do 
		{
			mxASSERT(current != INVALID_EDGE);
			if( current == INVALID_EDGE ) {
				break;
			}

			//Face & face = FaceFromEdge( current, m.adjacency );
			const FullEdgeInfo FEI2 = GetEdgeInfo( current, m );
			Face & face = m.m_faces[ FEI2.adjacent[0] ];
//ptPRINT("Face %d [%d]: Replace vertex %d -> %d", FEI2.adjacent[0], c++, face.v[ FEI2.relative[0] ], iVertexNew);
			face.v[ FEI2.relative[0] ] = iVertexNew;

			current = m.adjacency.NextEdgeAroundVertex( current );
		}
		while( current != edgeToCollapse );
	}

	// Delete the two triangles sharing this edge - they've become degenerate after collapsing the edge.
	HFace f0 = FEI.adjacent[0];
	HFace f1 = FEI.adjacent[1];
	TSort2(f0,f1);
//DBGOUT("Delete faces %d and %d", f1, f0);
#if 0
	m.m_faces.RemoveAt_Fast(f1);
	m.m_faces.RemoveAt_Fast(f0);
#else
	m.m_faces[FEI.adjacent[0]].deleted = true;
	m.m_faces[FEI.adjacent[1]].deleted = true;
#endif
}


ERet ProgressiveMesh::CollapseMinimumCostEdge()
{
	if( !nextVertexToRemove ) {
		return ERR_INVALID_PARAMETER;
	}

	const Meshok::Vertex& start = *nextVertexToRemove;
	const Meshok::HEdge edge = incidentEdges[ start.firstIncidentEdge + start.edgeToCollapse ];

	HalfEdgeCollapse( edge, *this );

	return ALL_OK;
}

void ProgressiveMesh::UpdateFaceNormals()
{
	for( UINT iTriangle = 0; iTriangle < m_faces.num(); iTriangle++ )
	{
		const Face& face = m_faces[ iTriangle ];
		const V3f pA = m_vertices[face.v[0]].position;
		const V3f pB = m_vertices[face.v[1]].position;
		const V3f pC = m_vertices[face.v[2]].position;
		const V3f faceNormal = V3_Cross( pB - pA, pC - pA );
		m_faces[ iTriangle ].normal = V3_Normalized( faceNormal );
	}
}
ERet ProgressiveMesh::Initialize( const TriMesh& mesh )
{
	const UINT numVertices = mesh.vertices.num();
	const UINT numFaces = mesh.triangles.num();
	const UINT numEdges = numFaces * 3;
	mxASSERT(numVertices < std::numeric_limits< HVertex::TYPE >::max());
	mxASSERT(numFaces < std::numeric_limits< HFace::TYPE >::max());		
	mxASSERT(numEdges < std::numeric_limits< HEdge::TYPE >::max());

	// Copy vertex positions.
	mxDO(m_vertices.setNum( numVertices ));
	for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		m_vertices[ iVertex ].position = mesh.vertices[ iVertex ];
		m_vertices[ iVertex ].cost = 0;
		m_vertices[ iVertex ].valence = 0;
		m_vertices[ iVertex ].firstIncidentEdge = -1;
	}

#if 0
	for( UINT i = 0; i < numVertices-1; i++ )
	{
		const Vertex& v1 = m_vertices[i];
		for( UINT j = i+1; j < numVertices; j++ )
		{
			const Vertex& v2 = m_vertices[j];
			mxASSERT(!V3_Equal(v1.position, v2.position));
		}
	}
#endif
//unsigned int v1; /*!< The index of the first vertex. */
//unsigned int v2; /*!< The index of the second vertex. */
//unsigned int v3; /*!< The index of the third vertex. */
	// Copy triangles.
	mxDO(m_faces.setNum( numFaces ));
	for( UINT iFace = 0; iFace < numFaces; iFace++ )
	{
		const Triangle& source = mesh.triangles[ iFace ];
		Face & destination = m_faces[ iFace ];
		destination.v[0] = HVertex(source.v[0]);
		destination.v[1] = HVertex(source.v[1]);
		destination.v[2] = HVertex(source.v[2]);
		destination.deleted = false;
	}

	iteration = 0;

	return ALL_OK;
}

ERet ProgressiveMesh::WriteResultingMesh( TriMesh & mesh ) const
{
	AllocatorI& scratch = MemoryHeaps::temporary();

	// Get rid of unused vertices and faces.
	const UINT maxVertices = m_vertices.num();

	// For each vertex, the number of faces sharing that vertex.
	DynamicArray< UINT >	vertexFaceCounts( scratch );
	mxDO(vertexFaceCounts.setNum( maxVertices ));

	// loop thru all faces and count used vertices
#if 0
	const UINT numFaces = m_faces.num();
	vertexFaceCounts.setAll(0);
	for( UINT iFace = 0; iFace < numFaces; iFace++ )
	{
		const Face& face = m_faces[ iFace ];
		vertexFaceCounts[ face.v[0] ]++;
		vertexFaceCounts[ face.v[1] ]++;
		vertexFaceCounts[ face.v[2] ]++;
	}
#else
	UINT numFaces = 0;
	vertexFaceCounts.setAll(0);
	for( UINT iFace = 0; iFace < m_faces.num(); iFace++ )
	{
		const Face& face = m_faces[ iFace ];
		if( face.deleted ) continue;
		vertexFaceCounts[ face.v[0] ]++;
		vertexFaceCounts[ face.v[1] ]++;
		vertexFaceCounts[ face.v[2] ]++;
		numFaces++;
	}
#endif

	// Copy used vertices.

	// new vertex indices, -1 for unreferenced vertices
	DynamicArray< UINT >	remappedVertexIndices( scratch );
	mxDO(remappedVertexIndices.setNum( maxVertices ));

	mesh.vertices.RemoveAll();
	for( UINT i = 0; i < maxVertices; i++ )
	{
		const bool vertexIsUsed = !!vertexFaceCounts[i];
		if( vertexIsUsed ) {
			remappedVertexIndices[i] = mesh.vertices.num();
			mesh.vertices.add( m_vertices[i].position );
		} else {
			remappedVertexIndices[i] = -1;
		}
	}

	// Write triangles.
	mxDO(mesh.triangles.setNum(numFaces));
#if 0
	for( UINT iFace = 0; iFace < numFaces; iFace++ )
	{
		Triangle & triangle = mesh.triangles[ iFace ];
		triangle.e[0] = remappedVertexIndices[ m_faces[iFace].v[0] ];
		triangle.e[1] = remappedVertexIndices[ m_faces[iFace].v[1] ];
		triangle.e[2] = remappedVertexIndices[ m_faces[iFace].v[2] ];
	}
#else
	mesh.triangles.RemoveAll();
	mxDO(mesh.triangles.reserve(numFaces));
	for( UINT iFace = 0; iFace < m_faces.num(); iFace++ )
	{
		const Face& face = m_faces[ iFace ];
		if( face.deleted ) continue;
		Triangle & triangle = mesh.triangles.AddNew();
		triangle.v[0] = remappedVertexIndices[ face.v[0] ];
		triangle.v[1] = remappedVertexIndices[ face.v[1] ];
		triangle.v[2] = remappedVertexIndices[ face.v[2] ];
	}
#endif
	return ALL_OK;
}
#endif
}//namespace Meshok

/*
Very cool, much better than half-edges:
https://fgiesen.wordpress.com/2012/04/03/half-edges-redux/
http://www.codercorner.com/Strips.htm

Source code:
http://www.gamedev.net/topic/656486-high-speed-quadric-mesh-simplification-without-problems-resolved/

3D Modeling and Parallel Mesh Simplification
November 21, 2009 
https://software.intel.com/en-us/articles/3d-modeling-and-parallel-mesh-simplification

3D Mesh Reduction:
http://www.euclideanspace.com/contacts/meshreduction.htm

http://upcoder.com/13/vector-meshes

http://www.cs.princeton.edu/courses/archive/spr00/cs598b/lectures/simplification/


very good explanation of Quadric-based mesh simplification:
https://classes.soe.ucsc.edu/cmps160/Spring05/finalpages/scyiu/

code:
http://lodbook.com/source/
http://www.gamedev.net/topic/656486-high-speed-quadric-mesh-simplification-without-problems-resolved/
http://kmamou.blogspot.ru/2015/06/a-simple-c-class-for-3d-mesh-decimation.html

https://github.com/sibvrv/carve/blob/master/include/carve/mesh_ops.hpp

http://kucgbowling.googlecode.com/svn/trunk/Term/WildMagic2/Source/Graphics/WmlTerrainPage.cpp



LoD selection and generation:
http://www.cbloom.com/3d/techdocs/vipm.txt
http://www.gamedev.ru/code/forum/?id=49320&page=2
https://github.com/realazthat/polyvox-tva/wiki/Lod-generation
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
