#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Public/Settings.h>


namespace Rendering
{

mxDEFINE_CLASS(RrSymmetricCascadeSettings);
mxBEGIN_REFLECTION(RrSymmetricCascadeSettings)
	mxMEMBER_FIELD(num_cascades),
	mxMEMBER_FIELD(cascade_radius),
mxEND_REFLECTION

mxDEFINE_CLASS(RrOffsettedCascadeSettings);
mxBEGIN_REFLECTION(RrOffsettedCascadeSettings)
	mxMEMBER_FIELD(cascade_max_delta_distance),
mxEND_REFLECTION

mxBEGIN_FLAGS( ShadowEnableFlagsT )
	mxREFLECT_BIT( enable_static_shadows, ShadowEnableFlags::enable_static_shadows ),
	mxREFLECT_BIT( enable_dynamic_shadows, ShadowEnableFlags::enable_dynamic_shadows ),
mxEND_FLAGS

mxBEGIN_REFLECT_ENUM( CSMPartitionModeT )
	mxREFLECT_ENUM_ITEM( Manual, CSM_Split_Manual ),
	mxREFLECT_ENUM_ITEM( Uniform, CSM_Split_Uniform ),
	mxREFLECT_ENUM_ITEM( Logarithmic, CSM_Split_Logarithmic ),
	mxREFLECT_ENUM_ITEM( Mixed_PSSM, CSM_Split_Mixed_PSSM ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(RrShadowSettings);
mxBEGIN_REFLECTION(RrShadowSettings)
	mxMEMBER_FIELD(cascade_max_delta_distance),

	mxMEMBER_FIELD(flags),

	mxMEMBER_FIELD(shadow_atlas_resolution_log2),

	mxMEMBER_FIELD(shadow_map_bias),
	mxMEMBER_FIELD(splitMode),
	mxMEMBER_FIELD(shadow_distance),
    //mxMEMBER_FIELD(min_cascade_distance),
    //mxMEMBER_FIELD(max_cascade_distance),
	mxMEMBER_FIELD(splitDistances),
	mxMEMBER_FIELD(PSSM_lambda),
	mxMEMBER_FIELD(csm_fit_to_scene),
	mxMEMBER_FIELD(enable_soft_shadows),
	mxMEMBER_FIELD(blend_between_cascades),
	mxMEMBER_FIELD(stabilize_cascades),
	mxMEMBER_FIELD(dbg_visualize_cascades),
	mxMEMBER_FIELD(dbg_show_shadowmaps),
mxEND_REFLECTION;

mxDEFINE_CLASS(RrVolumeTextureCommonSettings);
mxBEGIN_REFLECTION(RrVolumeTextureCommonSettings)
	mxMEMBER_FIELD(vol_tex_res_log2),
	//mxMEMBER_FIELD(vol_radius_log2),
	//mxMEMBER_FIELD(vol_tex_dim_in_world_space),
	//mxMEMBER_FIELD(position_bias),
mxEND_REFLECTION;

mxDEFINE_CLASS(Bloom_Parameters);
mxBEGIN_REFLECTION(Bloom_Parameters)
	mxMEMBER_FIELD(bloom_threshold),
	mxMEMBER_FIELD(bloom_scale),
mxEND_REFLECTION;

mxDEFINE_CLASS(HDR_Parameters);
mxBEGIN_REFLECTION(HDR_Parameters)
	mxMEMBER_FIELD(exposure_key),
	mxMEMBER_FIELD(adaptation_rate),
	//mxMEMBER_FIELD(middle_gray),
mxEND_REFLECTION;

mxDEFINE_CLASS(RrTerrainSettings);
mxBEGIN_REFLECTION(RrTerrainSettings)
	mxMEMBER_FIELD(distanceSplitFactor),
	mxMEMBER_FIELD(ambient_occlusion_scale),
	mxMEMBER_FIELD(ambient_occlusion_power),
	mxMEMBER_FIELD(debug_morph_factor),

	mxMEMBER_FIELD( flat_shading ),
	mxMEMBER_FIELD( enable_geomorphing ),
	mxMEMBER_FIELD( enforce_LoD_constraints ),
	mxMEMBER_FIELD( highlight_LOD_transitions ),
	mxMEMBER_FIELD( enable_vertex_colors ),
mxEND_REFLECTION;

mxDEFINE_CLASS(Sky_Parameters);
mxBEGIN_REFLECTION(Sky_Parameters)
	mxMEMBER_FIELD(sunThetaDegrees),
	mxMEMBER_FIELD(sunPhiDegrees),
	mxMEMBER_FIELD(skyTurbidity),
mxEND_REFLECTION;

mxBEGIN_FLAGS( DebugRenderFlagsT )
	mxREFLECT_BIT( DrawWireframe, DebugRenderFlags::DrawWireframe ),
	mxREFLECT_BIT( DrawModelAABBs, DebugRenderFlags::DrawModelAABBs ),
mxEND_FLAGS

mxBEGIN_REFLECT_ENUM( DebugSceneRenderModeT )
	mxREFLECT_ENUM_ITEM( None, DebugSceneRenderMode::None ),
	mxREFLECT_ENUM_ITEM( ShowDepthBuffer, DebugSceneRenderMode::ShowDepthBuffer ),
	mxREFLECT_ENUM_ITEM( ShowAlbedo, DebugSceneRenderMode::ShowAlbedo ),
	mxREFLECT_ENUM_ITEM( ShowAmbientOcclusion, DebugSceneRenderMode::ShowAmbientOcclusion ),
	mxREFLECT_ENUM_ITEM( ShowNormals, DebugSceneRenderMode::ShowNormals ),
	mxREFLECT_ENUM_ITEM( ShowSpecularColor, DebugSceneRenderMode::ShowSpecularColor ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(TbRendererDebugSettings);
mxBEGIN_REFLECTION(TbRendererDebugSettings)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(scene_render_mode),
	mxMEMBER_FIELD(wireframe),
mxEND_REFLECTION;

mxDEFINE_CLASS(SolidWireSettings);
mxBEGIN_REFLECTION(SolidWireSettings)
	mxMEMBER_FIELD( line_width ),
	mxMEMBER_FIELD( fade_distance ),
	mxMEMBER_FIELD( pattern_period ),
	mxMEMBER_FIELD( fill_color ),
	mxMEMBER_FIELD( wire_color ),
	mxMEMBER_FIELD( pattern_color ),
mxEND_REFLECTION;

mxBEGIN_FLAGS( RrAnimationFlags8 )
	mxREFLECT_BIT( unused, RrAnimationFlags::unused ),
mxEND_FLAGS

mxDEFINE_CLASS(RrAnimationSettings);
mxBEGIN_REFLECTION(RrAnimationSettings)
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(animation_speed_multiplier),
mxEND_REFLECTION;

mxDEFINE_CLASS(RrExplosionSettings);
mxBEGIN_REFLECTION(RrExplosionSettings)
	mxMEMBER_FIELD(time_to_live_seconds),
	mxMEMBER_FIELD(radius),
	mxMEMBER_FIELD(emission_rate_particles_per_second),
	mxMEMBER_FIELD(particle_life_span_seconds),

	mxMEMBER_FIELD(g_ParticleVel),
	mxMEMBER_FIELD(particle_buoyancy_vel),
	mxMEMBER_FIELD(g_fParticleMinSize),
	mxMEMBER_FIELD(g_fParticleMaxSize),
mxEND_REFLECTION;
RrExplosionSettings::RrExplosionSettings()
{
	time_to_live_seconds = 3.0f;
	radius = 1.0f;
	emission_rate_particles_per_second = 5.0f;
	particle_life_span_seconds = 5.0f;

	g_ParticleVel = 1.0f;
	particle_buoyancy_vel = 0.1f;
	g_fParticleMinSize = 0.1f;
	g_fParticleMaxSize = 1.0f;
}

mxDEFINE_CLASS(RrGameSpecificSettings);
mxBEGIN_REFLECTION(RrGameSpecificSettings)
	mxMEMBER_FIELD(weapon_FoV_Y_in_degrees),
	mxMEMBER_FIELD(near_clip_plane_for_weapon_depth_hack),
	mxMEMBER_FIELD(use_infinite_far_clip_plane),
	mxMEMBER_FIELD(far_clip_plane_for_weapon_depth_hack),
	mxMEMBER_FIELD(draw_fp_weapons_last),
mxEND_REFLECTION;
RrGameSpecificSettings::RrGameSpecificSettings()
{
	weapon_FoV_Y_in_degrees = 70.0f;
	near_clip_plane_for_weapon_depth_hack = 1e-2f;
	far_clip_plane_for_weapon_depth_hack = 1.0f;
	use_infinite_far_clip_plane = false;
	draw_fp_weapons_last = false;
}

mxDEFINE_CLASS(RrGlobalSettings);
mxBEGIN_REFLECTION(RrGlobalSettings)

	mxMEMBER_FIELD(game_specific),

	mxMEMBER_FIELD(animation),
	mxMEMBER_FIELD(explosions),

	mxMEMBER_FIELD(PBR_metalness),
	mxMEMBER_FIELD(PBR_roughness),

	mxMEMBER_FIELD(floating_origin_threshold),

	mxMEMBER_FIELD(sceneAmbientColor),

	mxMEMBER_FIELD(shadows),
	mxMEMBER_FIELD(vxgi),
	mxMEMBER_FIELD(ddgi),

	mxMEMBER_FIELD(debug),

	mxMEMBER_FIELD(drawModels),
	mxMEMBER_FIELD(debug_DrawModelBounds),
	mxMEMBER_FIELD(debug_draw_shadow_frustums),

	mxMEMBER_FIELD(camera_near_clip),
	mxMEMBER_FIELD(camera_far_clip),

	mxMEMBER_FIELD(enable_Fullscreen),
	mxMEMBER_FIELD(enable_VSync),

	//mxMEMBER_FIELD(use_tasking),
	mxMEMBER_FIELD(multithreaded_culling),
	mxMEMBER_FIELD(multithreaded_command_buffer_generation),

	mxMEMBER_FIELD(terrain),

	mxMEMBER_FIELD(whitepaper_quality),
	mxMEMBER_FIELD(enableHDR),
	mxMEMBER_FIELD(HDR),
	mxMEMBER_FIELD(enableBloom),
	mxMEMBER_FIELD(bloom),
	mxMEMBER_FIELD(enableFXAA),
	mxMEMBER_FIELD(enableGlobalFog),
	mxMEMBER_FIELD(enableBlendedOIT),

	mxMEMBER_FIELD(sky),
mxEND_REFLECTION;
RrGlobalSettings::RrGlobalSettings()
{
	floating_origin_threshold = 1000.0f;

	PBR_metalness = 1;
	PBR_roughness = 1;

	sceneAmbientColor = V3f::zero();

	drawModels = true;
	debug_DrawModelBounds = false;
	debug_draw_shadow_frustums = false;

	camera_near_clip = 1.0f;
	camera_far_clip = 9000.0f;

	enable_Fullscreen = false;
	enable_VSync = true;

	//use_tasking = false;
	multithreaded_culling = true;
	multithreaded_command_buffer_generation = true;

	enableHDR = false;
	enableBloom = false;
	enableFXAA = false;

	enableGlobalFog = false;

	enableBlendedOIT = false;

	whitepaper_quality = false;
}

}//namespace Rendering
