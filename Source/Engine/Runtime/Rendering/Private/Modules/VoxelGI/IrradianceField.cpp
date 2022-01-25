#include <Base/Base.h>
#pragma hdrstop
#include <Base/Util/Cubemap_Util.h>
#include <Base/Math/Random.h>

#include <Core/Memory/MemoryHeaps.h>
#include <Core/Client.h>	// NwTime

#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Public/render_context.h>
#include <GPU/Public/graphics_system.h>
#include <GPU/Public/Debug/GraphicsDebugger.h>

#include <Rendering/Public/Globals.h>	// NwCameraView, RenderPath
#include <Rendering/Private/Shaders/HLSL/VoxelGI/ddgi_interop.h>
#include <Rendering/Private/Shaders/HLSL/VoxelGI/vxgi_interop.h>	// G_VXGI_Index
#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>	// VoxelGrids
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceFieldUtils.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Rendering/Private/Shaders/HLSL/VoxelGI/bvh_interop.h>


namespace Rendering
{
namespace VXGI
{

namespace
{

///
#if DDGI_CONFIG_PACK_R11G11B10_INTO_UINT
	const DataFormatT IRRADIANCE_PROBE_ATLAS_FORMAT = NwDataFormat::R32_UINT;
#else
	const DataFormatT IRRADIANCE_PROBE_ATLAS_FORMAT = NwDataFormat::R11G11B10_FLOAT;
#endif

const DataFormatT VISIBILITY_PROBE_ATLAS_FORMAT = NwDataFormat::R16G16_FLOAT;


static GPU_DDGI	g_ddgi;

}//namespace

//tbPRINT_SIZE_OF(GPU_DDGI);

IrradianceField::IrradianceField()
{
}

IrradianceField::~IrradianceField()
{
}

ERet IrradianceField::Initialize( const IrradianceFieldSettings& settings )
{
	_ddgi_cb_handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_DDGI);

	//
	_need_to_recompute_probes = true;

	_settings = settings;

	this->_AllocateResources( settings );



	//
#if DEBUG_RAYMARCHING
	{
		const V2f	base_screen_res( 1024, 768 );
		const V2f	half_screen_res = base_screen_res * 0.5f;

		NwTexture2DDescription	debug_texture_description;
		{
			debug_texture_description.format		= NwDataFormat::RGBA8;
			debug_texture_description.width			= half_screen_res.x;
			debug_texture_description.height		= half_screen_res.y;
			debug_texture_description.numMips		= 1;
			debug_texture_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES;
		}

		_raytraced_debug_tex2d = NGpu::createTexture2D(
			debug_texture_description
			, nil
			IF_DEVELOPER, "RaytracedDebugOutput"
			);
		_raytraced_debug_tex2d_size.x = half_screen_res.x;
		_raytraced_debug_tex2d_size.y = half_screen_res.y;

#if MX_DEVELOPER
		Graphics::DebuggerTool::AddTexture2D(
			_raytraced_debug_tex2d
			, debug_texture_description
			, "RaytracedDebugOutput"
			);
#endif
	}
#endif // DEBUG_RAYMARCHING


	return ALL_OK;
}

void IrradianceField::Shutdown()
{
	this->_ReleaseResources();

	nwDESTROY_GLOBAL_CONSTANT_BUFFER(_ddgi_cb_handle);
}

ERet IrradianceField::ApplySettings( const IrradianceFieldSettings& new_settings )
{
	mxENSURE(new_settings.IsValid(), ERR_INVALID_PARAMETER, "");

	if( !_settings.equals( new_settings ) )
	{
		DBGOUT("[VXGI] Applying new settings...");

		const bool probe_grid_dim_changed
			= _settings.probe_grid_dim_log2 != new_settings.probe_grid_dim_log2
			;

		if(probe_grid_dim_changed)
		{
			DBGOUT("[DDGI] Re-Alocating Resources...");

			this->_ReleaseResources();
			this->_AllocateResources( new_settings );
		}

		_settings = new_settings;
	}

	return ALL_OK;
}
//
//ERet _DoApplyNewSettings( const IrradianceFieldSettings& new_settings )
//{
//	return ALL_OK;
//}

void IrradianceField::_AllocateResources( const IrradianceFieldSettings& settings )
{
	const UInt3 probe_counts(settings.probeGridResolution());

	const U32	atlas_width_in_probes = probe_counts.x * probe_counts.z;
	const U32	atlas_height_in_probes = probe_counts.y;


	// We encode spherical data in an octahedral parameterization, packing all the probes in an atlas.
	// One-pixel texture gutter/border ensures correct bilinear interpolation.


	// Irradiance
	U32	irradiance_probes_atlas_size = 0;
	{
		// 1-pixel of padding surrounding each probe
		const U32 irradiance_probe_size_with_padding = (1 + IRRADIANCE_PROBE_DIM + 1);
		const U32 irradiance_atlas_width = irradiance_probe_size_with_padding * atlas_width_in_probes;
		const U32 irradiance_atlas_height = irradiance_probe_size_with_padding * atlas_height_in_probes;
		mxASSERT(irradiance_atlas_width % 4 == 0);
		mxASSERT(irradiance_atlas_height % 4 == 0);

		//
		NwTexture2DDescription	irradiance_probes_atlas_description;
		{
			irradiance_probes_atlas_description.format		= IRRADIANCE_PROBE_ATLAS_FORMAT;
			irradiance_probes_atlas_description.width		= irradiance_atlas_width;
			irradiance_probes_atlas_description.height		= irradiance_atlas_height;
			irradiance_probes_atlas_description.numMips		= 1;
			irradiance_probes_atlas_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES;
		}

		// create two textures for ping-ponging, because we cannot RWTexture3D< float3 > in Direct3D 11
		_pingpong[0].irradiance_probes_tex2d = NGpu::createTexture2D(
			irradiance_probes_atlas_description
			, nil
			IF_DEVELOPER, "IrradianceField:Probes_0"
			);
		_pingpong[1].irradiance_probes_tex2d = NGpu::createTexture2D(
			irradiance_probes_atlas_description
			, nil
			IF_DEVELOPER, "IrradianceField:Probes_1"
			);

		_curr_dst_tex_idx = 0;

#if MX_DEVELOPER
		Graphics::DebuggerTool::AddTexture2D(
			_pingpong[0].irradiance_probes_tex2d
			, irradiance_probes_atlas_description
			, "IrradianceField:Probes"
			);
		Graphics::DebuggerTool::AddTexture2D(
			_pingpong[1].irradiance_probes_tex2d
			, irradiance_probes_atlas_description
			, "IrradianceField:Probes2"
			);
#endif
		_irradiance_probes_tex2d_size.x = irradiance_atlas_width;
		_irradiance_probes_tex2d_size.y = irradiance_atlas_height;

		irradiance_probes_atlas_size = irradiance_probes_atlas_description.CalcRawSize();
	}


	// Depth/Distance/Visibility
	U32	visibility_probes_atlas_size = 0;
	{
		const U32 depth_probe_size_with_padding = (1 + DEPTH_PROBE_DIM + 1);
		const int depth_atlas_width = depth_probe_size_with_padding * atlas_width_in_probes;
		const int depth_atlas_height = depth_probe_size_with_padding * atlas_height_in_probes;
		mxASSERT(depth_atlas_width % 4 == 0);
		mxASSERT(depth_atlas_height % 4 == 0);

		//
		NwTexture2DDescription	visibility_probes_atlas_description;
		{
			visibility_probes_atlas_description.format		= VISIBILITY_PROBE_ATLAS_FORMAT;
			visibility_probes_atlas_description.width		= depth_atlas_width;
			visibility_probes_atlas_description.height		= depth_atlas_height;
			visibility_probes_atlas_description.numMips		= 1;
			visibility_probes_atlas_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES;
		}

		_pingpong[0].visibility_probes_tex2d = NGpu::createTexture2D(
			visibility_probes_atlas_description
			, nil
			IF_DEVELOPER, "IrradianceField:Depths_0"
			);
		_pingpong[1].visibility_probes_tex2d = NGpu::createTexture2D(
			visibility_probes_atlas_description
			, nil
			IF_DEVELOPER, "IrradianceField:Depths_1"
			);
#if MX_DEVELOPER
		Graphics::DebuggerTool::AddTexture2D(
			_pingpong[0].visibility_probes_tex2d
			, visibility_probes_atlas_description
			, "IrradianceField:Depths_0"
			);
		Graphics::DebuggerTool::AddTexture2D(
			_pingpong[1].visibility_probes_tex2d
			, visibility_probes_atlas_description
			, "IrradianceField:Depths_1"
			);
#endif

		_visibility_probes_tex2d_size.x = depth_atlas_width;
		_visibility_probes_tex2d_size.y = depth_atlas_height;

		visibility_probes_atlas_size = visibility_probes_atlas_description.CalcRawSize();
	}


	//
	U32	raytraced_radiance_and_depth_texture_size = 0;
	{
		NwTexture2DDescription	raytraced_radiance_and_depth_texture_description;
		{
			raytraced_radiance_and_depth_texture_description.format		= NwDataFormat::RGBA16F;
			raytraced_radiance_and_depth_texture_description.width		= RAYS_PER_PROBE;
			raytraced_radiance_and_depth_texture_description.height		= settings.numProbes();
			raytraced_radiance_and_depth_texture_description.numMips	= 1;
			raytraced_radiance_and_depth_texture_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES;
		}
		raytraced_radiance_and_depth_texture_size = raytraced_radiance_and_depth_texture_description.CalcRawSize();

		_raytraced_radiance_and_distance_tex2d = NGpu::createTexture2D(
			raytraced_radiance_and_depth_texture_description
			, nil
			IF_DEVELOPER, "RayTracedRadianceAndDepths"
			);
		_raytraced_radiance_and_distance_tex2d_size.x = raytraced_radiance_and_depth_texture_description.width;
		_raytraced_radiance_and_distance_tex2d_size.y = raytraced_radiance_and_depth_texture_description.height;
#if MX_DEVELOPER
		Graphics::DebuggerTool::AddTexture2D(
			_raytraced_radiance_and_distance_tex2d
			, raytraced_radiance_and_depth_texture_description
			, "RayTracedRadianceAndDepths"
			);
#endif
	}


	//
	DEVOUT( "DDGI: Allocated: %d probes, irradiance atlas: %u KiB, visibility atlas: %u KiB, tracebuf: %u KiB -> %u KiB in total"
		, probe_counts.boxVolume()
		, irradiance_probes_atlas_size / mxKILOBYTE
		, visibility_probes_atlas_size / mxKILOBYTE
		, raytraced_radiance_and_depth_texture_size / mxKILOBYTE
		, (irradiance_probes_atlas_size + visibility_probes_atlas_size + raytraced_radiance_and_depth_texture_size) / mxKILOBYTE
		);

	//
	NGpu::NextFrame();
}

void IrradianceField::_ReleaseResources()
{
	for(int i = 0; i < mxCOUNT_OF(_pingpong); i++)
	{
		DDGIResources& resources = _pingpong[i];

		NGpu::DeleteTexture( resources.irradiance_probes_tex2d );
		resources.irradiance_probes_tex2d.SetNil();

		NGpu::DeleteTexture( resources.visibility_probes_tex2d );
		resources.visibility_probes_tex2d.SetNil();
	}

	NGpu::DeleteBuffer(_ddgi_cb_handle);
	_ddgi_cb_handle.SetNil();
}

void IrradianceField::SetDirty()
{
	_need_to_recompute_probes = true;
}

const DDGIResources& IrradianceField::GetDstIrradianceProbes() const
{
	return _pingpong[ _curr_dst_tex_idx ];
}

const DDGIResources& IrradianceField::GetPrevReadOnlyIrradianceProbes() const
{
	return _pingpong[ _curr_dst_tex_idx ^ 1 ];
}

ERet IrradianceField::UpdateProbes(
	const NwCameraView& scene_view
	, const VoxelGrids& voxel_grids
	, const NwTime& time
	, NGpu::NwRenderContext & render_context
	)
{
	{
		const int probe_grid_resolution = _settings.probeGridResolution();
		
		const float interprobe_spacing
			= _settings.bounds.sideLength() / (probe_grid_resolution - 1)
			;
		const V3f probe_grid_min_corner = _settings.bounds.minCorner();

		//
		g_ddgi.probe_grid_origin_and_spacing = V4f::set(
			probe_grid_min_corner,
			interprobe_spacing
			);

		g_ddgi.probe_grid_params0 = V4f::set(
			1.0f / interprobe_spacing,
			0, 0, 0
			);

		g_ddgi.probe_grid_res_params = UInt4::set(
			probe_grid_resolution,
			0, 0, 0
			);

		g_ddgi.probe_texture_params = V4f::set(
			1.0f / (float)_irradiance_probes_tex2d_size.x,
			1.0f / (float)_irradiance_probes_tex2d_size.y,
			1.0f / (float)_visibility_probes_tex2d_size.x,
			1.0f / (float)_visibility_probes_tex2d_size.y
			);

		g_ddgi.probe_max_ray_distance = _settings.probe_max_ray_distance;
		g_ddgi.probe_hysteresis = _settings.probe_hysteresis;
		g_ddgi.probe_distance_exponent = _settings.probe_distance_exponent;
		g_ddgi.probe_irradiance_encoding_gamma = _settings.probe_irradiance_encoding_gamma;

		g_ddgi.probe_change_threshold = _settings.probe_change_threshold;
		g_ddgi.probe_brightness_threshold = _settings.probe_brightness_threshold;
		g_ddgi.view_bias = _settings.view_bias;
		g_ddgi.normal_bias = _settings.normal_bias;

		//
		g_ddgi.debug_params = V4f::set(
			_settings.dbg_viz_light_probe_scale, 0, 0, 0
			);
	}


	//
	{
		//
		const NGpu::LayerID gen_rays_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_GenerateRays )
			;
		const NGpu::LayerID trace_rays_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_TraceProbeRays )
			;
		const NGpu::LayerID update_probes_pass_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_UpdateIrradianceProbes )
			;
		const NGpu::LayerID update_distance_probes_pass_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_UpdateDistanceProbes )
			;
		const NGpu::LayerID viz_probes_pass_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_VisualizeProbes )
			;

		//
		const NGpu::LayerID global_light_pass_index = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DeferredGlobalLights )
			;

		//for( UINT cascade_index = 0; cascade_index < cpu.clipmap.num_cascades; cascade_index++ )
		const UINT cascade_index = 0;
		{
			NGpu::SetLayerConstantBuffer(
				gen_rays_index + cascade_index,
				_ddgi_cb_handle,
				G_DDGI_Index
				);

			//
			NGpu::SetLayerConstantBuffer(
				trace_rays_index + cascade_index,
				_ddgi_cb_handle,
				G_DDGI_Index
				);
			NGpu::SetLayerConstantBuffer(
				trace_rays_index + cascade_index,
				voxel_grids.cb_handle,
				G_VXGI_Index
				);

			//
			NGpu::SetLayerConstantBuffer(
				update_probes_pass_index + cascade_index,
				_ddgi_cb_handle,
				G_DDGI_Index
				);
			NGpu::SetLayerConstantBuffer(
				update_distance_probes_pass_index + cascade_index,
				_ddgi_cb_handle,
				G_DDGI_Index
				);
		}

		//
		NGpu::SetLayerConstantBuffer(
			viz_probes_pass_index,
			_ddgi_cb_handle,
			G_DDGI_Index
			);
		NGpu::SetLayerConstantBuffer(
			viz_probes_pass_index,
			voxel_grids.cb_handle,
			G_VXGI_Index
			);


		//
		NGpu::SetLayerConstantBuffer(
			global_light_pass_index,
			_ddgi_cb_handle,
			G_DDGI_Index
			);
		NGpu::SetLayerConstantBuffer(
			global_light_pass_index,
			voxel_grids.cb_handle,
			G_VXGI_Index
			);
	}



	//
	mxDO(_GenerateRaysAndUpdateShaderConstants(
		time
		));

	//
	if(!_need_to_recompute_probes) {
		return ALL_OK;
	}

	mxDO(_TraceRays(
		voxel_grids
		, render_context
		));

	mxDO(_UpdateAndBlendProbes(
		render_context
		));
	//mxDO(_FixProbesBorders());

	//UNDONE;
	return ALL_OK;
}

ERet IrradianceField::_GenerateRaysAndUpdateShaderConstants(
	const NwTime& time
	)
{
	//
	if(
		_need_to_recompute_probes
		||
		!(_settings.flags & IrradianceFieldFlags::dbg_freeze_ray_directions)
		)
	{
		// Generate diffuse (probe) rays
		NwRandom	rng( time.real.frame_number );
		const Q4f	random_orientation = GetRandomQuaternion(rng);

		//
		const float inverse_samples_count = 1.0f / RAYS_PER_PROBE;

		//
		for(UINT ray_idx = 0; ray_idx < RAYS_PER_PROBE; ray_idx++)
		{
			const V3f dir = SphericalFibonacci(ray_idx, inverse_samples_count);
			const V3f rotated_dir = RotateVectorByQuaternion(dir, random_orientation);
			mxASSERT(fabs(rotated_dir.length() - 1) < 1e-5);
			g_ddgi.ray_directions[ ray_idx ] = V4f::set( rotated_dir, 0 );
		}
	}

	//
#if MX_DEVELOPER
	if(_settings.flags & IrradianceFieldFlags::dbg_visualize_rays)
	{
		for(UINT ray_idx = 0; ray_idx < RAYS_PER_PROBE; ray_idx++)
		{
			const V3f& rotated_dir = g_ddgi.ray_directions[ ray_idx ].asVec3();

			Globals::GetImmediateDebugRenderer().DrawLine(
				CV3f(0)
				, rotated_dir * 50.0f
				, RGBAf::GREEN
				);
		}
	}
#endif

	//
	{
		GPU_DDGI	*	gpu_ddgi;
		mxDO(NGpu::updateGlobalBuffer(
			_ddgi_cb_handle,
			&gpu_ddgi
			));

		//
		*gpu_ddgi = g_ddgi;
	}

	return ALL_OK;
}

ERet IrradianceField::_TraceRays(
								 const VoxelGrids& voxel_grids
								 , NGpu::NwRenderContext & render_context
								 )
{
	//
	const U32 trace_probe_rays_view_id = Globals::GetRenderPath()
		.getPassDrawingOrder( ScenePasses::DDGI_TraceProbeRays )
		;

	{
		NwShaderEffect* trace_probe_rays_technique;
		mxDO(Resources::Load(trace_probe_rays_technique, MakeAssetID("ddgi_trace_probe_rays")));

		//
		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(trace_probe_rays_view_id, 0)
			nwDBG_CMD_SRCFILE_STRING
		);

		/*mxDO*/(trace_probe_rays_technique->SetInput(
			nwNAMEHASH("t_sdf_atlas3d"),
			NGpu::AsInput(voxel_grids.brickmap->h_sdf_brick_atlas_tex3d)
		));

		mxDO(trace_probe_rays_technique->SetInput(
			nwNAMEHASH("t_radiance_opacity_volume"),
			NGpu::AsInput(voxel_grids._radiance_opacity_texture[0])
		));

		mxDO(trace_probe_rays_technique->setUav(
			mxHASH_STR("t_dst_hit_radiance_and_distance"),
			NGpu::AsOutput(_raytraced_radiance_and_distance_tex2d)
		));

		const NwShaderEffect::Pass& pass0 = trace_probe_rays_technique->getPasses()[0];

		//
		mxDO(trace_probe_rays_technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );
		cmd_writer.SetCBuffer(G_SDF_BVH_CBuffer_Index, voxel_grids.voxel_BVH->_bvh_cb_handle);

		//
		NGpu::Cmd_DispatchCS cmd_dispatch_compute_shader;
		{
			cmd_dispatch_compute_shader.clear();
			cmd_dispatch_compute_shader.encode( NGpu::CMD_DISPATCH_CS, 0, 0 );

			enum { THREAD_GROUP_SIZE_X = 32 };

			cmd_dispatch_compute_shader.program = pass0.default_program_handle;
			cmd_dispatch_compute_shader.groupsX = RAYS_PER_PROBE / THREAD_GROUP_SIZE_X;
			cmd_dispatch_compute_shader.groupsY = _settings.numProbes();
			cmd_dispatch_compute_shader.groupsZ = 1;
		}
		render_context._command_buffer.put( cmd_dispatch_compute_shader );
	}



#if DEBUG_RAYMARCHING
	{
		NwShaderEffect* dbg_technique;
		mxDO(Resources::Load(dbg_technique, MakeAssetID("vxgi_dbg_raymarching")));

		//
		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(trace_probe_rays_view_id, 2)
			nwDBG_CMD_SRCFILE_STRING
			);

		mxDO(dbg_technique->SetInput(
			nwNAMEHASH("t_radiance_opacity_volume"),
			NGpu::AsInput(voxel_grids._radiance_opacity_texture[0])
			));

		mxDO(dbg_technique->setUav(
			mxHASH_STR("t_output_color"),
			NGpu::AsOutput(_raytraced_debug_tex2d)
			));

		//
		mxDO(dbg_technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

		cmd_writer.SetCBuffer(G_SDF_BVH_CBuffer_Index, voxel_grids.voxel_BVH->_bvh_cb_handle);

		//
		enum { THREAD_GROUP_SIZE = 8 };

		cmd_writer.DispatchCS(
			dbg_technique->getDefaultVariant().program_handle
			, _raytraced_debug_tex2d_size.x / THREAD_GROUP_SIZE
			, _raytraced_debug_tex2d_size.y / THREAD_GROUP_SIZE
			, 1
			);
	}
#endif // DEBUG_RAYMARCHING

	return ALL_OK;
}

ERet IrradianceField::_UpdateAndBlendProbes(
	NGpu::NwRenderContext & render_context
	)
{
	const UInt3 probe_grid_dims = _settings.probeGridDimensions();


	const DDGIResources& dst_probes = GetDstIrradianceProbes();
	const DDGIResources& prev_probes = GetPrevReadOnlyIrradianceProbes();

	// Update irradiance probes.
	{
		const U32 update_probes_view_id = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_UpdateIrradianceProbes )
			;

		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(update_probes_view_id, 0)
			nwDBG_CMD_SRCFILE_STRING
			);

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("ddgi_update_irradiance_probes")));

		//
		mxDO(technique->SetInput(
			nwNAMEHASH("t_src_raytraced_probe_radiance_and_distance"),
			NGpu::AsInput(_raytraced_radiance_and_distance_tex2d)
			));

		mxDO(technique->SetInput(
			nwNAMEHASH("t_prev_probe_irradiance"),
			NGpu::AsInput(prev_probes.irradiance_probes_tex2d)
			));

		mxDO(technique->setUav(
			mxHASH_STR("t_probe_irradiance"),
			NGpu::AsOutput(dst_probes.irradiance_probes_tex2d)
			));

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

		//
		mxDO(cmd_writer.DispatchCS(
			technique->getDefaultVariant().program_handle
			, probe_grid_dims.x * probe_grid_dims.z
			, probe_grid_dims.y
			, 1
			));

		////
		//mxDO(cmd_writer.CopyTexture(
		//	GetPrevReadOnlyIrradianceProbesTexture()
		//	, GetDstIrradianceProbesTexture()
		//	));
		_curr_dst_tex_idx ^= 1;
	}

	// Update depth probes.
	if(0 == (_settings.flags & IrradianceFieldFlags::dbg_skip_updating_depth_probes))
	{
		const U32 update_probes_view_id = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_UpdateDistanceProbes )
			;

		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey(update_probes_view_id, 0)
			nwDBG_CMD_SRCFILE_STRING
			);

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("ddgi_update_distance_probes")));

		//
		mxDO(technique->SetInput(
			nwNAMEHASH("t_src_raytraced_probe_radiance_and_distance"),
			NGpu::AsInput(_raytraced_radiance_and_distance_tex2d)
			));

		mxDO(technique->SetInput(
			nwNAMEHASH("t_prev_probe_distance"),
			NGpu::AsInput(prev_probes.visibility_probes_tex2d)
			));

		mxDO(technique->setUav(
			mxHASH_STR("t_probe_distance"),
			NGpu::AsOutput(dst_probes.visibility_probes_tex2d)
			));

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

		//
		cmd_writer.DispatchCS(
			technique->getDefaultVariant().program_handle
			, probe_grid_dims.x * probe_grid_dims.z
			, probe_grid_dims.y
			, 1
			);
	}

	return ALL_OK;
}

ERet IrradianceField::DebugDrawIfNeeded(
	const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	)
{
	//
	if(_settings.flags & IrradianceFieldFlags::dbg_draw_grid_bounds)
	{
		Globals::GetImmediateDebugRenderer().DrawAABB(
			_settings.bounds.ToAabbMinMax()
			);
	}

	//
	if(_settings.flags & IrradianceFieldFlags::dbg_visualize_light_probes)
	{
		//
		const U32 view_id = Globals::GetRenderPath()
			.getPassDrawingOrder( ScenePasses::DDGI_VisualizeProbes )
			;
		//
		{
			NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
				render_context,
				NGpu::buildSortKey( view_id, 10 )	// first
			nwDBG_CMD_SRCFILE_STRING
				);

			//
			NwShaderEffect* technique;
			mxDO(Resources::Load(technique, MakeAssetID("ddgi_visualize_probes")));


			//
			(technique->SetInput(
				nwNAMEHASH("t_probe_irradiance"),
				NGpu::AsInput(GetDstIrradianceProbes().irradiance_probes_tex2d)
				));

			//Depth data is not used
			//(technique->SetInput(
			//	mxHASH_STR("t_probe_distance"),
			//	NGpu::AsInput(_visibility_probes_tex2d)
			//	));

			//
			const NwShaderEffect::Variant& defaultVariant = technique->getDefaultVariant();

			mxDO(technique->pushShaderParameters( render_context._command_buffer ));

			//
			NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

			//
			NGpu::Cmd_Draw	draw_command;
			draw_command.program = technique->getDefaultVariant().program_handle;

			//
			draw_command.input_layout.SetNil();

			draw_command.primitive_topology = NwTopology::PointList;
			draw_command.VB.SetNil();
			draw_command.IB.SetNil();

			draw_command.base_vertex = 0;
			draw_command.vertex_count = _settings.numProbes();
			draw_command.start_index = 0;
			draw_command.index_count = 0;

			//
			cmd_writer.SetRenderState(
				technique->getDefaultVariant().render_state
				);

			cmd_writer.Draw( draw_command );
		}
	}

	return ALL_OK;
}

#if 0

mxBEGIN_FLAGS( AmbientCubeFaceFlagsT )
	mxREFLECT_BIT( PosX, AmbientCubeFaceFlags::PosX ),
	mxREFLECT_BIT( NegX, AmbientCubeFaceFlags::NegX ),
	mxREFLECT_BIT( PosY, AmbientCubeFaceFlags::PosY ),
	mxREFLECT_BIT( NegY, AmbientCubeFaceFlags::NegY ),
	mxREFLECT_BIT( PosZ, AmbientCubeFaceFlags::PosZ ),
	mxREFLECT_BIT( NegZ, AmbientCubeFaceFlags::NegZ ),
mxEND_FLAGS

// Stolen from cmft - Cubemap Filtering Tool (by Dario Manesku).
namespace CubeMapUtil
{

/// Creates a cubemap containing tap vectors and solid angle of each texel on the cubemap.
/// Output consists of 6 faces of specified size containing (x,y,z,angle) floats for each texel.


static const V3f s_ambientCubeDirections[6] =
{
	CV3f( +1.0f,  0.0f,  0.0f ),
	CV3f( -1.0f,  0.0f,  0.0f ),

	CV3f(  0.0f, +1.0f,  0.0f ),
	CV3f(  0.0f, -1.0f,  0.0f ),

	CV3f(  0.0f,  0.0f, +1.0f ),
	CV3f(  0.0f,  0.0f, -1.0f ),
};



/// http://www.mpia-hd.mpg.de/~mathar/public/mathar20051002.pdf
/// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
static inline float areaElement(float _x, float _y)
{
    return atan2f(_x*_y, sqrtf(_x*_x + _y*_y + 1.0f));
}

/// _u and _v should be center addressing and in [-1.0+invSize..1.0-invSize] range.
static inline float texelSolidAngle(float _u, float _v, float _invFaceSize)
{
    // Specify texel area.
    const float x0 = _u - _invFaceSize;
    const float x1 = _u + _invFaceSize;
    const float y0 = _v - _invFaceSize;
    const float y1 = _v + _invFaceSize;

    // Compute solid angle of texel area.
    const float solidAngle = areaElement(x1, y1)
                           - areaElement(x0, y1)
                           - areaElement(x1, y0)
                           + areaElement(x0, y0)
                           ;

    return solidAngle;
}

static inline float solidAngleToConeApexHalfAngle( float solidAngle )
{
	return mmACos( 1.0f - solidAngle * mxINV_TWOPI );
}

static void precomputeRayTaps(
							  CubemapVectors *lut_
							  )
{
	const float cfs = float(GPU_LIGHTPROBE_CUBEMAP_RES);
	const float invCfs = 1.0f/cfs;

	float	cubemapFaceWeights[GPU_NUM_CUBEMAP_FACES] = { 0 };

	float	totalSolidAngle = 0;

	for( int cubemapFace = 0; cubemapFace < GPU_NUM_CUBEMAP_FACES; cubemapFace++ )
	{
		float	faceSolidAngle = 0;

		float yyf = 1.0f;
		for( int iY = 0; iY < GPU_LIGHTPROBE_CUBEMAP_RES; iY++, yyf+=2.0f )
		{
			float xxf = 1.0f;
			for( int iX = 0; iX < GPU_LIGHTPROBE_CUBEMAP_RES; iX++, xxf+=2.0f )
			{
				//scale up to [-1, 1] range (inclusive), offset by 0.5 to point to texel center.

				// From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
				// Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
				//      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
				const float uu = xxf*invCfs - 1.0f;
				const float vv = yyf*invCfs - 1.0f;

				const V3f	direction = Cubemap_Util::texelCoordToVec( uu, vv, cubemapFace );
				const float	coneSolidAngle = texelSolidAngle( uu, vv, invCfs );
				const float	coneApexHalfAngle = solidAngleToConeApexHalfAngle( coneSolidAngle );

				//
				lut_->cones[cubemapFace][iX][iY] = CV4f(
					direction,
					2.0f * mmTan( coneApexHalfAngle )
					);

				//
				for( int j = 0; j < GPU_NUM_CUBEMAP_FACES; j++ )
				{
					const float NdotL = V3f::dot( direction, s_ambientCubeDirections[j] );
					const float NdotL_01 = Clamp( NdotL, 0.0f, 1.0f );

					cubemapFaceWeights[j] += NdotL_01;
				}

				//
				//IF_DEVELOPER ptPRINT("[%d][%d]: solid_angle = %.3f, apex_half_angle = %.3f, weight = %f, dir = (%.3f, %.3f, %.3f)"
				//	, iX, iY, coneSolidAngle, RAD2DEG(coneSolidAngle), coneSolidAngle/mxFOUR_PI, direction.x, direction.y, direction.z);

				faceSolidAngle += coneSolidAngle;
			}//x
		}//y

		//IF_DEVELOPER ptPRINT("Face %d: face_solid_angle = %.3f", cubemapFace, faceSolidAngle);
		//face_solid_angle = 2.094

		totalSolidAngle += faceSolidAngle;
	}

	//
	for( int j = 0; j < GPU_NUM_CUBEMAP_FACES; j++ )
	{
		const float totalWeight = cubemapFaceWeights[j];
		lut_->weights[j] = 1.0f / totalWeight;

		//IF_DEVELOPER ptPRINT("Face[%d]: totalWeight = %.3f, scale = %f", j, totalWeight, lut_->weights[j]);
		//totalWeight = 99.306, scale = 0.010070
	}

	//IF_DEVELOPER ptPRINT("total_solid_angle = %.3f", totalSolidAngle);	// 12.566
}

}//namespace CubeMapUtil


IrradianceField::LightProbeGridCascade::LightProbeGridCascade()
{
	for( UINT cube_side = 0; cube_side < mxCOUNT_OF(texture_handles); cube_side++ )
	{
		texture_handles[cube_side].SetNil();
	}
}

void IrradianceField::LightProbeGridCascade::release()
{
	for( UINT cube_side = 0; cube_side < mxCOUNT_OF(texture_handles); cube_side++ )
	{
		if( texture_handles[cube_side].IsValid() )
		{
			GL::DeleteTexture( texture_handles[cube_side] );
			texture_handles[cube_side].SetNil();
		}
	}
}

IrradianceField::IrradianceField()
{
	_skyIrradianceMap.SetNil();
	_skyIrradianceMap_Staging.SetNil();
	mxZERO_OUT(_current_settings);
}

IrradianceField::~IrradianceField()
{
}

ERet IrradianceField::Initialize( const IrradianceFieldSettings& settings )
{
	mxZERO_OUT(_skyAmbientCube);

	//
	_cubemapVectors = mxNEW( MemoryHeaps::renderer(), CubemapVectors );

	CubeMapUtil::precomputeRayTaps( _cubemapVectors );

	//
	return this->ApplySettings( settings );
}

void IrradianceField::Shutdown()
{
	this->_ReleaseResources();

	mxDELETE( _cubemapVectors, MemoryHeaps::renderer() );
}

ERet IrradianceField::ApplySettings( const IrradianceFieldSettings& new_settings )
{
	if( !_current_settings.equals( new_settings ) )
	{
		this->_ReleaseResources();
		this->_AllocateResources( new_settings );
	}

	return ALL_OK;
}

void IrradianceField::_AllocateResources( const IrradianceFieldSettings& settings )
{
	this->_allocateIrrandianceVolumeTextures( settings );
	this->_allocateSkyIrrandianceBuffer( settings );

	_current_settings = settings;

	GL::NextFrame();
}

void IrradianceField::_allocateIrrandianceVolumeTextures( const IrradianceFieldSettings& settings )
{
	const UINT voxelGridResolution = settings.irradiance_volume_texture.volumeTextureResolution();

	NwTexture3DDescription	tex3d_description;
	tex3d_description.format	= NwDataFormat::R11G11B10_FLOAT;
	tex3d_description.width		= voxelGridResolution;
	tex3d_description.height	= voxelGridResolution;
	tex3d_description.depth		= voxelGridResolution;
	tex3d_description.numMips	= 1;
	tex3d_description.flags.raw_value = NwTextureCreationFlags::RANDOM_WRITES;

	for( UINT cube_side = 0; cube_side < 6; cube_side++ )
	{
		//IF_DEVELOPER Str::Format( tex3d_description.name, "IrradianceVolume_0_[%d]", cube_side );
		_cascade0.texture_handles[ cube_side ] = GL::createTexture3D(
			tex3d_description
			, nil
			IF_DEVELOPER , "IrradianceVolume"
			);
	}
}

void IrradianceField::_allocateSkyIrrandianceBuffer( const IrradianceFieldSettings& settings )
{
	NwBufferDescription	desc(_InitDefault);
	desc.type = Buffer_Data;
	desc.size = sizeof(SkyAmbientCube);
	desc.stride = sizeof(SkyAmbientCube);
	desc.flags.raw_value = NwBufferFlags::Write;

	_skyIrradianceMap = GL::CreateBuffer(desc);

	//
	desc.flags.raw_value = NwBufferFlags::Staging;
	_skyIrradianceMap_Staging = GL::CreateBuffer(desc);
}

void IrradianceField::_ReleaseResources()
{
	_cascade0.release();

	if( _skyIrradianceMap.IsValid() )
	{
		GL::DeleteBuffer( _skyIrradianceMap );
		_skyIrradianceMap.SetNil();
	}

	if( _skyIrradianceMap_Staging.IsValid() )
	{
		GL::DeleteBuffer( _skyIrradianceMap_Staging );
		_skyIrradianceMap_Staging.SetNil();
	}
}

ERet IrradianceField::generateSkyIrradianceMap(
	const GL::LayerID viewId
	, GL::NwRenderContext & render_context
	, const RrGlobalSettings& renderer_settings
	)
{
	const UINT irradiance_volume_texture_resolution = _current_settings.irradiance_volume_texture.volumeTextureResolution();

	//
	const GL::LayerID drawingOrder = viewId + 0;
	U32	sort_key = 0;

	{
		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("gi_generate_sky_irradiance_map")));

		//
		GL::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			GL::buildSortKey( drawingOrder, sort_key++ )
			);

		//
		CPP_PREALIGN_CBUFFER struct GenerateLightProbesUniforms
		{
			V4f		u_cones[GPU_NUM_CUBEMAP_FACES][GPU_LIGHTPROBE_CUBEMAP_RES][GPU_LIGHTPROBE_CUBEMAP_RES];
			float	u_faceWeight;
		};

		GenerateLightProbesUniforms	*	uniforms;
		mxDO(technique->BindCBufferData(
			render_context._command_buffer
			, &uniforms
			));
		{
			{
				for( int face = 0; face < GPU_NUM_CUBEMAP_FACES; face++ )
				{
					for( int iX = 0; iX < GPU_LIGHTPROBE_CUBEMAP_RES; iX++ )
					{
						for( int iY = 0; iY < GPU_LIGHTPROBE_CUBEMAP_RES; iY++ )
						{
							uniforms->u_cones[face][iX][iY] = _cubemapVectors->cones[face][iX][iY];
						}
					}
				}

				uniforms->u_faceWeight = _cubemapVectors->weights[0];
			}
		}

		//
		mxDO(technique->setUav( mxHASH_STR("rwsb_output"), GL::AsOutput(_skyIrradianceMap) ));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		GL::Cmd_DispatchCS cmd_dispatch_compute_shader;
		{
			cmd_dispatch_compute_shader.clear();
			cmd_dispatch_compute_shader.encode( GL::CMD_DISPATCH_CS, 0, 0 );

			cmd_dispatch_compute_shader.program = pass0.default_program_handle;

			cmd_dispatch_compute_shader.groupsX = 1;
			cmd_dispatch_compute_shader.groupsY = 1;
			cmd_dispatch_compute_shader.groupsZ = 1;
		}
		render_context._command_buffer.put( cmd_dispatch_compute_shader );
	}

	//
	{
		GL::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			GL::buildSortKey( drawingOrder, sort_key++ )
			);

		GL::Cmd_CopyResource	copyResourceCommand;
		{
			copyResourceCommand.encode( GL::CMD_COPY_RESOURCE, 0, 0 );

			copyResourceCommand.source = _skyIrradianceMap;
			copyResourceCommand.destination = _skyIrradianceMap_Staging;
		}
		render_context._command_buffer.put( copyResourceCommand );
	}

	return ALL_OK;
}

void IrradianceField::copySkyIrradianceMapFromStaging(
	const GL::LayerID viewId
	, GL::NwRenderContext & render_context
	)
{
	const GL::LayerID drawingOrder = viewId + 0;
	U32	sort_key = ~0;	// last

	GL::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		GL::buildSortKey( drawingOrder, sort_key++ )
		);

	GL::Cmd_MapRead	mapReadCommand;
	{
		mapReadCommand.encode( GL::CMD_MAP_READ, 0, 0 );

		mapReadCommand.buffer = _skyIrradianceMap_Staging;
		mapReadCommand.callback = &_copySkyIrradianceMapFromStaging_Callback;
		mapReadCommand.userData = this;
	}
	render_context._command_buffer.put( mapReadCommand );
}

void IrradianceField::_copySkyIrradianceMapFromStaging_Callback( void* userData, void *mappedData )
{
	IrradianceField* lpg = static_cast< IrradianceField* >( userData );

	SkyAmbientCube* sac = static_cast< SkyAmbientCube* >( mappedData );

	lpg->_skyAmbientCube = *sac;

	//ZZZ
	LogStream(LL_Warn),"posX: ",sac->irradiance_posX;
	LogStream(LL_Warn),"negX: ",sac->irradiance_negX;
	LogStream(LL_Warn),"posY: ",sac->irradiance_posY;
	LogStream(LL_Warn),"negY: ",sac->irradiance_negY;
	LogStream(LL_Warn),"posZ: ",sac->irradiance_posZ;
	LogStream(LL_Warn),"negZ: ",sac->irradiance_negZ;
}

ERet IrradianceField::generateLightProbes(
	const GL::LayerID viewId
	, const IrradianceField& voxelConeTracing
	, GL::NwRenderContext & render_context
	, const RrGlobalSettings& renderer_settings
	)
{
	const LightProbeGridCascade& lightProbeCascade = _cascade0;

	const UINT irradiance_volume_texture_resolution = _current_settings.irradiance_volume_texture.volumeTextureResolution();

	//
	const GL::LayerID drawingOrder = viewId + 0;
	U32	sort_key = 0;

	{
		NwShaderEffect* technique;
		mxDO(Resources::Load(technique, MakeAssetID("gi_generate_light_probes")));

		//
		GL::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			GL::buildSortKey( drawingOrder, sort_key++ )
			);

		//
		CPP_PREALIGN_CBUFFER struct GenerateLightProbesUniforms
		{
			UInt3	u_irradiance_grid_resolution;
			U32		pad0;

			V4f		u_irradiance_grid_to_voxel_uvw_space_muladd;

			V4f		u_cones[GPU_NUM_CUBEMAP_FACES][GPU_LIGHTPROBE_CUBEMAP_RES][GPU_LIGHTPROBE_CUBEMAP_RES];
			float	u_faceWeight;
		};

		GenerateLightProbesUniforms	*	uniforms;
		mxDO(technique->BindCBufferData(
			render_context._command_buffer
			, &uniforms
			));
		{
			uniforms->u_irradiance_grid_resolution = UInt3::setAll( irradiance_volume_texture_resolution );

			const float inv_irradiance_volume_texture_resolution = 1.0f / irradiance_volume_texture_resolution;

			// NOTE: for now, the light probe grid is aligned with the most-detailed voxel grid

			uniforms->u_irradiance_grid_to_voxel_uvw_space_muladd = CV4f(
				inv_irradiance_volume_texture_resolution,
				inv_irradiance_volume_texture_resolution,
				inv_irradiance_volume_texture_resolution,
				0.5 * inv_irradiance_volume_texture_resolution	// the light probe grid is dual to the voxel grid
				);

			{
				for( int face = 0; face < GPU_NUM_CUBEMAP_FACES; face++ )
				{
					for( int iX = 0; iX < GPU_LIGHTPROBE_CUBEMAP_RES; iX++ )
					{
						for( int iY = 0; iY < GPU_LIGHTPROBE_CUBEMAP_RES; iY++ )
						{
							uniforms->u_cones[face][iX][iY] = _cubemapVectors->cones[face][iX][iY];
						}
					}
				}

				//for( int face = 0; face < GPU_NUM_CUBEMAP_FACES; face++ )
				//{
				//	uniforms->u_faceWeight[face] = _cubemapVectors->weights[face];
				//}
				uniforms->u_faceWeight = _cubemapVectors->weights[0];
			}
		}

		voxelConeTracing.bindVoxelCascadeTexturesToShader( technique );

		//
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_posX"), GL::AsOutput(lightProbeCascade.texture_handles[0]) ));
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_negX"), GL::AsOutput(lightProbeCascade.texture_handles[1]) ));
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_posY"), GL::AsOutput(lightProbeCascade.texture_handles[2]) ));
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_negY"), GL::AsOutput(lightProbeCascade.texture_handles[3]) ));
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_posZ"), GL::AsOutput(lightProbeCascade.texture_handles[4]) ));
		mxDO(technique->setUav( mxHASH_STR("t_irradiance_volume_negZ"), GL::AsOutput(lightProbeCascade.texture_handles[5]) ));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		//
		mxDO(technique->pushShaderParameters( render_context._command_buffer ));

		//
		GL::Cmd_DispatchCS cmd_dispatch_compute_shader;
		{
			cmd_dispatch_compute_shader.clear();
			cmd_dispatch_compute_shader.encode( GL::CMD_DISPATCH_CS, 0, 0 );

			cmd_dispatch_compute_shader.program = pass0.default_program_handle;

			// Each thread group == a single light probe
			cmd_dispatch_compute_shader.groupsX = irradiance_volume_texture_resolution;
			cmd_dispatch_compute_shader.groupsY = irradiance_volume_texture_resolution;
			cmd_dispatch_compute_shader.groupsZ = irradiance_volume_texture_resolution;
		}
		render_context._command_buffer.put( cmd_dispatch_compute_shader );
	}

	return ALL_OK;
}

void IrradianceField::bindIrradianceVolumeTexturesToShader(
	NwShaderEffect* technique
	) const
{
	const LightProbeGridCascade& lightProbeCascade = _cascade0;

	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_posX"), GL::AsInput(lightProbeCascade.texture_handles[0]) ));
	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_negX"), GL::AsInput(lightProbeCascade.texture_handles[1]) ));
	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_posY"), GL::AsInput(lightProbeCascade.texture_handles[2]) ));
	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_negY"), GL::AsInput(lightProbeCascade.texture_handles[3]) ));
	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_posZ"), GL::AsInput(lightProbeCascade.texture_handles[4]) ));
	/*mxDO*/(technique->SetInput( mxHASH_STR("t_irradiance_volume_negZ"), GL::AsInput(lightProbeCascade.texture_handles[5]) ));
}

#endif


}//namespace VXGI
}//namespace Rendering
