#include "D:/research/_RF1/rf-reversed/v3d_format.h"

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_ModelLoader.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Experimental.h>


#define DEBUG_V3D_LOADER	(1)


namespace RF1
{

static char	gs_temp_buffer[mxKiB(4)];	// used for skipping - allows to inspect skipped data


#pragma pack(push,1)

struct v3d_bounding_sphere_t
{
	V3f		center; // bounding sphere position
    float	radius; // bounding sphere radius
};

struct v3d_batch_header_old // size: 56 (0x38)
{ // this is not used by RF - only read and then over-written by values from v3d_batch_info
    //char unknown[56];
	char unknown[40];
    uint16_t num_vertices; // see v3d_batch_info
    uint16_t num_triangles; // see v3d_batch_info
    uint16_t positions_size; // see v3d_batch_info
    uint16_t indices_size; // see v3d_batch_info
    uint16_t unknown_size; // see v3d_batch_info
	uint16_t bone_links_size; // see v3d_batch_info
	uint16_t tex_coords_size; // see v3d_batch_info
    uint16_t unknown2;
};
ASSERT_SIZEOF(v3d_batch_header_old, 56);
ASSERT_SIZEOF(v3d_batch_header_old, sizeof(v3d_batch_header));



struct v3d_triangle_t
{
    uint16_t indices[3];
    uint16_t flags; // v3d_triangle_flags
};
ASSERT_SIZEOF(v3d_triangle_t, 8);

struct v3d_vertex_bones_t
{
    // One vertex can be linked maximaly to 4 bones
    uint8_t weights[4]; // in range 0-255, 0 if slot is unused
    uint8_t bones[4]; // bone indexes, 0xFF if slot is unused

public:
	bool IsValid() const
	{
		for( int i = 0; i < 4; i++ )
		{
			if( bones[i] != 0xFF ) {
				return bones[i] < MAX_BONES;
			}
		}
		return true;
	}
};

/// Bone positions are already in model/object space (in the bind-pose) and independent of other bones.
struct V3D_Bone
{
    char name[24];
	float	rot[4];
    V3f	pos; // bone to model translation
    int32_t parent; // bone index (root has -1)
};
ASSERT_SIZEOF(V3D_Bone, 24+16+12+4);

struct v3d_col_sphere_t
{
    char name[24];
    int32_t bone; // bone index or -1
    V3f	rel_pos; // position relative to bone
    float r; // radius
};

#pragma pack (pop)



static
ERet readNullTerminatedString(
							  String *str_
							  , FileReader & reader
							  )
{
	str_->empty();

	unsigned char	c;

	for(;;)
	{
		mxDO(reader.Get( c ));

		mxDO(Str::Append( *str_, c ));

		if( '\0' == c ) {
			break;
		}
	}

	return ALL_OK;
}

#pragma pack(push,1)
struct v3d_lod_prop_t // size = 0x64 (not verified if its a prop point)
{
	char name[0x44]; // for example "thruster_11"
	float unknown[7]; // pos + rot?
	int32_t unknown2; // -1
};
#pragma pack(pop)
ASSERT_SIZEOF(v3d_lod_prop_t, 100);

struct SubmeshLodHeader
{
	uint32_t flags; // see v3d_lod_flags: 0x1|0x02 - characters, 0x20 - static meshes, 0x10 only driller01.v3m
	uint32_t unknown0;	// unknown, can be 0
};

static
ERet skipPadding( AReader & reader, const U32 start_offset )
{
	const U32 curr_offset = reader.Tell();

	U32 overshoot = ( curr_offset - start_offset ) % V3D_ALIGNMENT;
	if( overshoot ) {
		const U32 num_bytes_to_skip = V3D_ALIGNMENT - overshoot;
		mxDO(Stream_Skip( reader, num_bytes_to_skip, gs_temp_buffer, sizeof(gs_temp_buffer) ));
	}

	return ALL_OK;
}

static
ERet read(
		  AReader & reader
		  , void* dst_buf, size_t size
		  )
{
	return reader.Read( dst_buf, size );
}

static
ERet read16(
			AReader & reader
			, void* dst_buf, size_t size
			)
{
	const size_t	aligned_size = tbALIGN( size, V3D_ALIGNMENT );
	return reader.Read( dst_buf, aligned_size );
}


static U32 findStartOfNormalsData(
								  NwBlobReader & reader
								  )
{
	const size_t	current_offset = reader.Tell();
	mxASSERT(IS_16_BYTE_ALIGNED(current_offset));

	for( int shift_in_bytes = 0; shift_in_bytes < 4; shift_in_bytes++ )
	{
		const V3f* vecs = (V3f*) mxAddByteOffset(
			reader.bufferStart(),
			current_offset + shift_in_bytes
			);

		const size_t	bytes_remaining = reader.Size() - current_offset - shift_in_bytes;
		const int		vectors_remaining = bytes_remaining / sizeof(vecs[0]);

		for( int i = 0; i < vectors_remaining; i++ )
		{
			const V3f& v = vecs[i];
			if( V3_IsNormalized(v) ) {
				return current_offset + i * sizeof(vecs[0]);
			}
		}
	}

	mxUNREACHABLE;
	return current_offset;
}

static bool isValidVector( const V3f& v )
{
	return IsValidFloat(v.x)
		&& IsValidFloat(v.y)
		&& IsValidFloat(v.z)
		;
}

static bool isValidPosition( const V3f& v
							, const AABBf& aabb
							)
{
	return isValidVector(v)
		&& aabb.containsPoint(v)
		;
}

static bool isValidNormal( const V3f& v )
{
	return isValidVector(v)
		&& V3_IsNormalized(v)
		;
}

static U32 findStartOfPositionData(
								   NwBlobReader & reader
								   , const AABBf& aabb
								   , const U32 num_vertices
								   )
{
	const U32	normals_offset = findStartOfNormalsData( reader );
	
	return normals_offset - tbALIGN16(num_vertices * sizeof(V3f));
}

///
static ERet parse_batch_data(
							 NwBlobReader & reader
							 , const AABBf& aabb
							 , const SubmeshLodHeader& submesh_lod_header
							 , const v3d_batch_header_old& batch_header
							 , const v3d_batch_info& batch_info
							 , const U32 start_offset_in_file
							 , TestSkinnedModel * test_model
							 )
{
	//
	const U32	num_vertices = batch_info.num_vertices;
	const U32	num_triangles = batch_info.num_triangles;

	mxHACK("if invalid data at current offset, advance by 16 bytes forward");
	//
	const char* normals_data = mxAddByteOffset( reader.GetPtr(), tbALIGN16( num_vertices * sizeof(V3f) ) );

	const V3f	maybe_position = RF1::toMyVec3( *(V3f*)reader.GetPtr() );
	if(
		!isValidPosition( maybe_position, aabb )
		||
		!isValidNormal( *(V3f*)normals_data )
		)
	{
		const U32	current_offset = reader.Tell();
		const U32	found_positions_offset = findStartOfPositionData( reader, aabb, num_vertices );
		mxASSERT(found_positions_offset > current_offset);

		ptWARN("Achtung! Expected positions @ %u, but found them @ %u",
			start_offset_in_file + current_offset,
			start_offset_in_file + found_positions_offset
			);

		//
		V4f	pad16;
		reader.Get(pad16);
	}
	


	//
	const size_t	positions_offset = start_offset_in_file + reader.Tell();
	V3f positions[ MAX_SUBMESH_VERTICES ];
	mxZERO_OUT(positions);
	mxENSURE(num_vertices <= mxCOUNT_OF(positions), ERR_BUFFER_TOO_SMALL, "");
	mxDO(read16(
		reader,
		positions,
		sizeof(positions[0]) * num_vertices
		));

	for( UINT i = 0; i < num_vertices; i++ ) {
		positions[i] = RF1::toMyVec3( positions[i] );
	}

	ptPRINT("\t Positions start @ %u; (%f, %f, %f), ...",
		positions_offset,
		positions[0].x, positions[0].y, positions[0].z
		);

	//
#if DEBUG_V3D_LOADER

	//
	for( UINT iVertex = 0; iVertex < num_vertices; iVertex++ )
	{
		const V3f& pos = positions[ iVertex ];
		mxASSERT(
			aabb.containsPoint(pos)
			);
	}

#endif // DEBUG_V3D_LOADER

	mxASSERT(
		batch_header.positions_size ==
		tbALIGN16(sizeof(positions[0]) * num_vertices)
		);

	//
	const size_t	normals_offset = start_offset_in_file + reader.Tell();
	V3f normals[ MAX_SUBMESH_VERTICES ];
	mxZERO_OUT(normals);
	mxDO(read16(
		reader,
		normals,
		sizeof(normals[0]) * num_vertices
		));
	for( UINT i = 0; i < num_vertices; i++ ) {
		normals[i] = RF1::toMyVec3( normals[i] );
	}

	ptPRINT("\t Normals start @ %u; (%f, %f, %f), ...",
		normals_offset,
		normals[0].x, normals[0].y, normals[0].z
		);




	//
	const size_t	tex_coords_offset = start_offset_in_file + reader.Tell();
	V2f tex_coords[ MAX_SUBMESH_VERTICES ];
	mxZERO_OUT(tex_coords);
	mxDO(read16(
		reader,
		tex_coords,
		sizeof(tex_coords[0]) * num_vertices
		));

	ptPRINT("\t TexCoords start @ %u; (%f, %f), ...",
		tex_coords_offset,
		tex_coords[0].x, tex_coords[0].y
		);




	//
	const size_t	triangles_offset = start_offset_in_file + reader.Tell();
	v3d_triangle_t triangles[ MAX_SUBMESH_TRIANGLES ];
	mxZERO_OUT(triangles);
	mxENSURE(num_triangles <= mxCOUNT_OF(triangles), ERR_BUFFER_TOO_SMALL, "");
	mxDO(read16(
		reader,
		triangles,
		sizeof(triangles[0]) * num_triangles
		));

	//mxASSERT(
	//	batch_header.indices_size ==
	//	tbALIGN16(sizeof(triangles[0]) * num_triangles)
	//	);
	ptPRINT("\t Triangles start @ %u; (%u, %u, %u), ...",
		triangles_offset,
		triangles[0].indices[0], triangles[0].indices[1], triangles[0].indices[2]
		);



#if DEBUG_V3D_LOADER

	//
	for( UINT iNormal = 0; iNormal < num_vertices; iNormal++ )
	{
		float len = V3_Length( normals[ iNormal ] );
		bool is_normalized = mmAbs( len - 1.0f ) < 1e-4f;
		if( !is_normalized ) {
			ptWARN("Normal[%u] is not normalized! Length = %f.", iNormal, len);
		}
		mxASSERT( is_normalized );
	}

	//
	for( UINT iTriangle = 0; iTriangle < num_triangles; iTriangle++ )
	{
		const v3d_triangle_t& tri = triangles[ iTriangle ];

		const UINT i0 = tri.indices[ 0 ];
		const UINT i1 = tri.indices[ 1 ];
		const UINT i2 = tri.indices[ 2 ];

		mxASSERT(i0 < num_vertices);
		mxASSERT(i1 < num_vertices);
		mxASSERT(i2 < num_vertices);
	}

#endif // DEBUG_V3D_LOADER







	//
	V4f	triangle_planes[ MAX_SUBMESH_TRIANGLES ];
	mxZERO_OUT(triangle_planes);
	if( submesh_lod_header.flags & V3D_LOD_TRIANGLE_PLANES )
	{
		mxENSURE(num_triangles <= mxCOUNT_OF(triangle_planes), ERR_BUFFER_TOO_SMALL, "");
		mxDO(read16(
			reader,
			triangle_planes,
			sizeof(triangle_planes[0]) * num_triangles
			));
	}




	const size_t	vertex_offsets_size = num_vertices * sizeof(U16);
	mxDO(Stream_Skip(
		reader
		, tbALIGN16(vertex_offsets_size)
		, gs_temp_buffer, sizeof(gs_temp_buffer)
		));

	//
	ptPRINT("\t Bone Links start @ %u.",
		start_offset_in_file + reader.Tell()
		);
	//
	v3d_vertex_bones_t	bone_links[ MAX_SUBMESH_VERTICES ];
	mxZERO_OUT(bone_links);
	mxENSURE(num_vertices <= mxCOUNT_OF(bone_links), ERR_BUFFER_TOO_SMALL, "");
	if( batch_header.bone_links_size )
	{
		mxDO(read16(
			reader,
			bone_links,
			sizeof(bone_links[0]) * num_vertices
			));
		//
		for( UINT iBoneLink = 0; iBoneLink < num_vertices; iBoneLink++ )
		{
			const v3d_vertex_bones_t& bone_link = bone_links[ iBoneLink ];

			const bool bone_is_valid = bone_link.IsValid();
			if( !bone_is_valid )
			{
				ptPRINT("\t BoneLink[%u]: weights: (%u, %u, %u, %u), bones: (%u, %u, %u, %u)",
					iBoneLink,
					bone_link.weights[0], bone_link.weights[1], bone_link.weights[2], bone_link.weights[3],
					bone_link.bones[0], bone_link.bones[1], bone_link.bones[2], bone_link.bones[3]
				);
				mxASSERT(0);
			}
		}
	}


	//
	if( submesh_lod_header.flags & 0x1 )
	{
		char	unknown_data[ MAX_UNKNOWN_DATA_SIZE ];
		mxZERO_OUT(unknown_data);

		const size_t	unknown_elems_size = submesh_lod_header.unknown0 * 2 * sizeof(unknown_data[0]);
		mxASSERT(unknown_elems_size);
		mxENSURE(unknown_elems_size <= sizeof(unknown_data), ERR_BUFFER_TOO_SMALL, "");

		const size_t	aligned_unknown_elems_size = tbALIGN16( unknown_elems_size );

		//
		const size_t	current_offset_in_file = start_offset_in_file + reader.Tell();
		ptPRINT("\t Unknown LoD data starts at @ %u (size = %u bytes). Next batch positions start @ %u",
			current_offset_in_file
			, unknown_elems_size
			, current_offset_in_file + aligned_unknown_elems_size
			);

		mxDO(read16(
			reader,
			unknown_data,
			aligned_unknown_elems_size
			));
	}


	//
	const U32	num_vertices_written = test_model->_vertices.num();

	mxDO(test_model->_vertices.setNum( num_vertices_written + num_vertices ));

	for( UINT iVertex = 0; iVertex < num_vertices; iVertex++ )
	{
		const V3f& pos = positions[ iVertex ];
		//
		TestSkinnedModel::Vertex &dst_vertex = test_model->_vertices[ num_vertices_written + iVertex ];
		dst_vertex.pos = pos;
		//dst_vertex.bone_indices = pos;
		//dst_vertex.bone_weights = pos;
	}

	//
	const U32	num_triangles_written = test_model->_triangles.num();
	mxDO(test_model->_triangles.setNum( num_triangles_written + num_triangles ));

	for( UINT iTriangle = 0; iTriangle < num_triangles; iTriangle++ )
	{
		const v3d_triangle_t& src_tri = triangles[ iTriangle ];
		UInt3 &dst_tri = test_model->_triangles[ num_triangles_written + iTriangle ];
		//
		dst_tri.a[0] = src_tri.indices[0] + num_vertices_written;
		dst_tri.a[1] = src_tri.indices[1] + num_vertices_written;
		dst_tri.a[2] = src_tri.indices[2] + num_vertices_written;
	}

	//
	TestSkinnedModel::Submesh	new_submesh;
	test_model->_submeshes.add( new_submesh );

	return ALL_OK;
}

///
static ERet parse_LoD_mesh_data(
								NwBlobReader & reader
								, const AABBf& aabb
								, const SubmeshLodHeader& submesh_lod_header
								, const U32	start_offset_in_file
								, const TSpan< v3d_batch_info >& batch_infos
								, TestSkinnedModel * test_model
								)
{
	const uint16_t num_batches = batch_infos._count;

	//
	v3d_batch_header_old	batch_headers[ MAX_SUBMESH_BATCHES ];
	mxENSURE(num_batches <= MAX_SUBMESH_BATCHES, ERR_BUFFER_TOO_SMALL, "");
	mxZERO_OUT(batch_headers);

	for( UINT iBatch = 0; iBatch < num_batches; iBatch++ )
	{
		mxDO(reader.Get(batch_headers[ iBatch ]));

		//
		const v3d_batch_header_old& batch_header = batch_headers[ iBatch ];
		//DBGOUT("BatchHdr[%u]: num_vertices=%u, num_triangles=%u, positions_size=%u, indices_size=%u, bone_links_size=%u, tex_coords_size=%u",
		//	iBatch,
		//	batch_header.num_vertices, batch_header.num_triangles,
		//	batch_header.positions_size, batch_header.indices_size,
		//	batch_header.bone_links_size, batch_header.tex_coords_size
		//	);

		const v3d_batch_info& batch_info = batch_infos[ iBatch ];

		mxASSERT( batch_header.num_vertices == batch_info.num_vertices );
		mxASSERT( batch_header.num_triangles == batch_info.num_triangles );
		mxASSERT( batch_header.positions_size == batch_info.positions_size );
		mxASSERT( batch_header.indices_size == batch_info.indices_size );
		mxASSERT( batch_header.bone_links_size == batch_info.bone_links_size );
//		mxASSERT( batch_header.tex_coords_size == batch_info.tex_coords_size );

	}//For each batch.

	//
	mxDO(skipPadding( reader, 0 ));

	//
	for( UINT iBatch = 0; iBatch < num_batches; iBatch++ )
	{
		const v3d_batch_header_old& batch_header = batch_headers[ iBatch ];
		const v3d_batch_info& batch_info = batch_infos[ iBatch ];

		DBGOUT("\n==== Parsing Batch [%u] (offset: %u)...",
			iBatch,
			start_offset_in_file + reader.Tell()
			);

		//
		mxDO(parse_batch_data(
			reader
			, aabb
			, submesh_lod_header
			, batch_header
			, batch_info
			, start_offset_in_file
			, test_model
			));

		//
		const U32 curr_offset_in_file = start_offset_in_file + reader.Tell();
		mxASSERT(IS_16_BYTE_ALIGNED(reader.Tell()));
DBGOUT("\n==== After Parsing Batch (offset: %u)", curr_offset_in_file);

	}//For each batch.

	//

	return ALL_OK;
}

///
static ERet parse_submesh_LoD(
						FileReader & reader
						, const AABBf& aabb
						, TestSkinnedModel * test_model
						)
{
    SubmeshLodHeader submesh_lod_header;
	mxDO(reader.Get(submesh_lod_header));

	//
	uint16_t num_batches;
	mxDO(reader.Get(num_batches));
	mxASSERT(num_batches > 0);

	//
    uint32_t mesh_data_size;
	mxDO(reader.Get(mesh_data_size));

	//
	const U32	lod_mesh_data_offset = reader.Tell();

	//
	NwBlob	lod_mesh_data( MemoryHeaps::temporary() );
	mxDO(lod_mesh_data.setNum( mesh_data_size ));
	mxDO(reader.Read( lod_mesh_data.raw(), mesh_data_size ));

	//
	I32	unknown1; // -1, sometimes 0
	mxDO(reader.Get(unknown1));
	mxASSERT(unknown1 == -1 || unknown1 == 0);

	//
	v3d_batch_info	batch_infos[ MAX_SUBMESH_BATCHES ];
	mxENSURE(num_batches <= mxCOUNT_OF(batch_infos), ERR_BUFFER_TOO_SMALL, "");
	mxZERO_OUT(batch_infos);
	for( UINT iBatch = 0; iBatch < num_batches; iBatch++ )
	{
		v3d_batch_info & batch_info = batch_infos[ iBatch ];
		mxDO(reader.Get( batch_info ));
		//
		DBGOUT("BatchInfo[%u]: num_vertices=%u, num_triangles=%u, positions_size=%u, indices_size=%u, bone_links_size=%u, tex_coords_size=%u",
			iBatch,
			batch_info.num_vertices, batch_info.num_triangles,
			batch_info.positions_size, batch_info.indices_size,
			batch_info.bone_links_size, batch_info.tex_coords_size
			);
		//
		mxASSERT( batch_info.render_flags == 0x518C41 || batch_info.render_flags == 0x110C21 );
	}//For each batch.

	//
	uint32_t num_prop_points;	// number of prop points, usually 0 or 1
	mxDO(reader.Get(num_prop_points));
	mxASSERT(num_prop_points < MAX_SOCKETS);

	//
	uint32_t textures_count;
	mxDO(reader.Get(textures_count));

	for( UINT iTexture = 0; iTexture < textures_count; iTexture++ )
	{
		U8	id;
		mxDO(reader.Get( id ));

		//
		String32	diffuse_map_name;
		mxDO(readNullTerminatedString(
			&diffuse_map_name
			, reader
			));

		DBGOUT("Read texture '%s'.", diffuse_map_name.c_str());
	}//For each texture.

	//
	NwBlobReader	mesh_data_reader( lod_mesh_data );
	mxDO(parse_LoD_mesh_data(
		mesh_data_reader
		, aabb
		, submesh_lod_header
		, lod_mesh_data_offset
		, TSpan< v3d_batch_info >( batch_infos, num_batches )
		, test_model
		));

	return ALL_OK;
}

// see v3d_submesh_t
static ERet parse_submesh(
						FileReader & reader
						, TestSkinnedModel * test_model
						)
{
	char name[24]; // object name in 3ds max - zero terminated string
	mxDO(reader.Get(name));

	char unknown[24]; // "None" or duplicated name if object belongs to 3ds max group
	mxDO(reader.Get(unknown));

	uint32_t version; // 7 (submesh ver?) values < 7 doesnt work
	mxDO(reader.Get(version));

	//
	uint32_t lod_models_count; // 1 - 3
	mxDO(reader.Get(lod_models_count));

	DBGOUT("%d LoDs", lod_models_count);

	//
	mxENSURE(lod_models_count <= MAX_LOD_DISTANCES, ERR_BUFFER_TOO_SMALL, "");

	float lod_distances[ MAX_LOD_DISTANCES ]; // 0.0, 10.0
	mxZERO_OUT(lod_distances);
	for( UINT iLoD_model = 0; iLoD_model < lod_models_count; iLoD_model++ )
	{
		mxDO(reader.Get(lod_distances[ iLoD_model ]));
		DBGOUT("LoD distance [%u] = %.5f", iLoD_model, lod_distances[ iLoD_model ]);
	}

	//
	v3d_bounding_sphere_t bsphere; // bounding sphere
	mxDO(reader.Get(bsphere));
	bsphere.center = RF1::toMyVec3( bsphere.center );

	//
	AABBf	aabb; // axis aligned bounding box
	mxDO(reader.Get(aabb));
	aabb.min_corner = RF1::toMyVec3( aabb.min_corner );
	aabb.max_corner = RF1::toMyVec3( aabb.max_corner );

	//
	for( UINT iLoD_model = 0; iLoD_model < lod_models_count; iLoD_model++ )
	{
		DBGOUT("\n\n==== Parsing LoD [%u]...", iLoD_model);
		mxDO(parse_submesh_LoD(
			reader
			, aabb
			, test_model
			));
	}

	//
	uint32_t num_materials;
	mxDO(reader.Get(num_materials));

	v3d_material materials[ MAX_SUBMESH_MATERIALS ];
	mxZERO_OUT(materials);

	mxENSURE(num_materials <= mxCOUNT_OF(materials), ERR_BUFFER_TOO_SMALL, "");

	for( UINT iMaterial = 0; iMaterial < num_materials; iMaterial++ )
	{
		mxDO(reader.Get(materials[ iMaterial ]));

		DBGOUT("Material[%u]: '%s'", iMaterial, materials[ iMaterial ].diffuse_map_name);
	}

	//
	uint32_t unknown3; // always 1 in game files
	mxDO(reader.Get(unknown3));
	mxASSERT(unknown3 == 1);

	struct
	{
		char unknown[24]; // same as submesh name
		float unknown2; // always 0
	} unknown4[1];
	mxDO(reader.Get(unknown4));

	return ALL_OK;
}

///
static ERet parse_bones(
						FileReader & reader
						, TestSkinnedModel * test_model
						)
{
	uint32_t num_bones;
	mxDO(reader.Get(num_bones));

	//
	V3D_Bone bones[ MAX_BONES ]; // 0.0, 10.0
	mxENSURE(num_bones <= mxCOUNT_OF(bones), ERR_BUFFER_TOO_SMALL, "");
	mxZERO_OUT(bones);

	mxDO(test_model->_bones.setCountExactly( num_bones ));

	for( UINT iBone = 0; iBone < num_bones; iBone++ )
	{
		V3D_Bone & bone = bones[ iBone ];
		mxDO(reader.Get( bone ));

		//
		const V3f	bone_pos = RF1::toMyVec3( bone.pos );
		const Q4f	bone_quat = RF1::removeFlip( RF1::toMyQuat( Q4f::fromRawFloatPtr( bone.rot ) ) );

		//
		DEVOUT("Bone[%u]: '%s': parent=%d, rot=(%.3f, %.3f, %.3f, %.3f), pos=(%.3f, %.3f, %.3f)",
			iBone, bone.name, bone.parent,
			bone_quat.x, bone_quat.y, bone_quat.z, bone_quat.w,
			bone_pos.x, bone_pos.y, bone_pos.z
			);

		DEVOUT("\t Orientation: %s.", bone_quat.humanReadableString().c_str() );

		//
		TestSkinnedModel::Bone &dst_bone = test_model->_bones._data[ iBone ];
		dst_bone.orientation = bone_quat;
		dst_bone.position = bone_pos;

		dst_bone.parent = bone.parent;
		Str::CopyS( dst_bone.name, bone.name );

		//

		//
		dst_bone.obj_space_joint_mat = buildMat4(
			bone_quat
			, bone_pos
			);
	}//For each bone.


	DEVOUT("\n=== Relative bone orientations:\n");

	for( UINT iBone = 0; iBone < num_bones; iBone++ )
	{
		TestSkinnedModel::Bone & joint = test_model->_bones._data[ iBone ];

		const Q4f	bone_quat_obj_space = joint.orientation;

		if( joint.parent >= 0 )
		{
			const TestSkinnedModel::Bone& parent_joint = test_model->_bones._data[ joint.parent ];

			const Q4f		inverse_orientation_of_parent_joint = parent_joint.orientation.inverse();

			const Q4f		bone_quat_parent_space = Q4f::concat(
				inverse_orientation_of_parent_joint,
				bone_quat_obj_space
				);

			const V3f	joint_position_relative_to_parent = inverse_orientation_of_parent_joint
				.rotateVector( joint.position - parent_joint.position )
				;

			DEVOUT("Bone[%u]: '%s': parent=%d, rel_Q: (%f, %f, %f, %f), rel_P: (%f, %f, %f)",
				iBone, joint.name.c_str(), joint.parent,
				bone_quat_parent_space.x, bone_quat_parent_space.y, bone_quat_parent_space.z, bone_quat_parent_space.w,
				joint_position_relative_to_parent.x, joint_position_relative_to_parent.y, joint_position_relative_to_parent.z
				);
			DEVOUT("\t Rel. Orientation: %s.", bone_quat_parent_space.humanReadableString().c_str() );



			//
			const Q4f	restored_bone_quat_in_obj_space = Q4f::concat(
				parent_joint.orientation,
				bone_quat_parent_space
				);

			mxASSERT(restored_bone_quat_in_obj_space.equals(bone_quat_obj_space, 1e-4f));

			//
			const V3f	restored_joint_position_in_model_space
				= parent_joint.orientation.rotateVector( joint_position_relative_to_parent )
				+ parent_joint.position
				;
			mxASSERT(restored_joint_position_in_model_space.equals(joint.position, 1e-4f));
		}
		else
		{
			DEVOUT("Bone[%u]: '%s' is Root! Orientation: (%f, %f, %f, %f), Pos: (%f, %f, %f)",
				iBone, joint.name.c_str(),
				bone_quat_obj_space.x, bone_quat_obj_space.y, bone_quat_obj_space.z, bone_quat_obj_space.w,
				joint.position.x, joint.position.y, joint.position.z
				);
			DEVOUT("\t Orientation: %s.", bone_quat_obj_space.humanReadableString().c_str() );
		}

	}//For each bone.

	return ALL_OK;
}

///
static ERet parse_collision_sphere(
						FileReader & reader
						)
{
	v3d_col_sphere_t	col_sphere;
	mxDO(reader.Get( col_sphere ));

	col_sphere.rel_pos = RF1::toMyVec3( col_sphere.rel_pos );

	//
	DBGOUT("colsphere: '%s': bone=%d, pos=(%.3f, %.3f, %.3f), radius=%.3f",
		col_sphere.name, col_sphere.bone,
		col_sphere.rel_pos.x, col_sphere.rel_pos.y, col_sphere.rel_pos.z,
		col_sphere.r
	);

	return ALL_OK;
}

ERet load_V3D_from_file(
						TcModel *model_
						, const char* filename
						, ObjectAllocatorI * storage
						, TestSkinnedModel * test_model
						)
{
	DBGOUT("Loading '%s'...", filename);

	//
	FileReader	file;
	mxDO(file.Open(filename));

	//
	v3d_file_header	header;
	mxDO(file.Get(header));

	//
	if( header.signature == V3M_SIGNATURE ) {
		DBGOUT("This is a static mesh!");
	} else {
		DBGOUT("This must be a skinned mesh!");
	}

	//
	mxDO(test_model->_submeshes.ReserveExactly(
		header.num_submeshes
		));
	mxDO(test_model->_vertices.ReserveExactly(
		header.num_all_vertices
		));
	mxDO(test_model->_triangles.ReserveExactly(
		header.num_all_triangles
		));

	//
	UINT	num_parsed_submeshes = 0;

	//
	for(;;)
	{
		v3d_section_header	section_header;
		mxDO(file.Get(section_header));

		const char*	t = (char*) &section_header.type;

		switch( section_header.type )
		{
		case V3D_SUBMESH   :
			DBGOUT("\n==== Parsing Submesh [%u]...", num_parsed_submeshes++);
			mxDO(parse_submesh(
				file
				, test_model
				));
			continue;

		case V3D_COLSPHERE :
			DBGOUT("\n==== Collision spheres...");
			mxDO(parse_collision_sphere( file ));
			continue;

		case V3D_BONE      :
			DBGOUT("\n==== Parsing Bones...");
			mxDO(parse_bones(
				file
				, test_model
				));
			continue;

		case V3D_DUMB      :
			Stream_Skip( file, section_header.size, gs_temp_buffer, sizeof(gs_temp_buffer) );
			continue;

		case V3D_END       :
			DBGOUT("EOF reached.");
			goto L_End;

		default:
			DBGOUT("Unknown section type '%u' at offset %lu.", (UINT)section_header.type, file.Tell());
			goto L_End;
		}
	}//For each section.

L_End:
	;

	if( test_model->isSkinned() )
	{
		test_model->computeBindPoseJoints();
	}

	return ALL_OK;
}

}//namespace RF1
