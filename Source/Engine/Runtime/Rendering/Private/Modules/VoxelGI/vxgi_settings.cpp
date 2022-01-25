#include <Base/Base.h>
#pragma hdrstop
#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Private/Modules/VoxelGI/vxgi_settings.h>


namespace Rendering
{
namespace VXGI
{

mxDEFINE_CLASS(VolumeTextureSettings);
mxBEGIN_REFLECTION(VolumeTextureSettings)
	mxMEMBER_FIELD(dim_log2),
mxEND_REFLECTION;


mxDEFINE_CLASS(CascadeSettings);
mxBEGIN_REFLECTION(CascadeSettings)
	mxMEMBER_FIELD(num_cascades),
	mxMEMBER_FIELD(cascade_radius),
	mxMEMBER_FIELD(cascade_max_delta_distance),
mxEND_REFLECTION


mxBEGIN_FLAGS( RuntimeFlagsT )
	mxREFLECT_BIT( enable_GI, RuntimeFlags::enable_GI ),
	mxREFLECT_BIT( enable_AO, RuntimeFlags::enable_AO ),
mxEND_FLAGS

mxBEGIN_FLAGS( DebugFlagsT )
	mxREFLECT_BIT( revoxelize_each_frame, DebugFlags::dbg_revoxelize_each_frame ),
	mxREFLECT_BIT( freeze_cascades, DebugFlags::dbg_freeze_cascades ),

	mxREFLECT_BIT( draw_cascade_bounds, DebugFlags::dbg_draw_cascade_bounds ),

	mxREFLECT_BIT( show_voxels, DebugFlags::dbg_show_voxels ),

	mxREFLECT_BIT( show_voxels_cascade0, DebugFlags::dbg_show_voxels_cascade0 ),
	mxREFLECT_BIT( show_voxels_cascade1, DebugFlags::dbg_show_voxels_cascade1 ),
	mxREFLECT_BIT( show_voxels_cascade2, DebugFlags::dbg_show_voxels_cascade2 ),
	mxREFLECT_BIT( show_voxels_cascade3, DebugFlags::dbg_show_voxels_cascade3 ),
	mxREFLECT_BIT( show_voxels_cascade4, DebugFlags::dbg_show_voxels_cascade4 ),
	mxREFLECT_BIT( show_voxels_cascade5, DebugFlags::dbg_show_voxels_cascade5 ),
	mxREFLECT_BIT( show_voxels_cascade6, DebugFlags::dbg_show_voxels_cascade6 ),
	mxREFLECT_BIT( show_voxels_cascade7, DebugFlags::dbg_show_voxels_cascade7 ),
mxEND_FLAGS

mxDEFINE_CLASS(DebugSettings);
mxBEGIN_REFLECTION(DebugSettings)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(mip_level_to_show),
mxEND_REFLECTION;


mxDEFINE_CLASS(RuntimeSettings);
mxBEGIN_REFLECTION(RuntimeSettings)
	mxMEMBER_FIELD(flags),

	mxMEMBER_FIELD(debug),

	mxMEMBER_FIELD(cascades),

	mxMEMBER_FIELD(radiance_texture_settings),

	mxMEMBER_FIELD(indirect_light_multiplier),
	mxMEMBER_FIELD(ambient_occlusion_scale),
mxEND_REFLECTION;


mxBEGIN_FLAGS( IrradianceFieldFlagsT )
	mxREFLECT_BIT( enable_diffuse_GI, IrradianceFieldFlags::enable_diffuse_GI ),

	mxREFLECT_BIT( dbg_draw_grid_bounds, IrradianceFieldFlags::dbg_draw_grid_bounds ),
	mxREFLECT_BIT( dbg_visualize_light_probes, IrradianceFieldFlags::dbg_visualize_light_probes ),
	mxREFLECT_BIT( dbg_freeze_ray_directions, IrradianceFieldFlags::dbg_freeze_ray_directions ),
	mxREFLECT_BIT( dbg_visualize_rays, IrradianceFieldFlags::dbg_visualize_rays ),

	mxREFLECT_BIT( dbg_skip_updating_depth_probes, IrradianceFieldFlags::dbg_skip_updating_depth_probes ),
mxEND_FLAGS

mxDEFINE_CLASS(IrradianceFieldSettings);
mxBEGIN_REFLECTION(IrradianceFieldSettings)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(bounds),
	mxMEMBER_FIELD(dbg_viz_light_probe_scale),
	mxMEMBER_FIELD(probe_grid_dim_log2),

	mxMEMBER_FIELD(probe_max_ray_distance),
	mxMEMBER_FIELD(probe_hysteresis),
	mxMEMBER_FIELD(probe_distance_exponent),
	mxMEMBER_FIELD(probe_irradiance_encoding_gamma),

	mxMEMBER_FIELD(probe_change_threshold),
	mxMEMBER_FIELD(probe_brightness_threshold),
	mxMEMBER_FIELD(view_bias),
	mxMEMBER_FIELD(normal_bias),

	//mxMEMBER_FIELD(irradianceOctResolution),
	//mxMEMBER_FIELD(depthOctResolution),
	//mxMEMBER_FIELD(irradianceRaysPerProbe),
mxEND_REFLECTION

}//namespace VXGI
}//namespace Rendering
