#include "stdafx.h"
#pragma hdrstop
#include <Base/Math/MiniMath.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Core/Memory.h>
#include <Meshok/Meshok.h>
#include <Meshok/Volumes.h>	// ADebugView

namespace Meshok
{

// Euler's formula for solids bounded by 2-dimensional manifolds:

// V - Number of vertices
// E - Number of edges
// F - Number of faces
// H - Number of holes in faces
// 
U32 EulerNumber( U32 V, U32 E, U32 F, U32 H )
{
	return V - E + F - H;
}
// C - Number of connected components
// G - Number of holes in the solid (genus)
// For example, for a cube, V = 8, E = 12, F = 6, H = 0, C = 1, G = 0.
// Therefore, V - E + F - H = 2 = 2*(C - G).
// If Euler's formula is not satisfied, then there is an error in the model.
// 
bool EulerTest( U32 V, U32 E, U32 F, U32 H, U32 C, U32 G )
{
	return EulerNumber( V, E, F, H ) == 2 * (C - G);
}

// Creating simple volumes with different shapes, like spheres, ellipsoids, other shapes:
// https://github.com/marwan-abdellah/VolumeTools

ERet VoxelizeSphere( const float _radius, const U32 _gridSize, void *_voxels )
{
	U32 *	bits = (U32*) _voxels;

	const U32 gridSizeX = _gridSize;
	const U32 gridSizeY = _gridSize;
	const U32 gridSizeZ = _gridSize;
	const U32 halfSizeX = gridSizeX / 2;
	const U32 halfSizeY = gridSizeY / 2;
	const U32 halfSizeZ = gridSizeZ / 2;

	for( U32 iX = 0; iX < gridSizeX; iX++ )
	{
		for( U32 iY = 0; iY < gridSizeY; iY++ )
		{
			for( U32 iZ = 0; iZ < gridSizeZ; iZ++ )
			{
				const U32 index = iX + iY * gridSizeX + iZ * (gridSizeX * gridSizeY);

				const INT32 x = iX - halfSizeX;
				const INT32 y = iY - halfSizeY;
				const INT32 z = iZ - halfSizeZ;

				const float length = mmSqrt( x*x + y*y + z*z );
				const bool isSolid = (length <= _radius);

				// bits[ i / 32 ] |= (isSolid << (i % 32));
				bits[ index >> 5 ] |= (isSolid << (index & 31));
			}
		}
	}

	return ALL_OK;
}

// Dimensions of the model's bounding box in voxels.

}//namespace Meshok

mxSTOLEN("http://richardssoftware.net/Home/Post/60");
//https://github.com/ericrrichards/dx11/blob/b640855654aba2b30a3794f30c079812ee829aa2/DX11/Core/GeometryGenerator.cs

static FatVertex NewVertex(
							float Px, float Py, float Pz,
							float Nx, float Ny, float Nz,
							float Tx, float Ty, float Tz,
							float U, float V
							)
{
	FatVertex result;
	result.position = V3_Set( Px, Py, Pz );
	result.normal = V3_Set( Nx, Ny, Nz );
	result.tangent = V3_Set( Tx, Ty, Tz );
	result.texCoord = V2_Set( U, V );
	return result;
}

ERet CreateBox(
			   float width, float height, float depth,
			   TArray<FatVertex> &_vertices, TArray<int> &_indices
			   )
{
	_vertices.RemoveAll();
	_indices.RemoveAll();

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	const FatVertex vertices[] = {
		// front
		NewVertex(-w2, -h2, -d2, 0, 0, -1, 1, 0, 0, 0, 1),
		NewVertex(-w2, +h2, -d2, 0, 0, -1, 1, 0, 0, 0, 0),
		NewVertex(+w2, +h2, -d2, 0, 0, -1, 1, 0, 0, 1, 0),
		NewVertex(+w2, -h2, -d2, 0, 0, -1, 1, 0, 0, 1, 1),
		// back
		NewVertex(-w2, -h2, +d2, 0, 0, 1, -1, 0, 0, 1, 1),
		NewVertex(+w2, -h2, +d2, 0, 0, 1, -1, 0, 0, 0, 1),
		NewVertex(+w2, +h2, +d2, 0, 0, 1, -1, 0, 0, 0, 0),
		NewVertex(-w2, +h2, +d2, 0, 0, 1, -1, 0, 0, 1, 0),
		// top
		NewVertex(-w2, +h2, -d2, 0, 1, 0, 1, 0, 0, 0, 1),
		NewVertex(-w2, +h2, +d2, 0, 1, 0, 1, 0, 0, 0, 0),
		NewVertex(+w2, +h2, +d2, 0, 1, 0, 1, 0, 0, 1, 0),
		NewVertex(+w2, +h2, -d2, 0, 1, 0, 1, 0, 0, 1, 1),
		// bottom
		NewVertex(-w2, -h2, -d2, 0, -1, 0, -1, 0, 0, 1, 1),
		NewVertex(+w2, -h2, -d2, 0, -1, 0, -1, 0, 0, 0, 1),
		NewVertex(+w2, -h2, +d2, 0, -1, 0, -1, 0, 0, 0, 0),
		NewVertex(-w2, -h2, +d2, 0, -1, 0, -1, 0, 0, 1, 0),
		// left
		NewVertex(-w2, -h2, +d2, -1, 0, 0, 0, 0, -1, 0, 1),
		NewVertex(-w2, +h2, +d2, -1, 0, 0, 0, 0, -1, 0, 0),
		NewVertex(-w2, +h2, -d2, -1, 0, 0, 0, 0, -1, 1, 0),
		NewVertex(-w2, -h2, -d2, -1, 0, 0, 0, 0, -1, 1, 1),
		// right
		NewVertex(+w2, -h2, -d2, 1, 0, 0, 0, 0, 1, 0, 1),
		NewVertex(+w2, +h2, -d2, 1, 0, 0, 0, 0, 1, 0, 0),
		NewVertex(+w2, +h2, +d2, 1, 0, 0, 0, 0, 1, 1, 0),
		NewVertex(+w2, -h2, +d2, 1, 0, 0, 0, 0, 1, 1, 1),
	};
	mxDO(_vertices.AddMany( vertices, mxCOUNT_OF(vertices) ));

	const int indices[] = {
		0,1,2,0,2,3,
		4,5,6,4,6,7,
		8,9,10,8,10,11,
		12,13,14,12,14,15,
		16,17,18,16,18,19,
		20,21,22,20,22,23
	};
	mxDO(_indices.AddMany( indices, mxCOUNT_OF(indices) ));

	return ALL_OK;
}

// Creates a UVSphere in Blender terms - the density of vertices is greater around the poles.
ERet CreateSphere(
				  float radius, int sliceCount, int stackCount,
				  TArray<FatVertex> &_vertices, TArray<int> &_indices
				  )
{
	_vertices.RemoveAll();
	_indices.RemoveAll();

	{
		FatVertex& northPole = _vertices.AddNew();
		northPole.position	= V3_Set( 0, radius, 0 );
		northPole.normal	= V3_Set( 0, 1, 0 );
		northPole.tangent	= V3_Set( 1, 0, 0 );
		northPole.texCoord	= V2_Set( 0, 0 );
	}

	float phiStep = mxPI / stackCount;
	float thetaStep = 2.0f * mxPI / sliceCount;

	for( int i = 1; i <= stackCount - 1; i++ )
	{
		float phi = i * phiStep;

		float sinPhi, cosPhi;
		mmSinCos( phi, sinPhi, cosPhi );

		for( int j = 0; j <= sliceCount; j++ )
		{
			float theta = j * thetaStep;

			float sinTheta, cosTheta;			
			mmSinCos( theta, sinTheta, cosTheta );

			FatVertex& vertex = _vertices.AddNew();

			vertex.position = V3_Set(
				(radius * sinPhi * cosTheta),
				(radius * cosPhi),
				(radius * sinPhi * sinTheta)
			);

			vertex.normal = V3_Normalized( vertex.position );

			vertex.tangent = V3_Set(
				-radius * sinPhi * sinTheta,
				0,
				radius * sinPhi * cosTheta
			);
			vertex.tangent = V3_Normalized( vertex.tangent );

			vertex.texCoord = V2_Set(
				theta / (mxPI * 2),
				phi / mxPI
			);
		}
	}

	{
		FatVertex& southPole = _vertices.AddNew();
		southPole.position	= V3_Set( 0, -radius, 0 );
		southPole.normal	= V3_Set( 0, -1, 0 );
		southPole.tangent	= V3_Set( 1, 0, 0 );
		southPole.texCoord	= V2_Set( 0, 1 );
	}

	for( int i = 1; i <= sliceCount; i++ ) {
		_indices.add(0);
		_indices.add(i + 1);
		_indices.add(i);
	}
	int baseIndex = 1;
	int ringVertexCount = sliceCount + 1;
	for( int i = 0; i < stackCount - 2; i++ ) {
		for( int j = 0; j < sliceCount; j++ ) {
			_indices.add(baseIndex + i * ringVertexCount + j);
			_indices.add(baseIndex + i * ringVertexCount + j + 1);
			_indices.add(baseIndex + (i + 1) * ringVertexCount + j);

			_indices.add(baseIndex + (i + 1) * ringVertexCount + j);
			_indices.add(baseIndex + i * ringVertexCount + j + 1);
			_indices.add(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}
	int southPoleIndex = _vertices.num() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for( int i = 0; i < sliceCount; i++ ) {
		_indices.add(southPoleIndex);
		_indices.add(baseIndex + i);
		_indices.add(baseIndex + i + 1);
	}

	return ALL_OK;
}

#if 0
static V3f IcosahedronVertices[] = {
	{-0.525731f, 0, 0.850651f}, {0.525731f, 0, 0.850651f},
	{-0.525731f, 0, -0.850651f}, {0.525731f, 0, -0.850651f},
	{0, 0.850651f, 0.525731f}, {0, 0.850651f, -0.525731f},
	{0, -0.850651f, 0.525731f}, {0, -0.850651f, -0.525731f},
	{0.850651f, 0.525731f, 0}, {-0.850651f, 0.525731f, 0},
	{0.850651f, -0.525731f, 0}, {-0.850651f, -0.525731f, 0}
};

static int IcosahedronIndices[] = {
	1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
	1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
	3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
	10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
};

class Subdivider {
	List<Vertex> _vertices;
	List<int> _indices;
	Dictionary<Tuple<int, int>, int> _newVertices;

	public void Subdivide4(MeshData mesh) {
		_newVertices = new Dictionary<Tuple<int, int>, int>();
		_vertices = mesh.Vertices;
		_indices = new List<int>();
		var numTris = mesh.Indices.Count / 3;

		for (var i = 0; i < numTris; i++) {
			//       i2
			//       *
			//      / \
			//     /   \
			//   a*-----*b
			//   / \   / \
			//  /   \ /   \
			// *-----*-----*
			// i1    c      i3

			var i1 = mesh.Indices[i * 3];
			var i2 = mesh.Indices[i * 3 + 1];
			var i3 = mesh.Indices[i * 3 + 2];

			var a = GetNewVertex(i1, i2);
			var b = GetNewVertex(i2, i3);
			var c = GetNewVertex(i3, i1);

			_indices.AddRange(new[] {
				i1, a, c,
					i2, b, a,
					i3, c, b,
					a, b, c
			});
		}
#if DEBUG
		Console.WriteLine(mesh.Vertices.Count);
#endif
		mesh.Indices = _indices;
	}

	int GetNewVertex(int i1, int i2) {
		var t1 = new Tuple<int, int>(i1, i2);
		var t2 = new Tuple<int, int>(i2, i1);

		if (_newVertices.ContainsKey(t2)) {
			return _newVertices[t2];
		}
		if (_newVertices.ContainsKey(t1)) {
			return _newVertices[t1];
		}
		var newIndex = _vertices.Count;
		_newVertices.add(t1, newIndex);

		_vertices.add(new Vertex() { Position = (_vertices[i1].Position + _vertices[i2].Position) * 0.5f });

		return newIndex;
	}
}

ERet CreateGeodesicSphere(
						  float radius, int numSubdivisions, //[0..8]
						  TArray<FatVertex> &_vertices, TArray<int> &_indices
						  )
{
	tempMesh.Vertices = IcosahedronVertices.Select(p => new Vertex { Position = p }).ToList();
	tempMesh.Indices = IcosahedronIndices;

	var mh = new Subdivider();

	for (var i = 0; i < (int)numSubdivisions; i++) {
		mh.Subdivide4(tempMesh);
	}

	// Project vertices onto sphere and scale.
	for (var i = 0; i < tempMesh.Vertices.Count; i++) {
		// Project onto unit sphere.
		var n = V3f.Normalize(tempMesh.Vertices[i].Position);
		// Project onto sphere.
		var p = radius * n;

		// Derive texture coordinates from spherical coordinates.
		var theta = MathF.AngleFromXY(tempMesh.Vertices[i].Position.X, tempMesh.Vertices[i].Position.Z);
		var phi = MathF.Acos(tempMesh.Vertices[i].Position.Y / radius);
		var texC = new Vector2(theta / (2 * MathF.PI), phi / MathF.PI);

		// Partial derivative of P with respect to theta
		var tangent = new CV3f(
			-radius * MathF.Sin(phi) * MathF.Sin(theta),
			0,
			radius * MathF.Sin(phi) * MathF.Cos(theta)
			);
		tangent.Normalize();

		tempMesh.Vertices[i] = new Vertex(p, n, tangent, texC);
	}

	return ALL_OK;
}

#elif 0

void Subdivide( V3f *&dest, const V3f &v0, const V3f &v1, const V3f &v2, int level )
{
	if (level){
		level--;
		V3f v3 = V3_Normalized(v0 + v1);
		V3f v4 = V3_Normalized(v1 + v2);
		V3f v5 = V3_Normalized(v2 + v0);

		Subdivide(dest, v0, v3, v5, level);
		Subdivide(dest, v3, v4, v5, level);
		Subdivide(dest, v3, v1, v4, level);
		Subdivide(dest, v5, v4, v2, level);
	} else {
		*dest++ = v0;
		*dest++ = v1;
		*dest++ = v2;
	}
}

ERet CreateGeodesicSphere(
						  float radius, int numSubdivisions, //[0..8]
						  TArray<FatVertex> &_vertices, TArray<int> &_indices
						  )
{
	const int nVertices = 8 * 3 * (1 << (2 * numSubdivisions));
	mxDO(_vertices.setNum( nVertices ));
	V3f* vertices = _vertices.raw();

	// Tessellate a octahedron
	V3f px0(-1,  0,  0);
	V3f px1( 1,  0,  0);
	V3f py0( 0, -1,  0);
	V3f py1( 0,  1,  0);
	V3f pz0( 0,  0, -1);
	V3f pz1( 0,  0,  1);

	V3f *dest = vertices;
	Subdivide(dest, py0, px0, pz0, numSubdivisions);
	Subdivide(dest, py0, pz0, px1, numSubdivisions);
	Subdivide(dest, py0, px1, pz1, numSubdivisions);
	Subdivide(dest, py0, pz1, px0, numSubdivisions);
	Subdivide(dest, py1, pz0, px0, numSubdivisions);
	Subdivide(dest, py1, px0, pz1, numSubdivisions);
	Subdivide(dest, py1, pz1, px1, numSubdivisions);
	Subdivide(dest, py1, px1, pz0, numSubdivisions);

	mxASSERT(dest - vertices == nVertices);

	nIndices = nVertices;
	addBatch(0, nVertices);

	return ALL_OK;
}

public static DbgMeshData CreateCylinder(float bottomRadius, float topRadius, float height, int sliceCount, int stackCount) {
    var ret = new DbgMeshData();

    var stackHeight = height / stackCount;
    var radiusStep = (topRadius - bottomRadius) / stackCount;
    var ringCount = stackCount + 1;

    for (int i = 0; i < ringCount; i++) {
        var y = -0.5f * height + i * stackHeight;
        var r = bottomRadius + i * radiusStep;
        var dTheta = 2.0f * MathF.PI / sliceCount;
        for (int j = 0; j <= sliceCount; j++) {

            var c = MathF.Cos(j * dTheta);
            var s = MathF.Sin(j * dTheta);

            var v = new Vector3(r * c, y, r * s);
            var uv = new Vector2((float)j / sliceCount, 1.0f - (float)i / stackCount);
            var t = new Vector3(-s, 0.0f, c);

            var dr = bottomRadius - topRadius;
            var bitangent = new Vector3(dr * c, -height, dr * s);

            var n = Vector3.Normalize(Vector3.Cross(t, bitangent));

            ret.Vertices.add(new Vertex(v, n, t, uv));

        }
    }
    var ringVertexCount = sliceCount + 1;
    for (int i = 0; i < stackCount; i++) {
        for (int j = 0; j < sliceCount; j++) {
            ret.Indices.add(i * ringVertexCount + j);
            ret.Indices.add((i + 1) * ringVertexCount + j);
            ret.Indices.add((i + 1) * ringVertexCount + j + 1);

            ret.Indices.add(i * ringVertexCount + j);
            ret.Indices.add((i + 1) * ringVertexCount + j + 1);
            ret.Indices.add(i * ringVertexCount + j + 1);
        }
    }
    BuildCylinderTopCap(topRadius, height, sliceCount, ref ret);
    BuildCylinderBottomCap(bottomRadius, height, sliceCount, ref ret);
    return ret;
}

private static void BuildCylinderTopCap(float topRadius, float height, int sliceCount, ref DbgMeshData ret) {
    var baseIndex = ret.Vertices.Count;

    var y = 0.5f * height;
    var dTheta = 2.0f * MathF.PI / sliceCount;

    for (int i = 0; i <= sliceCount; i++) {
        var x = topRadius * MathF.Cos(i * dTheta);
        var z = topRadius * MathF.Sin(i * dTheta);

        var u = x / height + 0.5f;
        var v = z / height + 0.5f;
        ret.Vertices.add(new Vertex(x, y, z, 0, 1, 0, 1, 0, 0, u, v));
    }
    ret.Vertices.add(new Vertex(0, y, 0, 0, 1, 0, 1, 0, 0, 0.5f, 0.5f));
    var centerIndex = ret.Vertices.Count - 1;
    for (int i = 0; i < sliceCount; i++) {
        ret.Indices.add(centerIndex);
        ret.Indices.add(baseIndex + i + 1);
        ret.Indices.add(baseIndex + i);
    }
}

private static void BuildCylinderBottomCap(float bottomRadius, float height, int sliceCount, ref DbgMeshData ret) {
    var baseIndex = ret.Vertices.Count;

    var y = -0.5f * height;
    var dTheta = 2.0f * MathF.PI / sliceCount;

    for (int i = 0; i <= sliceCount; i++) {
        var x = bottomRadius * MathF.Cos(i * dTheta);
        var z = bottomRadius * MathF.Sin(i * dTheta);

        var u = x / height + 0.5f;
        var v = z / height + 0.5f;
        ret.Vertices.add(new Vertex(x, y, z, 0, -1, 0, 1, 0, 0, u, v));
    }
    ret.Vertices.add(new Vertex(0, y, 0, 0, -1, 0, 1, 0, 0, 0.5f, 0.5f));
    var centerIndex = ret.Vertices.Count - 1;
    for (int i = 0; i < sliceCount; i++) {
        ret.Indices.add(centerIndex);
        ret.Indices.add(baseIndex + i);
        ret.Indices.add(baseIndex + i + 1);
    }
}

public static DbgMeshData CreateGrid(float width, float depth, int m, int n) {
    var ret = new DbgMeshData();

    var halfWidth = width * 0.5f;
    var halfDepth = depth * 0.5f;

    var dx = width / (n - 1);
    var dz = depth / (m - 1);

    var du = 1.0f / (n - 1);
    var dv = 1.0f / (m - 1);

    for (var i = 0; i < m; i++) {
        var z = halfDepth - i * dz;
        for (var j = 0; j < n; j++) {
            var x = -halfWidth + j * dx;
            ret.Vertices.add(new Vertex(new Vector3(x, 0, z), new Vector3(0, 1, 0), new Vector3(1, 0, 0), new Vector2(j * du, i * dv)));
        }
    }
    for (var i = 0; i < m - 1; i++) {
        for (var j = 0; j < n - 1; j++) {
            ret.Indices.add(i * n + j);
            ret.Indices.add(i * n + j + 1);
            ret.Indices.add((i + 1) * n + j);

            ret.Indices.add((i + 1) * n + j);
            ret.Indices.add(i * n + j + 1);
            ret.Indices.add((i + 1) * n + j + 1);
        }
    }
    return ret;
}

public static DbgMeshData CreateFullScreenQuad() {
    var ret = new DbgMeshData();

    ret.Vertices.add(new Vertex(-1, -1, 0, 0, 0, -1, 1, 0, 0, 0, 1));
    ret.Vertices.add(new Vertex(-1, 1, 0, 0, 0, -1, 1, 0, 0, 0, 0));
    ret.Vertices.add(new Vertex(1, 1, 0, 0, 0, -1, 1, 0, 0, 1, 0));
    ret.Vertices.add(new Vertex(1, -1, 0, 0, 0, -1, 1, 0, 0, 1, 1));

    ret.Indices.AddRange(new[] { 0, 1, 2, 0, 2, 3 });


    return ret;
}

#endif


ERet MeshBuilder::Begin()
{
	vertices.RemoveAll();
	indices.RemoveAll();
	return ALL_OK;
}
int MeshBuilder::addVertex( const DrawVertex& vertex )
{
	int index = vertices.num();
	vertices.add(vertex);
	return index;
}
int MeshBuilder::addTriangle( int v1, int v2, int v3 )
{
	indices.add(v1);
	indices.add(v2);
	indices.add(v3);
	return indices.num() / 3;
}
ERet MeshBuilder::End( int &verts, int &tris )
{
	verts = vertices.num();
	tris = indices.num();
	return ALL_OK;
}


mxSTOLEN("OpenVDB");
float QuantizedUnitVec::sNormalizationWeights[MASK_SLOTS + 1];

////////////////////////////////////////
void
QuantizedUnitVec::StaticInitializeLUT()
{
	U16 xbits, ybits;
	double x, y, z, w;

	for (U16 b = 0; b < 8192; ++b) {

		xbits = U16((b & MASK_XSLOT) >> 7);
		ybits = b & MASK_YSLOT;

		if ((xbits + ybits) > 126) {
			xbits = U16(127 - xbits);
			ybits = U16(127 - ybits);
		}

		x = double(xbits);
		y = double(ybits);
		z = double(126 - xbits - ybits);
		w = 1.0 / ::sqrt(x*x + y*y + z*z);

		sNormalizationWeights[b] = float(w);
	}
}

VX::ADebugView VX::ADebugView::ms_dummy;

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
