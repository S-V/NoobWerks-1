// Mesh Simplification
#pragma once

#include <algorithm>	// std::sort

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Memory.h>
#include <Core/Util/ScopedTimer.h>
#include <Utility/Meshok/Meshok.h>
#include <Utility/Meshok/Morton.h>
#include <Utility/Meshok/SDF.h>
#include <Utility/MeshLib/TriMesh.h>
#include <Utility/Meshok/Quadrics.h>





namespace Meshok
{

#if 0
/// Objects are identified using positive integers, wrapped into named structs for type safety.
MAKE_ALIAS( HVertex, U32 );	//!< vertex index - absolute, i.e. not in [0..2] range (within triangle)
MAKE_ALIAS( HCorner, U32 );	//!< corner index
//MAKE_ALIAS( HTriple, int );	//!< triangle index
MAKE_ALIAS( HEdge, U32 );	//!< half-edge handle
MAKE_ALIAS( HFace, U32 );	//!< face index

/// EdgeTags represent half-edge adjacency. Each EdgeTag = faceId * 3 + vertexId.
MAKE_ALIAS( EdgeTag, U32 );
const EdgeTag INVALID_TAG(~0);
/// Each EdgeTag corresponding to an internal edge is paired with its oppositely oriented adjacent edge;
/// EdgeTags corresponding to boundary edges are -1.
const HEdge INVALID_EDGE(~0);

const HFace INVALID_FACE(~0);

/// half edges of a single face
typedef Tuple3< HEdge > FaceEdges;
#endif






#if 0
		struct Vertex
		{
			V3f		position;
			F32		cost;	//!< vertices with the least cost can be deleted during edge collapse

			U32		valence;//!< number of outgoing edges (aka 'degree')
			U32		firstIncidentEdge;//!< the index of the first outgoing half-edge
			UINT	edgeToCollapse;	//!< candidate vertex for collapse
			bool	onBoundary;	//!< set initially
		};
		struct Face
		{
			V3f		normal;   // unit vector orthogonal to this face
			HVertex	v[3];
			bool	deleted;
		};




/// returns the face index associated with this edge
inline HFace GetFaceIndex( EdgeTag edgeTag )	{ return HFace(edgeTag / 3); }
/// returns face-relative vertex index (within [0..3) range)
inline UINT GetFaceCorner( EdgeTag edgeTag )	{ return edgeTag % 3; }
/// returns the next face-relative vertex index (within [0..3) range)
inline UINT NextFaceVertex( UINT faceVertex )	{ return (faceVertex + 1) % 3; }
/// returns the previous face-relative vertex index (within [0..3) range)
inline UINT PrevFaceVertex( UINT faceVertex )	{ return (faceVertex + 2) % 3; }


/*
*  For the polygon reduction algorithm we use data structures
*  that contain a little bit more information than the usual
*  indexed face set type of data structure.
*  From a vertex we wish to be able to quickly get the
*  neighboring faces and vertices.
*/
//Which faces use this vertex?
//Which edges use this vertex?
//Which faces border this edge?
//Which edges border this face?
//Which faces are adjacent to this face? 

/// Mesh Connectivity/Adjacency data structure.
/// Builds tables for quickly finding half edges of each triangle.
/// Works only with 2-manifold triangle meshes with boundaries.
mxBIBREF("Out-of-Core Compression for Gigantic Polygon Meshes, 2003, 3.1 Half-Edge Data-Structure");
struct Adjacency
{
	DynamicArray< EdgeTag >		halfEdges;	//!< [HEdge] => (iFace, iStartVertex), contains paired half-edges (i.e. Count % 2 = 0).
	DynamicArray< FaceEdges >	faceEdges;	//!< [HFace] => iEdge - index into the above table, 3 per each triangle.
public:
	Adjacency( AllocatorI & heap )
		: halfEdges( heap ), faceEdges( heap )
	{}

	// corresponding half-edge in opposite direction
	HEdge TwinEdge( HEdge edge ) const
	{
		//mxASSERT(edge != INVALID_EDGE);
		if( edge == INVALID_EDGE ) return INVALID_EDGE;
		return (edge % 2) ? HEdge(edge - 1) : HEdge(edge + 1);
	}

	// next outgoing edge with same start vertex (CCW)
	HEdge NextEdgeAroundVertex( HEdge edge ) const
	{
		const EdgeTag tag = halfEdges[ edge ];
		return NextEdgeAroundVertex(tag);
	}
	HEdge PreviousEdgeAroundVertex( HEdge edge ) const
	{
		const EdgeTag tag = halfEdges[ TwinEdge(edge) ];
		return NextFaceEdge(tag);
	}
	// next outgoing edge with same start vertex (CCW)
	HEdge NextEdgeAroundVertex( EdgeTag tag ) const
	{
		return TwinEdge(PrevFaceEdge(tag));
	}
	//HEdge PreviousEdgeAroundVertex( EdgeTag tag ) const
	//{
	//	return NextFaceEdge(TwinEdge(tag));
	//	//return TwinEdge(NextFaceEdge(tag));
	//}
	// next edge in this face (CCW)
	HEdge NextFaceEdge( EdgeTag tag ) const
	{
		const FaceEdges& face = faceEdges[ GetFaceIndex( tag ) ];
		return face.v[ NextFaceVertex(GetFaceCorner( tag )) ];
	}
	// prev edge in this face (CCW)
	HEdge PrevFaceEdge( EdgeTag tag ) const
	{
		const FaceEdges& face = faceEdges[ GetFaceIndex( tag ) ];
		return face.v[ PrevFaceVertex(GetFaceCorner( tag )) ];
	}

//private:
	/// Temporary structure for sorting half-edges.
	struct HalfEdge
	{
		EdgeTag	tag;		//!< encoded face and vertex indices: tag = face*3 + vertex
		UINT	start, end;	//!< key for sorting, start < end !
	public:
		static HalfEdge Encode( UINT triangleIndex, UINT faceCorner, UINT startVertex, UINT endVertex )
		{
			// vertices must be ordered lexicographically, i.e. start < end!
			TSort2( startVertex, endVertex );
			mxASSERT(startVertex < endVertex);	//ensure that 'startVertex != endVertex'
			HalfEdge edge;
			edge.tag = EdgeTag( triangleIndex * 3 + faceCorner );
			edge.start = startVertex;
			edge.end = endVertex;
			return edge;
		}
		// lexicographic comparison
		bool operator < ( const HalfEdge& other ) const
		{
			return (this->start < other.start)
				|| (this->start == other.start && this->end < other.end);
		}
	};

	ERet Build( const DynamicArray< Face >& triangles, DynamicArray< Vertex > & vertices, AllocatorI & scratch )
	{
		mxPROFILE_FUNCTION;

		// Half-edge matching: traverse the entire mesh and test each edge's vertices with all other edge's vertices.
		// If the vertices of one edge match those of another, they are paired together.
		// This can be done in O(N) time and space using lexicographic sorting.

		const UINT numTriangles = triangles.num();
		const UINT numEdges = numTriangles * 3;
#if 0
		union SortKey
		{
			U64	value;
			struct {
				HVertex	start;
				HVertex	end;
			};
		};
		mxSTATIC_ASSERT(sizeof(SortKey) == sizeof(U64));
#endif
		/// A mesh is edge-manifold if no edge is shared by more than 2 facets.
		/// Neighboring triangles share a common edge.

		DynamicArray< HalfEdge >	sortedHalfEdges( scratch );

#if 0
		mxDO(sortedHalfEdges.setNum( numEdges ));
		for( UINT iTriangle = 0; iTriangle < numTriangles; iTriangle++ )
		{
			const Face& face = triangles[ iTriangle ];
			sortedHalfEdges[ iTriangle*3 + 0 ] = HalfEdge::Encode( iTriangle, 0, face.v[ 0 ], face.v[ 1 ] );
			sortedHalfEdges[ iTriangle*3 + 1 ] = HalfEdge::Encode( iTriangle, 1, face.v[ 1 ], face.v[ 2 ] );
			sortedHalfEdges[ iTriangle*3 + 2 ] = HalfEdge::Encode( iTriangle, 2, face.v[ 2 ], face.v[ 0 ] );
		}
#else
		sortedHalfEdges.RemoveAll();
		for( UINT iTriangle = 0; iTriangle < numTriangles; iTriangle++ )
		{
			const Face& face = triangles[ iTriangle ];
			if( face.deleted ) continue;
			sortedHalfEdges.add( HalfEdge::Encode( iTriangle, 0, face.v[ 0 ], face.v[ 1 ] ) );
			sortedHalfEdges.add( HalfEdge::Encode( iTriangle, 1, face.v[ 1 ], face.v[ 2 ] ) );
			sortedHalfEdges.add( HalfEdge::Encode( iTriangle, 2, face.v[ 2 ], face.v[ 0 ] ) );
		}
#endif
		std::sort( sortedHalfEdges.begin(), sortedHalfEdges.end() );
		// now oppositely-oriented adjacent edges are stored in pairs

		mxDO(faceEdges.setNum(numTriangles));
		{
			const FaceEdges isolatedTriangle = { INVALID_EDGE, INVALID_EDGE, INVALID_EDGE };
			faceEdges.setAll(isolatedTriangle);
		}

		// remove duplicates and build adjacency
		this->halfEdges.RemoveAll();
		mxDO(this->halfEdges.reserve( numEdges ));
#if 0 // WRONG
		for( UINT iEdge = 1; iEdge < sortedHalfEdges.num(); iEdge++ )
		{
			const HalfEdge previous = sortedHalfEdges[ iEdge - 1 ];
			const HalfEdge current = sortedHalfEdges[ iEdge ];
			if( current.start == previous.start && current.end == previous.end )
			{
				this->ConnectOppositeHalfEdges( previous.tag, current.tag );
			}
		}
#else
		mxBIBREF("Out-of-Core Compression for Gigantic Polygon Meshes, 2003, 3.3 Building the Out-of-Core Mesh, Matching of Inverse Half-Edges");
		{
			UINT	lastStartVertex = -1;//sortedHalfEdges[0].start;
			UINT	lastEndVertex = -1;//sortedHalfEdges[0].end;
			EdgeTag	tempBuffer[3];	// enough space for the shortest invalid sequence
			UINT	runLength = 0;

			for( UINT iEdge = 0; iEdge < sortedHalfEdges.num(); iEdge++ )
			{
				const HalfEdge current = sortedHalfEdges[ iEdge ];
				if( current.start == lastStartVertex && current.end == lastEndVertex )
				{
					// Current edge is the same as last one
					tempBuffer[ runLength++ ] = current.tag;	// Store face and vertex ID
					// Only works with 2-manifold meshes (i.e. an edge is not shared by more than 2 triangles)
					if( runLength > 2 ) {
						LogStream log;
						log << "Start " << lastStartVertex << ", end " << lastEndVertex;
						for( int i = 0; i < runLength; i++ ) {
							const EdgeTag tag = tempBuffer[i];
							const HFace iFace = GetFaceIndex(tag);
							log << "\nFace " << iFace << ", vertices: " << triangles[iFace].v[0] << ", " << triangles[iFace].v[1] << ", " << triangles[iFace].v[2];
						}
					}
					//if( runLength > mxCOUNT_OF(tempBuffer) )
					//{
					//	mxASSERT(false);
					//	return ERR_INVALID_PARAMETER;
					//}

					// non-manifold edge: more than two matched half-edges
					mxENSURE( runLength < 3, ERR_INVALID_PARAMETER, "The mesh is not edge-manifold");
				}
				else
				{
					// if Count==1 => This is a boundary edge: it belongs to exactly one triangle.
					if( runLength == 1 )
					{
						const EdgeTag t = tempBuffer[0];
						const HVertex iVertex = triangles[ GetFaceIndex(t) ].v[ GetFaceCorner(t) ];
						vertices[ iVertex ].onBoundary = true;// an unmatched half-edge is a border edge
						//this->StoreBoundaryHalfEdge( tempBuffer[0] );
					}
					// if Count==2 => This is an internal edge (aka 'full-edge')
					// which consists of two opposite half edges and is shared by two adjacent triangles.
					if( runLength == 2 )
					{
						// manifold edge: two matched half-edges with opposite orientation
						// Pair-up consecutive entries - they are opposite edges.
						this->ConnectOppositeHalfEdges( tempBuffer[0], tempBuffer[1] );
					}

					// not-oriented edge: two matched half-edges with identical orientation

					// Reset for next edge
					runLength = 0;
					tempBuffer[ runLength++ ] = current.tag;
					lastStartVertex = current.start;
					lastEndVertex = current.end;
				}
			}
			if( runLength == 2 ) {
				this->ConnectOppositeHalfEdges( tempBuffer[0], tempBuffer[1] );
			}
		}
#endif
//			mxASSERT( this->halfEdges.num() <= sortedHalfEdges.num() );

		//DBGOUT("BuildAdjacency: %d triangles, %d -> %d half-edges",
		//	numTriangles, sortedHalfEdges.num(), this->halfEdges.num());

		mxASSERT2(this->halfEdges.num()%2 == 0, "The table must contain an even number of half-edges");

		return ALL_OK;
	}
	void ConnectOppositeHalfEdges( const EdgeTag t0, const EdgeTag t1 )
	{
		const HEdge iNewHalfEdge = HEdge(this->halfEdges.num());
		const HEdge iOppositeHalfEdge = HEdge(iNewHalfEdge + 1);
		mxASSERT(TwinEdge(iNewHalfEdge) == iOppositeHalfEdge);
		mxASSERT(iNewHalfEdge % 2 == 0);

		this->halfEdges.add( t0 );
		this->halfEdges.add( t1 );

		// Update the half-edge references in two adjacent triangles.
		this->faceEdges[ GetFaceIndex(t0) ].e[ GetFaceCorner(t0) ] = iNewHalfEdge;
		this->faceEdges[ GetFaceIndex(t1) ].e[ GetFaceCorner(t1) ] = HEdge(iNewHalfEdge+1);
	}
	void StoreBoundaryHalfEdge( const EdgeTag t )
	{
		const HEdge iNewHalfEdge = HEdge(this->halfEdges.num());
		mxASSERT(iNewHalfEdge % 2 == 0);
		this->halfEdges.add( t );
		this->halfEdges.add( INVALID_TAG );
		this->faceEdges[ GetFaceIndex(t) ].e[ GetFaceCorner(t) ] = iNewHalfEdge;
	}
};
	struct ProgressiveMesh
	{
		DynamicArray< Vertex >	m_vertices;	//!< an array of vertex attributes
		DynamicArray< Face >	m_faces;

		// arrays of edges around each vertex
		DynamicArray< HEdge >	incidentEdges;
		Adjacency				adjacency;

		TPtr< const Vertex >	nextVertexToRemove;
		int						iteration;

	public:
		ProgressiveMesh( AllocatorI& heap = MemoryHeaps::temporary() )
			: m_vertices( heap ), m_faces( heap ), incidentEdges( heap ), adjacency( heap )
		{}
		ERet Initialize( const TriMesh& mesh );

		ERet SelectNextEdgeToCollapse( float error_threshold );
		ERet CollapseMinimumCostEdge();

		ERet WriteResultingMesh( TriMesh & mesh ) const;


	HVertex GetStartVertex( const HEdge edge ) const
	{
		const EdgeTag tag = adjacency.halfEdges[ edge ];
		const HFace faceIndex = GetFaceIndex( tag );
		const Face& face = m_faces[ faceIndex ];
		const UINT faceCorner = GetFaceCorner( tag );
		const HVertex vertexIndex = face.v[ faceCorner ];
		return vertexIndex;
	}
	HVertex GetEndVertex( const HEdge edge ) const
	{
		return GetStartVertex( adjacency.TwinEdge(edge) );
	}

	private:
		void UpdateFaceNormals();
	};

#endif




/// Mesh decimator based on vertex clustering (Rossignac-Borrel, 1993)
class DecimatorVC
{
	enum { MAX_GRID_SIZE = 1024 };	//!< only 10 bits for each coord, see CalculateClusterID()
	/// a sortable item
	struct VertexKey
	{
		U32	clusterID;		//!< vertices in the same cell will have the same ID
		U32	vertexIndex;	//!< index of the original vertex
	public:
		bool operator < ( const VertexKey& other ) const
		{
			return this->clusterID < other.clusterID;
		}
	};
public:
	ERet Decimate(
		TriMesh & mesh,
		const AABBf& bounds,
		const int gridSize,
		AllocatorI & scratch,
		DynamicArray< U32 > &_vertexRemap	//!< maps old vertex index => new vertex index ('many to one')
		)
	{
		const U32 oldNumVertices = mesh.vertices.num();
		const U32 oldNumTriangles = mesh.triangles.num();

		mxASSERT(oldNumVertices > 0);
		mxASSERT(gridSize > 0 && gridSize <= MAX_GRID_SIZE);	// only 10 bits for each coord, see CalculateClusterID()

		ScopedTimer	timer;
		DBGOUT("INPUT: %u verts, %u tris", oldNumVertices, oldNumTriangles);

		DynamicArray< VertexKey >	vertexKeys( scratch );	// vertices to be clustered
		mxDO(vertexKeys.setCountExactly(oldNumVertices));
		mxDO(_vertexRemap.setCountExactly(oldNumVertices));

		const V3f invAabbSize = V3_Reciprocal(AABBf_FullSize(bounds));

		for( UINT iVertex = 0; iVertex < oldNumVertices; iVertex++ )
		{
			const V3f& position = mesh.vertices[ iVertex ].xyz;
			//const V3f normalizedPosition = AABBf_GetNormalizedPosition( bounds, position );
			mxASSERT(AABBf_ContainsPoint( bounds, position ));
			const V3f relativePosition = position - bounds.min_corner;	// [0..aabb_size]
			const V3f normalizedPosition = V3_Multiply( relativePosition, invAabbSize );// [0..1]

			const U32 clusterID = CalculateClusterID(
				normalizedPosition.x, normalizedPosition.y, normalizedPosition.z,
				gridSize
			);

			VertexKey & newKey = vertexKeys[ iVertex ];
			newKey.clusterID = clusterID;
			newKey.vertexIndex = iVertex;
		}

		std::sort( vertexKeys.begin(), vertexKeys.end() );

		DynamicArray< V3f >	newVertices( scratch );	// new vertices

		DynamicArray< VertexKey > currentRun( scratch );
		mxDO(currentRun.reserve(32));	// preallocated to max number of vertices in a cell

		currentRun.add(vertexKeys[0]);
		U32 previousCluster = vertexKeys[0].clusterID;

		for( UINT i = 1; i < vertexKeys.num(); i++ )
		{
			const VertexKey& key = vertexKeys[i];

			if( previousCluster == key.clusterID ) {
				currentRun.add( key );
			} else {
				const UINT runLength = currentRun.num();
				if( runLength ) {
					// these vertices belong to the same cluster, merge them and compute the representative vertex
					mxDO(MergeVertices( currentRun.raw(), runLength, mesh, newVertices, _vertexRemap ));
				}
				currentRun.RemoveAll();
				currentRun.add(key);
				previousCluster = key.clusterID;
			}
		}
		// finish leftovers
		if( currentRun.num() ) {
			mxDO(MergeVertices( currentRun.raw(), currentRun.num(), mesh, newVertices, _vertexRemap ));
		}
		{
			// remove invalid triangles in-place
			UINT numValidTriangles = 0;
			for( UINT i = 0; i < oldNumTriangles; i++ )
			{
				const TriMesh::Triangle& tri = mesh.triangles[i];
				const UINT iV0 = mesh.vertices[ tri.a[0] ].tag;
				const UINT iV1 = mesh.vertices[ tri.a[1] ].tag;
				const UINT iV2 = mesh.vertices[ tri.a[2] ].tag;
				// skip degenerated triangles
				if( iV0 != iV1 && iV1 != iV2 && iV2 != iV0 ) {
					mesh.triangles[ numValidTriangles++ ] = TriMesh::Triangle( iV0, iV1, iV2 );
				}
			}
			mxDO(mesh.triangles.setCountExactly(numValidTriangles));
		}

		const U32 newNumVertices = newVertices.num();
		const U32 newNumTriangles = mesh.triangles.num();
		mxDO(mesh.vertices.setCountExactly( newNumVertices ));
		for( UINT iVertex = 0; iVertex < newNumVertices; iVertex++ ) {
			const V3f& position = newVertices[ iVertex ];
			mesh.vertices[ iVertex ].xyz = position;
		}

		DBGOUT("OUTPUT: %u verts, %u tris (%d milliseconds)",
			newNumVertices, newNumTriangles, timer.ElapsedMilliseconds());

		return ALL_OK;
	}

	U32 CalculateClusterID( float x, float y, float z, float gridSize )
	{
		mxASSERT( x >= 0.0f && x <= 1.0f );
		mxASSERT( y >= 0.0f && y <= 1.0f );
		mxASSERT( z >= 0.0f && z <= 1.0f );
		// get integer coordinates in range [0..gridSize)
		const U32 iX = mmFloorToInt( x * gridSize );
		const U32 iY = mmFloorToInt( y * gridSize );
		const U32 iZ = mmFloorToInt( z * gridSize );
		// compose sort key (MAX_GRID_SIZE == 1024)
		return ((iZ & 1023) << 20u) | ((iY & 1023) << 10u) | (iX & 1023u);
	}

	/// Merges the vertices and creates a new vertex.
	ERet MergeVertices( const VertexKey* vertices, UINT numVertices, TriMesh & mesh, DynamicArray< V3f > & newVertices, DynamicArray< U32 > &_vertexRemap )
	{
		// create a new vertex in the simplified mesh
		const UINT newVertexIndex = newVertices.num();

		V3f averagePosition = V3_Zero();
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			const VertexKey& key = vertices[ iVertex ];
			const Vertex& v = mesh.vertices[ key.vertexIndex ];
			averagePosition += v.xyz;
			_vertexRemap[ key.vertexIndex ] = newVertexIndex;
		}
		averagePosition *= (1.0f / numVertices);

		mxDO(newVertices.add(averagePosition));
		return ALL_OK;
	}
};


mxBIBREF("Out-of-Core Simplification of Large Polygonal Models, Peter Lindstrom, 2000")
/// Vertex clustering with quadrics
mxBUG("invalid normals cause stretched polys");
class DecimatorVC2
{
	/// a sortable item
	struct VertexKey
	{
		U32	clusterID;	//!< the (unique) locational code of this node
		U32	dataIndex;	//!<
	public:
		bool operator < ( const VertexKey& other ) const
		{
			return this->clusterID < other.clusterID;
		}
	};

	//NOTE: don't mix floats with doubles - it gives worse results and is slower than using either floats or doubles!
	typedef float REAL;
	/// 4x4 symmetric matrix (10 real values)
	typedef Quadric<REAL> QuadricF;

	// data structure depicting a vertex cluster
	struct VertexData
	{
		QuadricF	Q;	//!< The quadric error metric matrix Q for this vertex.
		V3f			position;	//!< representative vertex
	};

	/// contains the triangle ID (tid) and vertex offset inside the triangle (tvertex).
	struct Ref
	{
		unsigned triangle : 30;	//!< triangle index
		unsigned vertex   :  2;	//!< face-relative vertex index (within [0..3) range)
	};

	DynamicArray< VertexData >	m_V;	//!< array of vertex attributes

public:
	DecimatorVC2( AllocatorI & heap )
		: m_V( heap )
	{
	}

	struct Settings
	{
		AABBf	bounds;	//!< mesh bounds
		int		gridSize;
	public:
		Settings()
		{
			AABBf_Clear(&bounds);
			gridSize = 0;
		}
	};
	ERet Decimate( TriMesh & mesh, const Settings& settings )
	{
		mxASSERT(mesh.vertices.num() > 0);
		mxASSERT(settings.gridSize > 0 && settings.gridSize <= 1024);	// only 10 bits for each coord, see CalculateClusterID()

		ScopedTimer	timer;
		DBGOUT("INPUT: %u verts, %u tris", mesh.vertices.num(), mesh.triangles.num());

		AllocatorI & scratch = MemoryHeaps::temporary();

		const UINT numTriangles = mesh.triangles.num();
		const UINT numVertices = mesh.vertices.num();

		// Initialize vertices.
		mxDO(m_V.setCountExactly( numVertices ));
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			VertexData & vertex = m_V[ iVertex ];
			vertex.Q = QuadricF(0);
			vertex.position = mesh.vertices[ iVertex ].xyz;
		}

		// Compute the error matrix for each vertex in the mesh.
		for( UINT iFace = 0; iFace < numTriangles; iFace++ )
		{
			const TriMesh::Triangle& triangle = mesh.triangles[ iFace ];

			const V4f plane = mesh.CalculatePlane( triangle );

			// Compute the fundamental error quadric matrix for this triangle's plane.
			const QuadricF Kp( plane.x, plane.y, plane.z, plane.w );

			VertexData & v0 = m_V[triangle.a[0]];
			VertexData & v1 = m_V[triangle.a[1]];
			VertexData & v2 = m_V[triangle.a[2]];

			// Before any mesh simplification happens,
			// a quadric or set of quadrics associated with a vertex
			// will evaluate to 0 if the vertex location is evaluated using the quadric.
			// Vertices are evaluated against a quadric by multiplying the transpose of a vertex with the set of quadrics
			// then with the vertex again.
			v0.Q += Kp;
			v1.Q += Kp;
			v2.Q += Kp;	
		}


		DynamicArray< VertexKey >	vertexKeys(scratch);
		mxDO(vertexKeys.setCountExactly(mesh.vertices.num()));

		const V3f invAabbSize = V3_Reciprocal(AABBf_FullSize(settings.bounds));

		for( UINT iVertex = 0; iVertex < mesh.vertices.num(); iVertex++ )
		{
			const V3f& position = mesh.vertices[ iVertex ].xyz;
			//const V3f normalizedPosition = AABBf_GetNormalizedPosition( bounds, position );
			mxASSERT(AABBf_ContainsPoint( settings.bounds, position ));
			const V3f relativePosition = position - settings.bounds.min_corner;	// [0..aabb_size]
			const V3f normalizedPosition = V3_Multiply( relativePosition, invAabbSize );// [0..1]

			const U32 clusterID = CalculateClusterID
				(
					normalizedPosition.x, normalizedPosition.y, normalizedPosition.z,
					settings.gridSize
				);

			VertexKey & newKey = vertexKeys[ iVertex ];
			newKey.clusterID = clusterID;
			newKey.dataIndex = iVertex;
		}

		std::sort( vertexKeys.begin(), vertexKeys.end() );


		DynamicArray< V3f >	newVertices(scratch);	// new vertices

		TInplaceArray< VertexKey, 32 > currentRun;	// max vertices in a cell

		currentRun.add(vertexKeys[0]);
		U32 previousCluster = vertexKeys[0].clusterID;

		for( UINT i = 1; i < vertexKeys.num(); i++ )
		{
			const VertexKey& key = vertexKeys[i];

			if( previousCluster == key.clusterID )
			{
				currentRun.add( key );
			}
			else
			{
				const UINT runLength = currentRun.num();
				if( runLength )
				{
					mxDO(MergeVertices( settings, currentRun.raw(), runLength, mesh, newVertices ));
				}

				currentRun.RemoveAll();
				currentRun.add(key);

				previousCluster = key.clusterID;
			}
		}
		if( currentRun.num() ) {
			mxDO(MergeVertices( settings, currentRun.raw(), currentRun.num(), mesh, newVertices ));
		}

		{
			// remove invalid triangles in-place
			UINT numValidTriangles = 0;
			for( UINT i = 0; i < mesh.triangles.num(); i++ )
			{
				const TriMesh::Triangle& tri = mesh.triangles[i];
				const UINT iV0 = mesh.vertices[ tri.a[0] ].tag;
				const UINT iV1 = mesh.vertices[ tri.a[1] ].tag;
				const UINT iV2 = mesh.vertices[ tri.a[2] ].tag;
				// skip degenerated triangles
				if( iV0 != iV1 && iV1 != iV2 && iV2 != iV0 )
				{
					mesh.triangles[ numValidTriangles++ ] = TriMesh::Triangle( iV0, iV1, iV2 );
				}
			}
			mxDO(mesh.triangles.setCountExactly(numValidTriangles));
		}

		mxDO(mesh.vertices.setCountExactly(newVertices.num()));
		for( UINT iVertex = 0; iVertex < mesh.vertices.num(); iVertex++ )
		{
			const V3f& position = newVertices[ iVertex ];
			mesh.vertices[iVertex].xyz = position;
		}

		DBGOUT("OUTPUT: %u verts, %u tris (%d milliseconds)",
			mesh.vertices.num(), mesh.triangles.num(), timer.ElapsedMilliseconds());

		return ALL_OK;
	}

	U32 CalculateClusterID( float x, float y, float z, float gridSize )
	{
		mxASSERT( x >= 0.0f && x <= 1.0f );
		mxASSERT( y >= 0.0f && y <= 1.0f );
		mxASSERT( z >= 0.0f && z <= 1.0f );
		// get integer coordinates in range [0..gridSize)
		const U32 iX = mmFloorToInt( x * gridSize );
		const U32 iY = mmFloorToInt( y * gridSize );
		const U32 iZ = mmFloorToInt( z * gridSize );
		// compose sort key
		return ((iZ & 1023) << 20) | ((iY & 1023) << 10) | (iX & 1023);
	}

	const AABBf CalculateClusterBounds( const U32 clusterID, const Settings& settings )
	{
		const int iX = (clusterID & 1023);
		const int iY = ((clusterID >> 10) & 1023);
		const int iZ = ((clusterID >> 20) & 1023);

		const V3f clusterSize = AABBf_FullSize( settings.bounds ) / float(settings.gridSize);

		const V3f mins =
			settings.bounds.min_corner + V3_Set( clusterSize.x * iX, clusterSize.y * iY, clusterSize.z * iZ );
		const V3f maxs = mins + clusterSize;
		const AABBf result = { mins, maxs };
		return result;
	}

	// Merges the vertices and creates a new vertex.
	ERet MergeVertices( const Settings& settings, const VertexKey* vertices, UINT numVertices, TriMesh & mesh, DynamicArray< V3f > & newVertices )
	{
		const AABBf bounds = CalculateClusterBounds( vertices[0].clusterID, settings );

		const UINT newVertexIndex = newVertices.num();	// vertex index in the simplified mesh

		// Compute the matrix to solve for the optimal vertex location.
		QuadricF Q( 0 );
		V3f averagePosition = V3_Zero();
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			const VertexKey& key = vertices[ iVertex ];
			const VertexData& v = m_V[ key.dataIndex ];
			Q += v.Q;
			averagePosition += v.position;
			mesh.vertices[ key.dataIndex ].tag = newVertexIndex;
		}
		averagePosition *= (1.0f / numVertices);

		V3f newVertexPosition = averagePosition;	// if the qem is not invertible, we'll use the average position

		// Try inverting the matrix Q to compute the minimum-error vertex.
		const REAL det = Q.Determinant( 0, 1, 2, 1, 4, 5, 2, 5, 7 );
		if ( det != 0 )
		{
			const REAL invDet = REAL(1) / det;
			const V3f representativeVertexPosition = {
				-invDet * Q.Determinant( 1, 2, 3, 4, 5, 6, 5, 7, 8 ),	// vx = A41/det(q_delta)
				+invDet * Q.Determinant( 0, 2, 3, 1, 5, 6, 2, 7, 8 ),	// vy = A42/det(q_delta)
				-invDet * Q.Determinant( 0, 1, 3, 1, 4, 6, 2, 5, 8 )	// vz = A43/det(q_delta)
			};
			// if the representative vertex falls in the cluster
			if( AABBf_ContainsPoint( bounds, newVertexPosition ) ) {
				newVertexPosition = representativeVertexPosition;
			}
		}

		mxDO(newVertices.add(newVertexPosition));
		return ALL_OK;
	}
};

#if 0
/// Adaptive (octree-based) vertex clustering with quadrics.
class DecimatorVC3

	struct VertexKey
	{
		U32	morton;	//!< the (unique) locational code of this node
		U32	dataIndex;	//!<
	public:
		bool operator < ( const VertexKey& other ) const
		{
			// smaller(=deeper) nodes must be placed first in the array
			return this->morton > other.morton;
		}
	};
#endif


#define DETECT_MESH_FOLDING			(1)
#define ALLOW_FULL_EDGE_COLLAPSES	(1)

mxBIBREF("Garland, M. and Heckbert, P. S. 1997. Surface simplification using quadric error metrics. In Proceedings of the 24th Annual Conference on Computer Graphics and interactive Techniques International Conference on Computer Graphics and Interactive Techniques. ACM Press/Addison-Wesley Publishing Co., New York, NY, 209-216.");
mxSTOLEN("Based on https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification");
/// based on edge collapse
class MeshSimplifier
{
	/// Objects are identified using positive integers, wrapped into named structs for type safety.
	MAKE_ALIAS( HVertex, U32 );	//!< vertex index

	//NOTE: don't mix floats with doubles - it gives worse results and is slower than using either floats or doubles!
	typedef float REAL;
	/// 4x4 symmetric matrix (10 real values)
	typedef Quadric<REAL> QuadricF;

	struct Triangle
	{
		V4f			plane;		//!< unit vector orthogonal to this face
		REAL		edgeError[3];	//!< the cost of collapsing each edge
		U32			edgeIndices[3];	//!< edge indices ([0..3)) ordered by edge costs
		mxBool32	deleted;	//!< A boolean value indicating whether or not this triangle has been deleted.
		HVertex		v[3];		//!< vertex indices
		U32			iteration;	//!< last iteration when this triangle was updated
		mxBool32	isBorder;	//!< true if at least one vertex lies on the mesh boundary
	public:
		bool IsDirty( UINT currentIteration ) const
		{
			// the triangle was modified at the current iteration
			return this->iteration == currentIteration;
		}
		// marks the triangle as no longer valid to use at the current iteration
		void MarkDirty( UINT currentIteration )
		{
			this->iteration = currentIteration;
		}
	};
	mxSTATIC_ASSERT(sizeof(Triangle) == 64*sizeof(char));

	//NOTE: Vertex::refCount must be kept in sync with these constants
	enum { MAX_VERTEX_VALENCE = 64 };
	typedef U64 DeletedFacesMask;

	struct Vertex
	{
		QuadricF	Q;	//!< The quadric error metric matrix Q for this vertex.
		V3f			position;
		U32			material;	//!< per vertex materials
		U32			startRef;	//!< the index of the first Ref ({TriangleId; iVertex} pair)
		U16			refCount;	//!< the number of (triangles||edges) referencing this vertex (aka 'valence' or 'degree')
		U8			isBorder;	//!< A boolean value indicating whether or not this vertex lies on the mesh boundary.
		U8			_padding;
	};
	mxSTATIC_ASSERT(sizeof(Vertex) == 64*sizeof(char));

	/// contains the triangle ID (tid) and vertex offset inside the triangle (tvertex).
	struct Ref
	{
		unsigned tid : 30;		//!< triangle index (HFace)
		unsigned tvertex : 2;	//!< face-relative vertex index (within [0..3) range)
	};
	mxSTATIC_ASSERT(sizeof(Ref) == sizeof(U32));

	DynamicArray< Triangle >	m_T;	//!< array of triangles
	DynamicArray< Vertex >		m_V;	//!< array of vertex attributes
	DynamicArray< Ref >			m_R;	//!< array of {TriangleId; iVertex} pairs, 3 per each triangle.
	AllocatorI &	m_heap;

public:
	MeshSimplifier( AllocatorI & heap )
		: m_V( heap ), m_T( heap ), m_R( heap ), m_heap( heap )
	{}

	/// NOTE: 'error threshold' has higher priority than triangle count;
	/// if the model should be reduced to a certain triangle count,
	/// the 'error threshold' should be set to std::numeric_limits<float>::infinity().
	struct Settings
	{
		float	error_threshold;	//!< collapse all edges which cost is less than this value
		UINT	maximum_iterations;
		UINT	target_number_of_triangles;	//!< 
	public:
		Settings()
		{
			error_threshold = 1e-2f;
			maximum_iterations = ~0;
			target_number_of_triangles = 0;
		}
	};
	ERet Simplify( TriMesh & mesh, const Settings& settings )
	{
		//PROFILE;
		//DBGOUT("INPUT: %u vertices, %u triangles", mesh.vertices.num(), mesh.triangles.num());

		mxASSERT( mesh.vertices.num() && mesh.triangles.num() );

		//ScopedTimer	timer;
		//// these are declared float for calculating ratio
		//const float originalTriangleCount = mesh.triangles.num();
		//const float originalVertexCount = mesh.vertices.num();

		mxDO(this->InitializeFrom( mesh ));

		mxDO(this->RebuildAdjacency());

		this->IdentifyBorderVertices();

		const UINT initialNumberOfTriangles = mesh.triangles.num();
//		const UINT initialNumberOfVertices = mesh.vertices.num();

		UINT iteration = 0;
		for( iteration = 0; iteration < settings.maximum_iterations; iteration++ )
		{
			UINT numDeletedTriangles = 0;	// number of triangles deleted at this iteration

			for( UINT iFace = 0; iFace < m_T.num(); iFace++ )
			{
				Triangle & triangle = m_T[ iFace ];
				if( triangle.deleted ||	// skip degenerated triangles
					triangle.isBorder ||	// border triangles cannot be removed
					triangle.IsDirty( iteration )	// an edge belonging to this triangle was already collapsed
					)
				{
					continue;
				}

				// After we collapse the minimal-error edge,
				// the triangle becomes degenerated and must be removed.
				// A minimal-error edge can be collapsed if it doesn't cause mesh folding.

				// check the edge with minimum cost against the threshold
				//if( triangle.edgeError[ triangle.edgeIndices[0] ] < settings.error_threshold )
				{
					// for each edge of the triangle
					for( UINT i = 0; i < 3; i++ )
					{
						// start from the edge with the minimal cost
						if( triangle.edgeError[i] < settings.error_threshold )
						{
							if( TryCollapseEdge( triangle, i, (i + 1) % 3, iteration ) )
							{
								triangle.deleted = true;	// Mark the (degenerate) triangle as deleted.
								numDeletedTriangles++;
								break;	// this triangle is no longer valid
							}
						}
					}
				}
			}//for each triangle

			// check if we need to stop
			if( !numDeletedTriangles ||	// no more edges can be collapsed
				(m_T.num() <= settings.target_number_of_triangles)	// target triangle count reached
				)
			{
				break;
			}

			// update the mesh once in a while
			if( iteration % 4 == 0 )
			{
				mxDO(this->RebuildAdjacency());
			}
		}

		this->RemoveDeletedTriangles();

		mxDO(this->WriteToMesh( mesh ));

		//DBGOUT("OUTPUT: %u vertices (x%.1f), %u triangles (x%.1f) (%d iterations, %u msec)",
		//	mesh.vertices.num(), originalVertexCount / mesh.vertices.num(),
		//	mesh.triangles.num(), originalTriangleCount / mesh.triangles.num(),
		//	iteration, timer.ElapsedMilliseconds()
		//);

		return ALL_OK;
	}

	ERet RemoveDeletedTriangles()
	{
		// compact the array of triangles
		UINT numValidTriangles = 0;
		for( UINT i = 0; i < m_T.num(); i++ )
		{
			if( !m_T[ i ].deleted ) {
				m_T[ numValidTriangles++ ] = m_T[ i ];
			}
		}
		mxDO(m_T.setCountExactly( numValidTriangles ));
		return ALL_OK;
	}

	/// Rebuilds connectivity
	ERet RebuildAdjacency()
	{
		//PROFILE;

		// compact the array of triangles
		this->RemoveDeletedTriangles();

		// Init Reference ID list
		const UINT numVertices = m_V.num();
		const UINT numTriangles = m_T.num();

		// Set the valence value for each vertex to 0.
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			Vertex & vertex = m_V[ iVertex ];
			vertex.startRef = 0;
			vertex.refCount = 0;
		}
		// Compute vertex valences.
		for( UINT iFace = 0; iFace < numTriangles; iFace++ )
		{
			const Triangle& triangle = m_T[ iFace ];
			m_V[ triangle.v[0] ].refCount++;
			m_V[ triangle.v[1] ].refCount++;
			m_V[ triangle.v[2] ].refCount++;
		}
		{
			UINT	runningOffset = 0;
			for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
			{
				Vertex & vertex = m_V[ iVertex ];
//mxASSERT2(vertex.refCount > 0, "Isolated vertex found");
				if( vertex.refCount > MAX_VERTEX_VALENCE ) {
					DBGOUT("Vertex degree is too big: %u", vertex.refCount);
					return ERR_UNSUPPORTED_FEATURE;
				}

				vertex.startRef = runningOffset;
				runningOffset += vertex.refCount;
				// valences will be reused to track how many triangles were encountered for each vertex
				vertex.refCount = 0;
			}
		}
		// Fill in the adjacency tables.
		mxDO(m_R.setCountExactly( numTriangles * 3 ));
		for( UINT iFace = 0; iFace < numTriangles; iFace++ )
		{
			const Triangle& triangle = m_T[ iFace ];
			for( int i = 0; i < 3; i++ )
			{
				Vertex & v = m_V[ triangle.v[i] ];
				Ref & r = m_R[ v.startRef + v.refCount ];
				r.tid = iFace;
				r.tvertex = i;
				v.refCount++;
			}
		}
		return ALL_OK;
	}

	ERet IdentifyBorderVertices()
	{
		//PROFILE;

		const UINT numVertices = m_V.num();

		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			Vertex & vertex = m_V[ iVertex ];
			vertex.isBorder = FALSE;
		}

		DynamicArray<int> vCounts( m_heap );
		DynamicArray<int> vIDs( m_heap );

		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			Vertex & vertex = m_V[ iVertex ];

			// Go through all the triangles that use this vertex and see
			// how many times a neighboring vertex is used by those triangles.
			// If the vertex is used only once, then this vertex is on an edge.
			vCounts.RemoveAll();
			vIDs.RemoveAll();

			// for each triangle around this vertex
			for( UINT iRef = 0; iRef < vertex.refCount; iRef++ )
			{
				Ref & r = m_R[ vertex.startRef + iRef ];
				Triangle & t = m_T[ r.tid ];	//!< a triangle using this vertex

				// for each vertex of the triangle
				for( int i = 0; i < 3; i++ )
				{
					HVertex id = t.v[i];

					int k = 0;
					while( k < vCounts.num() ) {
						if( vIDs[ k ] == id ) {
							break;
						}
						k++;
					}
					if( k == vCounts.num() ) {
						vCounts.add(1);
						vIDs.add( id );
					} else {
						vCounts[k]++;
					}
				}
			}

			for( int i = 0; i < vCounts.num(); i++ )
			{
				if( vCounts[i] == 1 ) {
					m_V[ vIDs[i] ].isBorder = 1;
				}
			}
		}

		for( UINT iFace = 0; iFace < m_T.num(); iFace++ )
		{
			Triangle & t = m_T[ iFace ];
			const Vertex& v0 = m_V[ t.v[0] ];
			const Vertex& v1 = m_V[ t.v[1] ];
			const Vertex& v2 = m_V[ t.v[2] ];
			//t.isBorder = v0.isBorder || v1.isBorder || v2.isBorder;
			t.isBorder |= v0.isBorder | v1.isBorder | v2.isBorder;
		}

		return ALL_OK;
	}

	bool TryCollapseEdge(
		Triangle & triangle,
		const UINT iVertexA,
		const UINT iVertexB,
		const UINT currentIteration
		)
	{
		//PROFILE;
		mxASSERT(!triangle.deleted);

		const HVertex vertexIDs[2] = {
			triangle.v[ iVertexA ],
			triangle.v[ iVertexB ]
		};
		const Vertex& vertexA = m_V[ vertexIDs[0] ];
		const Vertex& vertexB = m_V[ vertexIDs[1] ];
		mxASSERT( !vertexA.isBorder && !vertexB.isBorder );

		// Compute vertex to collapse to
		V3f newPosition;
		CalculateEdgeCost( vertexA, vertexB, newPosition );

		// Pick the vertex with the lowest number of edges as the collapse target.
		const int iVertexFrom = MapTo01(vertexA.refCount < vertexB.refCount);
		const int iVertexTo = iVertexFrom ^ 1;
		return TryCollapseHalfEdge(
			vertexIDs[iVertexTo],
			vertexIDs[iVertexFrom],
			newPosition,
			currentIteration
		);
	}

	bool TryCollapseHalfEdge(
		const HVertex iVertexTo,
		const HVertex iVertexFrom,
		const V3f& newVertexPosition,
		const UINT currentIteration
		)
	{
		//PROFILE;

		Vertex & vTo = m_V[ iVertexTo ];
		const Vertex & vFrom = m_V[ iVertexFrom ];

		mxASSERT( !vTo.isBorder && !vFrom.isBorder );
		mxASSERT( vTo.refCount <= vFrom.refCount );	// we try to collapse to vertex with minimum number of connections

		// We will be deleting vertex 'From' and moving vertex 'To' to vNew.

		// find degenerated triangles which should later be deleted.
		DeletedFacesMask deleted1 = 0;
		DeletedFacesMask deleted2 = 0;

#if DETECT_MESH_FOLDING
		// Check to make sure this edge collapse won't invert any triangles.
		if(
			BecomesFlipped( vTo, iVertexTo, newVertexPosition, deleted1 )
			||
			BecomesFlipped( vFrom, iVertexTo, newVertexPosition, deleted2 )
			)
		{
			return false;
		}
#endif
		// not flipped, so remove edge

		// Once you have QEMs Q1 and Q2 for vertex pair (v1, v2),
		// you can approximate an error matrix for the collapsed vertex vNew as Q = Q1 + Q2.
		vTo.Q = vTo.Q + vFrom.Q;
		// Update the 'to' vertex to have the optimal collapsed position.
		vTo.position = newVertexPosition;	// vertex A is now vNew

		vTo.material = largest( vTo.material, vFrom.material );

		// Update the triangles that shared the 'to' and 'from' vertices.
		const UINT iNewRef = m_R.num();
		UpdateTrianglesAroundVertex( vTo, iVertexTo, deleted1, currentIteration );
		UpdateTrianglesAroundVertex( vFrom, iVertexTo, deleted2, currentIteration );
		const UINT nNewRefCount = m_R.num() - iNewRef;

		if( nNewRefCount <= vTo.refCount )
		{
			// the vertex has less connections than before
			if( nNewRefCount ) {
				memcpy( &m_R[vTo.startRef], &m_R[iNewRef], nNewRefCount*sizeof(Ref) );
			}
		}
		else
		{
			// append
			vTo.startRef = iNewRef;
		}

		vTo.refCount = nNewRefCount;

		return true;
	}

	/// Checks if the triangle flips when the 'oldVertex' is moved into 'newVertexPosition' is removed.
	bool BecomesFlipped(
		const Vertex& oldVertex,
		const HVertex newVertexIndex,
		const V3f& newVertexPosition,
		DeletedFacesMask &facesToDelete
		)
	{
		//PROFILE;

		for( UINT iRef = 0; iRef < oldVertex.refCount; iRef++ )
		{
			const Ref& r = m_R[ oldVertex.startRef + iRef ];
			const Triangle& t = m_T[ r.tid ];
			// Check only triangles that aren't removed.
			if( !t.deleted )
			{
				const HVertex iV0 = t.v[ r.tvertex ];
				const HVertex iV1 = t.v[ (r.tvertex + 1) % 3 ];
				const HVertex iV2 = t.v[ (r.tvertex + 2) % 3 ];

				if( newVertexIndex == iV1 || newVertexIndex == iV2 )
				{
					// This triangle becomes degenerate after the edge collapse.
					facesToDelete |= (1ULL << iRef);	// it must be removed
					continue;
				}

				// avoid creating almost degenerated triangles
				const V3f d1 = V3_Normalized( m_V[ iV1 ].position - newVertexPosition );
				const V3f d2 = V3_Normalized( m_V[ iV2 ].position - newVertexPosition );
				if( fabs( d1 * d2 ) > REAL(0.999) ) {
					return true;	// the triangle will have very small angles and become 'thin'
				}

				// If the normals don't point in the same direction, then the triangle
				// would be reversed by this collapse and the mesh would fold over.
				// Don't do this edge collapse because it makes the model degenerate.
				const V3f newNormal = V3_Normalized( V3_Cross( d1, d2 ) );
				if( newNormal * (V3f&)t.plane < REAL(0.2) ) {	// compare with a small epsilon instead of zero
					return true;
				}
			}
		}
		return false;
	}

	/// Update triangle connections and edge errors after the edge is collapsed.
	void UpdateTrianglesAroundVertex(
		const Vertex& oldVertex,
		const HVertex iNewVertex,
		const DeletedFacesMask deletedFaces,
		const UINT currentIteration
		)
	{
		for( UINT iRef = 0; iRef < oldVertex.refCount; iRef++ )
		{
			const Ref r = m_R[ oldVertex.startRef + iRef ];	// NOTE: make a copy because the array 'm_R' may grow
			Triangle & t = m_T[ r.tid ];
			// Check only triangles that aren't removed.
			if( !t.deleted )
			{
			#if DETECT_MESH_FOLDING
				if( deletedFaces & (1ULL << iRef) )
				{
					// Mark the (degenerate) triangle as deleted.
					t.deleted = true;
					continue;
				}
			#endif
				// Update this triangle with the new vertex index.
				t.v[ r.tvertex ] = iNewVertex;

				// Mark this triangle as dirty - it cannot be collapsed at this stage.
				t.MarkDirty( currentIteration );

			#if !DETECT_MESH_FOLDING
				const HVertex iVertexB = t.v[ (r.tvertex + 1) % 3 ];
				const HVertex iVertexC = t.v[ (r.tvertex + 2) % 3 ];

				if( iNewVertex == iVertexB || iNewVertex == iVertexC )
				{
					// degenerated triangles must be removed
					t.deleted = true;
					continue;
				}
			#endif
				// Recompute the cost for all edge collapses that involve the old vertex.
				RecomputeEdgeCosts( t );

				m_R.add( r );
			}
		}
	}

	/// Computes the cost of collapsing this edge.
	REAL CalculateEdgeCost( const Vertex& a, const Vertex& b, V3f &p_result )
	{
		//PROFILE;

		const bool border = a.isBorder & b.isBorder;

		// Compute the matrix to solve for the optimal vertex location.
		const QuadricF Q = a.Q + b.Q;

		// The algorithm first tries to find a position for vNew that minimizes error.
		REAL error = 0;

#if ALLOW_FULL_EDGE_COLLAPSES
		// Try inverting the matrix Q to compute the minimum-error vertex.
		const REAL det = Q.Determinant( 0, 1, 2, 1, 4, 5, 2, 5, 7 );

		if ( det != 0 && !border )
		{
			// q_delta is invertible
			const REAL invDet = REAL(1) / det;
			p_result.x = -invDet * Q.Determinant( 1, 2, 3, 4, 5, 6, 5, 7, 8 );	// vx = A41/det(q_delta)
			p_result.y = +invDet * Q.Determinant( 0, 2, 3, 1, 5, 6, 2, 7, 8 );	// vy = A42/det(q_delta)
			p_result.z = -invDet * Q.Determinant( 0, 1, 3, 1, 4, 6, 2, 5, 8 );	// vz = A43/det(q_delta)
			error = Q.ComputeVertexError( p_result.x, p_result.y, p_result.z );
		}
		else
#endif
		{
			// If the search fails, it looks for an optimal position along the edge v1 - v2.
			// As a fallback, the algorithm chooses the position with the smallest error from among v1, v2, and (v1+v2)*0.5.
			const V3f p[3] = {
				a.position,
				b.position,
				(a.position + b.position) * 0.5f
			};
			const REAL errors[3] = {
				Q.ComputeVertexError( p[0].x, p[0].y, p[0].z ),
				Q.ComputeVertexError( p[1].x, p[1].y, p[1].z ),
				Q.ComputeVertexError( p[2].x, p[2].y, p[2].z ),
			};
			// Inversion failed so pick the vertex or midpoint with the lowest cost.
			const int minErrorIndex = Min3Index( errors[0], errors[1], errors[2] );
			error = errors[ minErrorIndex ];
			p_result = p[ minErrorIndex ];
		}
		return error;
	}

	ERet InitializeFrom( const TriMesh& mesh )
	{
		//PROFILE;

		const bool bHasPerVertexMaterials = ( mesh.materials.num() > 0 );

		const UINT numTriangles = mesh.triangles.num();
		const UINT numVertices = mesh.vertices.num();
		mxENSURE(numVertices < std::numeric_limits< HVertex::TYPE >::max(),
				ERR_UNSUPPORTED_FEATURE, "Too many vertices: %u", numVertices);
		//mxENSURE(numTriangles < std::numeric_limits< HFace::TYPE >::max(),
		//		ERR_UNSUPPORTED_FEATURE, "Too many triangles: %u", numTriangles);

		// Initialize vertices.
		mxDO(m_V.setCountExactly( numVertices ));
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			Vertex & vertex = m_V[ iVertex ];
			vertex.Q = QuadricF(0);
			vertex.position = mesh.vertices[ iVertex ].xyz;

			if( bHasPerVertexMaterials ) {
				vertex.material = mesh.materials[ iVertex ].v;
			} else {
				vertex.material = 0;
			}

			vertex.isBorder = FALSE;
			vertex.refCount = 0;
			vertex.startRef = ~0;
		}

		// Initialize triangles.
		mxDO(m_T.setCountExactly( numTriangles ));
		for( UINT iFace = 0; iFace < numTriangles; iFace++ )
		{
			const TriMesh::Triangle& source = mesh.triangles[ iFace ];

			Triangle & triangle = m_T[ iFace ];

			triangle.v[0] = HVertex(source.a[0]);
			triangle.v[1] = HVertex(source.a[1]);
			triangle.v[2] = HVertex(source.a[2]);

			// Compute the triangle's normal and plane quadric.
			Vertex & v0 = m_V[source.a[0]];
			Vertex & v1 = m_V[source.a[1]];
			Vertex & v2 = m_V[source.a[2]];

			// Recomputing face normals during simplification is not required,
			// but may improve results for closed meshes
			const V4f plane = mesh.CalculatePlane( source );
			triangle.plane = plane;

			// Compute the fundamental error quadric matrix for this triangle's plane.
			const QuadricF Kp( plane.x, plane.y, plane.z, plane.w );

			// Compute the error matrix for each vertex in the mesh.
			v0.Q += Kp;
			v1.Q += Kp;
			v2.Q += Kp;
			// Before any mesh simplification happens,
			// a quadric or set of quadrics associated with a vertex
			// will evaluate to 0 if the vertex location is evaluated using the quadric.
			// Vertices are evaluated against a quadric by multiplying the transpose of a vertex with the set of quadrics
			// then with the vertex again.

			triangle.deleted = false;
			triangle.iteration = ~0;
			//triangle.isBorder = false;

			if( bHasPerVertexMaterials )
			{
				const UByte4 material_at_vertex0 = mesh.materials[ source.a[0] ];
				const UByte4 material_at_vertex1 = mesh.materials[ source.a[1] ];
				const UByte4 material_at_vertex2 = mesh.materials[ source.a[2] ];
				const bool edge_01_has_different_materials = ( material_at_vertex0.v != material_at_vertex1.v );
				const bool edge_12_has_different_materials = ( material_at_vertex1.v != material_at_vertex2.v );
				const bool edge_02_has_different_materials = ( material_at_vertex0.v != material_at_vertex2.v );

				triangle.isBorder = edge_01_has_different_materials | edge_12_has_different_materials | edge_02_has_different_materials;
			}
			else
			{
				triangle.isBorder = false;
			}
		}

		// Calculate edge errors.
		for( UINT iFace = 0; iFace < numTriangles; iFace++ )
		{
			const TriMesh::Triangle& source = mesh.triangles[ iFace ];

			Triangle & triangle = m_T[ iFace ];

			const Vertex& vertexA = m_V[source.a[0]];
			const Vertex& vertexB = m_V[source.a[1]];
			const Vertex& vertexC = m_V[source.a[2]];

			// Compute the initial costs for all edges in the mesh.
			RecomputeEdgeCosts( triangle );
		}

		return ALL_OK;
	}

	void RecomputeEdgeCosts( Triangle & triangle )
	{
		const Vertex& v0 = m_V[triangle.v[0]];
		const Vertex& v1 = m_V[triangle.v[1]];
		const Vertex& v2 = m_V[triangle.v[2]];

		V3f	unused;
		triangle.edgeError[0] = CalculateEdgeCost( v0, v1, unused );
		triangle.edgeError[1] = CalculateEdgeCost( v1, v2, unused );
		triangle.edgeError[2] = CalculateEdgeCost( v2, v0, unused );
		Sort3IndicesAscending( triangle.edgeError, triangle.edgeIndices );
	}

	ERet WriteToMesh( TriMesh &mesh )
	{
		//PROFILE;

		const UINT newNumVertices = m_V.num();

		const bool bHasPerVertexMaterials = ( mesh.materials.num() > 0 );
		mesh.materials.RemoveAll();

		// Copy only used (referenced) vertices.
		mesh.vertices.RemoveAll();

		UINT maxVertexDegree = 0;	// stats

		//NOTE: we reuse the Vertex::startRef field to store a new vertex index
		for( UINT i = 0; i < newNumVertices; i++ )
		{
			Vertex & vertex = m_V[ i ];
			maxVertexDegree = largest(maxVertexDegree, vertex.refCount);
			const bool vertexIsUsed = (vertex.refCount > 0);
			if( vertexIsUsed ) {
				const U32 newVertexIndex = mesh.vertices.num();

				Meshok::Vertex newVertex;
				newVertex.xyz = vertex.position;
				mxDO(mesh.vertices.add( newVertex ));

				vertex.startRef = newVertexIndex;

				if( bHasPerVertexMaterials ) {
					const UByte4 newMat = { vertex.material };
					mxDO(mesh.materials.add( newMat ));
				}
			} else {
				vertex.startRef = ~0;
			}
		}
		//DBGOUT("maxVertexDegree = %u", maxVertexDegree);

		//mesh.RecomputeAABB();

		// Write triangles.
		mxDO(mesh.triangles.setCountExactly( m_T.num() ));
		for( UINT iFace = 0; iFace < m_T.num(); iFace++ )
		{
			const Triangle& face = m_T[ iFace ];
			mxASSERT( !face.deleted );
			TriMesh::Triangle & triangle = mesh.triangles[ iFace ];
			triangle.a[0] = m_V[ face.v[0] ].startRef;
			triangle.a[1] = m_V[ face.v[1] ].startRef;
			triangle.a[2] = m_V[ face.v[2] ].startRef;
		}

		return ALL_OK;
	}
};

// Decimator_Melax
// cosine of the dihedral angle
mxBIBREF("A simple, fast, and effective polygon reduction algorithm, 1998");
mxBIBREF("A GPU-based Approach for Massive Model Rendering with Frame-to-Frame Coherence, 2012");
/// based on half-edge collapse
//class MeshDecimator
//{
//};

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
