#include <algorithm>

#include <jq/jq.h>

#include <MathGeoLib/Geometry/Sphere.h>
#pragma comment( lib, "MathGeoLib.lib" )

#include <Remotery/Remotery.h>
#pragma comment( lib, "Remotery.lib" )

#include "Demo_ForwardPlus.h"
//#include <Core/bitsquid/hash.h>
//#include <Core/bitsquid/temp_allocator.h>
#include <Core/JobSystem.h>
#include <Graphics/private/backend_d3d11.h>
//#include "ViewFrustum.h"

// Binary powering, aka exponentiation by squaring.
int powi( int base, unsigned int exp )
{
	int res = 1;
	while (exp) {
		if (exp & 1)
			res *= base;
		exp >>= 1;
		base *= base;
	}
	return res;
}

DrawVertex MakeDrawVertex( const V3F& xyz, const V3F& N, const V3F& T, const V2F& UV )
{
	DrawVertex result;
	result.xyz = xyz;
	result.uv = V2F_To_Half2( UV );
	result.N = PackNormal( N );
	result.T = PackNormal( T );
	return result;
}

// given two points, take midpoint and project onto this sphere
inline DrawVertex CalcMidpoint(const DrawVertex& a, const DrawVertex& b)
{
	DrawVertex midpoint;
	midpoint.xyz = (a.xyz + b.xyz) * 0.5f;
	return midpoint;
}

void Subdivide(
			   int level,
			   const DrawVertex &v0, const DrawVertex &v1, const DrawVertex &v2,
			   TArray< DrawVertex > &vertices, TArray< int > &indices
			   )
{
	if( level > 0 )
	{
		level--;

		DrawVertex v3 = CalcMidpoint(v0, v1);
		DrawVertex v4 = CalcMidpoint(v1, v2);
		DrawVertex v5 = CalcMidpoint(v2, v0);

		//       v1
		//       *
		//      / \
		//     /   \
		//  v3*-----*v4
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    v5     v2

		Subdivide(level, v0, v3, v5, vertices, indices);
		Subdivide(level, v3, v4, v5, vertices, indices);
		Subdivide(level, v3, v1, v4, vertices, indices);
		Subdivide(level, v5, v4, v2, vertices, indices);
	}
	else
	{
		int vertexIndex = vertices.Num();
		vertices.Add(v0);
		vertices.Add(v1);
		vertices.Add(v2);
		indices.Add(vertexIndex + 0);
		indices.Add(vertexIndex + 1);
		indices.Add(vertexIndex + 2);
	}
}

// Useful references:
// Sphere Generation by Paul Bourke:
// http://paulbourke.net/geometry/circlesphere/
// Quadrilateralized spherical cube (Quad Sphere or QLSC):
// http://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
// Generating spheres from octahedron:
// http://sol.gfxile.net/sphere/
// https://www.binpress.com/tutorial/creating-an-octahedron-sphere/162

inline int GetMidpointIndex(
						  int iA, int iB,
						  TArray< DrawVertex > &vertices, TArray< int > &indices,
						  THashMap<U64, int> & _edges	// maps edge -> mid-point index
							 )
{
	TSort2( iA, iB );
	const uint64_t edgeKey = (uint64_t(iA) << 32) | iB;
	int vertexIndex = _edges.FindRef( edgeKey );
	if( vertexIndex != -1 ) {
		return vertexIndex;
	}

	vertexIndex = vertices.Num();

	DrawVertex & newVertex = vertices.AddNew();
	{
		const DrawVertex& a = vertices[iA];
		const DrawVertex& b = vertices[iB];
		newVertex.xyz = (a.xyz + b.xyz) * 0.5f;
	}

	_edges.Set( edgeKey, vertexIndex );

	return vertexIndex;
}

void SplitTriangleIntoFourRecursive(
									int level,
									TArray< DrawVertex > &vertices, TArray< int > &indices,
									const int iA, const int iB, const int iC,
									THashMap<U64, int> & _edges	// maps edge -> mid-point index
									)
{
	if( level > 0 )
	{
		--level;

		// Create four smaller triangles.

		const int iAB = GetMidpointIndex( iA, iB, vertices, indices, _edges );
		const int iBC = GetMidpointIndex( iB, iC, vertices, indices, _edges );
		const int iAC = GetMidpointIndex( iC, iA, vertices, indices, _edges );

		//        B
		//        *
		//       / \
		//      /   \
		//   ab*-----*bc
		//    / \   / \
		//   /   \ /   \
		//  *-----*-----*
		// A     ac      C

		const int newTriangles[3*4] = {
			iA, iAB, iAC,	//<= lower left
			iB, iBC, iAB,	//<= top
			iC, iAC, iBC,	//<= lower right
			iAB,iBC, iAC	//<= central
		};

		for( int faceIndex = 0; faceIndex < mxCOUNT_OF(newTriangles)/3; faceIndex++ )
		{
			const int iA = newTriangles[ faceIndex*3 + 0 ];
			const int iB = newTriangles[ faceIndex*3 + 1 ];
			const int iC = newTriangles[ faceIndex*3 + 2 ];
			SplitTriangleIntoFourRecursive( level, vertices, indices, iA, iB, iC, _edges );
		}
	}
	else
	{
		indices.Add(iA);
		indices.Add(iB);
		indices.Add(iC);
	}
}

/// Starts with an Octahedron (8 faces, 6 vertices, 12 edges).
///NOTE: it creates (4 ^ generation) * 8 triangles.
void Tesselate_Octahedron(
						  //int subdivisionIterations,
				  int generations, // 0..8
				 float sizeX, float sizeY, float sizeZ,
				 TArray< DrawVertex > &vertices, TArray< int > &indices,
				 bool ccwIsFrontFacing = true
				 )
{
	// Explanation from http://math.stackexchange.com/questions/1304971/how-many-unique-vertices-in-octahedron-based-sphere-approximation
	// We take an octahedron and bisect the edges of its facets to form 4 triangles from each triangle:
	//
	//          /\                  /\                  /\
	//         /  \                /  \                /__\
	//        /    \              /    \              /\  /\
	//       /      \            /______\            /__\/__\
	//      /        \          /\      /\          /\  /\  /\
	//     /          \        /  \    /  \        /__\/__\/__\
	//    /            \      /    \  /    \      /\  /\  /\  /\
	//   /______________\    /______\/______\    /__\/__\/__\/__\
	//    0th generation      1st generation      2nd generation
	//
	// This happens for every face so 8 times for the first generation.
	//
	// Between each generation new vertices are pushed to the surface of the sphere.
	// 
	// The number of facets will be (4^generations)*8
	// Some facets will share vertices: in the 0th generation there are 6 unique vertices (it's an octahedron).

	// The number of vertices in generation N is (4 ^ (N+1) + 2).

	const int totalVertexCount = (1U << (2 * (generations+1))) + 2;	// powi(4, generations+1) + 2

	// Ok, start with a 1x1x1 octahedron ('diamond') as the first sphere approximation.

	enum { INITIAL_VERTICES = 6, INITIAL_TRIANGLES = 8 };

	// initial 6 vertices of octahedron
	const V3F OctahedronVertices[INITIAL_VERTICES] = {
		{-1,  0,  0},	// left
		{+1,  0,  0},	// right
		{ 0, -1,  0},	// front
		{ 0, +1,  0},	// back
		{ 0,  0, -1},	// bottom
		{ 0,  0, +1},	// top
	};

	static const int OctahedronIndicesCCW[INITIAL_TRIANGLES*3] =
	{
		3,1,5,	// yp,xp,zp,
		1,3,4,	// xp,yp,zn,
		2,5,1,	// yn,zp,xp,
		2,1,4,	// yn,xp,zn,
		5,0,3,	// zp,xn,yp,
		3,0,4,	// yp,xn,zn,
		2,0,5,	// yn,xn,zp,
		0,2,4,	// xn,yn,zn,
	};
	static const int OctahedronIndicesCW[INITIAL_TRIANGLES*3] =
	{
		3,5,1,	// yp,zp,xp,
		1,4,3,	// xp,zn,yp,
		2,1,5,	// yn,xp,zp,
		2,4,1,	// yn,zn,xp,
		5,3,0,	// zp,yp,xn,
		3,4,0,	// yp,zn,xn,
		2,5,0,	// yn,zp,xn,
		0,4,2,	// xn,zn,yn,
	};
	const int* OctahedronIndices = ccwIsFrontFacing ? OctahedronIndicesCCW : OctahedronIndicesCW;

	{
		vertices.Reserve(totalVertexCount);
		vertices.SetNum(INITIAL_VERTICES);
		for( int i = 0; i < INITIAL_VERTICES; i++ )
		{
			vertices[ i ].xyz = OctahedronVertices[ i ];
		}
	}

	THashMap< U64, int >	edgeMap;

	for( int faceIndex = 0; faceIndex < INITIAL_TRIANGLES; faceIndex++ )
	{
		const int iA = OctahedronIndices[ faceIndex*3 + 0 ];
		const int iB = OctahedronIndices[ faceIndex*3 + 1 ];
		const int iC = OctahedronIndices[ faceIndex*3 + 2 ];
		SplitTriangleIntoFourRecursive( generations, vertices, indices, iA, iB, iC, edgeMap );
	}

	DBGOUT("Created %u vertices, %u indices (%d generations)\n", vertices.Num(), indices.Num(), generations);

	// Calculate vertex positions, normals and texture coordinates.
	// Project vertices onto sphere.
	for( int i = 0; i < vertices.Num(); i++ )
	{
		DrawVertex & vertex = vertices[i];
		// Project onto unit sphere.
		const V3F N = V3_Normalized( vertex.xyz );
		vertex.N = PackNormal( N );
		// Project onto sphere.
		vertex.xyz.x = N.x * sizeX;
		vertex.xyz.y = N.y * sizeY;
		vertex.xyz.z = N.z * sizeZ;

		// Derive texture coordinates from spherical coordinates.

		// We can convert vertex positions into texture coordinates by using inverse trigonometric functions. 

		//NOTE: phi and theta can denote either vertical or horizontal angle, depending on convention
		// horizontal (azimuthal) angle [-PI..+PI]
		const float phi = atan2( N.y, N.x );	// Principal arc tangent of y/x, in the interval [-pi,+pi] radians.
		// vertical (polar, zenith, inclination) angle [0..PI]
		const float theta = acos( N.z );	// Principal arc cosine of x, in the interval [0,pi] radians.

		// Compute texture coordinates.
		// see "Finding UV on a sphere": https://en.wikipedia.org/wiki/UV_mapping

		//const float u = phi * mxINV_TWOPI;	// [-PI..+PI] -> [-0.5..+0.5]
		const float u = phi * mxINV_TWOPI + 0.5f;	// [-PI..+PI] -> [0..1]
		//const float v = theta * mxINV_PI;			// [0..PI] -> [0..1]
		const float v = 0.5 - asin(N.z) * mxINV_PI;			// [0..PI] -> [0..1]

		//const float v = (N.z + 1) / (2 * 1);


		//mxASSERT(u >= 0.0f && u <= 1.0f);
		//mxASSERT(v >= 0.0f && v <= 1.0f);
//float2(
//	   atan2(TRIANGLE(temp[i]).a.y, TRIANGLE(temp[i]).a.x) / (2.f * 3.141592654f) + 0.5f,
//	   (TRIANGLE(temp[i]).a.z + r) / (2.f * r));
//outUV[3*j] = float2(atan2(TRIANGLE(temp[i]).a.y, TRIANGLE(temp[i]).a.x) / (2.f * 3.141592654f) + 0.5f, (TRIANGLE(temp[i]).a.z + r) / (2.f * r));
//outUV[3*j+1] = float2(atan2(TRIANGLE(temp[i]).b.y, TRIANGLE(temp[i]).b.x) / (2.f * 3.141592654f) + 0.5f, (TRIANGLE(temp[i]).b.z + r) / (2.f * r));
//outUV[3*j+2] = float2(atan2(TRIANGLE(temp[i]).c.y, TRIANGLE(temp[i]).c.x) / (2.f * 3.141592654f) + 0.5f, (TRIANGLE(temp[i]).c.z + r) / (2.f * r));

		vertex.uv = V2F_To_Half2(V2F_Set( u, v ));

		// Compute tangents.

		float sinPhi, cosPhi;
		Float_SinCos( phi, sinPhi, cosPhi );

		float sinTheta, cosTheta;
		Float_SinCos( theta, sinTheta, cosTheta );

		//V3F tangent = V3_Set(sin(theta)*cos(phi), cos(theta), sin(theta)*sin(phi));
		//V3F binormal = V3_Set( cross(input.Normal,tangent) );

		//vertex.T = PackNormal(V3_Normalized(tangent));

		// Partial derivatives of position with respect to texture coordinates
		//vertex.T = PackNormal(V3_Normalized(V3_Set(
		//	-sizeX * sinPhi * sinTheta,
		//	+sizeX * sinPhi * cosPhi,
		//	0
		//)));
	}

	//ASSERT(dest - vertices == nVertices);

	//addStream(TYPE_VERTEX, 3, nVertices, (float *) vertices, NULL, false);
	//nIndices = nVertices;
	//addBatch(0, nVertices);
}

#if 0
///
/// The icosahedron (more properly called the regular icosahedron) is the regular polyhedron and Platonic solid
/// having 12 polyhedron vertices, 30 polyhedron edges, and 20 equivalent equilateral triangle faces
///
// define the icosahedron's 12 vertices:
static V3F IcosahedronVertices[12] = {
	{-0.525731f, 0, 0.850651f}, {0.525731f, 0, 0.850651f},
	{-0.525731f, 0, -0.850651f}, {0.525731f, 0, -0.850651f},
	{0, 0.850651f, 0.525731f}, {0, 0.850651f, -0.525731f},
	{0, -0.850651f, 0.525731f}, {0, -0.850651f, -0.525731f},
	{0.850651f, 0.525731f, 0}, {-0.850651f, 0.525731f, 0},
	{0.850651f, -0.525731f, 0}, {-0.850651f, -0.525731f, 0}
};
// the icosahedron's 20 triangular faces:
static int IcosahedronIndices[20*3] = {
	1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
	1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
	3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
	10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
};

// given two points, take midpoint and project onto this sphere
inline V3F CalcMidpoint(const V3F& a, const V3F& b)
{
	V3F midpoint;
	midpoint = (a + b) * 0.5f;
	return midpoint;
}

// returns index of point in the middle of A and B
inline int GetMidpointIndex(
							 const int iA, const int iB,
							 THashMap<U64, int> _vertexPairs,
							 TArray< DrawVertex > &vertices, TArray< int > &indices							 
							 )
{
	mxASSERT(iA != iB);

	// first check if we have it already
	int i1 = iA;
	int i2 = iB;
	TSort2( i1, i2 );

	const uint64_t key = (uint64_t(i1) << 32) | i2;

	//THashMap<U64, int> _vertexPairs
	const int existingVertexIndex = _vertexPairs.FindRef( key, -1 );
	if( existingVertexIndex != -1 ) {
		return existingVertexIndex;
	}

	// not in cache, calculate it
	const int newVertexIndex = vertices.Num();

	DrawVertex & newVertex = vertices.AddNew();
	{
		const DrawVertex& a = vertices[iA];
		const DrawVertex& b = vertices[iB];
		newVertex.xyz = (a.xyz + b.xyz) * 0.5f;
	}

	// store it, return index
	indices.Add( newVertexIndex );

	_vertexPairs.Set( key, newVertexIndex );

	return newVertexIndex;
}

void SplitEachTriangleIntoFour(
				THashMap<U64, int> & _vertexPairs,
			   TArray< DrawVertex > &vertices, TArray< int > &indices
			   )
{
	mxASSERT(vertices.Num() > 0);
	mxASSERT(indices.Num() > 0 && (indices.Num()%3 == 0));
	for( int triangleIndex = 0; triangleIndex < indices.Num() / 3; triangleIndex++ )
	{
		const int iA = indices[triangleIndex * 3 + 0];
		const int iB = indices[triangleIndex * 3 + 1];
		const int iC = indices[triangleIndex * 3 + 2];

		const int iAB = GetMidpointIndex( iA, iB, _vertexPairs, vertices, indices );
		const int iBC = GetMidpointIndex( iB, iC, _vertexPairs, vertices, indices );
		const int iAC = GetMidpointIndex( iC, iA, _vertexPairs, vertices, indices );

		//        B
		//        *
		//       / \
		//      /   \
		//   ab*-----*bc
		//    / \   / \
		//   /   \ /   \
		//  *-----*-----*
		// A     ac      C

		const int newTriangles[3*4] = {
			iA, iAB, iAC,
			iB, iBC, iAB,
			iC, iAC, iBC,
			iAB,iBC, iAC
		};

		indices.AddMany( newTriangles, mxCOUNT_OF(newTriangles) );
	}
}

// An icosahedron is made up of equilateral triangles,
// so if you subdivide the triangles into equilateral triangles,
// the geometry is evenly distributed across the mesh.
void CreateGeoSphere(
					 int subdivisionIterations, // 0..8
					 float sizeX, float sizeY, float sizeZ,
					 TArray< DrawVertex > &vertices, TArray< int > &indices
					 )
{
	// Start with a unit icosahedron.

	vertices.SetNum(mxCOUNT_OF(IcosahedronVertices));
	indices.SetNum(mxCOUNT_OF(IcosahedronIndices));

	for( int ii = 0; ii < mxCOUNT_OF(IcosahedronIndices); ii++ )
	{
		const int vertexIndex = IcosahedronIndices[ii];
		vertices[ vertexIndex ].xyz = IcosahedronVertices[ vertexIndex ];
	}

	indices.CopyFromArray( IcosahedronIndices );

	THashMap< U64, int >	vertexPairs;

	for( int i = 0; i < subdivisionIterations; i++ )
	{
		SplitEachTriangleIntoFour( vertexPairs, vertices, indices );
	}

	// Calculate vertex positions, normals and texture coordinates.
	// Project vertices onto sphere.
	for( int i = 0; i < vertices.Num(); i++ )
	{
		DrawVertex & vertex = vertices[i];
		// Project onto unit sphere.
		const V3F N = V3_Normalized( vertex.xyz );
		vertex.N = PackNormal( N );
		// Project onto sphere.
		vertex.xyz.x = N.x * sizeX;
		vertex.xyz.y = N.y * sizeY;
		vertex.xyz.z = N.z * sizeZ;

		// Derive texture coordinates from spherical coordinates.

		// We can convert vertex positions into texture coordinates by using inverse trigonometric functions. 

		//NOTE: phi and theta can denote either vertical or horizontal angle, depending on convention
		// horizontal (azimuthal) angle [-PI..+PI]
		const float phi = atan2( N.y, N.x );	// Principal arc tangent of y/x, in the interval [-pi,+pi] radians.
		// vertical (polar, zenith, inclination) angle [0..PI]
		const float theta = acos( N.z );	// Principal arc cosine of x, in the interval [0,pi] radians.

		// Compute texture coordinates.

		const float u = phi * mxINV_TWOPI + 0.5f;	// [-PI..+PI] -> [0..1]
		const float v = theta * mxINV_PI;			// [0..PI] -> [0..1]

		mxASSERT(u >= 0.0f && u <= 1.0f);
		mxASSERT(v >= 0.0f && v <= 1.0f);

		vertex.uv = V2F_To_Half2(V2F_Set( u, v ));

		// Compute tangents.

		float sinPhi, cosPhi;
		Float_SinCos( phi, sinPhi, cosPhi );

		float sinTheta, cosTheta;
		Float_SinCos( theta, sinTheta, cosTheta );

		//V3F tangent = V3_Set(sin(theta)*cos(phi), cos(theta), sin(theta)*sin(phi));
		//V3F binormal = V3_Set( cross(input.Normal,tangent) );

		//vertex.T = PackNormal(V3_Normalized(tangent));

		// Partial derivatives of position with respect to texture coordinates
		//vertex.T = PackNormal(V3_Normalized(V3_Set(
		//	-sizeX * sinPhi * sinTheta,
		//	+sizeX * sinPhi * cosPhi,
		//	0
		//)));
	}

	//ASSERT(dest - vertices == nVertices);

	//addStream(TYPE_VERTEX, 3, nVertices, (float *) vertices, NULL, false);
	//nIndices = nVertices;
	//addBatch(0, nVertices);
}

#endif

// Quadrilateralized spherical cube (Quad Sphere or QLSC).
// A nice property of the projection is that is equal-area,
// so that a recursive devision in the cube face space
// should result in 4 equal-area spherical quads on the earth.
// https://en.wikipedia.org/wiki/Quadrilateralized_spherical_cube
// http://mathproofs.blogspot.com.au/2005/07/mapping-cube-to-sphere.html
// http://acko.net/blog/making-worlds-1-of-spheres-and-cubes/
// http://www.sai.msu.su/~megera/wiki/SphereCube
void CreateQuadSphere(
				 float length, float height, float depth,
				 TArray< DrawVertex > &vertices, TArray< UINT16 > &indices
				 )
{

}

/// Creates a UVSphere in Blender terms - the density of vertices is greater around the poles.
/// NOTE: Z is up (X - right, Y - forward).
/// see http://paulbourke.net/geometry/circlesphere/
/// http://vterrain.org/Textures/spherical.html
//
ERet CreateUVSphere(
				  float radius, int sliceCount, int stackCount,
				  TArray< DrawVertex > &_vertices, TArray<int> &_indices
				  )
{
	_vertices.Empty();
	_indices.Empty();

	{
		DrawVertex& northPole = _vertices.AddNew();
		northPole.xyz	= V3_Set( 0, 0, radius );
		northPole.N		= PackNormal(V3_Set( 0, 0, 1 ));
		northPole.T		= PackNormal(V3_Set( 1, 0, 0 ));
		northPole.uv	= V2F_To_Half2(V2F_Set( 0, 0 ));
	}

	float phiStep = mxPI / stackCount;	// vertical angle delta
	float thetaStep = 2.0f * mxPI / sliceCount;	// horizontal angle delta

	for( int i = 1; i <= stackCount - 1; i++ )
	{
		float phi = i * phiStep;	// latitude

		float sinPhi, cosPhi;
		Float_SinCos( phi, sinPhi, cosPhi );

		for( int j = 0; j <= sliceCount; j++ )
		{
			float theta = j * thetaStep;	// longitude

			float sinTheta, cosTheta;			
			Float_SinCos( theta, sinTheta, cosTheta );

			DrawVertex & vertex = _vertices.AddNew();

			vertex.xyz = V3_Set(
				(radius * sinPhi * cosTheta),
				(radius * sinPhi * sinTheta),
				(radius * cosPhi)				
			);

			vertex.N = PackNormal(V3_Normalized( vertex.xyz ));

			const V3F tangent = V3_Set(
				-radius * sinPhi * sinTheta,
				radius * sinPhi * cosTheta,
				0				
			);
			vertex.T = PackNormal(V3_Normalized( tangent ));

			vertex.uv = V2F_To_Half2(V2F_Set(
				theta / (mxPI * 2),
				phi / mxPI
			));
		}
	}

	{
		DrawVertex& southPole = _vertices.AddNew();
		southPole.xyz	= V3_Set( 0, 0, -radius );
		southPole.N	= PackNormal(V3_Set( 0, 0, -1 ));
		southPole.T	= PackNormal(V3_Set( 1, 0, 0 ));
		southPole.uv	= V2F_To_Half2(V2F_Set( 0, 1 ));
	}

	// cap
	for( int i = 1; i <= sliceCount; i++ ) {
		_indices.Add(0);		
		_indices.Add(i);
		_indices.Add(i + 1);
	}
	// middle
	int baseIndex = 1;
	int ringVertexCount = sliceCount + 1;
	for( int i = 0; i < stackCount - 2; i++ ) {
		for( int j = 0; j < sliceCount; j++ ) {
			_indices.Add(baseIndex + i * ringVertexCount + j);
			_indices.Add(baseIndex + (i + 1) * ringVertexCount + j);
			_indices.Add(baseIndex + i * ringVertexCount + j + 1);

			_indices.Add(baseIndex + (i + 1) * ringVertexCount + j);
			_indices.Add(baseIndex + (i + 1) * ringVertexCount + j + 1);
			_indices.Add(baseIndex + i * ringVertexCount + j + 1);
		}
	}
	// bottom
	int southPoleIndex = _vertices.Num() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for( int i = 0; i < sliceCount; i++ ) {
		_indices.Add(southPoleIndex);
		_indices.Add(baseIndex + i + 1);
		_indices.Add(baseIndex + i);
	}

	return ALL_OK;
}

void MakeBoxMesh(
				 float length, float height, float depth,
				 TArray< DrawVertex > &vertices, TArray< int > &indices
				 )
{
	enum { NUM_VERTICES = 24 };
	enum { NUM_INDICES = 36 };

	// Create vertex buffer.

	vertices.SetNum( NUM_VERTICES );

	const float HL = 0.5f * length;
	const float HH = 0.5f * height;
	const float HD = 0.5f * depth;

	// Fill in the front face vertex data.
	vertices[0]  = MakeDrawVertex( V3_Set( -HL, -HH, -HD ),	V3_Set( 0.0f, 0.0f, -1.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[1]  = MakeDrawVertex( V3_Set( -HL,  HH, -HD ),	V3_Set( 0.0f, 0.0f, -1.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[2]  = MakeDrawVertex( V3_Set(  HL,  HH, -HD ),	V3_Set( 0.0f, 0.0f, -1.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[3]  = MakeDrawVertex( V3_Set(  HL, -HH, -HD ),	V3_Set( 0.0f, 0.0f, -1.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the back face vertex data.
	vertices[4]  = MakeDrawVertex( V3_Set( -HL, -HH, HD ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 1.0f )	);
	vertices[5]  = MakeDrawVertex( V3_Set(  HL, -HH, HD ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[6]  = MakeDrawVertex( V3_Set(  HL,  HH, HD ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[7]  = MakeDrawVertex( V3_Set( -HL,  HH, HD ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 0.0f )	);

	// Fill in the top face vertex data.
	vertices[8]  = MakeDrawVertex( V3_Set( -HL, HH, -HD ),	V3_Set( 0.0f, 1.0f, 0.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[9]  = MakeDrawVertex( V3_Set( -HL, HH,  HD ),	V3_Set( 0.0f, 1.0f, 0.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[10] = MakeDrawVertex( V3_Set(  HL, HH,  HD ),	V3_Set( 0.0f, 1.0f, 0.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[11] = MakeDrawVertex( V3_Set(  HL, HH, -HD ),	V3_Set( 0.0f, 1.0f, 0.0f ), 	V3_Set( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the bottom face vertex data.
	vertices[12] = MakeDrawVertex( V3_Set( -HL, -HH, -HD ),	V3_Set( 0.0f, -1.0f, 0.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 1.0f )	);
	vertices[13] = MakeDrawVertex( V3_Set(  HL, -HH, -HD ),	V3_Set( 0.0f, -1.0f, 0.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[14] = MakeDrawVertex( V3_Set(  HL, -HH,  HD ),	V3_Set( 0.0f, -1.0f, 0.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[15] = MakeDrawVertex( V3_Set( -HL, -HH,  HD ),	V3_Set( 0.0f, -1.0f, 0.0f ), 	V3_Set( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 0.0f )	);

	// Fill in the left face vertex data.
	vertices[16] = MakeDrawVertex( V3_Set( -HL, -HH,  HD ),	V3_Set( -1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, -1.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[17] = MakeDrawVertex( V3_Set( -HL,  HH,  HD ),	V3_Set( -1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, -1.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[18] = MakeDrawVertex( V3_Set( -HL,  HH, -HD ),	V3_Set( -1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, -1.0f ),	V2F_Set( 1.0f, 0.0f )	);
	vertices[19] = MakeDrawVertex( V3_Set( -HL, -HH, -HD ),	V3_Set( -1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, -1.0f ),	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the right face vertex data.
	vertices[20] = MakeDrawVertex( V3_Set(  HL, -HH, -HD ),	V3_Set( 1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[21] = MakeDrawVertex( V3_Set(  HL,  HH, -HD ),	V3_Set( 1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[22] = MakeDrawVertex( V3_Set(  HL,  HH,  HD ),	V3_Set( 1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[23] = MakeDrawVertex( V3_Set(  HL, -HH,  HD ),	V3_Set( 1.0f, 0.0f, 0.0f ), 	V3_Set( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	//// Scale the box.
	//{
	//	M44F4 scaleMatrix = Matrix_Scaling( length, height, depth );
	//	M44F4 scaleMatrixIT = Matrix_Transpose( Matrix_Inverse( scaleMatrix ) );

	//	for( UINT i = 0; i < NUM_VERTICES; ++i )
	//	{
	//	vertices[i].xyz = Matrix_TransformPoint( scaleMatrix, vertices[i].xyz );
	//		scaleMatrixIT.TransformNormal( vertices[i].Normal );
	//		scaleMatrixIT.TransformNormal( vertices[i].Tangent );
	//	}
	//}

	// Create the index buffer.

	indices.SetNum( NUM_INDICES );

	// Fill in the front face index data
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 0; indices[4] = 2; indices[5] = 3;

	// Fill in the back face index data
	indices[6] = 4; indices[7]  = 5; indices[8]  = 6;
	indices[9] = 4; indices[10] = 6; indices[11] = 7;

	// Fill in the top face index data
	indices[12] = 8; indices[13] =  9; indices[14] = 10;
	indices[15] = 8; indices[16] = 10; indices[17] = 11;

	// Fill in the bottom face index data
	indices[18] = 12; indices[19] = 13; indices[20] = 14;
	indices[21] = 12; indices[22] = 14; indices[23] = 15;

	// Fill in the left face index data
	indices[24] = 16; indices[25] = 17; indices[26] = 18;
	indices[27] = 16; indices[28] = 18; indices[29] = 19;

	// Fill in the right face index data
	indices[30] = 20; indices[31] = 21; indices[32] = 22;
	indices[33] = 20; indices[34] = 22; indices[35] = 23;
}





ERet MyApp::Initialize( const EngineSettings& _settings )
{
	mxDO(DemoApp::Initialize(_settings));

	Clump* deferredRendererResources;
	mxDO(Resources::Load(deferredRendererResources, MakeAssetID("deferred")));

	Clump* forwardPlusRendererResources;
	mxDO(Resources::Load(forwardPlusRendererResources, MakeAssetID("forward+")));

	mxDO(m_deferredRenderer.Initialize());
//	mxDO(m_forwardPlusRenderer.Initialize(forwardPlusRendererResources));
	m_currentRenderer = &m_deferredRenderer;
	//DemoUtil::DBG_PrintClump(*m_rendererData);

	mxDO(m_prim_batcher.Initialize(&m_batch_renderer));

	this->m_gizmo_renderer.BuildGeometry();

	mxDO(CreateScene());

	// Main game mode
//	stateMgr.PushState( &mainMode );
//	WindowsDriver::SetRelativeMouseMode(true);
//	stateMgr.PushState( &m_menuState );

	FileReader	fontFile("R:/testbed/Assets/fonts/bigfont.font");
	m_debugFont.Load(fontFile);
	//m_debugFont.m_spacing = -10;

	m_fontRenderer.Initialize();


	return ALL_OK;
}

void MyApp::Shutdown()
{
	SON::SaveClumpToFile(*m_scene_data, "R:/scene.son");

	m_scene_data->Clear();
	delete m_scene_data;
	m_scene_data = NULL;

	m_prim_batcher.Shutdown();
	m_currentRenderer = NULL;
	m_deferredRenderer.Shutdown();
//	m_forwardPlusRenderer.Shutdown();

	DemoApp::Shutdown();

	//dev_save_state(GameActions::dev_save_state,IS_Pressed,0);
}

ERet MyApp::CreateScene()
{
	mxPROFILE_SCOPE("CreateScene()");

	//ByteBuffer cubemapData;
	//mxDO(Util_LoadFileToBlob(
	//	"D:/sdk/dx_sdk/Samples/Media/Light Probes/uffizi_cross.dds",
	//	cubemapData
	//));
	//m_cubemapTexture = llgl::CreateTexture(cubemapData.Raw(), cubemapData.RawSize());



	m_scene_data = new Clump();

	{
		{
			RrGlobalLight* globalLight;
			mxDO(m_scene_data->New(globalLight));
			globalLight->m_direction = V3_Set(0,0,-1);
			globalLight->m_color = V3_Set(0.2,0.4,0.3);
		}

		//RrLocalLight* lights;
		/*mxDO*/(m_scene_data->CreateObjectList<RrLocalLight>(1024));

		{
			RrLocalLight* localLight;
			mxDO(m_scene_data->New(localLight));
			localLight->position = V3_Set(0,0,100);
			localLight->radius = 200;
			localLight->color = V3_Set(1,1,1);
			m_localLight = localLight;
		}

		for( int x = -1000; x < 1000; x+=100 )
		{
			for( int y = -1000; y < 1000; y+=100 )
			{
				RrLocalLight* localLight;
				mxDO(m_scene_data->New(localLight));
				localLight->position = V3_Set(x,y,10);
				localLight->radius = 50;
				localLight->color = V3_Set(1,1,1);
			}
		}
	}


	{
		const float S = 1000.0f;

		DrawVertex planeVertices[4] =
		{
			/* south-west */{ V3_Set( -S, -S, 0.0f ),  V2F_To_Half2(V2F_Set(    0, 1.0f )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
			/* north-west */{ V3_Set( -S,  S, 0.0f ),  V2F_To_Half2(V2F_Set(    0,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
			/* north-east */{ V3_Set(  S,  S, 0.0f ),  V2F_To_Half2(V2F_Set( 1.0f,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
			/* south-east */{ V3_Set(  S, -S, 0.0f ),  V2F_To_Half2(V2F_Set( 1.0f, 1.0f )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
		};

		const UINT16 planeIndices[6] = {
			0, 2, 1, 0, 3, 2,
		};

		RrMesh* mesh = NULL;
		mxDO(RrMesh::Create(
			mesh,
			m_scene_data,
			planeVertices, mxCOUNT_OF(planeVertices), sizeof(planeVertices),
			VTX_Draw,
			planeIndices, sizeof(planeIndices),
			Topology::TriangleList
		));

		RrModel* model = RrModel::Create( mesh, m_scene_data );

		RrMaterial*	defaultMaterial;
		mxDO(Resources::Load( defaultMaterial, MakeAssetID("material_plastic"), m_scene_data ));
//			defaultMaterial->m_uniforms[0] = V4F_Replicate(1);
		model->m_batches.Add(defaultMaterial);

		m_mesh = mesh;
	}

	{
		TSmallArray< DrawVertex, 64 >	vertices;
		TSmallArray< int, 64 >	indices;

		{
			MakeBoxMesh(100,100,100,vertices,indices);

			mxDO(RrMesh::Create(
				m_cubeMesh.Ptr,
				m_scene_data,
				vertices.Raw(), vertices.Num(), vertices.RawSize(),
				VTX_Draw,
				indices.Raw(), indices.RawSize(),
				Topology::TriangleList
				));

			//RrModel* model = RrModel::Create( m_cubeMesh, m_scene_data );

			//RrMaterial*	defaultMaterial;
			//mxDO(Resources::Load( defaultMaterial, MakeAssetID("material_plastic"), m_scene_data ));
			//model->m_batches.Add(defaultMaterial);
		}

		{
			vertices.Empty();
			indices.Empty();

#if 1
			Tesselate_Octahedron(4, 100,100,100,vertices,indices, false);
			//CreateGeoSphere(0, 100,100,100,vertices,indices);
			//CreateUVSphere(100,16,16,vertices,indices);
#else
			math::Sphere	sphere(vec::zero, 100);
			int numVertices = 24*powi(4,3);
			vertices.SetNum();
			indices.Empty();

			sphere.Triangulate();
#endif

			m_vertices = vertices;
			m_indices = indices;


			RrMesh* mesh = NULL;
			mxDO(RrMesh::Create(
				mesh,
				m_scene_data,
				vertices.Raw(), vertices.Num(), vertices.RawSize(),
				VTX_Draw,
				indices.Raw(), indices.RawSize(),
				Topology::TriangleList, USE_32_BIT_INDICES
				));

			RrModel* model = RrModel::Create( mesh, m_scene_data );

			RrMaterial*	defaultMaterial;
			mxDO(Resources::Load( defaultMaterial, MakeAssetID("material_plastic"), m_scene_data ));

mxDO(Resources::Load( defaultMaterial->m_textures[0].texture, MakeAssetID("uv-test"), m_scene_data ));
			

			model->m_batches.Add(defaultMaterial);
		}
	}


	//{
	//	Texture2DDescription	desc;
	//	desc.format = DataFormat::RGBA8;
	//	desc.width = 1024;
	//	desc.height = 768;
	//	desc.numMips = 1;
	//	desc.randomWrites = true;
	//	m_CS_output = llgl::CreateTexture(desc);
	//}
	//{
	//	BufferDescription	desc(_InitDefault);
	//	desc.type = Buffer_Data;
	//	desc.size = sizeof(V4F)*1024*768;
	//	desc.stride = sizeof(V4F);
	//	desc.sample = true;
	//	desc.randomWrites = true;
	//	m_CS_output = llgl::CreateBuffer(desc);
	//}

	{
		BufferDescription	desc(_InitDefault);

		desc.name.SetReference("number_of_debug_lines");
		desc.type = Buffer_Uniform;
		desc.size = 16;
		m_UBO_number_of_structures = llgl::CreateBuffer(desc);
	}

	const UINT32 currentTimeMSec = mxGetTimeInMilliseconds();
//		stats.Print(currentTimeMSec - startTimeMSec);

	return ALL_OK;
}

ERet MyApp::BeginRender(const double deltaSeconds)
{
	mxPROFILE_SCOPE("Render()");

	const InputState& inputState = WindowsDriver::GetState();

	const M44F4 viewMatrix = camera.BuildViewMatrix();

	const int screenWidth = inputState.window.width;
	const int screenHeight = inputState.window.height;

	const float aspect_ratio = (float)screenWidth / (float)screenHeight;
	const float FoV_Y_degrees = DEG2RAD(60);
	const float near_clip	= 1.0f;
	const float far_clip	= 1000.0f;

	const M44F4 projectionMatrix = Matrix_Perspective(FoV_Y_degrees, aspect_ratio, near_clip, far_clip);
	const M44F4 viewProjectionMatrix = Matrix_Multiply(viewMatrix, projectionMatrix);
	const M44F4 worldMatrix = Matrix_Identity();
	const M44F4 worldViewProjectionMatrix = Matrix_Multiply(worldMatrix, viewProjectionMatrix);


	SceneView sceneView;
	sceneView.viewMatrix = viewMatrix;
	sceneView.projectionMatrix = projectionMatrix;
	sceneView.viewProjectionMatrix = viewProjectionMatrix;
	sceneView.worldSpaceCameraPos = camera.m_eyePosition;
	sceneView.viewportWidth = screenWidth;
	sceneView.viewportHeight = screenHeight;
	sceneView.nearClip = near_clip;
	sceneView.farClip = far_clip;


	//m_deferredRenderer
	//m_forwardPlusRenderer
	//	.RenderScene(sceneView, *m_scene_data, &m_sceneStats);
	{
		rmt_ScopedCPUSample(RenderScene);
		mxDO(m_currentRenderer->RenderScene( sceneView, *m_scene_data, &m_sceneStats ));
	}


	{
		TPtr< TbDepthTarget >	mainDepthStencil;
		mxDO(GetByName(ClumpList::g_loadedClumps, "MainDepthStencil", mainDepthStencil.Ptr));

		llgl::ViewState	viewState;
		{
			viewState.Reset();
			viewState.colorTargets[0].SetDefault();
			viewState.depthTarget = mainDepthStencil->handle;
			viewState.targetCount = 1;
			//viewState.flags = llgl::ClearAll;
		}
		llgl::SetView(llgl::GetMainContext(), viewState);
	}


	{
		mxGPU_SCOPE(Wireframe);

		TbShader* shader;
		mxDO(Resources::Load(shader,MakeAssetID("solid_wireframe"),m_scene_data));
		{
			const Demos::SolidWireSettings settings;// = g_settings.solidWire;
			mxDO(FxSlow_SetUniform(shader,"LineWidth",&settings.LineWidth));
			mxDO(FxSlow_SetUniform(shader,"FadeDistance",&settings.FadeDistance));
			mxDO(FxSlow_SetUniform(shader,"PatternPeriod",&settings.PatternPeriod));
			mxDO(FxSlow_SetUniform(shader,"WireColor",&settings.WireColor));
			mxDO(FxSlow_SetUniform(shader,"PatternColor",&settings.PatternColor));
		}
		mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));

		TPtr< FxStateBlock >	stateSolidWireframe;
		mxDO(GetByName(ClumpList::g_loadedClumps, "SolidWireframe", stateSolidWireframe.Ptr));
		FxSetRenderState(llgl::GetMainContext(), *stateSolidWireframe);

		Rendering::DBG_Draw_Models_Wireframe(sceneView, *m_scene_data, *shader);
	}


#if 1
	{
		mxGPU_SCOPE(Skybox);

		Rendering::RendererGlobals & globals = Rendering::tr;

		TPtr< FxStateBlock >	stateLessEqual;
		mxDO(GetByName(ClumpList::g_loadedClumps, "SkyBox", stateLessEqual.Ptr));
		FxSetRenderState(llgl::GetMainContext(), *stateLessEqual);

		TbShader* shader;
		mxDO(Resources::Load(shader, MakeAssetID("skybox"), m_scene_data));

		TPtr< RrTexture >	cubeMap;
		//mxDO(Resources::Load(cubeMap.Ptr, MakeAssetID("nightsky.texture"), m_scene_data));
		mxDO(Resources::Load(cubeMap.Ptr, MakeAssetID("sky-cubemap"), m_scene_data));

		mxDO(FxSlow_SetInput(shader, "cubeMap", cubeMap->m_resource));


		ARenderer::DBG_Draw_Mesh( *m_cubeMesh, *shader);
	}
#endif







#if 1
	if( m_debugSaveFrustum )
	{
		m_debugFrustumView = sceneView;
		m_debugSaveFrustum = false;
		m_debugDrawFrustum = true;
		mxGPU_SCOPE(READBACK);


		using namespace llgl;
		ContextD3D11* ctx = (ContextD3D11*) llgl::GetMainContext().ptr;
		ID3D11DeviceContext *	immCtx11 = ctx->m_deviceContext;

		// Copy the number of items counter from the UAV into a constant buffer
		immCtx11->CopyStructureCount(
			ctx->m_be->buffers[m_UBO_number_of_structures.id].m_ptr,
			0,
			ctx->m_be->buffers[m_deferredRenderer.u_debugPoints.id].m_UAV
		);


		U32	structureCount[4];// = {0};
		llgl::Debug_ReadBack(llgl::GetMainContext(), m_UBO_number_of_structures, structureCount, sizeof(structureCount));

		//DBGOUT("structureCount[0] = %u", structureCount[0]);


		const int numLines = smallest(structureCount[0], DeferredRenderer::MAX_DEBUG_POINTS);

		//ScopedStackAlloc	programAlloc( gCore.frameAlloc );
		//DebugLine *	debugLines = programAlloc.AllocMany<DebugLine>( numLines );

		m_debugPoints.SetNum(numLines);
		llgl::Debug_ReadBack(llgl::GetMainContext(), m_deferredRenderer.u_debugPoints, m_debugPoints.Raw(), m_debugPoints.RawSize());

		struct {
			bool operator()(const DebugPoint& a, const DebugPoint& b)
			{ 
				int indexA = a.groupId.y * (1024/TILE_SIZE_X) + a.groupId.x;
				int indexB = b.groupId.y * (1024/TILE_SIZE_X) + b.groupId.x;
				return indexA < indexB;
			}   
		} customLess;
		std::sort(m_debugPoints.Raw(), m_debugPoints.Raw() + m_debugPoints.Num(), customLess);


		int minGroupIdX = MAX_INT32;
		int maxGroupIdX = MIN_INT32;
		int minGroupIdY = MAX_INT32;
		int maxGroupIdY = MIN_INT32;
		for( U32 iPoint = 0; iPoint < m_debugPoints.Num(); iPoint++ )
		{
			const DebugPoint& point = m_debugPoints[ iPoint ];

			int groupIdX = point.groupId.x;
			int groupIdY = point.groupId.y;
			minGroupIdX = smallest(minGroupIdX, groupIdX);
			maxGroupIdX = largest(maxGroupIdX, groupIdX);
			minGroupIdY = smallest(minGroupIdY, groupIdY);
			maxGroupIdY = largest(maxGroupIdY, groupIdY);
			//LogStream(LL_Debug) << iPoint << " GroupId:"  << point.groupId << " Rect:"  << point.additional << " Pos:"  << point.position;
		}

		LogStream(LL_Debug) << V4F4_Set( minGroupIdX, maxGroupIdX, minGroupIdY, maxGroupIdY );
	//	ptDBG_BREAK;
	}
#else
	if( m_debugSaveFrustum )
	{
		m_debugFrustumView = sceneView;
		m_debugSaveFrustum = false;
		m_debugDrawFrustum = true;
	}
#endif





	static bool s_visualizeLightCount = false;


#if DEBUG_EMIT_POINTS
	if(s_visualizeLightCount)
	{
		mxGPU_SCOPE(READBACK);

		using namespace llgl;
		ContextD3D11* ctx = (ContextD3D11*) llgl::GetMainContext().ptr;
		ID3D11DeviceContext *	immCtx11 = ctx->m_deviceContext;

		// Copy the number of items counter from the UAV into a constant buffer
		immCtx11->CopyStructureCount(
			ctx->m_be->buffers[m_UBO_number_of_structures.id].m_ptr,
			0,
			ctx->m_be->buffers[m_deferredRenderer.u_debugPoints.id].m_UAV
		);


		U32	structureCount[4];// = {0};
		llgl::Debug_ReadBack(llgl::GetMainContext(), m_UBO_number_of_structures, structureCount, sizeof(structureCount));

		const int numPoints = smallest(structureCount[0], DeferredRenderer::MAX_DEBUG_POINTS);

		m_debugPoints.SetNum(numPoints);
		llgl::Debug_ReadBack(llgl::GetMainContext(), m_deferredRenderer.u_debugPoints, m_debugPoints.Raw(), m_debugPoints.RawSize());



		Rendering::RendererGlobals & globals = Rendering::tr;

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);

		for( U32 iPoint = 0; iPoint < m_debugPoints.Num(); iPoint++ )
		{
			const DebugPoint& point = m_debugPoints[ iPoint ];

			int groupIdX = point.groupId.x;
			int groupIdY = point.groupId.y;
			int numLightsAffectingTile = point.groupId.z;


			wchar_t	buf[32];
			swprintf_s(buf, L"%d", numLightsAffectingTile);


			llgl::Batch	batch;
			batch.Clear();

			TbShader* debubFontShader;
			mxDO(Resources::Load(debubFontShader, MakeAssetID("DebugFont"), m_scene_data));
			mxDO(FxSlow_SetInput(debubFontShader, "t_font", llgl::AsInput(m_debugFont.m_textureAtlas)));
			BindShaderInputs(batch, *debubFontShader);
			batch.program = debubFontShader->programs[0];

			m_fontRenderer.DrawTextW(
				llgl::GetMainContext(),
				batch,
				screenWidth, screenHeight,
				&m_debugFont,
				(groupIdX * TILE_SIZE_X)*0.5f,// - TILE_SIZE_X/2,
				(groupIdY * TILE_SIZE_Y)*0.5f,// - TILE_SIZE_Y/2,
				buf
				);
			llgl::Draw(llgl::GetMainContext(), batch);
		}
	}
#endif






#if 0

	if(m_debugDrawFrustum)
	{
		mxGPU_SCOPE(DEBUG_DRAW_TILES);

		Rendering::RendererGlobals & globals = Rendering::tr;

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);

		TbShader* shader;
		mxDO(GetByName(ClumpList::g_loadedClumps, "auxiliary_rendering", shader));
		m_prim_batcher.SetShader(shader);


		M44F4 inverseProj = Matrix_Inverse(m_debugFrustumView.projectionMatrix);
		M44F4 inverseViewProj = Matrix_Inverse(m_debugFrustumView.viewProjectionMatrix);

		for( U32 iLine = 0; iLine < m_debugPoints.Num(); iLine++ )
		{
			const DebugPoint& point = m_debugPoints[ iLine ];

			V4F endWS = point.position;
			endWS.w = 1;
			endWS = Matrix_Project( inverseViewProj, endWS );

			m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(endWS), point.color );
		}

#if 0
		for( U32 iLine = 0; iLine < m_debugPoints.Num(); iLine++ )
		{
			const DebugLine& line = m_debugPoints[ iLine ];

			//m_prim_batcher.DrawLine( V4F_As_V3F(line.start), V4F_As_V3F(line.end), RGBAf::RED );

			V4F startWS = line.start;
			V4F endWS = line.end;

			startWS.w = 1;
			endWS.w = 1;

			startWS = Matrix_Project( inverseViewProj, startWS );
			endWS = Matrix_Project( inverseViewProj, endWS );

			m_prim_batcher.DrawLine( V4F_As_V3F(startWS), V4F_As_V3F(endWS), RGBAf::RED );
		}
#endif
	}

#endif

	{
		mxGPU_SCOPE(Gizmos);

		Rendering::RendererGlobals & globals = Rendering::tr;

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);

		TbShader* shader;
		mxDO(GetByName(ClumpList::g_loadedClumps, "auxiliary_rendering", shader));
		m_prim_batcher.SetShader(shader);
		mxDO(FxSlow_SetUniform(shader,"u_modelViewProjMatrix",&worldViewProjectionMatrix));
		mxDO(FxSlow_SetUniform(shader,"u_modelMatrix",&worldMatrix));
		mxDO(FxSlow_SetUniform(shader,"u_color",RGBAf::WHITE.Raw()));
		mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));



#if 0
		{
			TObjectIterator< RrLocalLight >	lightIt( *m_scene_data );
			while( lightIt.IsValid() )
			{
				const RrLocalLight& light = lightIt.Value();

				if(m_debugDrawFrustum)
				{
				}

				m_prim_batcher.DrawCircle(
					light.position,
					camera.m_rightDirection,
					camera.m_upDirection,
					RGBAf(light.color.x, light.color.y, light.color.z, 1.0f),
					light.radius
				);

				lightIt.MoveToNext();
			}
		}
#endif




#if 1
		if(m_debugDrawFrustum)
		{
#if 0
			const U32 TILE_SIZE_X = 128;
			const U32 TILE_SIZE_Y = 64;

			const U32 tilesX = (U32)(m_debugFrustumView.viewportWidth/TILE_SIZE_X);
			const U32 tilesY = (U32)(m_debugFrustumView.viewportHeight/TILE_SIZE_Y);
#else
			const U32 tilesX = CalculateNumTilesX(m_debugFrustumView.viewportWidth);
			const U32 tilesY = CalculateNumTilesY(m_debugFrustumView.viewportHeight);
#endif

			const M44F4 inverseView = Matrix_Inverse(m_debugFrustumView.viewMatrix);
			const M44F4 inverseProj = Matrix_Inverse(m_debugFrustumView.projectionMatrix);
			const M44F4 inverseViewProj = Matrix_Inverse(m_debugFrustumView.viewProjectionMatrix);

			float WW = (float)(TILE_SIZE_X*tilesX);//uWindowWidthEvenlyDivisibleByTileRes
			float HH = (float)(TILE_SIZE_Y*tilesY);//uWindowHeightEvenlyDivisibleByTileRes

#if 0
			for( U32 iTileX = 0; iTileX < tilesX; iTileX++ )
			{
				for( U32 iTileY = 0; iTileY < tilesY; iTileY++ )
				{
					// plane equations for the four sides of the frustum, with the positive half-space outside the frustum
					V4F frustumPlanes[4];

					// tile corners in viewport space
					U32 px_min = TILE_SIZE_X * iTileX;	// [0 .. ScreenWidth - TILE_SIZE_X]
					U32 py_min = TILE_SIZE_Y * iTileY;	// [0 .. ScreenHeight - TILE_SIZE_Y]
					U32 px_max = TILE_SIZE_X * (iTileX+1);
					U32 py_max = TILE_SIZE_Y * (iTileY+1);

					// tile corners in clip space
					const float px_left = px_min/WW;
					const float px_right = px_max/WW;
					const float py_top = (HH-py_min)/HH;
					const float py_bottom = (HH-py_max)/HH;

					// four corners of the tile, clockwise from top-left
					V4F frustumCornersCS[4];
					frustumCornersCS[0] = V4F4_Set( px_left*2.f-1.f, py_top*2.f-1.f, 1.f,1.f);
					frustumCornersCS[1] = V4F4_Set( px_right*2.f-1.f, py_top*2.f-1.f, 1.f,1.f);
					frustumCornersCS[2] = V4F4_Set( px_right*2.f-1.f, py_bottom*2.f-1.f, 1.f,1.f);
					frustumCornersCS[3] = V4F4_Set( px_left*2.f-1.f, py_bottom*2.f-1.f, 1.f,1.f);

					V4F frustumCornersWS[4];
					frustumCornersWS[0] = Matrix_Project( inverseViewProj, frustumCornersCS[0] );
					frustumCornersWS[1] = Matrix_Project( inverseViewProj, frustumCornersCS[1] );
					frustumCornersWS[2] = Matrix_Project( inverseViewProj, frustumCornersCS[2] );
					frustumCornersWS[3] = Matrix_Project( inverseViewProj, frustumCornersCS[3] );

					m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[0]) );
					m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[1]) );
					m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[2]) );
					m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[3]) );
#if 0
					// four corners of the tile, clockwise from top-left
					V4F frustum[4];
					frustum[0] = ConvertProjToView( V4F( pxm/(float)WW*2.f-1.f, (HH-pym)/(float)HH*2.f-1.f, 1.f,1.f) );
					frustum[1] = ConvertProjToView( V4F( pxp/(float)WW*2.f-1.f, (HH-pym)/(float)HH*2.f-1.f, 1.f,1.f) );
					frustum[2] = ConvertProjToView( V4F( pxp/(float)WW*2.f-1.f, (HH-pyp)/(float)HH*2.f-1.f, 1.f,1.f) );
					frustum[3] = ConvertProjToView( V4F( pxm/(float)WW*2.f-1.f, (HH-pyp)/(float)HH*2.f-1.f, 1.f,1.f) );

					// create plane equations for the four sides of the frustum, 
					// with the positive half-space outside the frustum (and remember, 
					// sceneView space is left handed, so use the left-hand rule to determine 
					// cross product direction)
					for(U32 i=0; i<4; i++)
						frustumPlanes[i] = CreatePlaneEquation( frustum[i], frustum[(i+1)&3] );
#endif
				}
			}

#else

			m_prim_batcher.DrawWireFrustum(m_debugFrustumView.viewProjectionMatrix, RGBAf::YELLOW);

			{
				static V4F verticalFrustumPlanes[1024];
				static V4F horizontalFrustumPlanes[1024];

				mxDO(ComputeTileFrusta(
					m_debugFrustumView.viewportWidth,
					m_debugFrustumView.viewportHeight,
					inverseViewProj,
					verticalFrustumPlanes,
					mxCOUNT_OF(verticalFrustumPlanes),
					horizontalFrustumPlanes,
					mxCOUNT_OF(horizontalFrustumPlanes)
					));

				for( U32 iTileX = 0; iTileX < tilesX; iTileX++ )
				{
					for( U32 iTileY = 0; iTileY < tilesY; iTileY++ )
					{
					}
				}
			}


			// tile corners in clip space
			const float px_left = 0;
			const float px_right = 1;
			const float py_top = 1;
			const float py_bottom = 0;

			// four corners of the tile, clockwise from top-left
			V4F frustumCornersCS[4];
			frustumCornersCS[0] = V4F4_Set( px_left*2.f-1.f, py_top*2.f-1.f, 1.f,1.f);
			frustumCornersCS[1] = V4F4_Set( px_right*2.f-1.f, py_top*2.f-1.f, 1.f,1.f);
			frustumCornersCS[2] = V4F4_Set( px_right*2.f-1.f, py_bottom*2.f-1.f, 1.f,1.f);
			frustumCornersCS[3] = V4F4_Set( px_left*2.f-1.f, py_bottom*2.f-1.f, 1.f,1.f);

			V4F frustumCornersWS[4];
			frustumCornersWS[0] = Matrix_Project( inverseViewProj, frustumCornersCS[0] );
			frustumCornersWS[1] = Matrix_Project( inverseViewProj, frustumCornersCS[1] );
			frustumCornersWS[2] = Matrix_Project( inverseViewProj, frustumCornersCS[2] );
			frustumCornersWS[3] = Matrix_Project( inverseViewProj, frustumCornersCS[3] );

			m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[0]) );
			m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[1]) );
			m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[2]) );
			m_prim_batcher.DrawLine( m_debugFrustumView.worldSpaceCameraPos, V4F_As_V3F(frustumCornersWS[3]) );

			V4F frustumPlanesWS[6];


			const V3F a = m_debugFrustumView.worldSpaceCameraPos;
//m_debugFrustumView.viewMatrix

			const M44F4 camWorldMtx = inverseView;
			const V3F forward = V3_Normalized(V4F_As_V3F(camWorldMtx.r1));
			const V3F backward = V3_Negate(forward);
			frustumPlanesWS[0] = Plane_FromPointNormal( a + forward * m_debugFrustumView.nearClip, backward );
			frustumPlanesWS[1] = Plane_FromPointNormal( a + forward * m_debugFrustumView.farClip, forward );


//CORRECT!
const M44F4 planeWorldToViewSpace = Matrix_Transpose( Matrix_Inverse( m_debugFrustumView.viewMatrix ) );
const V4F nearPlaneInViewSpace = Matrix_Transform(planeWorldToViewSpace, frustumPlanesWS[0]);
const V4F farPlaneInViewSpace = Matrix_Transform(planeWorldToViewSpace, frustumPlanesWS[1]);


			for( int i = 0; i < 4; i++ )
			{
				const V3F b = V4F_As_V3F(frustumCornersWS[i]);
				//int next = i + 1;
				//if( next > 5 ) {
				//	next = 2;
				//}
				const V3F c = V4F_As_V3F(frustumCornersWS[(i+1)&3]);

				V3F N = V3_Normalized( V3_Cross( c - a, b - a ) );
				(V3F&)frustumPlanesWS[i+2] = N;

				frustumPlanesWS[i+2] = Plane_FromPointNormal( a, N );
			}


			for( int i = 2; i < 6; i++ )
			{
				m_prim_batcher.DrawLine(
					m_debugFrustumView.worldSpaceCameraPos,
					m_debugFrustumView.worldSpaceCameraPos + Plane_GetNormal(frustumPlanesWS[i]) * 100,
					RGBAf::WHITE, RGBAf::aquamarine
					);
			}

			{
				TObjectIterator< RrLocalLight >	lightIt( *m_scene_data );
				while( lightIt.IsValid() )
				{
					RrLocalLight& light = lightIt.Value();

#if 0
					bool inFrustum = true;
					for( int i = 0; i < 6; ++i )
					{
						float d = Plane_PointDistance( frustumPlanesWS[i], light.position );
						inFrustum = inFrustum && (d <= light.radius);
					}
#else
					const bool inFrustum = Frustum_IntersectSphere(frustumPlanesWS, light.position, light.radius);
#endif

					RGBAf lightColor(light.color.x, light.color.y, light.color.z, 1.0f);
					if(inFrustum)
					{
						lightColor = RGBAf::RED;
					}
					else
					{
						lightColor = RGBAf::GREEN;
					}

					light.color = (V3F&)lightColor;

					//m_prim_batcher.DrawCircle(
					//	light.position,
					//	camera.m_rightDirection,
					//	camera.m_upDirection,
					//	lightColor,
					//	light.radius
					//);

					lightIt.MoveToNext();
				}
			}
#endif



		}
#endif

		
#if 0
		for( U32 iDebugLine = 0; iDebugLine < globals.debugLines.Num(); iDebugLine++ )
		{
			const RrDebugLine& debugLine = globals.debugLines[ iDebugLine ];

			m_prim_batcher.DrawLine(
				debugLine.start,
				debugLine.end,
				RGBAf::FromRGBA32(debugLine.startColor),
				RGBAf::FromRGBA32(debugLine.endColor)
				);
		}
		globals.debugLines.Empty();
#endif


		
		DrawGizmo(m_gizmo_renderer, worldMatrix, camera.m_eyePosition, m_prim_batcher);


		m_prim_batcher.Flush();
	}



	if( m_closestVertexIndex != -1 )
	{
		m_prim_batcher.DrawSolidSphere(
			m_vertices[m_closestVertexIndex].xyz,
			1,
			RGBAf::RED
			);
		m_prim_batcher.Flush();
		//m_prim_batcher.DrawCircle(
		//	m_vertices[m_closestVertexIndex].xyz,
		//	camera.m_rightDirection,
		//	camera.m_upDirection,
		//	RGBAf::RED,
		//	3.f
		//	);
	}



	{
		TObjectIterator< RrLocalLight >	lightIt( *m_scene_data );
		while( lightIt.IsValid() )
		{
			RrLocalLight& light = lightIt.Value();

			RGBAf lightColor(light.color.x, light.color.y, light.color.z, 1.0f);

#if 0
			m_prim_batcher.DrawCircle(
				light.position,
				camera.m_rightDirection,
				camera.m_upDirection,
				lightColor,
				light.radius
				);
#endif

			if( animateLights )
			{
				static float s_angle_radians = 0;
				s_angle_radians += deltaSeconds*0.0001f;

				if( s_angle_radians >= mxTWO_PI ) {
					s_angle_radians = s_angle_radians - mxTWO_PI;
				}

				//V3F pos2d = light.position;
				//pos2d.z = 0;

				//float radius = V3_Length(pos2d);

				////float cosf =  / light.position.x;
				//float angle = cosf(light.position.x);

				const M44F4 rot = Matrix_RotationZ(s_angle_radians);

				light.position = Matrix_TransformPoint(rot, light.position);
			}

			lightIt.MoveToNext();
		}
		m_prim_batcher.Flush();
	}





	//{
	//	llgl::ViewState	viewState;
	//	{
	//		viewState.Reset();
	//		viewState.colorTargets[0].SetDefault();
	//		viewState.targetCount = 1;
	//	}
	//	llgl::SetView(llgl::GetMainContext(), viewState);
	//}



#if 1
	if(s_visualizeLightCount)
	{
		Rendering::RendererGlobals & globals = Rendering::tr;

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);

		TbShader* shader;
		mxDO(GetByName(ClumpList::g_loadedClumps, "auxiliary_rendering", shader));
		m_prim_batcher.SetShader(shader);


		const M44F4 identity = Matrix_Identity();

		M44F4 viewportToClip = identity;

		mxDO(FxSlow_SetUniform(shader,"u_modelViewProjMatrix",&viewportToClip));
		mxDO(FxSlow_SetUniform(shader,"u_modelMatrix",&identity));
		mxDO(FxSlow_SetUniform(shader,"u_color",RGBAf::WHITE.Raw()));
		mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));


		const U32 tilesX = CalculateNumTilesX(screenWidth);
		const U32 tilesY = CalculateNumTilesY(screenHeight);
		float WW = (float)(TILE_SIZE_X*tilesX);//uWindowWidthEvenlyDivisibleByTileRes
		float HH = (float)(TILE_SIZE_Y*tilesY);//uWindowHeightEvenlyDivisibleByTileRes

		for( int iTileX = 0; iTileX < tilesX; iTileX++ )
		{
			int x_viewport = TILE_SIZE_X * iTileX;
			float x_NDC = (1.0f/screenWidth) * x_viewport * 2.0f - 1.0f;
			m_prim_batcher.DrawLine(
				V3_Set(x_NDC, -1.0f, 0.0f),
				V3_Set(x_NDC, +1.0f, 0.0f)
			);
		}
		for( int iTileY = 0; iTileY < tilesY; iTileY++ )
		{
			int y_viewport = TILE_SIZE_Y * iTileY;
			float y_NDC = 1.0f - (1.0f/screenHeight) * y_viewport * 2.0f;
			m_prim_batcher.DrawLine(
				V3_Set(-1.0f, y_NDC, 0.0f),
				V3_Set(+1.0f, y_NDC, 0.0f)
				);
		}
		m_prim_batcher.Flush();
	}
#endif






	if(debug_renderTargets)
	{
		mxGPU_SCOPE(Show_RTs);

		Rendering::RendererGlobals & globals = Rendering::tr;

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);


		TbShader* shader;
		mxDO(Resources::Load(shader, MakeAssetID("screen_quad"), m_scene_data));



		const int QW = screenWidth*0.25f;
		const int QH = screenHeight*0.25f;

		int quad_offset_x = 0;

		// Albedo
		mxDO(FxSlow_SetInput(shader, "t_sourceTexture", llgl::AsInput(m_deferredRenderer.m_colorRT1->handle)));
		mxDO(DrawScreenQuad2(shader, QW, QH, quad_offset_x+10, 10, &quad_offset_x));

		// Normals
		mxDO(FxSlow_SetInput(shader, "t_sourceTexture", llgl::AsInput(m_deferredRenderer.m_colorRT0->handle)));
		mxDO(DrawScreenQuad2(shader, QW, QH, quad_offset_x+10, 10, &quad_offset_x));

		// CS output
		TPtr< TbTexture2D >	litTexture;
		mxDO(GetByName(ClumpList::g_loadedClumps, "LightingTarget", litTexture.Ptr));
		mxDO(FxSlow_SetInput(shader, "t_sourceTexture", llgl::AsInput(litTexture->handle)));
		//mxDO(FxSlow_SetInput(shader, "t_sourceTexture", llgl::AsInput(m_CS_output)));		
		mxDO(DrawScreenQuad2(shader, QW*2, QH*2, quad_offset_x+10, 10, &quad_offset_x));

#if 0
		{
			{
				viewState.Reset();
				viewState.colorTargets[0].SetDefault();
				viewState.targetCount = 1;
			}
			llgl::SetView(llgl::GetMainContext(), viewState);

			TbShader* debugShader;
			mxDO(Resources::Load(debugShader, MakeAssetID("debug_show_structured_buffer.shader"), m_scene_data));

			mxDO(FxSlow_SetInput(debugShader, "t_sourceBuffer", llgl::AsInput(m_CS_output)));
			mxDO(DrawScreenQuad(debugShader, 0.25, 0.25, quad_offset_x, 10, &quad_offset_x));
		}
#endif
	}



#if 0
	{
		mxGPU_SCOPE(DebugText);

		Rendering::RendererGlobals & globals = Rendering::tr;

		llgl::ViewState	viewState;
		{
			viewState.Reset();
			viewState.colorTargets[0].SetDefault();
			viewState.targetCount = 1;
		}
		llgl::SetView(llgl::GetMainContext(), viewState);

		FxSetRenderState(llgl::GetMainContext(), *globals.state_NoCull);



		llgl::Batch	batch;
		batch.Clear();

		TbShader* debubFontShader;
		mxDO(Resources::Load(debubFontShader, MakeAssetID("DebugFont"), m_scene_data));
		mxDO(FxSlow_SetInput(debubFontShader, "t_font", llgl::AsInput(m_debugFont.m_textureAtlas)));
		BindShaderInputs(batch, *debubFontShader);
		batch.program = debubFontShader->programs[0];

		//m_fontRenderer.RenderText(
		//	llgl::GetMainContext(),
		//	batch,
		//	screenWidth, screenHeight,
		//	&m_debugFont,
		//	10, 10,
		//	"Hello, world!"
		//);

		m_fontRenderer.DrawTextW(
			llgl::GetMainContext(),
			batch,
			screenWidth, screenHeight,
			&m_debugFont,
			10, 10,
			L"Текст для теста"
		);



		llgl::Draw(llgl::GetMainContext(), batch);
	}
#endif


	__super::BeginGUI();
	{
		if(ImGui::Begin("Render Stats"))
		{
			String128 rendererInfo;
			m_currentRenderer->VGetInfo(rendererInfo);
			ImGui::Text("Rendering Pipeline: %s", rendererInfo.c_str());
		
			ImGui::Checkbox("animateLights", &animateLights);

			ImGui::Checkbox("s_visualizeLightCount", &s_visualizeLightCount);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Visualize light count:\ndraw tiles, etc.");

			ImGui::Checkbox("Show Render Targets", &debug_renderTargets);

			ImGui::Text("Models: %d", m_sceneStats.models_rendered);
			ImGui::Text("Deferred lights: %d", m_sceneStats.deferred_lights);

			ImGui::Text("Tile Size: %dx%d (%dx%d)", TILE_SIZE_X, TILE_SIZE_Y, m_sceneStats.tilesX, m_sceneStats.tilesY);



			ImGui::Text("Relative mouse mode: %d", !!inputState.grabsMouseCursor);
#if 0

	//static int r_showDebugRT = 1;

			const char* items[] = {
				"None",
				"Show Albedo",
				"Show Normals",
			};
			ImGui::Combo("Show RT", &r_showDebugRT, items, mxCOUNT_OF(items));
#endif

			ImGui::End();
		}
		ImGui_Window_Text_Float3(camera.m_eyePosition);

		ImGui::Text("m_rotationSpeed: %f", camera.m_rotationSpeed);
		ImGui::Text("m_pitchSensitivity: %f", camera.m_pitchSensitivity);
		ImGui::Text("m_yawSensitivity: %f", camera.m_yawSensitivity);
	}
	return ALL_OK;
}

//void MyJob1( const TbJobData& data, const ThreadContext& context )
//{
//	::Sleep(2);
//	//DBGOUT("MyJob1: Begin");
//}
//void MyJob2( const TbJobData& data, const ThreadContext& context )
//{
//	::Sleep(1);
//	//DBGOUT("MyJob2");
//}


ERet MyEntryPoint( const String& root_folder )
{
	TbJobManagerSettings jobManSettings;
	jobManSettings.CalculateOptimalValues(3);	// main, asyncIO, render
	//jobManSettings.numWorkerThreads =0;
	mxDO(JobMan::Initialize(jobManSettings));


	//JobHandle hJob1 = JobMan::Add(TbJobData(), &MyJob1, "Job1");
	//JobHandle hJob2 = JobMan::Add(TbJobData(), &MyJob2, "Job2");

	//JobMan::WaitFor(hJob1);


	MyApp	app;

	FlyingCamera& camera = app.camera;
	camera.m_eyePosition = V3_Set(0, -100, 20);
	camera.m_movementSpeed = 100;
	camera.m_strafingSpeed = 100;
	camera.m_rotationSpeed = 0.1;
	camera.m_invertYaw = true;
	camera.m_invertPitch = true;

	SON::LoadFromFile(gUserINI->GetString("path_to_camera_state"),camera);

	{
		InputBindings& input = app.m_bindings;
		mxDO(input.handlers.Reserve(64));
		mxDO(input.contexts.Reserve(4));

		{
			String512 path_to_key_bindings( root_folder );
			Str::AppendS( path_to_key_bindings, "Config/input_mappings.son" );
			mxDO(SON::LoadFromFile( path_to_key_bindings.c_str(), input.contexts ));
		}

		input.handlers.Add( ActionBindingT(GameActions::move_forward, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_forward,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_back, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_back,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_left, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_left,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_right, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_right,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_up, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_up,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_down, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_down,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::rotate_pitch, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_pitch,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::rotate_yaw, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_yaw,camera)) );
	}


	WindowsDriver::Settings	options;
#if 0
	options.window_width = 1024*2;
	options.window_height = 512*2;
	options.window_x = 0;
	options.window_y = 0;
#else
	options.window_width = 1024;
	options.window_height = 512;
	options.window_x = 900;
	options.window_y = 30;
#endif
	//options.window_width = 1400;
	//options.window_height = 1024;


//	options.create_OpenGL_context = true;
	mxDO(WindowsDriver::Initialize(options));


	llgl::Settings settings;
	settings.window = WindowsDriver::GetNativeWindowHandle();
	llgl::Initialize(settings);


	mxDO(app.Setup());


	InputState inputState;
	while( WindowsDriver::BeginFrame( &inputState, &app ) )
	{
		rmt_ScopedCPUSample(MainFrame);

		app.Dev_ReloadChangedAssets();

		if( inputState.hasFocus )
		{
			app.Update(inputState.deltaSeconds);
		}
		else
		{
			mxSleepMilliseconds(100);
		}

		app.BeginRender(inputState.deltaSeconds);

		{

			ImGui::Text("dt: %.3f", inputState.deltaSeconds);
		}

		app.EndRender();

		llgl::NextFrame();

		mxSleepMilliseconds(15);

		mxPROFILE_INCREMENT_FRAME_COUNTER;
	}

	app.Shutdown();

	WindowsDriver::Shutdown();


	SON::SaveToFile(camera,gUserINI->GetString("path_to_camera_state"));


	JobMan::Shutdown();

	return ALL_OK;
}

int main(int /*_argc*/, char** /*_argv*/)
{
	SetupBaseUtil	setupBase;
	FileLogUtil		fileLog;
	SetupCoreUtil	setupCore;
	CConsole		consoleWindow;
UNDONE;
#if 0
	// Try to load the INI config first.
	String512	root_folder;
	GetRootFolder(root_folder);

	String512	path_to_INI_config;
	Str::Copy( path_to_INI_config, root_folder );
	Str::AppendS( path_to_INI_config, "Config/Engine.son" );

	String512	path_to_User_INI;
	Str::Copy( path_to_User_INI, root_folder );
	Str::AppendS( path_to_User_INI, "Config/client.son" );

	SON::TextConfig	engineINI;
	engineINI.LoadFromFile( path_to_INI_config.c_str() );
	gINI = &engineINI;

	SON::TextConfig	clientINI;
	clientINI.LoadFromFile( path_to_User_INI.c_str() );
	gUserINI = &clientINI;
#endif

	// Create the main instance of Remotery.
	// You need only do this once per program.
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

	MyEntryPoint( root_folder );

	// Destroy the main instance of Remotery.
	rmt_DestroyGlobalInstance(rmt);

	return 0;
}
