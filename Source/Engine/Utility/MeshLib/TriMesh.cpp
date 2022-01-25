#include <Base/Base.h>
#pragma hdrstop

#include <algorithm>	// std::sort

#include <xnamath.h>

#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/Containers/Blob.h> 
#include <Core/Memory.h>	// scratch heap
#include <Core/Text/TextWriter.h>	// Debug_SaveToObj
#include <Core/Util/ScopedTimer.h>

#include <MeshLib/TriMesh.h>

#include <GPU/Public/graphics_device.h>
#include <Rendering/Public/Core/Mesh.h>

namespace Meshok
{

mxDEFINE_CLASS(Vertex);
mxBEGIN_REFLECTION(Vertex)
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( tag ),
mxEND_REFLECTION;

//mxDEFINE_CLASS(TriMesh::Triangle);
//mxBEGIN_REFLECTION(TriMesh::Triangle)
//	mxMEMBER_FIELD( v ),
//mxEND_REFLECTION;
//
mxDEFINE_CLASS_NO_DEFAULT_CTOR(TriMesh);
mxBEGIN_REFLECTION(TriMesh)
	mxMEMBER_FIELD( triangles ),
	mxMEMBER_FIELD( vertices ),
	mxMEMBER_FIELD( normals ),
	mxMEMBER_FIELD( tangents ),
	//mxMEMBER_FIELD( materials ),
	mxMEMBER_FIELD( localAabb ),
mxEND_REFLECTION;

void TriMesh::FitInto( const AABBf& theBox, bool preserveRelativeScale )
{
	const V3f oldSize = preserveRelativeScale ?
		V3_SetAll( V3_Maxs( AABBf_FullSize(localAabb) ) )
		:
		AABBf_FullSize(localAabb)
		;
	const V3f newSize = AABBf_FullSize(theBox);
	const V3f oldCenter = AABBf_Center(localAabb);
	const V3f newCenter = AABBf_Center(theBox);

	const V3f scale = V3_Divide( newSize, oldSize );

	for( UINT iVertex = 0; iVertex < vertices.num(); iVertex++ )
	{
		Vertex & vertex = vertices[ iVertex ];
		V3f relativePos = vertex.xyz - oldCenter;
		V3f scaledPos = V3_Multiply( relativePos, scale );
		vertex.xyz = scaledPos + newCenter;
	}
	this->localAabb = theBox;
}

ERet TriMesh::Debug_SaveToObj( const char* filename ) const
{
	FileWriter	file;
	mxDO(file.Open(filename));

	TextWriter	writer(file);

	writer.Emitf("# %d vertices\n", this->vertices.num());
	for( UINT iVertex = 0; iVertex < this->vertices.num(); iVertex++ )
	{
		const V3f& pos = this->vertices[ iVertex ].xyz;
		writer.Emitf("v %lf %lf %lf\n", pos.x, pos.y, pos.z);
	}
	writer.Emitf("# %d triangles\n", this->triangles.num());
	for( UINT iFace = 0; iFace < this->triangles.num(); iFace++ )
	{
		const Triangle& face = this->triangles[ iFace ];
		writer.Emitf("f %d// %d// %d//\n", face.a[0]+1, face.a[1]+1, face.a[2]+1);
	}
	return ALL_OK;
}

void TriMesh::ReverseWinding()
{
	for( UINT i = 0; i < this->triangles.num(); i++ )
	{
		Triangle & tri = this->triangles[ i ];
		TSwap( tri.a[1], tri.a[2] );
	}
}

ERet TriMesh::Append( const TriMesh& _other )
{	
	const UINT oldTriCount = triangles.num();
	const UINT oldVtxCount = vertices.num();
	mxDO(Arrays::append( vertices, _other.vertices ));
	mxDO(Arrays::append( colors, _other.colors ));
	mxDO(Arrays::append( normals, _other.normals ));
	mxDO(Arrays::append( tangents, _other.tangents ));
	mxDO(Arrays::append( materials, _other.materials ));

	mxDO(triangles.setCountExactly( oldTriCount + _other.triangles.num() ));	
	for( UINT i = 0; i < _other.triangles.num(); i++ )
	{
		const Triangle& src = _other.triangles[ i ];
		Triangle &dst = triangles[ oldTriCount + i ];
		dst.a[0] = src.a[0] + oldVtxCount;
		dst.a[1] = src.a[1] + oldVtxCount;
		dst.a[2] = src.a[2] + oldVtxCount;
	}
	return ALL_OK;
}

AABBf TriMesh::RecomputeAABB()
{
	this->localAabb = AABBf_From_Points_Aligned(
		this->vertices.num(), &this->vertices[0].xyz, sizeof(this->vertices[0])
		);
	return this->localAabb;
}

V3f TriMesh::ComputeCenterOfMass() const
{
	const int numVertices = this->vertices.num();
#if 0
	V3f centerOfMass(0.0f);
	for( int iVertex = 0; iVertex < numVertices; iVertex++ ) {
		centerOfMass += this->vertices[ iVertex ].xyz;
	}
	centerOfMass *= ( 1.0f / this->vertices.num() );
	return centerOfMass;
#else
	Vector4 centerOfMass = g_MM_Zero;
	const Vector4* src = (Vector4*) this->vertices.raw();
	for( int iVertex = 0; iVertex < numVertices; iVertex++ ) {
		centerOfMass = V4_ADD( centerOfMass, src[ iVertex ] );
	}
	centerOfMass = V4_DIV( centerOfMass, _mm_set_ps1( numVertices ) );
	return (V3f&)centerOfMass;
#endif
}

void TriMesh::TransformVertices( const M44f& _transform )
{
	const int numVertices = this->vertices.num();
	Vector4 * src = (Vector4*) this->vertices.raw();
	M44_TransformVector4_Array( src, numVertices, _transform, src );
}

ERet TriMesh::ComputeNormals_Naive()
{
	const UINT numVertices = this->vertices.num();
	mxDO(this->normals.setCountExactly( numVertices ));
	{
		Vertex zeroNormal;
		zeroNormal.xyz = V3_Zero();
		Arrays::setAll( this->normals, zeroNormal );
	}

	for( UINT iFace = 0; iFace < this->triangles.num(); iFace++ )
	{
		const TriMesh::Triangle& face = this->triangles[ iFace ];
		const V3f a = this->vertices[ face.a[0] ].xyz;
		const V3f b = this->vertices[ face.a[1] ].xyz;
		const V3f c = this->vertices[ face.a[2] ].xyz;
		const V3f faceNormal = V3_Cross( b - a, c - a );

		this->normals[ face.a[0] ].xyz += faceNormal;
		this->normals[ face.a[1] ].xyz += faceNormal;
		this->normals[ face.a[2] ].xyz += faceNormal;
	}

	for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		this->normals[ iVertex ].xyz = V3_Normalized( this->normals[ iVertex ].xyz );
	}
	return ALL_OK;
}

ERet TriMesh::reserve( UINT numVertices, UINT numTriangles, int mask /*= 0*/ )
{
	mxDO(this->vertices.reserve( numVertices ));
	if( mask & WANT_NORMALS ) {
		mxDO(this->normals.reserve( numVertices ));
	}
	if( mask & WANT_COLORS ) {
		mxDO(this->colors.reserve( numVertices ));
	}
	if( mask & WANT_MATERIALS ) {
		mxDO(this->materials.reserve( numVertices ));
	}
	mxDO(this->triangles.reserve( numTriangles ));
	return ALL_OK;
}

void TriMesh::empty()
{
	triangles.RemoveAll();
	vertices.RemoveAll();
	colors.RemoveAll();
	normals.RemoveAll();
	tangents.RemoveAll();
	materials.RemoveAll();
	AABBf_Clear( &localAabb );
}

ERet TriMesh::Load( AReader& _stream )
{
	_stream >> triangles;
	_stream >> vertices;
	_stream >> normals;
	_stream >> localAabb;
	return ALL_OK;
}

ERet TriMesh::Save( AWriter &_stream ) const
{
	_stream << triangles;
	_stream << vertices;
	_stream << normals;
	_stream << localAabb;
	return ALL_OK;
}

void ComputeFaceNormals_Scalar(
							   const Meshok::TriMesh& _mesh,
							   Meshok::Vertex *_faceNormals
							   )
{
	const Meshok::Vertex * __restrict vertices = _mesh.vertices.raw();
	const UINT numFaces = _mesh.triangles.num();
	for( UINT iFace = 0; iFace < numFaces; iFace++ )
	{
		const TriMesh::Triangle& face = _mesh.triangles[ iFace ];
		const V3f a = vertices[ face.a[0] ].xyz;
		const V3f b = vertices[ face.a[1] ].xyz;
		const V3f c = vertices[ face.a[2] ].xyz;
		const V3f faceNormal = V3_Cross( b - a, c - a );
		_faceNormals[ iFace ].xyz = V3_Normalized( faceNormal );
	}
}

void ComputeFaceNormals_SSE2(
							const Meshok::TriMesh& _mesh,
							Meshok::Vertex *_faceNormals
							)
{
	mxSTATIC_ASSERT(sizeof(Meshok::Vertex) == sizeof(Vector4));

	const Vector4 * __restrict vertices = (Vector4*) _mesh.vertices.raw();
	Vector4 * __restrict faceNormals = (Vector4*) _faceNormals;
	mxASSERT(IS_16_BYTE_ALIGNED(vertices));
	mxASSERT(IS_16_BYTE_ALIGNED(faceNormals));

	const UINT numFaces = _mesh.triangles.num();
	for( UINT iFace = 0; iFace < numFaces; iFace++ )
	{
		const TriMesh::Triangle& face = _mesh.triangles[ iFace ];
		const Vector4 a = vertices[ face.a[0] ];
		const Vector4 b = vertices[ face.a[1] ];
		const Vector4 c = vertices[ face.a[2] ];
		const Vector4 faceNormal = Vector4_Cross3( V4_SUB( b, a ), V4_SUB( c, a ) );
		faceNormals[ iFace ] = XMVector3Normalize( faceNormal );
	}
}

/// Duplicates sharp vertices. New vertices are assigned the normals of the corresponding faces.
/// Preserves vertex tags (Meshok::Vertex.tag).
ERet ComputeNormals_SplitSharpVertices(
	Meshok::TriMesh & _mesh,	//!< vertex positions are expected to be normalized (in [0..1] range) wrt to local bounds
	AllocatorI & scratchpad	//!< temp storage for allocating per-face normals
	)
{
	//FIXME: branch
	const bool hasColors = !_mesh.colors.IsEmpty();	// do we need to copy vertex colors?

	// 1. Compute face normals.
	const UINT numFaces = _mesh.triangles.num();

	DynamicArray< Meshok::Vertex >	faceNormals( scratchpad );
	mxDO(faceNormals.setCountExactly( numFaces ));
	{
		Meshok::Vertex zeroNormal;
		zeroNormal.xyz = V3_Zero();
		Arrays::setAll( faceNormals, zeroNormal );
	}

	//ComputeFaceNormals_Scalar( _mesh, faceNormals.raw() );
	ComputeFaceNormals_SSE2( _mesh, faceNormals.raw() );

	// 2. Calculate vertex <-> face adjacency.
	Meshok::VertexFaceAdjacency	adj( scratchpad );
	mxDO(adj.FromTriangleMesh( _mesh ));

	// 3. find 'sharp' vertices.
	const UINT numOriginalVertices = _mesh.vertices.num();

	// Tag vertices located on sharp features (i.e. on sharp corners or edges).
	enum { TAG_NONE = 0, TAG_SHARP_VERTEX = 1 };
	// Pre-allocate vertex normals.
	mxDO(_mesh.normals.setCountExactly( numOriginalVertices ));
	{
		Meshok::Vertex zeroNormal;
		zeroNormal.xyz = V3_Zero();
		zeroNormal.tag = TAG_NONE;
		Arrays::setAll( _mesh.normals, zeroNormal );
	}

	U32 numIsolatedVertices = 0;
	for( UINT iVertex = 0; iVertex < numOriginalVertices; iVertex++ )
	{
		const Meshok::Vertex& vertex = _mesh.vertices[ iVertex ];
		Meshok::Vertex &normal = _mesh.normals[ iVertex ];

		// Estimate the angle of the normal cone.
		mxBIBREF("Kobbelt L., Botsch M., Schwanecke U., Seidel. P. Feature sensitive surface extraction from volume data, SIGGRAPH 2001, p.7");
		// find the maximal angle formed by normals => find the minimum cos(angle).
		const Meshok::VertexFaceAdjacency::Vertex& v = adj.m_V[ iVertex ];
		const UINT iFirstFace = v.startRef;
		const UINT numAdjFaces = v.refCount;
		//mxASSERT2(numAdjFaces, "Isolated vertex found: %d", iVertex);
		if( !numAdjFaces ) {
			numIsolatedVertices++;
		}

		float minimumTheta = 1.0f;	// the lower this value, the 'sharper' the feature

		CV3f averageNormal(0);

		for( UINT i = 0; i < numAdjFaces; i++ )
		{
			const Meshok::VertexFaceAdjacency::Ref& refA = adj.m_R[ iFirstFace + i ];
			const Meshok::Vertex& faceNormalA = faceNormals[ refA.tid ];
			averageNormal += faceNormalA.xyz;

			for( UINT j = i; j < numAdjFaces; j++ )
			{
				const Meshok::VertexFaceAdjacency::Ref& refB = adj.m_R[ iFirstFace + j ];
				const Meshok::Vertex& faceNormalB = faceNormals[ refB.tid ];
				const float dp = V3f::dot( faceNormalA.xyz, faceNormalB.xyz );
				if( dp < minimumTheta ) {
					minimumTheta = dp;	// minimum cos(angle) => maximum angle
				}
			}//face j
		}//face i

		averageNormal *= (1.0f / (int)numAdjFaces);
		averageNormal.normalize();

		normal.xyz = averageNormal;
		normal.tag = ( minimumTheta < 0.8f );	// true if sharp feature
	}


	//computePerVertexAmbientOcclusion();


	// 4. Split sharp vertices.

	for( UINT iVertex = 0; iVertex < numOriginalVertices; iVertex++ )
	{
		// make a copy, because '_mesh.vertices' array might have been resized
		const Meshok::Vertex sourceVertex = _mesh.vertices[ iVertex ];
		const Meshok::Vertex normal = _mesh.normals[ iVertex ];

		//const UByte4 color = _mesh.colors[ iVertex ];
		UByte4 color;
		if( hasColors ) {
			color = _mesh.colors[ iVertex ];
		}

		//if( cellData.sharp )
		//	_dbgView.addSharpFeatureVertex(sourceVertex->xyz);

		// if this is a sharp feature vertex (i.e. lying on a sharp corner/edge)...
		if( normal.tag )
		{
			// ... create as many vertices as there are faces using this vertex.
			const Meshok::VertexFaceAdjacency::Vertex& v = adj.m_V[ iVertex ];
			mxASSERT(v.refCount > 0);

			// For each face using this vertex:

			// update the normal of this vertex
			{
				const Meshok::VertexFaceAdjacency::Ref& face0 = adj.m_R[ v.startRef ];
				const Meshok::Vertex& faceNormal0 = faceNormals[ face0.tid ];	// 'flat' normal
				_mesh.normals[ iVertex ].xyz = faceNormal0.xyz;	// assign face normal to the vertex
			}
			// create additional vertices for remaining faces:
			for( UINT iFaceRef = 1; iFaceRef < v.refCount; iFaceRef++ )
			{
				const Meshok::VertexFaceAdjacency::Ref& faceRef = adj.m_R[ v.startRef + iFaceRef ];
				const Meshok::Vertex& faceNormal = faceNormals[ faceRef.tid ];	// 'flat' normal

				const UINT newVertexIndex = _mesh.vertices.num();

				// copy old position and boundary tag
				Meshok::Vertex	newVertexPosition;
				newVertexPosition.xyz = sourceVertex.xyz;
				newVertexPosition.tag = sourceVertex.tag;

				// assign 'flat', face normal
				Meshok::Vertex	newVertexNormal;
				newVertexNormal.xyz = faceNormal.xyz;
				newVertexNormal.tag = TAG_SHARP_VERTEX;

				mxDO(_mesh.vertices.add( newVertexPosition ));	// copy vertex position
				mxDO(_mesh.normals.add( newVertexNormal ));	// assign face normal
				if( hasColors ) {
					mxDO(_mesh.colors.add( color ));	// copy vertex color
				}

				UInt3 & tri = _mesh.triangles[ faceRef.tid ];	//!< the triangle using this vertex
				tri.a[ faceRef.tvertex ] = newVertexIndex;	// update the vertex index in the triangle
			}//for each face around the sharp vertex
		}//If this is a sharp vertex.
	}//For each cell vertex.

#if MX_DEBUG
	if( numIsolatedVertices ) {
		mxTEMP
//		ptWARN("%d isolated vertices!", numIsolatedVertices);
	}
#endif // MX_DEBUG
	return ALL_OK;
}

ERet EdgeFaceAdjacency::FromTriangleMesh( const TriMesh& _mesh, AllocatorI & scratchpad )
{
	// Init Reference ID list
	const UINT numVertices = _mesh.vertices.num();
	const UINT numTriangles = _mesh.triangles.num();
	const UInt3* triangles = _mesh.triangles.raw();

	mxDO(m_FE.setCountExactly(numTriangles));
	{
		const FaceEdges isolatedFace = { INVALID_EDGE, INVALID_EDGE, INVALID_EDGE, INVALID_EDGE };
		Arrays::setAll( m_FE, isolatedFace );
	}

	/// Temporary structure for sorting half-edges.
	struct HalfEdge
	{
		EdgeTag	tag;	//!< encoded face and vertex indices: tag = face*4 + vertex
		U32	start, end;	//!< vertex indices are keys for sorting, start < end !
		U32	padding;
	public:
		static HalfEdge Encode( UINT triangleIndex, UINT faceCorner, UINT startVertex, UINT endVertex )
		{
			// vertices must be ordered lexicographically, i.e. start < end!
			TSort2( startVertex, endVertex );	//ensure that 'startVertex != endVertex'
			HalfEdge edge;
			edge.tag = EdgeTag( triangleIndex * 4 + faceCorner );
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

	// Half-edge matching: traverse the entire mesh and test each edge's vertices with all other edge's vertices.
	// If the vertices of one edge match those of another, they are paired together.
	// This can be done in O(N) time and space using lexicographic sorting.
	DynamicArray< HalfEdge >	sortedHalfEdges( scratchpad );
	{
		const UINT numEdges = numTriangles * 4;	// we always works with quads and pretend that they are triangles
		mxDO(sortedHalfEdges.setCountExactly( numEdges ));
	}
	for( UINT iTriangle = 0; iTriangle < numTriangles; iTriangle++ )
	{
		const UInt3& face = triangles[ iTriangle ];
		sortedHalfEdges[ iTriangle*4 + 0 ] = HalfEdge::Encode( iTriangle, 0, face.a[ 0 ], face.a[ 1 ] );
		sortedHalfEdges[ iTriangle*4 + 1 ] = HalfEdge::Encode( iTriangle, 1, face.a[ 1 ], face.a[ 2 ] );
		sortedHalfEdges[ iTriangle*4 + 2 ] = HalfEdge::Encode( iTriangle, 2, face.a[ 2 ], face.a[ 0 ] );
	}

	std::sort( sortedHalfEdges.begin(), sortedHalfEdges.end() );
	// now oppositely-oriented adjacent edges are stored in pairs

	// Build edge-face adjacency.
	mxBIBREF("Out-of-Core Compression for Gigantic Polygon Meshes, 2003, 3.3 Building the Out-of-Core Mesh, Matching of Inverse Half-Edges");
	{
		UINT	lastStartVertex = -1;//sortedHalfEdges[0].start;
		UINT	lastEndVertex = -1;//sortedHalfEdges[0].end;
		EdgeTag	currentRun[3];	// enough space for the shortest invalid sequence
		UINT	runLength = 0;

		for( UINT iEdge = 0; iEdge < sortedHalfEdges.num(); iEdge++ )
		{
			const HalfEdge current = sortedHalfEdges[ iEdge ];
			// If the current edge is the same as last one
			if( current.start == lastStartVertex && current.end == lastEndVertex )
			{
				currentRun[ runLength++ ] = current.tag;	// store edge tag: face and vertex idx within face
				// Only works with 2-manifold meshes (i.e. an edge is not shared by more than 2 triangles)
				if( runLength > 2 ) {
					LogStream log;
					log << "Start " << lastStartVertex << ", end " << lastEndVertex;
					for( int i = 0; i < runLength; i++ ) {
						const EdgeTag tag = currentRun[i];
						const HFace iFace = GetFaceIndex(tag);
						log << "\nFace " << iFace << ", vertices: " << triangles[iFace].a[0] << ", " << triangles[iFace].a[1] << ", " << triangles[iFace].a[2];
					}
				}
				// non-manifold edge: more than two matched half-edges
				mxENSURE( runLength < 3, ERR_INVALID_PARAMETER, "The mesh is not edge-manifold");
			}
			else
			{
				// if Count==1 => This is a boundary edge: it belongs to exactly one triangle.
				if( runLength == 1 )
				{
					const EdgeTag t = currentRun[0];
					//const HVertex iVertex = triangles[ GetFaceIndex(t) ].v[ GetFaceCorner(t) ];
					//vertices[ iVertex ].onBoundary = true;// an unmatched half-edge is a border edge
					this->ConnectToBoundaryHalfEdge( t );
				}
				// if Count==2 => This is an internal edge (aka 'full-edge')
				// which consists of two opposite half edges and is shared by two adjacent triangles.
				else if( runLength == 2 )
				{
					// manifold edge: two matched half-edges with opposite orientation
					// Pair-up consecutive entries - they are opposite edges.
					this->ConnectOppositeHalfEdges( currentRun[0], currentRun[1] );
				}
				else {
					mxUNREACHABLE;
				}

				// not-oriented edge: two matched half-edges with identical orientation

				// Reset for next edge
				runLength = 0;
				currentRun[ runLength++ ] = current.tag;
				lastStartVertex = current.start;
				lastEndVertex = current.end;
			}
		}
		if( runLength == 2 ) {
			this->ConnectOppositeHalfEdges( currentRun[0], currentRun[1] );
		}
	}

	mxASSERT2(m_HE.num()%2 == 0, "The table must contain an even number of half-edges");

	return ALL_OK;
}

// The creaseAngle field, used by the ElevationGrid, Extrusion, and IndexedFaceSet nodes, affects how default normals are generated. If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are smooth-shaded across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced. For example, a crease angle of 0.5 radians means that an edge between two adjacent polygonal faces will be smooth shaded if the geometric normals of the two faces form an angle that is less than 0.5 radians. Otherwise, the faces will appear faceted. Crease angles shall be greater than or equal to 0.0.

/// Saves the indices in the specified (16- or 32-bit) format.
void CopyIndices(
				 void *mxRESTRICT_PTR(dstIndices)
				 , bool dstIndices32bit
				 , const TSpan< UInt3 >& srcTriangles
				 )
{
	//mxASSERT(IS_16_BYTE_ALIGNED(dstIndices));
	if( dstIndices32bit ) {
		ASSERT_SIZEOF_ARE_EQUAL( srcTriangles[0][0], U32 );
		memcpy( dstIndices, srcTriangles.raw(), srcTriangles.rawSize() );
	} else {
		mxOPTIMIZE("Fast conversion of indices: 32 => 16");
		U16 *dstIndices16 = (U16*) dstIndices;
		for( UINT iTriangle = 0; iTriangle < srcTriangles.num(); iTriangle++ ) {
			const UInt3& triangle = srcTriangles[ iTriangle ];
			dstIndices16[ iTriangle*3 + 0 ] = triangle.a[ 0 ];
			dstIndices16[ iTriangle*3 + 1 ] = triangle.a[ 1 ];
			dstIndices16[ iTriangle*3 + 2 ] = triangle.a[ 2 ];
		}
	}
}

bool DBG_Check_Vertex_Indices_Are_In_Range(
	const UINT num_vertices,
	const void* _indices, const UINT _numIndices,
	const bool _indicesAre16bit
	)
{
	if( _indicesAre16bit ) {
		const U16* indices16 = (U16*) _indices;
		for( UINT ii = 0; ii < _numIndices; ii++ ) {
			const UINT vert_idx = indices16[ ii ];
			if(!CHK( vert_idx < num_vertices )) {
				return false;
			}
		}
	} else {
		const U32* indices32 = (U32*) _indices;
		for( UINT ii = 0; ii < _numIndices; ii++ ) {
			const UINT vert_idx = indices32[ ii ];
			if(!CHK( vert_idx < num_vertices )) {
				return false;
			}
		}
	}
	return true;
}

}//namespace Meshok

/*
sharp feature/edge/crease preservation
https://github.com/PixarAnimationStudios/OpenSubdiv/blob/master/opensubdiv/sdc/crease.cpp
*/
//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
