// Triangle Mesh structures.
// Conventions:
// - vertices are counter-clockwise as seen from the “outside” or “front”
// - surface normal points towards the outside (“outward facing normals”)
#pragma once


struct TriMeshI
{
	virtual ~TriMeshI() {}

	virtual U32 numTriangles() const = 0;

	virtual void getTriangleAtIndex(
		U32 index
		, V3f &v0
		, V3f &v1
		, V3f &v2
	) const = 0;
};


namespace Meshok
{

/// a SIMD-aligned vertex which can store a floating-point position/normal and a 4-byte user data
mxPREALIGN(16) struct Vertex : CStruct
{
	V3f	xyz;
	U32	tag;	//!< scratch space with user-defined semantics and padding for SIMD
public:
	mxDECLARE_CLASS(Vertex, CStruct);
	mxDECLARE_REFLECTION;
	mxIMPLEMENT_SERIALIZABLE_AS_POD(Vertex);
}
mxPOSTALIGN(16);

mxSTATIC_ASSERT(sizeof(Vertex) == 16);

/// indexed triangle mesh
struct TriMesh : CStruct
{
	typedef U32 Index;

	/// contains indices of three vertices
	typedef CTuple3< Index >	Triangle;

	// per-face data:
	DynamicArray< Triangle >	triangles;	//!< triangle list (32-bit indices into array of vertices)

	// per-vertex data:
	DynamicArray< Vertex >		vertices;	//!< vertex positions
	DynamicArray< UByte4 >		colors;		//!< [optional] vertex colors (RGBA32)
	DynamicArray< Vertex >		normals;	//!< [optional] vertex normals
	DynamicArray< Vertex >		tangents;	//!< [optional] vertex tangents
	DynamicArray< UByte4 >		materials;	//!< [optional] per-vertex materials - for voxel terrain
	DynamicArray< U32 >			keys;		//!< [optional] used by the voxel engine to store Morton keys
	DynamicArray< U32 >			counts;		//!< [optional] application-specific

	AABBf						localAabb;	//!< bounding box in local space - must be in sync with the above

public:
	mxDECLARE_CLASS(TriMesh, CStruct);
	mxDECLARE_REFLECTION;

	TriMesh( AllocatorI& heap /*= Memory::Scratch()*/ )
		: triangles( heap )
		, vertices( heap )
		, colors( heap )
		, normals( heap ), tangents( heap )
		, materials( heap )
		, keys( heap )
		, counts( heap )
	{
		AABBf_Clear( &localAabb );
	}

	///NOTE: requires an up-to-date local-space AABB to be computed
	void FitInto( const AABBf& theBox, bool preserveRelativeScale = true );

	ERet ImportFromFile( const char* filepath );

	ERet Load( AReader& _stream );
	ERet Save( AWriter &_stream ) const;

	const V4f CalculatePlane( const Triangle& triangle ) const
	{
		const Vertex& v0 = vertices[triangle.a[0]];
		const Vertex& v1 = vertices[triangle.a[1]];
		const Vertex& v2 = vertices[triangle.a[2]];

		const V3f p0 = v0.xyz;
		const V3f p1 = v1.xyz;
		const V3f p2 = v2.xyz;

		const V3f N = V3_Cross( p1 - p0, p2 - p0 );
		return Plane_FromPointNormal( p0, V3_Normalized( N ) );
	}

	//// Export surfaces in .OBJ file format.
	ERet Debug_SaveToObj( const char* filename ) const;

	void ReverseWinding();

	ERet Append( const TriMesh& _other );

	AABBf RecomputeAABB();

	V3f ComputeCenterOfMass() const;

	void TransformVertices( const M44f& _transform );

	/// recalculates vertex normals by average face normals
	ERet ComputeNormals_Naive();

	enum EReserveMask {
		WANT_NORMALS	= BIT(0),
		WANT_COLORS		= BIT(1),
		WANT_MATERIALS	= BIT(2),
		WANT_ALL		= ~0
	};
	ERet reserve( UINT numVertices, UINT numTriangles, int mask = 0 );
	void empty();

	bool IsValid() const {
		return this->vertices.num() && this->triangles.num();
	}
};

void ComputeFaceNormals_Scalar(
							   const Meshok::TriMesh& _mesh,
							   Meshok::Vertex *_faceNormals
							   );
void ComputeFaceNormals_SSE2(
							const Meshok::TriMesh& _mesh,
							Meshok::Vertex *_faceNormals
							);

/// Duplicates sharp vertices. New vertices are assigned the normals of the corresponding faces.
/// Preserves vertex tags (Meshok::Vertex.tag).
ERet ComputeNormals_SplitSharpVertices(
	Meshok::TriMesh & _mesh,	//!< vertex positions are expected to be normalized (in [0..1] range) wrt to local bounds
	AllocatorI & scratchpad	//!< temp storage for allocating per-face normals
	//float angleDeg = RAD2DEG(acos(0.8f))
	);

/*
=======================================================================
	MESH ADJACENCY DATA STRUCTURES
=======================================================================
*/

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

///
/// face is a triangle or a quad
struct VertexFaceAdjacency
{
	/// contains the triangle ID (tid) and vertex offset inside the triangle (tvertex).
	struct Ref
	{
		unsigned	tid : 30;		//!< triangle index (HFace)
		unsigned	tvertex : 2;	//!< face-relative vertex index (within [0..3) range)
	};
	mxSTATIC_ASSERT(sizeof(Ref) == sizeof(U32));

	struct Vertex
	{
		unsigned	refCount;	//!< the number of (triangles||edges) referencing this vertex (aka 'valence' or 'degree')
		unsigned	startRef;	//!< the index of the first Ref ({TriangleId; iVertex} pair)
	public:
		Vertex()
		{
			refCount = 0;
			startRef = 0;
		}
	};

	DynamicArray< Vertex >		m_V;	//!< array of vertex attributes
	DynamicArray< Ref >			m_R;	//!< array of {TriangleId; iVertex} pairs, 3 per each triangle.

public:
	VertexFaceAdjacency( AllocatorI & heap )
		: m_V( heap ), m_R( heap )
	{}

	ERet FromTriangleMesh( const TriMesh& _mesh )
	{
		// Init Reference ID list
		const unsigned numVertices = _mesh.vertices.num();
		const unsigned numTriangles = _mesh.triangles.num();

		mxDO(m_V.setCountExactly( numVertices ));	// Sets the valence value for each vertex to 0.

		// Compute vertex valences.
		for( unsigned iFace = 0; iFace < numTriangles; iFace++ ) {
			const UInt3& triangle = _mesh.triangles[ iFace ];
			m_V[ triangle.a[0] ].refCount++;
			m_V[ triangle.a[1] ].refCount++;
			m_V[ triangle.a[2] ].refCount++;
		}
		{
			unsigned	runningOffset = 0;
			for( unsigned iVertex = 0; iVertex < numVertices; iVertex++ )
			{
				Vertex & vertex = m_V[ iVertex ];
//				mxASSERT2(vertex.refCount > 0, "Isolated vertex found");
				vertex.startRef = runningOffset;
				runningOffset += vertex.refCount;
				// valences will be reused to track how many triangles were encountered for each vertex
				vertex.refCount = 0;
			}
		}
		// Fill in the adjacency tables.
		mxDO(m_R.setCountExactly( numTriangles * 3 ));
		for( unsigned iFace = 0; iFace < numTriangles; iFace++ )
		{
			const UInt3& triangle = _mesh.triangles[ iFace ];
			for( int iFaceVert = 0; iFaceVert < 3; iFaceVert++ ) {
				Vertex & v = m_V[ triangle.a[ iFaceVert ] ];
				Ref & r = m_R[ v.startRef + v.refCount ];
				r.tid = iFace;
				r.tvertex = iFaceVert;
				v.refCount++;
			}
		}

		return ALL_OK;
	}
};


/// Objects are identified using positive integers, wrapped into named structs for type safety.
MAKE_ALIAS( HVertex, U32 );	//!< vertex index - absolute, i.e. not in [0..4) range (within quad/triangle)
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

/// Mesh Connectivity/Adjacency data structure.
/// Builds tables for quickly finding half edges of each triangle.
/// Works only with 2-manifold triangle meshes with boundaries.
/// A mesh is edge-manifold if no edge is shared by more than 2 facets,
/// i.e. any two neighboring triangles share only one common edge.
/// Face can be a triangle or a quad.
/// We always works with quads and pretend that they are triangles.
mxBIBREF("Out-of-Core Compression for Gigantic Polygon Meshes, 2003, 3.1 Half-Edge Data-Structure");
struct EdgeFaceAdjacency
{
	/// returns the face index associated with this edge
	static inline HFace GetFaceIndex( EdgeTag edgeTag )	{ return HFace(edgeTag / 4); }
	/// returns face-relative vertex index (within [0..4) range)
	static inline UINT GetFaceCorner( EdgeTag edgeTag )	{ return edgeTag % 4; }
	/// returns the next face-relative vertex index (within [0..4) range)
	static inline UINT NextFaceVertex( UINT faceVertex )	{ return (faceVertex + 1) % 4; }
	/// returns the previous face-relative vertex index (within [0..4) range)
	static inline UINT PrevFaceVertex( UINT faceVertex )	{ return (faceVertex + 2) % 4; }

public:
	/// half edges of a single face
	struct FaceEdges {
		HEdge	e[4];	//!< outgoing half-edge from each vertex
	};

	// half-edge table
	DynamicArray< EdgeTag >		m_HE;	//!< HEdge => {iFace, iStartVertex}, contains paired half-edges (i.e. Count % 2 = 0).
	// face table
	DynamicArray< FaceEdges >	m_FE;	//!< HFace => iEdge - index into the above table, 4 per each face (a quad or triangle).

public:
	EdgeFaceAdjacency( AllocatorI & heap )
		: m_HE( heap ), m_FE( heap )
	{}

	// corresponding half-edge in opposite direction
	HEdge TwinEdge( HEdge edge ) const
	{
		mxASSERT(edge != INVALID_EDGE);
		if( edge == INVALID_EDGE ) return INVALID_EDGE;
		return (edge % 2) ? HEdge(edge - 1) : HEdge(edge + 1);
	}

	// next outgoing edge with same start vertex (CCW)
	HEdge NextEdgeAroundVertex( HEdge edge ) const
	{
		const EdgeTag tag = m_HE[ edge ];
		return NextEdgeAroundVertex(tag);
	}
	HEdge PreviousEdgeAroundVertex( HEdge edge ) const
	{
		const EdgeTag tag = m_HE[ TwinEdge(edge) ];
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
		const FaceEdges& face = m_FE[ GetFaceIndex( tag ) ];
		return face.e[ NextFaceVertex(GetFaceCorner( tag )) ];
	}
	// prev edge in this face (CCW)
	HEdge PrevFaceEdge( EdgeTag tag ) const
	{
		const FaceEdges& face = m_FE[ GetFaceIndex( tag ) ];
		return face.e[ PrevFaceVertex(GetFaceCorner( tag )) ];
	}

	ERet FromTriangleMesh( const TriMesh& _mesh, AllocatorI & scratchpad );

private:
	void ConnectOppositeHalfEdges( const EdgeTag t0, const EdgeTag t1 )
	{
		const HEdge e0 = HEdge(m_HE.num());	// new half-edge
		const HEdge e1 = HEdge(e0 + 1);	// opposite half-edge
		mxASSERT(e0 % 2 == 0 && TwinEdge(e0) == e1);

		m_HE.add( t0 );
		m_HE.add( t1 );

		// Update the half-edge references in two adjacent triangles.
		m_FE[ GetFaceIndex(t0) ].e[ GetFaceCorner(t0) ] = e0;
		m_FE[ GetFaceIndex(t1) ].e[ GetFaceCorner(t1) ] = HEdge(e0+1);
	}
	void ConnectToBoundaryHalfEdge( const EdgeTag t )
	{
		const HEdge e = HEdge(m_HE.num());
		mxASSERT(e % 2 == 0);
		m_HE.add( t );
		m_HE.add( INVALID_TAG );	// boundary half-edge
		m_FE[ GetFaceIndex(t) ].e[ GetFaceCorner(t) ] = e;
	}
};

/// Saves the indices in the specified (16- or 32-bit) format.
void CopyIndices(
				 void *dstIndices
				 , bool dstIndices32bit
				 , const TSpan< UInt3 >& srcTriangles
				 );

bool DBG_Check_Vertex_Indices_Are_In_Range(
	const UINT num_vertices,
	const void* _indices, const UINT _numIndices,
	const bool _indicesAre16bit
);

}//namespace Meshok
