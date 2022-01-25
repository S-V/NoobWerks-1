//
#pragma once

///
struct RawTriMesh: NonCopyable
{
	//TODO: smoothing groups?
	struct Vertex
	{
		V3f		xyz;
		V2f		uv;
		V3f		N;
	};
	struct Triangle
	{
		U32		vertex_indices[3];
	};
	struct Part
	{
		U32		start_triangle;	//!< Offset of the first triangle
		U32		triangle_count;	//!< Number of triangles

		U32		material_id;	// for triplanar texturing, -1 = none
		U32		texture_id;		// for UV texturing, -1 = none
	};

	DynamicArray< Vertex >		vertices;
	DynamicArray< Triangle >	triangles;
	DynamicArray< Part >		parts;

	AABBf		local_AABB;

public:
	RawTriMesh( AllocatorI& allocator )
		: vertices( allocator )
		, triangles( allocator )
		, parts( allocator )
	{
		local_AABB.clear();
	}
};


namespace CubeMeshUtil
{

/*
//	Faces are numbered in the order -X, +X, -Y, +Y, -Z, +Z:
//	Face index - face normal:
//	0 (Left)   is -X,
//	1 (Right)  is +X,
//	2 (Front)  is -Y,
//	3 (Back)   is +Y,
//	4 (Bottom) is -Z,
//	5 (Top)    is +Z.
//
//	Cubes are unrolled in the following order,
//	looking along +Y direction (labelled '1'):
//			_____
//			| 5 |
//		-----------------
//		| 0 | 2 | 1 | 3 |
//		-----------------
//			| 4 |
//			-----
//
//			   +----------+
//			   |+Y       5|
//			   | ^  +Z    |
//			   | |        |
//			   | +---->+X |
//	+----------+----------+----------+----------+
//	|+Z       0|+Z       2|+Z       1|+Z       3|
//	| ^  -X    | ^  -Y    | ^  +X    | ^  +Y    |
//	| |        | |        | |        | |        |
//	| +---->-Y | +---->+X | +---->+Y | +---->-X |
//	+----------+----------+----------+----------+
//			   |-Y       4|
//			   | ^  -Z    |
//			   | |        |
//			   | +---->+X |
//			   +----------+
//
//	Face vertices are numbered as follows:
//
//   +--------------> U
//   |
//   |  3-------2
//   |  |       |
//	 |  |       |
//	 |  |       |
//   |  0-------1
//   |
//   V
//
//*/

struct CubeFace
{
	V3f	xyz[4];
	V2f	UVs[4];
	V3f	N;
};

struct CubeMesh
{
	V3f	positions[24];
	V3f	normals[24];
	V2f	UVs[24];
};

void buildCubeMesh( CubeMesh *cube_mesh_ );

ERet buildCubeMesh( RawTriMesh *raw_trimesh_ );

extern const U16 g_cube_mesh_indices[36];


}//namespace CubeMeshUtil
