#include <Base/Base.h>
#pragma hdrstop

#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Public/graphics_system.h>

#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Rendering/Private/Shaders/HLSL/VoxelGI/vxgi_interop.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Public/Utility/DrawPointList.h>


namespace Rendering
{
namespace VXGI
{

namespace
{
	AllocatorI& GetAllocator() { return MemoryHeaps::renderer(); }
}

const DataFormatT VoxelGrids::VOXEL_RADIANCE_TEXTURE_FORMAT = NwDataFormat::RGBA8;
const DataFormatT VoxelGrids::VOXEL_DENSITY_TEXTURE_FORMAT = NwDataFormat::R8_UNORM;



static RenderCallbacks	gs_voxelization_callbacks;

ERet RegisterCallbackForVoxelizingEntitiesOfType(
							const ERenderEntity entity_type
							, RenderCallback* render_callback
							, void* user_data
							)
{
	mxENSURE(
		nil == gs_voxelization_callbacks.code[ entity_type ],
		ERR_DUPLICATE_OBJECT,
		""
		);

	gs_voxelization_callbacks.code[ entity_type ] = render_callback;
	gs_voxelization_callbacks.data[ entity_type ] = user_data;

	return ALL_OK;
}


ClipMap::ClipMap()
{
	for( UINT cascade_index = 0; cascade_index < mxCOUNT_OF(cascades); cascade_index++ )
	{
		Cascade & cascade = cascades[ cascade_index ];
		cascade.center = CV3f(0);
		cascade.radius = 10.f * (1 << cascade_index);
	}
	num_cascades = 1;
	dirty_cascades = 0;
	TSetStaticArray( delta_distance, 1.0f );
}

void ClipMap::OnCameraMoved( const V3f& new_position_in_world_space )
{
	for( UINT cascade_index = 0; cascade_index < num_cascades; cascade_index++ )
	{
		Cascade & cascade = cascades[ cascade_index ];

		const float max_delta_dist_sq = squaref( delta_distance[cascade_index] );

		const float delta_dist_sq = ( cascade.center - new_position_in_world_space ).lengthSquared();

		if( delta_dist_sq > max_delta_dist_sq )
		{
			cascade.center = new_position_in_world_space;

			dirty_cascades |= BIT(cascade_index);

//LogStream(LL_Warn)<<"VXGI Cascade [",cascade_index,"] -> ",cascade;
		}
	}
}

void ClipMap::ApplySettings( const CascadeSettings& new_settings )
{
	num_cascades = new_settings.num_cascades;

	for( UINT cascade_index = 0; cascade_index < num_cascades; cascade_index++ )
	{
		Cascade & cascade = cascades[ cascade_index ];

		cascade.radius = new_settings.cascade_radius[ cascade_index ];
		delta_distance[ cascade_index ] = new_settings.cascade_max_delta_distance[ cascade_index ];
	}

	SetAllCascadesDirty();
}





VoxelGrids::VoxelGrids()
{
	radiance_voxel_grid_buffer.SetNil();

	for( UINT cascade_index = 0; cascade_index < MAX_GI_CASCADES; cascade_index++ )
	{
		_radiance_opacity_texture[cascade_index].SetNil();
	}

	mxZERO_OUT(settings);
}

VoxelGrids::~VoxelGrids()
{
}

ERet VoxelGrids::Initialize( const RuntimeSettings& settings )
{
	//
	mxDO(nwCREATE_OBJECT(
		brickmap._ptr
		, GetAllocator()
		, BrickMap
		, GetAllocator()
		));
	mxDO(brickmap->Initialize(
		BrickMap::Config()
		));

	//
	mxDO(nwCREATE_OBJECT(
		voxel_BVH._ptr
		, GetAllocator()
		, VoxelBVH
		, GetAllocator()
		));
	mxDO(voxel_BVH->Initialize(
		VoxelBVH::Config()
		));

	//
	cb_handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_VXGI);

	return this->ApplySettings( settings );
}

void VoxelGrids::Shutdown()
{
	this->_ReleaseResources();

	nwDESTROY_GLOBAL_CONSTANT_BUFFER(cb_handle);

	//
	voxel_BVH->Shutdown();
	nwDESTROY_OBJECT(voxel_BVH._ptr, GetAllocator());

	//
	brickmap->Shutdown();
	nwDESTROY_OBJECT(brickmap._ptr, GetAllocator());
	brickmap = nil;
}

ERet VoxelGrids::ApplySettings( const RuntimeSettings& new_settings )
{
	mxENSURE(new_settings.IsValid(), ERR_INVALID_PARAMETER, "");

	if( !settings.equals( new_settings ) )
	{
		DBGOUT("[VXGI] Applying new settings...");

		if(
			!settings.radiance_texture_settings.equals(new_settings.radiance_texture_settings)
			||
			settings.cascades.num_cascades < new_settings.cascades.num_cascades
			)
		{
			DBGOUT("[VXGI] Re-Alocating Resources...");

			this->_ReleaseResources();
			this->_AllocateResources( new_settings );
		}

		cpu.clipmap.ApplySettings( new_settings.cascades );

		settings = new_settings;
	}

	return ALL_OK;
}

void VoxelGrids::_AllocateResources( const RuntimeSettings& settings )
{
	this->_AllocateVoxelGridTextures( settings );
	this->_AllocateVoxelizationBuffer( settings );

	NGpu::NextFrame();
}

void VoxelGrids::_AllocateVoxelizationBuffer( const RuntimeSettings& settings )
{
	const UINT numMipLevels = settings.radiance_texture_settings.dim_log2;
	const UINT voxelGridResolution = 1 << numMipLevels;

	struct Voxel
	{
		// RGB - albedo, A - opacity
		U32	albedo_opacity;	
	};

	NwBufferDescription	buffer_description;
	buffer_description.type = Buffer_Data;
	buffer_description.size = ToCube(voxelGridResolution) * sizeof(Voxel);
	buffer_description.stride = sizeof(Voxel);
	buffer_description.flags.raw_value = NwBufferFlags::Sample | NwBufferFlags::Write;
	radiance_voxel_grid_buffer = NGpu::CreateBuffer(
		buffer_description
		, nil
		IF_DEVELOPER , "Voxel Grid Buffer"
		);
}

void VoxelGrids::_AllocateVoxelGridTextures( const RuntimeSettings& settings )
{
	const UINT numMipLevels = settings.radiance_texture_settings.dim_log2;
	const UINT voxelGridResolution = 1 << numMipLevels;

	NwTexture3DDescription	radiance_texture_description;
	{
		radiance_texture_description.format		= VOXEL_RADIANCE_TEXTURE_FORMAT;
		radiance_texture_description.width		= voxelGridResolution;
		radiance_texture_description.height		= voxelGridResolution;
		radiance_texture_description.depth		= voxelGridResolution;
		radiance_texture_description.numMips	= numMipLevels;
		radiance_texture_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES | NwTextureCreationFlags::GENERATE_MIPS;
	}

	//NwTexture3DDescription	density_texture_description;
	//{
	//	density_texture_description.format		= VOXEL_DENSITY_TEXTURE_FORMAT;
	//	density_texture_description.width		= voxelGridResolution;
	//	density_texture_description.height		= voxelGridResolution;
	//	density_texture_description.depth		= voxelGridResolution;
	//	density_texture_description.numMips		= 1;
	//	// Leave default flags, because the texture will be updated via UpdateSubresource().
	//}

	for( UINT cascade_index = 0; cascade_index < settings.cascades.num_cascades; cascade_index++ )
	{
		//IF_DEVELOPER DEVOUT( "VXGI: Creating '%s' of resolution %u ^ 3", radiance_texture_description.name.c_str(), voxelGridResolution );
		
		//IF_DEVELOPER Str::Format( radiance_texture_description.name, "VXGI Radiance Volume [%d]", cascade_index );
		_radiance_opacity_texture[cascade_index] = NGpu::createTexture3D(
			radiance_texture_description
			, nil
			IF_DEVELOPER, "VX Radiance Volume"
			);

		////IF_DEVELOPER Str::Format( density_texture_description.name, "VXGI Density Volume [%d]", cascade_index );
		//_density_textures[cascade_index] = NGpu::createTexture3D(
		//	density_texture_description
		//	, nil
		//	IF_DEVELOPER, "VX Density Volume"
		//	);
	}

#if MX_DEVELOPER

	const UINT radianceTextureMemorySize = estimateTextureSize(
		radiance_texture_description.format
		, radiance_texture_description.width
		, radiance_texture_description.height
		, radiance_texture_description.depth
		, radiance_texture_description.numMips
		);

	//const UINT densityTextureMemorySize = estimateTextureSize(
	//	density_texture_description.format
	//	, density_texture_description.width
	//	, density_texture_description.height
	//	, density_texture_description.depth
	//	, density_texture_description.numMips
	//	);
	const UINT densityTextureMemorySize = 0;

	const UINT combinedTextureMemorySize = radianceTextureMemorySize + densityTextureMemorySize;

	IF_DEVELOPER DEVOUT( "VXGI: Size of voxel textures, per cascade: %u KiB (radiance: %u KiB, density: %u KiB) -> %u KiB in total (%u cascades)"
		, combinedTextureMemorySize / mxKILOBYTE
		, radianceTextureMemorySize / mxKILOBYTE
		, densityTextureMemorySize / mxKILOBYTE
		, (combinedTextureMemorySize * settings.cascades.num_cascades) / mxKILOBYTE
		, settings.cascades.num_cascades
		);

#endif // MX_DEVELOPER
}

void VoxelGrids::_ReleaseResources()
{
	if( radiance_voxel_grid_buffer.IsValid() )
	{
		NGpu::DeleteBuffer( radiance_voxel_grid_buffer );
		radiance_voxel_grid_buffer.SetNil();
	}

	for( UINT cascade_index = 0; cascade_index < MAX_GI_CASCADES; cascade_index++ )
	{
		if( _radiance_opacity_texture[cascade_index].IsValid() )
		{
			NGpu::DeleteTexture( _radiance_opacity_texture[cascade_index] );
			_radiance_opacity_texture[cascade_index].SetNil();
		}
	}
}

void VoxelGrids::bindVoxelCascadeTexturesToShader(
	NwShaderEffect* technique
	) const
{
	const NameHash32 radianceTexturesNameHashes[] = {
		mxHASH_STR("t_radianceVoxelGrid0"),
		mxHASH_STR("t_radianceVoxelGrid1"),
		mxHASH_STR("t_radianceVoxelGrid2"),
		mxHASH_STR("t_radianceVoxelGrid3"),
	};

	for( int i = 0; i < mxCOUNT_OF(_radiance_opacity_texture); i++ )
	{
		if( _radiance_opacity_texture[i].IsValid() )
		{
			UNDONE;
			//technique->SetInput(
			//	radianceTexturesNameHashes[i],
			//	NGpu::AsInput(_radiance_opacity_texture[i])
			//	);
		}
	}
}

void VoxelGrids::OnRegionChanged(
	const AABBf& region_bounds_in_world_space
	)
{
	for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
	{
		const Cascade& cascade = cpu.clipmap.cascades[ cascade_index ];

		if( region_bounds_in_world_space.intersects( cascade.ToAabbMinMax() ) )
		{
			cpu.clipmap.dirty_cascades |= BIT(cascade_index);
		}
	}
}

ERet VoxelGrids::UpdateAndReVoxelizeDirtyCascades(
	const SpatialDatabaseI& spatial_database
	, const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const RrGlobalSettings& renderer_settings
	)
{
	if(settings.debug.flags & DebugFlags::dbg_revoxelize_each_frame) {
		cpu.clipmap.dirty_cascades = ~0;
	}

	if(!cpu.clipmap.dirty_cascades) {
		return ALL_OK;
	}

	//
	mxDO(_UpdateShaderConstants());

	//
	for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
	{
		if( cpu.clipmap.dirty_cascades & BIT(cascade_index) )
		{
			mxDO(_UpdateCascade(
				cascade_index
				, scene_view
				, spatial_database
				, render_context
				, renderer_settings
				));
			CLEAR_BITS( cpu.clipmap.dirty_cascades, BIT(cascade_index) );
			break;	// only one cascade is updated each frame
		}
	}//for

	cpu.clipmap.dirty_cascades = 0;

	return ALL_OK;
}

ERet VoxelGrids::_UpdateShaderConstants()
{
	{
		GPU_VXGI	*	gpu_vxgi;
		mxDO(NGpu::updateGlobalBuffer(
			cb_handle,
			&gpu_vxgi
			));

		//
		const int volume_texture_mip_count = settings.radiance_texture_settings.dim_log2;
		const int volume_texture_resolution = (int) settings.radiance_texture_settings.GetResolution();

		//
		for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
		{
			const Cascade& cascade = cpu.clipmap.cascades[ cascade_index ];
			GPU_VXGI_Cascade &gpu_vxgi_cascade = gpu_vxgi->cascades[ cascade_index ];

			//
			const float voxelGridSizeWorld = cascade.sideLength();
			const float voxelSizeWorld = voxelGridSizeWorld / volume_texture_resolution;

			//
			gpu_vxgi_cascade.voxel_radiance_grid_params = V4f::set(
				volume_texture_resolution,
				1.0f / volume_texture_resolution,
				voxelSizeWorld,
				1.0f / voxelSizeWorld
				);

			gpu_vxgi_cascade.voxel_radiance_grid_integer_params = UInt4::set(
				volume_texture_resolution,
				volume_texture_mip_count,
				0,
				0
				);

			gpu_vxgi_cascade.voxel_radiance_grid_offset_and_inverse_size_world = V4f::set(
				V3f::fromXYZ(cascade.minCorner()),
				1.0f / voxelGridSizeWorld
				);

			//gpu_vxgi_cascade.world_to_uvw_space_muladd = V4f::set(
			//	V3f::fromXYZ(cascade.minCorner()),
			//	1.0f / voxelGridSizeWorld
			//	);
		}

#if 0
		//
		{
			for(UINT cascade_index = 0;
				cascade_index < cpu.clipmap.num_cascades;
				cascade_index++)
			{
				const Cascade& this_cascade = cpu.clipmap.cascades[ cascade_index ];

				const V3f uvwOffset = ( finerCascade.minCorner() - this_cascade.minCorner() ) / this_cascade.sideLength();
				const float uvwScale = finerCascade.radius / this_cascade.radius;

				// transform from the UVW-space (volume texture) of the finer cascade into this cascade's space
				gpu_vxgi->cascades[ cascade_index ].world_to_uvw_space_muladd = V4f::set(
					V3f::fromXYZ( uvwOffset ),
					uvwScale
					);
			}
		}
#endif
#if 0
		{
			gpu_vxgi->cascades[ 0 ].transform_to_this_cascade = V4f::set(
				CV3f(0),
				1.0f
				);
			for( UINT cascade_index = 1; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
			{
				const Cascade& this_cascade = cpu.clipmap.cascades[ cascade_index ];
				const Cascade& finerCascade = cpu.clipmap.cascades[ cascade_index - 1 ];

				const V3f uvwOffset = ( finerCascade.minCorner() - this_cascade.minCorner() ) / this_cascade.sideLength();
				const float uvwScale = finerCascade.radius / this_cascade.radius;

				// transform from the UVW-space (volume texture) of the finer cascade into this cascade's space
				gpu_vxgi->cascades[ cascade_index ].transform_to_this_cascade = V4f::set(
					V3f::fromXYZ( uvwOffset ),
					uvwScale
					);
			}
		}
#endif


		//
		gpu_vxgi->params_uint_0 = UInt4::set(
			cpu.clipmap.num_cascades,
			(UINT) volume_texture_resolution,
			(UINT) volume_texture_mip_count,
			0
			);

		gpu_vxgi->params_float_1 = V4f::set(
			(float) volume_texture_resolution,
			1.0f / volume_texture_resolution,
			(float) volume_texture_mip_count,
			0
			);

		gpu_vxgi->params_float_2 = V4f::set(
			1,//giSettings.indirect_light_multiplier,
			1,//giSettings.ambient_occlusion_scale,
			0,
			0
			);
	}//Update CB

	//
	const NGpu::LayerID voxelize_pass_index = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::VXGI_Voxelize )
		;
	const NGpu::LayerID build_voxel_texture_pass_index = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::VXGI_BuildVolumeTexture )
		;
	const NGpu::LayerID unlit_pass_index = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::Unlit )
		;

	for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
	{
		NGpu::SetLayerConstantBuffer(
			voxelize_pass_index + cascade_index,
			cb_handle,
			G_VXGI_Index
			);
		NGpu::SetLayerConstantBuffer(
			build_voxel_texture_pass_index + cascade_index,
			cb_handle,
			G_VXGI_Index
			);
	}

	NGpu::SetLayerConstantBuffer(
		unlit_pass_index,
		cb_handle,
		G_VXGI_Index
		);

	return ALL_OK;
}

ERet VoxelGrids::_UpdateCascade(
								   const U32 cascade_index
								   , const NwCameraView& scene_view
								   , const SpatialDatabaseI& spatial_database
								   , NGpu::NwRenderContext & render_context
								   , const RrGlobalSettings& renderer_settings
								   )
{
	//DEVOUT("Re-Voxelizing objects in cascade %d", cascade_index);

	const U32 cascade_texture_resolution = settings.radiance_texture_settings.GetResolution();

	//
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	{
		//
		const NGpu::LayerID drawingOrder = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::VXGI_Voxelize ) + cascade_index;

		{
			NGpu::ViewState	viewState;
			viewState.clear();
			{
				viewState.viewport.width = cascade_texture_resolution;
				viewState.viewport.height = cascade_texture_resolution;
			}
			NGpu::SetViewParameters( drawingOrder, viewState );
		}

		//
		const Cascade& cascade = cpu.clipmap.cascades[ cascade_index ];

		const AABBf cascade_aabb_world = cascade.ToAabbMinMax();

		//
		RenderEntityList	visible_entities(MemoryHeaps::temporary());
		mxDO(spatial_database.GetEntitiesIntersectingBox(
			visible_entities
			, cascade_aabb_world
			));

		//
		RenderVisibleObjects(
			visible_entities
			, cascade_index
			, CubeMLf::create(
			cascade.minCorner(),
			cascade.sideLength()
			)
			, scene_view
			, gs_voxelization_callbacks
			, Globals::GetRenderPath()
			, renderer_settings
			, render_context
			);
	}

	//
	enum VXGI_Stage
	{
		Voxelize,
		CopyTo3DTexture,
		Filter3DTexture,
	};

	// Copy RWBuffer to 3D texture.
	{
		const U32 view_id = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::VXGI_BuildVolumeTexture ) + cascade_index
			;

		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("vxgi_build_volume_texture")));

		//
		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(view_id, CopyTo3DTexture)
			nwDBG_CMD_SRCFILE_STRING
		);

//		mxDO(technique->SetInput( mxHASH_STR("t_voxelDensityGrid"), NGpu::AsInput(_density_textures[cascade_index]) ));
		mxDO(technique->setUav( mxHASH_STR("srcVoxelGridBuffer"), NGpu::AsOutput(radiance_voxel_grid_buffer) ));
		mxDO(technique->setUav( mxHASH_STR("dstVolumeTexture"), NGpu::AsOutput(_radiance_opacity_texture[cascade_index]) ));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		cmd_writer.SetCBuffer(G_VXGI_Index, cb_handle);

		//
		NGpu::Cmd_DispatchCS cmd_dispatch_compute_shader;
		{
			cmd_dispatch_compute_shader.clear();
			cmd_dispatch_compute_shader.encode( NGpu::CMD_DISPATCH_CS, 0, 0 );

			enum { THREAD_GROUP_SIZE = 8 };
			const int numGroups = cascade_texture_resolution / THREAD_GROUP_SIZE;

			cmd_dispatch_compute_shader.program = pass0.default_program_handle;
			cmd_dispatch_compute_shader.groupsX = numGroups;
			cmd_dispatch_compute_shader.groupsY = numGroups;
			cmd_dispatch_compute_shader.groupsZ = numGroups;
		}
		render_context._command_buffer.put( cmd_dispatch_compute_shader );
	}

	// Filter 3D texture.
	{
		const U32 view_id = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::VXGI_BuildVolumeTexture ) + cascade_index
			;

		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(view_id, Filter3DTexture)
		nwDBG_CMD_SRCFILE_STRING
			);
		//
		NGpu::Cmd_GenerateMips cmd_GenerateMips;
		cmd_GenerateMips.encode(
			NGpu::CMD_GENERATE_MIPS
			, 0
			, NGpu::AsInput(_radiance_opacity_texture[cascade_index]).id
			);

		//
		cmd_writer.SetCBuffer(G_VXGI_Index, cb_handle);

		render_context._command_buffer.put( cmd_GenerateMips );
	}

	return ALL_OK;
}

ERet VoxelGrids::DebugDrawIfNeeded(
									  const NwCameraView& scene_view
									  , NGpu::NwRenderContext & render_context
									  )
{
	if(!settings.debug.flags) {
		return ALL_OK;
	}

	//
	if(settings.debug.flags & DebugFlags::dbg_draw_cascade_bounds)
	{
		for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
		{
			const Cascade& cascade = cpu.clipmap.cascades[ cascade_index ];

			const AABBf cascade_aabb_world = cascade.ToAabbMinMax();

			Globals::GetImmediateDebugRenderer().DrawAABB(cascade_aabb_world);
		}//for
	}

	//
	if(settings.debug.flags & DebugFlags::dbg_show_voxels)
	{
		for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
		{
			_Dbg_VizualizeVolumeTexture(
				cascade_index
				, scene_view
				, render_context
				);
		}//for
	}

	return ALL_OK;
}

ERet VoxelGrids::_Dbg_VizualizeVolumeTexture(
	const U32 cascade_index
	, const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	) const
{
	const Cascade& cascade = cpu.clipmap.cascades[ cascade_index ];

	//
	const U32 view_id = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::Unlit )
		;

	//
	UINT	mip_level_to_visualize = settings.debug.mip_level_to_show;
	mip_level_to_visualize = Clamp<UINT>( mip_level_to_visualize, 0, settings.radiance_texture_settings.dim_log2 - 1 );

	const UINT tex3d_res = 1 << settings.radiance_texture_settings.dim_log2;

	//
	{
		NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			NGpu::buildSortKey( view_id, cascade_index )	// first
		nwDBG_CMD_SRCFILE_STRING
			);

IF_DEVELOPER NGpu::Dbg::GpuScope	dbgscope( render_context._command_buffer, RGBAi::GREEN, "VXGI Dbg Viz Volume (cascade=%d)", cascade_index);

		struct Uniforms
		{
			// depends on the mip level
			UInt3	u_voxel_radiance_grid_size;

			// 0 - the most detailed LoD (the smallest voxels)
			U32		u_mip_level_to_visualize;

			//
			V3f		u_color;
			F32		padding;

			// projects from [0..1] grid space into screen space
			M44f	u_voxel_grid_to_screen_space;
		};

		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("vxgi_dbg_viz_volume_texture")));

		//

		//
		Uniforms	*	uniforms;
		mxDO(technique->BindCBufferData(
			&uniforms
			, render_context._command_buffer
			));
		{
			UInt3	grid_res( tex3d_res >> mip_level_to_visualize );
			uniforms->u_voxel_radiance_grid_size = grid_res;

			uniforms->u_mip_level_to_visualize = mip_level_to_visualize;

			uniforms->u_color = CV3f(1);

			const V3f voxelGridMinCornerWorld = cascade.minCorner();

			M44f	xform = M44_uniformScaling( cascade.sideLength() )
							* M44_buildTranslationMatrix( voxelGridMinCornerWorld )
							* scene_view.view_projection_matrix;

			uniforms->u_voxel_grid_to_screen_space = M44_Transpose( xform );
		}

		mxDO(technique->SetInput(
			nwNAMEHASH("t_voxel_radiance"),
			NGpu::AsInput(_radiance_opacity_texture[cascade_index])
			));

		//
		const NwShaderEffect::Variant& defaultVariant = technique->getDefaultVariant();

		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

		//
		//cmd_writer.SetCBuffer(G_VXGI_Index, cb_handle);

		//
		NGpu::Cmd_Draw	draw_command;
		draw_command.program = technique->getDefaultVariant().program_handle;

		//
		draw_command.input_layout.SetNil();

		draw_command.primitive_topology = NwTopology::PointList;
		draw_command.VB.SetNil();
		draw_command.IB.SetNil();

		draw_command.base_vertex = 0;
		draw_command.vertex_count = ToCube( tex3d_res );
		draw_command.start_index = 0;
		draw_command.index_count = 0;

		//
		cmd_writer.SetRenderState(technique->getDefaultVariant().render_state);

		cmd_writer.Draw( draw_command );
	}//

	return ALL_OK;
}

}//namespace VXGI
}//namespace Rendering
