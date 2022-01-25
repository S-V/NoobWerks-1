#include <Base/Base.h>
#pragma hdrstop

#include <VoxelsSupport/VoxelTerrainMaterials.h>

#include <Rendering/Public/Core/Material.h>
// G_VoxelTerrain
#include <Rendering/Private/ShaderInterop.h>
#include <Rendering/Private/RenderSystemData.h>


mxBEGIN_REFLECT_ENUM( VoxMatID )
	mxREFLECT_ENUM_ITEM( empty, VoxelMaterialIDs::VoxMat_None ),
	mxREFLECT_ENUM_ITEM( Ground, VoxelMaterialIDs::VoxMat_Ground ),
	mxREFLECT_ENUM_ITEM( Grass, VoxelMaterialIDs::VoxMat_Grass ),
	mxREFLECT_ENUM_ITEM( GrassB, VoxelMaterialIDs::VoxMat_GrassB ),
	mxREFLECT_ENUM_ITEM( RockA, VoxelMaterialIDs::VoxMat_RockA ),
	mxREFLECT_ENUM_ITEM( RockB, VoxelMaterialIDs::VoxMat_RockB ),
	mxREFLECT_ENUM_ITEM( RockC, VoxelMaterialIDs::VoxMat_RockC ),
	mxREFLECT_ENUM_ITEM( Dirt, VoxelMaterialIDs::VoxMat_Dirt ),
	mxREFLECT_ENUM_ITEM( Sand, VoxelMaterialIDs::VoxMat_Sand ),
	mxREFLECT_ENUM_ITEM( Floor, VoxelMaterialIDs::VoxMat_Floor ),
	mxREFLECT_ENUM_ITEM( Wall, VoxelMaterialIDs::VoxMat_Wall ),
	mxREFLECT_ENUM_ITEM( Snow, VoxelMaterialIDs::VoxMat_Snow ),
	mxREFLECT_ENUM_ITEM( Snow2, VoxelMaterialIDs::VoxMat_Snow2 ),	
mxEND_REFLECT_ENUM

UByte4 g_average_terrain_material_colors[VoxMat_MAX];

ERet VoxelTerrainMaterials::load(
								 NGpu::NwRenderContext & context,
								 AllocatorI & scratch,
								 NwClump & storage
								 )
{
	HTexture hTerrainDiffuseTextures;
	HTexture hTerrainNormalTextures;
	{
		enum { TEXTURE_ARRAY_SIZE = VoxMat_MAX };

		String32 diffuseTextureNames[TEXTURE_ARRAY_SIZE];
		String32 normalTextureNames[TEXTURE_ARRAY_SIZE];

		//NOTE: the first material ID is null (zero index == empty space/air)
		const UINT start_index = 1;

		for( int i = start_index; i < TEXTURE_ARRAY_SIZE; i++ )
		{
			Str::CopyS(diffuseTextureNames[i], "snow");
			normalTextureNames[i] = diffuseTextureNames[i];
			Str::AppendS(normalTextureNames[i], "_n");
		}

		Str::CopyS(diffuseTextureNames[VoxMat_Grass], "ground_grass2");
		Str::CopyS(diffuseTextureNames[VoxMat_GrassB], "grass");
		Str::CopyS(diffuseTextureNames[VoxMat_Ground], "ground_earth");
		Str::CopyS(diffuseTextureNames[VoxMat_RockA], "rock");
		Str::CopyS(diffuseTextureNames[VoxMat_RockB], "rock_b");

		// removed for Half-Assed FPS
		//Str::CopyS(diffuseTextureNames[VoxMat_RockC], "rock_c");
		Str::CopyS(diffuseTextureNames[VoxMat_RockC], "rock_b");

		Str::CopyS(diffuseTextureNames[VoxMat_Dirt], "dirt");
		Str::CopyS(diffuseTextureNames[VoxMat_Sand], "desert");
		Str::CopyS(diffuseTextureNames[VoxMat_Floor], "floor_1");
		Str::CopyS(diffuseTextureNames[VoxMat_Wall], "wall4");
		Str::CopyS(diffuseTextureNames[VoxMat_Snow], "snow");
		//Str::CopyS(diffuseTextureNames[VoxMat_Snow2], "snow2");
		Str::CopyS(diffuseTextureNames[VoxMat_Snow2], "snow");

//		Str::CopyS(diffuseTextureNames[VoxMat_Water], "Water");
		Str::CopyS(diffuseTextureNames[VoxMat_Mirror], "snow");


		//
		AssetID diffuseTextureIds[TEXTURE_ARRAY_SIZE];
		AssetID normalTextureIds[TEXTURE_ARRAY_SIZE];
		for( int i = start_index; i < TEXTURE_ARRAY_SIZE; i++ )
		{
			diffuseTextureIds[i] = make_AssetID_from_raw_string(diffuseTextureNames[i].c_str());
			normalTextureIds[i] = make_AssetID_from_raw_string(normalTextureNames[i].c_str());
		}

		//
		NwTexture2DDescription	texArraydesc;
		texArraydesc.format = NwDataFormat::BC1;	// DXT1
		texArraydesc.width = 1024;
		texArraydesc.height = 1024;
		texArraydesc.numMips = TLog2<1024>::value + 1;	// 11
		texArraydesc.arraySize = TEXTURE_ARRAY_SIZE;
		//texArraydesc.flags |= GrTextureCreationFlags::GENERATE_MIPS;
		hTerrainDiffuseTextures = NGpu::createTexture2D(texArraydesc);
		hTerrainNormalTextures = NGpu::createTexture2D(texArraydesc);

		mxDO(LoadTextureArray( context, texArraydesc, hTerrainDiffuseTextures, diffuseTextureIds, g_average_terrain_material_colors, scratch, start_index ));
		mxDO(LoadTextureArray( context, texArraydesc, hTerrainNormalTextures, normalTextureIds, nil, scratch, start_index ));
		NGpu::NextFrame();
	}

	{
		const NGpu::Memory *	memory_block = NGpu::Allocate(sizeof(G_VoxelTerrain));
		G_VoxelTerrain *	voxel_terrain_cb_data = (G_VoxelTerrain*) memory_block->data;	// ~4 KiB
		mxENSURE(voxel_terrain_cb_data, ERR_OUT_OF_MEMORY, "failed to allocate data for updating constant buffer for voxel terrain materials");

		for( UINT iMaterial = 0; iMaterial < VoxMat_MAX; iMaterial++ )
		{
			VoxelMaterial& material = voxel_terrain_cb_data->g_voxel_materials[iMaterial];
			//
			material.textureIDs.x = iMaterial;
			material.textureIDs.y = iMaterial;
			material.textureIDs.z = iMaterial;
			material.textureIDs.w = 0;
			//
			material.pbr_params.x = 0.04f;	// metalness
			material.pbr_params.y = 1.0f;	// roughness
			material.pbr_params.z = 0.0f;
			material.pbr_params.w = 0.0f;
		}

		{
			VoxelMaterial& material = voxel_terrain_cb_data->g_voxel_materials[VoxMat_Grass];
			material.textureIDs.x = VoxMat_RockB;
			material.textureIDs.y = VoxMat_RockB;
			//material.textureIDs.y = VoxMat_RockC;
			material.textureIDs.z = VoxMat_Grass;	// top
			material.textureIDs.w = 0;
//material.textureIDs.x = VoxMat_Grass;
//material.textureIDs.y = VoxMat_Grass;
//material.textureIDs.z = VoxMat_Grass;
		}
		{
			VoxelMaterial& material = voxel_terrain_cb_data->g_voxel_materials[VoxMat_Ground];
			material.textureIDs.x = VoxMat_Ground;
			material.textureIDs.y = VoxMat_Ground;
			material.textureIDs.z = VoxMat_Ground;
			material.textureIDs.w = 0;
		}

		{
			VoxelMaterial& material = voxel_terrain_cb_data->g_voxel_materials[VoxMat_Mirror];
			material.textureIDs.x = VoxMat_Mirror;
			material.textureIDs.y = VoxMat_Mirror;
			material.textureIDs.z = VoxMat_Mirror;
			material.textureIDs.w = 0;

			material.pbr_params.x = 1.0f;	// metalness
			material.pbr_params.y = 0.0f;	// roughness
		}

		NGpu::updateBuffer(
			Rendering::Globals::g_data->_global_uniforms.voxel_terrain.handle,
			memory_block
			);
		NGpu::NextFrame();
	}

	//
	this->hTerrainDiffuseTextures = hTerrainDiffuseTextures;
	this->hTerrainNormalTextures = hTerrainNormalTextures;

	// Load materials.
	{
		// NOTE: optimization! The renderer sorts batches first by shader and then by depth, etc.
		// We load DLOD material first, so that batches with DLOD will be rendered before batches with CLOD.
		// DLOD batches are centered around the camera, they should be rendered in front of CLOD to better utilize HiZ culling.
		mxDO(Resources::Load(
			this->triplanar_material._ptr,
			MakeAssetID("material_voxel_terrain_dlod"),
			&storage
			));

		//triplanar_material->command_buffer.DbgPrint();

		// Set shader parameters.
		mxDO(this->triplanar_material->SetInput(
			nwNAMEHASH("t_diffuseMaps"),
			NGpu::AsInput(hTerrainDiffuseTextures)
			));

		// the material may not reference "t_normalMaps" if normal mapping is disabled
		mxDO(this->triplanar_material->SetInput(
			nwNAMEHASH("t_normalMaps"),
			NGpu::AsInput(hTerrainNormalTextures)
			));

#if 0
		//
		{
			Rendering::Material *	test_mat;
			Resources::Load(test_mat, MakeAssetID("material_voxel_terrain_uv_textured"));

			//
			mxDO(this->textured_materials.setNum(VoxMat_MAX));

			{
				Rendering::Material *	mat_inst;
				mxDO(createMaterialInstance( &mat_inst, test_mat, storage ));
				//
				NwTexture *	texture;
				mxDO(Resources::Load( texture, MakeAssetID("missing_texture"), &storage ));
				mxDO(mat_inst->SetInput(
					nwNAMEHASH("t_diffuseMap"),
					texture->m_resource
					));

				this->textured_materials._data[0] = mat_inst;	//"material_default";
			}
			{
				Rendering::Material *	mat_inst;
				mxDO(createMaterialInstance( &mat_inst, test_mat, storage ));
				////
				//NwTexture *	texture;
				//mxDO(Resources::Load( &texture, MakeAssetID("missing_texture"), storage ));
				//mxDO(mat_inst->SetInput(nwNAMEHASH("t_diffuseMap"), texture->m_resource ));

				this->textured_materials._data[1] = mat_inst;	//"uv-test";
			}
		}
#endif
		//{
		//	Rendering::Material *	terr_mat;
		//	Resources::Load(terr_mat, MakeAssetID("voxel_terrain_textured_pbr"));

		//	Rendering::Material *	mat_inst;
		//	mxDO(createMaterialInstance( &mat_inst, terr_mat, storage ));

		//	//
		//	mxDO(mat_inst->setTexture( mxHASH_STR("t_base_color"),
		//		MakeAssetID("WaterBottle_baseColor"),
		//		&storage
		//		));

		//	mxDO(mat_inst->setTexture( mxHASH_STR("t_roughness_metallic"),
		//		MakeAssetID("WaterBottle_occlusionRoughnessMetallic"),
		//		&storage
		//		));

		//	mxDO(mat_inst->setTexture( mxHASH_STR("t_normals"),
		//		MakeAssetID("WaterBottle_normal"),
		//		&storage
		//		));

		//	this->textured_materials._data[VoxMat_PBR_WaterBottle] = mat_inst;	//"uv-test";
		//}
	}

	return ALL_OK;
}