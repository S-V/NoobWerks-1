//
// Loader for level files (*.rfl) from Red Faction 1.
// 
// Based on work done by Rafal Harabien:
// https://github.com/rafalh/rf-reversed
//
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Util/Color.h>	// R8G8B8A8
#include <Core/Assets/AssetReference.h>
//#include <Core/ObjectModel/NwClump.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Rendering/Public/Globals.h>	// Rendering::NwCameraView

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>

#define RF_RENDERER_SUPPORT_LIGHTMAPS	(0)

static mxFORCEINLINE
V3f FromRF( const V3f& p )
{
	//return V3_Set(p.x, p.y, p.z);	// mirrored and lying on its side

mxBUG("mirrored");
//return V3_Set(-p.x, p.z, p.y);	// mirrored

//return V3_Set(-p.x, p.z, -p.y);	// WRONG - distorted geometry

	//return V3_Set(-p.x, -p.z, p.y);	// WRONG - distorted geometry

return V3_Set(p.x, -p.z, p.y);	// mirrored
}


struct RFL_FaceVertex
{
    U32 idx;
    F32 u, v;
    F32 lm_u, lm_v;
};

// A face is a convex polygon.
struct RFL_Face
{
    V3f vNormal;
	F32 fDist;
	TInplaceArray<RFL_FaceVertex,16> vertices;	// triangle fan

	I32 iTexture;
    I32 iLightmap;

	U32 iRoom;
    U32 iPortal;
    U8 uFlags;
};

/*
From Red Help Ver. 1.1:

Red Faction textures support different material types that create different sounds when they are walked on, shot at, etc. By added the material prefix (i.e. "mtl_") to a texture's file name, you can give it certain sound properties as well as other special characteristics in some cases. The valid material types are as follows:

Material Prefix 
Metal mtl 
Rock rck 
Cement cem 
Solid sld 
Glass gls 
Security Glass sgl 
Sand snd 
Water wtr 
Ice ice 
Flesh fle 
Lava lva 

Notes

- Breakable glass textures should always use the "gls" prefix. This is a code issue.
- Unbreakable glass textures should always the "sgl" prefix. This is also a code issue. 
- Textures that use the "ice" prefix will be cause the player to slide if they walk on a face that has an "ice_" texture on it.
- Use the "wtr" prefix on a texture if the player can walk in water that is shallow (i.e. knee deep). This way, they will sound like they are walking on water. 

*/
struct RFL_Material
{
	TResPtr< NwTexture >	texture;
	bool is_translucent;
	bool is_invisible;
};

struct RFL_Room
{
	AABBf bounds;
	U32 glass_index;
	bool is_detail;
	bool is_sky;	// sky box or sky plane
};

// level geometry is centered around origin (big levels => loss of precision)
struct RFL_LevelGeo : CStruct
{
	AABBf					bounds;

	TArray< RFL_Face >		faces;
	TArray< V3f >			positions;
	TArray< RFL_Room >		rooms;
	TArray< RFL_Material >	materials;
	TArray< U32 >			lightmap_vertices;

	// sorted by texture to minimize state changes during rendering
	TArray< U16 >			sorted_face_indices;

public:
	mxDECLARE_CLASS( RFL_LevelGeo, CStruct );
	mxDECLARE_REFLECTION;

	ERet recalcRenderingData();
};

///
struct RFL_Lightmap
{
	HTexture			texture_handle;

	TArray< R8G8B8A8 >	texture_data;
	U32					texture_width;
	U32					texture_height;
};

struct RFL_Level : CStruct
{
	AABBf		bounds;

	U32			file_format_version;

	TArray< RFL_LevelGeo >	chunks;
	TArray< RFL_Lightmap >	lightmaps;

	String64	name;
	String64	author;
	String64	date;

public:
	mxDECLARE_CLASS( RFL_Level, CStruct );
	mxDECLARE_REFLECTION;

	ERet recalcRenderingData();
};

///
ERet Load_RFL_from_File(
						RFL_Level &level_
						, const char* level_filepath
						);

namespace RedFaction
{

	ERet drawLevel(
		RFL_Level & level	// non-const, because we load textures on-demand
		, const Rendering::NwCameraView& camera_view
		, NwClump * asset_storage
		, UINT *num_draw_calls_
		);

}//namespace RedFaction
