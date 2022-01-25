// Loader for level files (*.rfl) from Red Faction 1.

// for std::sort()
#include <algorithm>

#include <Base/Base.h>

#include "RedFaction_LevelLoader.h"
#include <RedFaction/rfl_format.h>


static char	gs_temp_buffer[mxKiB(4)];	// used for skipping - allows to inspect skipped data

mxDEFINE_CLASS(RFL_LevelGeo);
mxBEGIN_REFLECTION(RFL_LevelGeo)
mxEND_REFLECTION

mxDEFINE_CLASS(RFL_Level);
mxBEGIN_REFLECTION(RFL_Level)
mxEND_REFLECTION


const char* ELevelSection_To_Chars( const rfl_section_type_t e )
{
#define CASE_ENUM_TO_STR( e )	case e : return #e

	switch( e )
	{
		CASE_ENUM_TO_STR( RFL_END                   );
		CASE_ENUM_TO_STR( RFL_STATIC_GEOMETRY       );
		CASE_ENUM_TO_STR( RFL_GEO_REGIONS           );
		CASE_ENUM_TO_STR( RFL_LIGHTS                );
		CASE_ENUM_TO_STR( RFL_CUTSCENE_CAMERAS      );
		CASE_ENUM_TO_STR( RFL_AMBIENT_SOUNDS        );
		CASE_ENUM_TO_STR( RFL_EVENTS                );
		CASE_ENUM_TO_STR( RFL_MP_RESPAWNS           );
		CASE_ENUM_TO_STR( RFL_LEVEL_PROPERTIES      );
		CASE_ENUM_TO_STR( RFL_PARTICLE_EMITTERS     );
		CASE_ENUM_TO_STR( RFL_GAS_REGIONS           );
		CASE_ENUM_TO_STR( RFL_ROOM_EFFECTS          );
		CASE_ENUM_TO_STR( RFL_BOLT_EMITTERS         );
		CASE_ENUM_TO_STR( RFL_TARGETS               );
		CASE_ENUM_TO_STR( RFL_DECALS                );
		CASE_ENUM_TO_STR( RFL_PUSH_REGIONS          );
		//CASE_ENUM_TO_STR( RFL_UNKNOWN_SECT        );
		CASE_ENUM_TO_STR( RFL_MOVERS                );
		CASE_ENUM_TO_STR( RFL_MOVING_GROUPS         );
		CASE_ENUM_TO_STR( RFL_CUT_SCENE_PATH_NODES  );
		//CASE_ENUM_TO_STR( RFL_UNKNOWN_SECT        );
		CASE_ENUM_TO_STR( RFL_TGA_UNKNOWN           );
		CASE_ENUM_TO_STR( RFL_VCM_UNKNOWN           );
		CASE_ENUM_TO_STR( RFL_MVF_UNKNOWN           );
		CASE_ENUM_TO_STR( RFL_V3D_UNKNOWN           );
		CASE_ENUM_TO_STR( RFL_VFX_UNKNOWN           );
		CASE_ENUM_TO_STR( RFL_EAX_EFFECTS           );
		//CASE_ENUM_TO_STR( RFL_UNKNOWN_SECT        );
		CASE_ENUM_TO_STR( RFL_NAV_POINTS            );
		CASE_ENUM_TO_STR( RFL_ENTITIES              );
		CASE_ENUM_TO_STR( RFL_ITEMS                 );
		CASE_ENUM_TO_STR( RFL_CLUTTERS              );
		CASE_ENUM_TO_STR( RFL_TRIGGERS              );
		CASE_ENUM_TO_STR( RFL_PLAYER_START          );
		CASE_ENUM_TO_STR( RFL_LEVEL_INFO            );
		CASE_ENUM_TO_STR( RFL_BRUSHES               );
		CASE_ENUM_TO_STR( RFL_GROUPS                );
	}

#undef CASE_ENUM_TO_STR

	return "RFL_Unknown";
}

ERet RFL_Read_String( AReader& reader, String &_string )
{
	U16 length;
	mxDO(reader.Get(length));

	if( length > 0 ) {
		_string.resize(length);
		mxDO(reader.Read(_string.raw(), length));
	} else {
		_string.empty();
	}

	return ALL_OK;
}

ERet LoadStaticGeometry(
						RFL_LevelGeo &level_chunk_
						, AReader& reader
						, const RFL_Level& level	// for validation
						)
{
	// unknown
	mxDO(Stream_Skip(
		reader
		, level.file_format_version == 0xB4 ? 6 : 10
		, gs_temp_buffer, sizeof(gs_temp_buffer)
		));

	level_chunk_.bounds.clear();

	//
	U32 num_textures;
	mxDO(reader.Get(num_textures));

	mxDO(level_chunk_.materials.setNum( num_textures ));

	for( U32 iFace = 0; iFace < num_textures; ++iFace )
    {
		RFL_Material & material = level_chunk_.materials[ iFace ];

		String64	texture_id;
		mxDO(RFL_Read_String(reader, texture_id));

		ptPRINT("uses texture: '%s'", texture_id.c_str());

		material.texture._setId( make_AssetID_from_raw_string(texture_id.c_str()) );

		material.is_translucent = ( strstr( texture_id.c_str(), "gls_" ) == texture_id.c_str() );

		material.is_invisible = ( nil != strstr( texture_id.c_str(), "invisible" ) );
			//Str::EqualS(material.texture_id, "sld_invisible01.tga") ||
			//Str::EqualS(material.texture_id, "sld_invisible02.tga") ||
			//Str::EqualS(material.texture_id, "sld_invisible03.tga") ||
			//Str::EqualS(material.texture_id, "rck_invisible04.tga")
			//;
	}

	//
    U32 scroll_anim;
	mxDO(reader.Get(scroll_anim));
	mxDO(Stream_Skip(reader, scroll_anim * 12, gs_temp_buffer, sizeof(gs_temp_buffer)));

	U32 num_rooms;
	mxDO(reader.Get(num_rooms));

	ptPRINT("Loading %u rooms...\n", num_rooms);

	mxDO(level_chunk_.rooms.setNum(num_rooms));

	U32 glass_index = 0;

	for( U32 iRoom = 0; iRoom < num_rooms; ++iRoom )
	{
		RFL_Room & room = level_chunk_.rooms[ iRoom ];

		U32 id;
		mxDO(reader.Get(id));

		mxDO(reader.Get(room.bounds.min_corner));
		mxDO(reader.Get(room.bounds.max_corner));

		AABBf_Merge( room.bounds, level_chunk_.bounds, level_chunk_.bounds );

		//LogStream(LL_Debug) << "Room " << iRoom << " center: " << room.center;

		U8 is_sky;
		mxDO(reader.Get(is_sky));

		U8 is_cold;
		U8 is_outside;
		U8 is_airlock;
		mxDO(reader.Get(is_cold));
		mxDO(reader.Get(is_outside));
		mxDO(reader.Get(is_airlock));

		U8 is_liquid_room;
		U8 has_ambient_light;
		U8 is_detail;
		mxDO(reader.Get(is_liquid_room));
		mxDO(reader.Get(has_ambient_light));
		mxDO(reader.Get(is_detail));

		U8 unknown1;
		mxDO(reader.Get(unknown1));
		mxASSERT(unknown1 <= 1);

		F32 life;
		mxDO(reader.Get(life));
		mxASSERT(life >= -1.0f && life <= 10000.0f);

		const bool is_glass = is_detail && (life >= 0.0f);

		String64 eax_effect;
		mxDO(RFL_Read_String(reader, eax_effect));

		if(is_liquid_room)
		{
			U32 liquid_depth;
			mxDO(reader.Get(liquid_depth));

			U32 liquid_color;
			mxDO(reader.Get(liquid_color));

			String64 liquid_surface_texture;
			mxDO(RFL_Read_String(reader, liquid_surface_texture));

			// liquid_visibility, liquid_type, liquid_alpha, liquid_unknown,
			// liquid_waveform, liquid_surface_texture_scroll_u, liquid_surface_texture_scroll_b
			mxDO(Stream_Skip(reader, 12 + 13 + 12, gs_temp_buffer, sizeof(gs_temp_buffer)));
		}

		if(has_ambient_light)
		{
			U32 ambient_color;
			mxDO(reader.Get(ambient_color));
		}

		room.glass_index = is_glass ? (glass_index++) : 0;
		room.is_sky = is_sky;
	}

	U32 cUnknown;
	mxDO(reader.Get(cUnknown));
	if(num_rooms != cUnknown) {
		ptPRINT("Warning! num_rooms(%u) != cUnknown(%u)\n",
		num_rooms, cUnknown
		);
	}

	for( U32 iFace = 0; iFace < cUnknown; ++iFace )
	{
		U32 index;
		mxDO(reader.Get(index));

		U32 num_links;
		mxDO(reader.Get(num_links));

		// links
		mxDO(Stream_Skip(reader, num_links * 4, gs_temp_buffer, sizeof(gs_temp_buffer)));
	}

	U32 cUnknown2;
	mxDO(reader.Get(cUnknown2));
	mxDO(Stream_Skip(reader, cUnknown2 * 32, gs_temp_buffer, sizeof(gs_temp_buffer)));

	U32 num_vertices;
	mxDO(reader.Get(num_vertices));

	mxDO(level_chunk_.positions.setNum(num_vertices));

	for( U32 iFace = 0; iFace < num_vertices; ++iFace )
	{
		mxDO(reader.Get(level_chunk_.positions[iFace]));
	}

	U32 num_faces;
	mxDO(reader.Get(num_faces));

	mxDO(level_chunk_.faces.setNum(num_faces));

	// stats
	U32 max_verts_per_face = 0;

	for(U32 iFace = 0; iFace < num_faces; ++iFace)
	{
		const size_t nPos = reader.Tell();
		RFL_Face & face = level_chunk_.faces[iFace];

		// Use normal from unknown plane
		mxDO(reader.Get(face.vNormal));
		mxASSERT(V3_IsNormalized(face.vNormal));

		float fDist = 0.0f;
		mxDO(reader.Get(fDist));
		face.fDist = fDist;

		mxDO(reader.Get(face.iTexture));
		if(face.iTexture != 0xFFFFFFFF) {
			mxASSERT(face.iTexture < num_textures);
		}

		mxDO(reader.Get(face.iLightmap)); // its used later (lightmap?)
		mxASSERT(face.iLightmap == 0xFFFFFFFF || face.iLightmap < num_faces);

		U32 Unknown3;
		mxDO(reader.Get(Unknown3));

		U32 Unknown4;
		mxDO(reader.Get(Unknown4));
		mxASSERT(Unknown4 == 0xFFFFFFFF);

		U32 Unknown4_2;
		mxDO(reader.Get(Unknown4_2));
		mxASSERT(Unknown4_2 == 0xFFFFFFFF);

		mxDO(reader.Get(face.iPortal)); // its used later (it's not 0 for portals)

		mxDO(reader.Get(face.uFlags));
		if((face.uFlags & ~(RFL_FF_MASK)))
			ptPRINT("Unknown face flags 0x%x\n", face.uFlags & ~(RFL_FF_MASK));

		uint8_t uLightmapRes = ReadUInt8(reader);
		if(uLightmapRes >= 0x22) {
			ptPRINT("Unknown lightmap resolution 0x%x\n", uLightmapRes);
		}

		uint16_t Unk6 = ReadUInt16(reader);
		uint32_t Unk6_2 = ReadUInt32(reader);
		mxASSERT(Unk6 == 0);

		face.iRoom = ReadUInt32(reader);
		mxASSERT(face.iRoom < num_rooms);

		U32 num_face_vertices = ReadUInt32(reader);
		mxASSERT(num_face_vertices >= 3);
		//ptPRINT("Face %u vertices: %u, texture: %x (pos: 0x%x)\n", iFace, num_face_vertices, iTexture, nPos);
		if(num_face_vertices > 100)
			ptPRINT("Warning! Face %u has %u vertices (level can be corrupted) (pos: 0x%x)\n",
			iFace, num_face_vertices, nPos
			);

		max_verts_per_face = largest(max_verts_per_face, num_face_vertices);

		mxDO(face.vertices.setNum(num_face_vertices));

		for(U32 iVertex = 0; iVertex < num_face_vertices; ++iVertex)
		{
			RFL_FaceVertex & Idx = face.vertices[ iVertex ];

			Idx.idx = ReadUInt32(reader);
			mxASSERT(Idx.idx < num_vertices);

			Idx.u = ReadFloat32(reader);
			Idx.v = ReadFloat32(reader);
			if(face.iLightmap != 0xFFFFFFFF)
			{
				Idx.lm_u = ReadFloat32(reader);
				Idx.lm_v = ReadFloat32(reader);
				mxASSERT(Idx.lm_u >= 0.0f && Idx.lm_u <= 1.0f);
				mxASSERT(Idx.lm_v >= 0.0f && Idx.lm_v <= 1.0f);
			}
		}
	}

	U32 num_lightmap_vertices = ReadUInt32(reader);
	mxDO(level_chunk_.lightmap_vertices.setNum(num_lightmap_vertices));

	for(U32 iVertex = 0; iVertex < num_lightmap_vertices; ++iVertex)
	{
		U32 iLightmap = ReadUInt32(reader);

		mxASSERT(iLightmap < level.lightmaps.num());
		level_chunk_.lightmap_vertices[iVertex] = iLightmap;

		mxDO(Stream_Skip(reader, 92, gs_temp_buffer, sizeof(gs_temp_buffer))); // unknown4
	}

	if( level.file_format_version == 0xB4 ) {
		mxDO(Stream_Skip(reader, 4, gs_temp_buffer, sizeof(gs_temp_buffer))); // unknown5
	}

	//
	ptPRINT("Loaded geometry: %u textures, %u rooms, %u vertices, %u faces (max %u verts per face)",
		num_textures, num_rooms, num_vertices, num_faces, max_verts_per_face
		);

	return ALL_OK;
}

ERet LoadLightmaps(
				   RFL_Level &level_
				   , AReader& reader
				   )
{
	U32 num_lightmaps;
	mxDO(reader.Get(num_lightmaps));

	mxDO(level_.lightmaps.setNum(num_lightmaps));

	for( U32 iLightmap = 0; iLightmap < num_lightmaps; iLightmap++ )
	{
		RFL_Lightmap & lightmap = level_.lightmaps[ iLightmap ];

		//
		lightmap.texture_handle.SetNil();

		//
		U32 lightmap_width;
		U32 lightmap_height;
		mxDO(reader.Get(lightmap_width));
		mxDO(reader.Get(lightmap_height));
		mxASSERT(lightmap_width <= 1024 && lightmap_height <= 1024);

		//
		lightmap.texture_width = lightmap_width;
		lightmap.texture_height = lightmap_height;

		const U32 num_texels = lightmap_width * lightmap_height;

		//
		TRY_ALLOCATE_SCRACH(R8G8B8*, textureData, num_texels );
		mxDO(reader.Read( textureData, num_texels*sizeof(textureData[0]) ));

		//

#if RF_RENDERER_SUPPORT_LIGHTMAPS

		mxDO(lightmap.texture_data.setNum( num_texels ));

		R8G8B8A8 *swizzled = lightmap.texture_data.raw();
		for( U32 i = 0; i < num_texels; i++ )
		{
			swizzled[i].R = textureData[i].R;
			swizzled[i].G = textureData[i].G;
			swizzled[i].B = textureData[i].B;
			swizzled[i].A = ~0;
		}

#endif

	}//for each lightmap

	ptPRINT("Loaded %u lightmaps", num_lightmaps);
	return ALL_OK;
}

ERet Parse_LevelInfo(
					 AReader& reader,
					 RFL_Level &_level
					 )
{
	U32 unknown;	// 0x00000001
	mxDO(reader.Get(unknown));

	mxDO(RFL_Read_String(reader, _level.name));
	mxDO(RFL_Read_String(reader, _level.author));
	mxDO(RFL_Read_String(reader, _level.date));

	ptPRINT("Level name: %s", _level.name.c_str());
	ptPRINT("Level date: %s", _level.date.c_str());
	ptPRINT("Level author: %s", _level.author.c_str());	

	U8 unknown2;
	mxDO(reader.Get(unknown2));

	U8 is_multiplayer_level; // 0 or 1
	mxDO(reader.Get(is_multiplayer_level));

	mxDO(Stream_Skip(reader, 220, gs_temp_buffer, sizeof(gs_temp_buffer))); // unknown

	return ALL_OK;
}

///
ERet Load_RFL_from_File(
						RFL_Level &level_
						, const char* level_filepath
						)
{
	FileReader	file;
	mxDO(file.Open( level_filepath ));

	rfl_header_t level_header;
	mxDO(file.Get(level_header));

	String64 level_name;
	mxDO(RFL_Read_String(file, level_name));
	ptPRINT("Level name: %s (version: 0x%X)", level_name.c_str(), level_header.version);

	String64 mod_name;
	mxDO(RFL_Read_String(file, mod_name));
	ptPRINT("Mod name: %s", mod_name.c_str());

	ptPRINT("Num sections: %u", level_header.sections_count);

	//
	level_.bounds.clear();

	level_.file_format_version = level_header.version;

	//
	U32 num_static_geometries = 0;

	for(;;)
	{
		rfl_section_header_t section_header;
		mxDO(file.Get(section_header));

		const rfl_section_type_t section_type = static_cast<rfl_section_type_t>( section_header.type );

		if( section_type == RFL_END ) {
			break;
		}

		// Save current position - we will need it
		const size_t section_offset = file.Tell();

		ptPRINT("Parsing section '%s' (0x%x) (%u bytes at 0x%x)...",
			ELevelSection_To_Chars(section_type), section_type, section_header.size, section_offset/*-sizeof(section_header)*/
			);

		switch( section_type )
		{
			case RFL_STATIC_GEOMETRY:
				{
					RFL_LevelGeo	level_chunk;
					mxDO(LoadStaticGeometry( level_chunk, file, level_ ));

					mxDO(level_.chunks.add( level_chunk ));

					AABBf_Merge( level_chunk.bounds, level_.bounds, level_.bounds );

					num_static_geometries++;
				}
				break;

            case RFL_LIGHTMAPS:
				mxDO(LoadLightmaps( level_, file ));
                break;

			case RFL_LEVEL_INFO:
				Parse_LevelInfo( file, level_ );
				break;
		}

		const size_t current_offset = file.Tell();

        if( section_offset == current_offset )
        {
            // Move to the next section
			mxDO(Stream_Skip(file, section_header.size, gs_temp_buffer, sizeof(gs_temp_buffer)));

			ptPRINT("Warning! Section '%s' (0x%x) was not completely processed.",
				ELevelSection_To_Chars(section_type), section_type
				);
        }
	}

	LogStream(LL_Info) << "Level AABB (RF): " << level_.bounds << ", num_static_geometries: " << num_static_geometries;

	return ALL_OK;
}

ERet RFL_LevelGeo::recalcRenderingData()
{
	mxDO(this->sorted_face_indices.Reserve( this->faces.num() ));
	//
	for( U32 iFace = 0; iFace < this->faces.num(); iFace++ )
	{
		const RFL_Face& face = this->faces[ iFace ];
		const RFL_Room& room = this->rooms[ face.iRoom ];

		if( room.is_sky ) {
			continue;
		}

		RFL_Material* material = (face.iTexture != -1) ? &this->materials[ face.iTexture ] : NULL;
		if( !material ) {
			continue;
		}

		const bool is_invisible = (face.iPortal != 0) || (face.uFlags & RFL_FF_SHOW_SKY) || (material && material->is_invisible);
		if( is_invisible ) {
			continue;
		}

		//
		this->sorted_face_indices.add( iFace );
	}

	///
	class CompareFaces
	{
		const RFL_LevelGeo& _geo;
	public:
		inline CompareFaces( const RFL_LevelGeo& geo )
			: _geo( geo )
		{}
		inline bool operator () ( const U16 iFaceA, const U16 iFaceB )
		{
			const RFL_Face& faceA = _geo.faces[ iFaceA ];
			const RFL_Face& faceB = _geo.faces[ iFaceB ];

#if RF_RENDERER_SUPPORT_LIGHTMAPS

			if( faceA.iTexture < faceB.iTexture ) {
				return true;
			}
			if( faceA.iTexture > faceB.iTexture ) {
				return false;
			}

			return faceA.iLightmap < faceB.iLightmap;
#else

			return faceA.iTexture < faceB.iTexture;

#endif
		}
	};

	std::sort(
		this->sorted_face_indices.begin(),
		this->sorted_face_indices.end(),
		CompareFaces( *this )
		);

	return ALL_OK;
}

ERet RFL_Level::recalcRenderingData()
{
	for( UINT iGeo = 0; iGeo < this->chunks.num(); iGeo++ )
	{
		mxDO(this->chunks[iGeo].recalcRenderingData());
	}

	return ALL_OK;
}
