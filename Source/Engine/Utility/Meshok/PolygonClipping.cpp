#include "stdafx.h"
#pragma hdrstop
#include <MeshLib/TriMesh.h>
#include <Meshok/PolygonClipping.h>

namespace Meshok
{

UINT ClipConvexPolygon(
					   const V4f& _plane,
					   const V3f* __restrict _vertices, const UINT num_vertices,
					   V3f *__restrict _outputVertices,
					   const float _epsilon = 1e-5f
					   )
{
	V3f prevVertex;
	float prevDist;
	bool prevInFront;

	V3f currVertex = _vertices[ num_vertices - 1 ];
	float currDist = Plane_PointDistance( _plane, currVertex );
	bool currInFront = currDist > _epsilon;

	UINT numOutputVertices = 0;
	for( UINT i = 0; i < num_vertices; i++ )
	{
		prevVertex = currVertex;
		prevDist = currDist;
		prevInFront = currInFront;

		currVertex = _vertices[ i ];
		currDist = Plane_PointDistance( _plane, currVertex );
		currInFront = currDist > _epsilon;

		if( prevInFront ) {
			_outputVertices[ numOutputVertices++ ] = prevVertex;
		}
		if( prevInFront != currInFront ) {
			const float d = prevDist / ( prevDist - currDist );
			//const float frac = 1 - d;
			_outputVertices[ numOutputVertices++ ] = TLerp( prevVertex, currVertex, d );	// prev + (curr - prev) * d
		}
	}

	return numOutputVertices;
}

/// Splits the mesh by the plane and retains the part in front of the plane.
/// The portion on the negative plane side is discarded.
/// "Clipping a Mesh Against a Plane"
void ClipMeshToPlane(
					 const Meshok::TriMesh& _mesh, const V4f& _plane,
					 Meshok::TriMesh& _result, AllocatorI & scratchpad
					 )
{
	for( UINT iTri = 0; iTri < _mesh.triangles.num(); iTri++ )
	{
		const UInt3& tri = _mesh.triangles[ iTri ];
		const Meshok::Vertex& v0 = _mesh.vertices[ tri.a[0] ];
		const Meshok::Vertex& v1 = _mesh.vertices[ tri.a[1] ];
		const Meshok::Vertex& v2 = _mesh.vertices[ tri.a[2] ];

		V3f pos[3];
		pos[0] = v0.xyz;
		pos[1] = v1.xyz;
		pos[2] = v2.xyz;

		V3f clippedVerts[4];
		const UINT numClippedVertices = ClipConvexPolygon( _plane, pos, 3, clippedVerts );

		if( numClippedVertices )
		{
			const UINT iBaseIndex = _result.vertices.num();
			for( UINT i = 0; i < numClippedVertices; i++ )
			{
				Meshok::Vertex	newVertex;
				newVertex.xyz = clippedVerts[ i ];
				_result.vertices.add( newVertex );
			}

			const UINT numNewTriangles = numClippedVertices - 2;
			for( UINT t = 0; t < numNewTriangles; t++ )
			{
				UInt3 new_triangle(
					iBaseIndex,
					iBaseIndex + t + 1,
					iBaseIndex + t + 2
				);
				_result.triangles.add(new_triangle);
			}
		}
	}
}

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
