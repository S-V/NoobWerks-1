#pragma once

#include <GPU/Public/graphics_types.h>	// HTexture
#include <Rendering/ForwardDecls.h>
//#include <Rendering/Public/Globals.h>



/// Voxel terrain materials
enum VoxelMaterialIDs
{
	/// Empty Space/Air material.
	/// NOTE: solid materials cannot be zero!
	VoxMat_None = 0,

	VoxMat_Ground,
	VoxMat_Grass,
	VoxMat_GrassB,
	VoxMat_RockA,
	VoxMat_RockB,
	VoxMat_RockC,
	VoxMat_Dirt,
	VoxMat_Sand,
	VoxMat_Floor,
	VoxMat_Wall,
	VoxMat_Snow,
	VoxMat_Snow2,

	VoxMat_Mirror,	// for testing and debugging specular reflections

//	VoxMat_MissingTexture,
//	VoxMat_UVTest,
//	VoxMat_PBR_WaterBottle,

	VoxMat_MAX
};
mxDECLARE_ENUM( VoxelMaterialIDs, U8, VoxMatID );

// the average texture color extracted from the coarsest mip - used for voxel cone tracing
extern UByte4 g_average_terrain_material_colors[VoxMat_MAX];

///
struct VoxelTerrainMaterials: NonCopyable
{
	DynamicArray< Rendering::Material* >	textured_materials;

	TPtr< Rendering::Material >	triplanar_material;
	TPtr< Rendering::Material >	uv_textured_material;

	//TPtr<Material>	mat_terrain_DLOD;	//!< quantized format for Discrete Level Of Detail
	//TPtr<Material>	mat_terrain_CLOD;	//!< full-precision format for Continuous Level Of Detail

	HTexture hTerrainDiffuseTextures;
	HTexture hTerrainNormalTextures;

public:
	VoxelTerrainMaterials( AllocatorI& allocator )
		: textured_materials( allocator )
	{
	}

	ERet load(
		NGpu::NwRenderContext & context,
		AllocatorI & scratch,
		NwClump & storage
		);
};
