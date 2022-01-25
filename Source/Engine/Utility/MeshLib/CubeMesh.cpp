#include <Base/Base.h>
#pragma hdrstop

#include <Core/ObjectModel/Clump.h>
#include <Rendering/Public/Core/Mesh.h>

#include <MeshLib/CubeMesh.h>


namespace CubeMeshUtil
{

static
void buildCubeFaces( CubeFace faces_[6] )
{
	const float width = 1.0f;	// X
	const float depth = 1.0f;	// Y
	const float height = 1.0f;	// Z

	const float w2 = 0.5f * width;
	const float d2 = 0.5f * depth;
	const float h2 = 0.5f * height;

	//
	const V2f standard_UVs[4] = {
		V2f( 0, 1 ),
		V2f( 1, 1 ),
		V2f( 1, 0 ),
		V2f( 0, 0 )
	};

	//
	CubeFace	negX_face, posX_face;
	CubeFace	negY_face, posY_face;
	CubeFace	negZ_face, posZ_face;

	// -X ( left )
	negX_face.xyz[0] = CV3f( -w2, +d2, -h2 );
	negX_face.xyz[1] = CV3f( -w2, -d2, -h2 );
	negX_face.xyz[2] = CV3f( -w2, -d2, +h2 );
	negX_face.xyz[3] = CV3f( -w2, +d2, +h2 );
	TCopyStaticArray( negX_face.UVs, standard_UVs );
	negX_face.N = CV3f(-1,0,0);
	// +X ( right )
	posX_face.xyz[0] = CV3f( +w2, -d2, -h2 );
	posX_face.xyz[1] = CV3f( +w2, +d2, -h2 );
	posX_face.xyz[2] = CV3f( +w2, +d2, +h2 );
	posX_face.xyz[3] = CV3f( +w2, -d2, +h2 );
	TCopyStaticArray( posX_face.UVs, standard_UVs );
	posX_face.N = CV3f(+1,0,0);

	// -Y ( front )
	negY_face.xyz[0] = CV3f( -w2, -d2, -h2 );
	negY_face.xyz[1] = CV3f( +w2, -d2, -h2 );
	negY_face.xyz[2] = CV3f( +w2, -d2, +h2 );
	negY_face.xyz[3] = CV3f( -w2, -d2, +h2 );
	TCopyStaticArray( negY_face.UVs, standard_UVs );
	negY_face.N = CV3f(0,-1,0);
	// +Y ( back )
	posY_face.xyz[0] = CV3f( +w2, +d2, -h2 );
	posY_face.xyz[1] = CV3f( -w2, +d2, -h2 );
	posY_face.xyz[2] = CV3f( -w2, +d2, +h2 );
	posY_face.xyz[3] = CV3f( +w2, +d2, +h2 );
	TCopyStaticArray( posY_face.UVs, standard_UVs );
	posY_face.N = CV3f(0,+1,0);

	// -Z ( bottom )
	negZ_face.xyz[0] = CV3f( -w2, +d2, -h2 );
	negZ_face.xyz[1] = CV3f( +w2, +d2, -h2 );
	negZ_face.xyz[2] = CV3f( +w2, -d2, -h2 );
	negZ_face.xyz[3] = CV3f( -w2, -d2, -h2 );
	TCopyStaticArray( negZ_face.UVs, standard_UVs );
	negZ_face.N = CV3f(0,0,-1);
	// +Z ( top )
	posZ_face.xyz[0] = CV3f( -w2, -d2, +h2 );
	posZ_face.xyz[1] = CV3f( +w2, -d2, +h2 );
	posZ_face.xyz[2] = CV3f( +w2, +d2, +h2 );
	posZ_face.xyz[3] = CV3f( -w2, +d2, +h2 );
	TCopyStaticArray( posZ_face.UVs, standard_UVs );
	posZ_face.N = CV3f(0,0,+1);

	//
	faces_[0] = negX_face;
	faces_[1] = posX_face;
	faces_[2] = negY_face;
	faces_[3] = posY_face;
	faces_[4] = negZ_face;
	faces_[5] = posZ_face;
}

static const U16 g_cube_mesh_indices[36] =
{
	0,1,2,0,2,3,
	4,5,6,4,6,7,
	8,9,10,8,10,11,
	12,13,14,12,14,15,
	16,17,18,16,18,19,
	20,21,22,20,22,23
};

void buildCubeMesh( CubeMesh * cube_mesh_ )
{
	CubeFace cube_faces[6];
	buildCubeFaces( cube_faces );
	//
	for( int iFace = 0; iFace < 6; iFace++ )
	{
		const CubeFace& cube_face = cube_faces[ iFace ];

		cube_mesh_->positions[ iFace*4 + 0 ] = cube_face.xyz[ 0 ];
		cube_mesh_->positions[ iFace*4 + 1 ] = cube_face.xyz[ 1 ];
		cube_mesh_->positions[ iFace*4 + 2 ] = cube_face.xyz[ 2 ];
		cube_mesh_->positions[ iFace*4 + 3 ] = cube_face.xyz[ 3 ];

		cube_mesh_->normals[ iFace*4 + 0 ] = cube_face.N;
		cube_mesh_->normals[ iFace*4 + 1 ] = cube_face.N;
		cube_mesh_->normals[ iFace*4 + 2 ] = cube_face.N;
		cube_mesh_->normals[ iFace*4 + 3 ] = cube_face.N;

		cube_mesh_->UVs[ iFace*4 + 0 ] = cube_face.UVs[ 0 ];
		cube_mesh_->UVs[ iFace*4 + 1 ] = cube_face.UVs[ 1 ];
		cube_mesh_->UVs[ iFace*4 + 2 ] = cube_face.UVs[ 2 ];
		cube_mesh_->UVs[ iFace*4 + 3 ] = cube_face.UVs[ 3 ];
	}
}

#if 0
static
void dbg_getCubeSubmeshes( TInplaceArray< Submesh, 8 > &submeshes_ )
{
	// Face -X
	Submesh	submesh0;
	submesh0.start_index = 0;
	submesh0.index_count = 3;
	new_mesh->_submeshes.add( submesh0 );
	new_mesh->_default_materials.add( default_material );

	// Other faces
	Submesh	submesh1;
	submesh1.start_index = 3;
	submesh1.index_count = 27;
	new_mesh->_submeshes.add( submesh1 );
	new_mesh->_default_materials.add( uv_test_material );

	// Face +Z
	Submesh	submesh2;
	submesh2.start_index = 30;
	submesh2.index_count = 6;
	new_mesh->_submeshes.add( submesh2 );
	new_mesh->_default_materials.add( default_material );


	//
	RawTriMesh::Part &	submesh0 = raw_trimesh_->parts[0];
	submesh0.start_triangle = 0;
	submesh0.triangle_count = 1;
	submesh0.material_id = 0;	// MakeAssetID("material_default")
	submesh0.texture_id = 0;

	// Other faces
	RawTriMesh::Part &	submesh1 = raw_trimesh_->parts[1];
	submesh1.start_triangle = 1;
	submesh1.triangle_count = 9;
	submesh1.material_id = 1;	// MakeAssetID("uv-test")
	submesh1.texture_id = 1;

	// Face +Z
	RawTriMesh::Part &	submesh2 = raw_trimesh_->parts[2];
	submesh2.start_triangle = 10;
	submesh2.triangle_count = 2;
	submesh2.material_id = 0;	// MakeAssetID("material_default")
	submesh2.texture_id = 0;
}
#endif

ERet buildCubeMesh( RawTriMesh *raw_trimesh_ )
{
	CubeMesh	cube_mesh;
	buildCubeMesh( &cube_mesh );

	//
	mxDO(raw_trimesh_->vertices.setNum(24));
	for( int iVertex = 0; iVertex < 24; iVertex++ )
	{
		RawTriMesh::Vertex &	dst_vertex = raw_trimesh_->vertices[ iVertex ];

		dst_vertex.xyz = cube_mesh.positions[ iVertex ];
		dst_vertex.uv = cube_mesh.UVs[ iVertex ];
		dst_vertex.N = cube_mesh.normals[ iVertex ];
	}

	//
	const UINT num_triangles = mxCOUNT_OF(g_cube_mesh_indices) / 3;
	mxDO(raw_trimesh_->triangles.setNum( num_triangles ));
	for( int iTriangle = 0; iTriangle < num_triangles; iTriangle++ )
	{
		RawTriMesh::Triangle &	dst_triangle = raw_trimesh_->triangles[ iTriangle ];
		dst_triangle.vertex_indices[0] = g_cube_mesh_indices[ iTriangle*3 + 0 ];
		dst_triangle.vertex_indices[1] = g_cube_mesh_indices[ iTriangle*3 + 1 ];
		dst_triangle.vertex_indices[2] = g_cube_mesh_indices[ iTriangle*3 + 2 ];
	}

	//
#if 0	
	mxDO(raw_trimesh_->parts.setNum(1));

	//
	RawTriMesh::Part &	submesh0 = raw_trimesh_->parts[0];
	submesh0.start_triangle = 0;
	submesh0.triangle_count = raw_trimesh_->triangles.num();
	submesh0.material_id = 1;	// MakeAssetID("uv-test")
	submesh0.texture_id = 1;	// MakeAssetID("uv-test")

#else
	mxDO(raw_trimesh_->parts.setNum(3));
	// Face -X
	RawTriMesh::Part &	submesh0 = raw_trimesh_->parts[0];
	submesh0.start_triangle = 0;
	submesh0.triangle_count = 1;
	submesh0.material_id = 0;	// MakeAssetID("material_default")
	submesh0.texture_id = 0;

	// Other faces
	RawTriMesh::Part &	submesh1 = raw_trimesh_->parts[1];
	submesh1.start_triangle = 1;
	submesh1.triangle_count = 9;
	submesh1.material_id = 1;	// MakeAssetID("uv-test")
	submesh1.texture_id = 1;

	// Face +Z
	RawTriMesh::Part &	submesh2 = raw_trimesh_->parts[2];
	submesh2.start_triangle = 10;
	submesh2.triangle_count = 2;
	submesh2.material_id = 0;	// MakeAssetID("material_default")
	submesh2.texture_id = 0;
#endif

	//
	raw_trimesh_->local_AABB.min_corner = CV3f(-0.5f);
	raw_trimesh_->local_AABB.max_corner = CV3f(+0.5f);

	return ALL_OK;
}

#if 0
ERet createCubeMesh(
					NwMesh::Ref &mesh_
					, Clump & storage
					)
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	//
	NwMesh	*new_mesh;
	mxDO(storage.New( new_mesh ));
	mesh_ = new_mesh;

	//
	CubeMesh	cube_mesh;
	buildCubeMesh( &cube_mesh );

	//
	DrawVertex	draw_vertices[24];
	for( int iVertex = 0; iVertex < 24; iVertex++ )
	{
		DrawVertex &	draw_vertex = draw_vertices[ iVertex ];

		draw_vertex.xyz = cube_mesh.positions[ iVertex ];
		draw_vertex.uv = V2_To_Half2( cube_mesh.UVs[ iVertex ] );
		draw_vertex.N = PackNormal( cube_mesh.normals[ iVertex ] );
	}

	//
	new_mesh->geometry_buffers.updateWithRawData(
		draw_vertices, sizeof(draw_vertices),
		g_cube_mesh_indices, sizeof(g_cube_mesh_indices)
		);

	//
	Material *	default_material;
	Material * 	uv_test_material;

	mxDO(Resources::Load( default_material, MakeAssetID("material_default"), &storage ));
	mxDO(Resources::Load( uv_test_material, MakeAssetID("uv-test"), &storage ));


	// Face -X
	Submesh	submesh0;
	submesh0.start_index = 0;
	submesh0.index_count = 3;
	new_mesh->_submeshes.add( submesh0 );
	new_mesh->_default_materials.add( default_material );

	// Other faces
	Submesh	submesh1;
	submesh1.start_index = 3;
	submesh1.index_count = 27;
	new_mesh->_submeshes.add( submesh1 );
	new_mesh->_default_materials.add( uv_test_material );

	// Face +Z
	Submesh	submesh2;
	submesh2.start_index = 30;
	submesh2.index_count = 6;
	new_mesh->_submeshes.add( submesh2 );
	new_mesh->_default_materials.add( default_material );


	//
	new_mesh->m_flags = 0;

	new_mesh->m_numVertices = 24;
	new_mesh->m_numIndices = 36;

	new_mesh->vertex_format = &DrawVertex::metaClass();

	new_mesh->m_topology = Topology::TriangleList;

	new_mesh->local_aabb.clear();

	return ALL_OK;
}
#endif
}//namespace CubeMeshUtil
