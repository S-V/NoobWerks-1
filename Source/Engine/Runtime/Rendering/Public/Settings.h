// application-wide renderer settings
#pragma once

#include <Rendering/BuildConfig.h>

//#if nwRENDERER_CFG_WITH_VOXEL_GI
#include <Rendering/Private/Modules/VoxelGI/vxgi_settings.h>
//#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
//#endif


namespace Rendering
{

/*
=======================================================================
	Common settings for nested volumetric grids.
=======================================================================
*/
#pragma pack (push,1)

/// all cascades share the same center
struct RrSymmetricCascadeSettings: CStruct
{
	enum { MAX_CASCADES = 4 };

	U32		num_cascades;	//!< [0..4]

	/// 
	F32	cascade_radius[MAX_CASCADES];

public:
	mxDECLARE_CLASS(RrSymmetricCascadeSettings, CStruct);
	mxDECLARE_REFLECTION;

	RrSymmetricCascadeSettings()
	{
		num_cascades = 0;
		TSetStaticArray( cascade_radius, 0.0f );
	}
};

/// all cascades are not exactly centered
/// A common setting VXGI and Light Probe Grid.
struct RrOffsettedCascadeSettings: RrSymmetricCascadeSettings
{
	/// max distance for camera movement before re-centering the cascade
	F32	cascade_max_delta_distance[MAX_CASCADES];

public:
	mxDECLARE_CLASS(RrOffsettedCascadeSettings, RrSymmetricCascadeSettings);
	mxDECLARE_REFLECTION;

	RrOffsettedCascadeSettings()
	{
		TSetStaticArray( cascade_max_delta_distance, 0.0f );
	}
};

/*
=======================================================================
	DYNAMIC SHADOWS
=======================================================================
*/
struct ShadowEnableFlags {
	enum Enum {
		/// shadows from semi-static geometry like voxel terrain
		enable_static_shadows = BIT(0),

		/// shadows from moving objects like rigid bodies
		enable_dynamic_shadows = BIT(1),
	};
};
mxDECLARE_FLAGS( ShadowEnableFlags::Enum, U8, ShadowEnableFlagsT );

enum CSMPartitionMode
{
	CSM_Split_Manual,
	CSM_Split_Uniform,
	CSM_Split_Logarithmic,
	CSM_Split_Mixed_PSSM,
	//CSM_Split_Automatic,
};
mxDECLARE_ENUM( CSMPartitionMode, U8, CSMPartitionModeT );

struct RrShadowSettings: RrSymmetricCascadeSettings
{
	/// max distance for camera movement before updating the shadow map
	F32	cascade_max_delta_distance;

	ShadowEnableFlagsT	flags;

	/// size of (square) shadow map for each cascade (default = 10 or 1024 texels)
	U8	shadow_atlas_resolution_log2;	// must always be > 1
	CSMPartitionModeT	splitMode;

	/// Settings for Cascaded Shadow Maps / Parallel-Split Shadow Maps.
	float	shadow_map_bias;	////!< NOTE: cannot be zero - we would get shadow acne everywhere (default = -1e-2f)
	
	float	shadow_distance;//!< 0 = use the light's shadow fade distance
    //float	min_cascade_distance;	//!< The closest depth that is covered by the shadow cascades, [0..1).
    //float	max_cascade_distance;	//!< The furthest depth that is covered by the shadow cascades, (0..1].
	V4f		splitDistances;	//!< normalized split distances in [0..1) range (fractions of far - near) (manual mode only)
	float	PSSM_lambda;	//!< lambda scales between between uniform and logarithmic split scheme (for CSM_Split_Mixed_PSSM) (0 == uniform)

	bool	csm_fit_to_scene;	//!< 'Fit to Scene' or 'Fit to Cascade'? (see Cascaded Shadow Maps sample from DirectX SDK)

	bool 	blend_between_cascades;

	// enable soft directional light shadows
	bool 	enable_soft_shadows;
	// enable soft spot light shadows
	//static bool	g_cvar_spot_light_soft_shadows;

	bool	stabilize_cascades;

	/// visualize PSSM/CSM splits
	bool	dbg_visualize_cascades;
	bool	dbg_show_shadowmaps;

public:
	mxDECLARE_CLASS(RrShadowSettings, RrSymmetricCascadeSettings);
	mxDECLARE_REFLECTION;
	RrShadowSettings()
	{
		flags.raw_value = 0;

		cascade_max_delta_distance = 0;

		shadow_atlas_resolution_log2 = 10;

		shadow_map_bias = -1e-2f;
		splitMode = CSM_Split_Mixed_PSSM;
		shadow_distance = 0.0f;
		splitDistances[0] = 0.05f;
		splitDistances[1] = 0.15f;
		splitDistances[2] = 0.50f;
		splitDistances[3] = 1.00f;
		PSSM_lambda = 0.5f;
		csm_fit_to_scene = false;
		blend_between_cascades = true;
		enable_soft_shadows = true;
		stabilize_cascades = true;		
		dbg_visualize_cascades = false;
		dbg_show_shadowmaps = false;
	}

	bool equals( const RrShadowSettings& other ) const { return structsBitwiseEqual( *this, other ); }

	void FixBadValues()
	{
		shadow_atlas_resolution_log2 = largest(shadow_atlas_resolution_log2, 5);	// 32^2 at min
	}

	bool areEnabled() const
	{
		return (flags
			& (ShadowEnableFlags::enable_dynamic_shadows|ShadowEnableFlags::enable_static_shadows)
			) != 0;
	}
};

/*
=======================================================================
	GLOBAL ILLUMINATION
=======================================================================
*/
struct RrVolumeTextureCommonSettings: CStruct
{
	U8	vol_tex_res_log2;
//	U8	vol_radius_log2;	// radius in LoD 0 chunks (voxel terrain)

//	V3f		volume_min_corner_in_world_space;
//	float	vol_tex_dim_in_world_space;

	/// The position of the minimal corner of the voxel volume, in view space;
	/// it's used for converting pixel positions from view space into volume texture space:
//	V3f		volume_origin_in_view_space;

	//float	position_bias;

public:
	mxDECLARE_CLASS(RrVolumeTextureCommonSettings, CStruct);
	mxDECLARE_REFLECTION;
	RrVolumeTextureCommonSettings()
	{
		vol_tex_res_log2 = 5;	// 32^3 cells by default
		//vol_radius_log2 = 1;
		//volume_min_corner_in_world_space = CV3f(0);
		//vol_tex_dim_in_world_space = 0;
		//volume_origin_in_view_space = CV3f(0);
		//position_bias = 0;
	}

	UINT volumeTextureResolution() const
	{
		const UINT num_mips = vol_tex_res_log2;
		const UINT tex3d_res = 1 << num_mips;
		return tex3d_res;
	}

	bool equals( const RrVolumeTextureCommonSettings& other ) const { return structsBitwiseEqual( *this, other ); }
};



/*
=======================================================================
	POST-PROCESSING
=======================================================================
*/
struct Bloom_Parameters: CStruct
{
	float	bloom_threshold;//!< the lower limit for intensity values that will contribute to bloom (aka 'bright pass threshold')
	float	bloom_scale;	//!< scales the bloom value prior to adding the bloom value to the material color (aka 'bloom multiplier/intensity')

public:
	mxDECLARE_CLASS(Bloom_Parameters, CStruct);
	mxDECLARE_REFLECTION;
	Bloom_Parameters()
	{
		bloom_threshold = 3.0f;
		bloom_scale = 0.06f;
	}
};

struct HDR_Parameters: CStruct
{
	float	exposure_key;	//!< (aka 'Tau')
	float	adaptation_rate;
	//float	middle_gray;	//!< MIDDLE_GREY is used to scale the color values for tone mapping

public:
	mxDECLARE_CLASS(HDR_Parameters, CStruct);
	mxDECLARE_REFLECTION;
	HDR_Parameters()
	{
		exposure_key = 0.3f;
		adaptation_rate = 0.5f;
		//middle_gray = 0.08f;
	}
};

/*
=======================================================================
	VOXEL TERRAIN
=======================================================================
*/

/// voxel terrain
struct RrTerrainSettings: CStruct
{
	//=== Terrain LOD blending (geomorphing/CLOD)
	float	distanceSplitFactor;	//!< see Proland documentation, '3.2.1. Distance-based subdivision': http://proland.inrialpes.fr
	float	ambient_occlusion_scale;	//!< AO Intensity, [0..1] (0 - AO disabled, 1 - full AO)
	float	ambient_occlusion_power;	//!< AO Power, [1..10]
	float	debug_morph_factor;

	bool	flat_shading;	//!< for sharp features
	bool	enable_geomorphing;	//!< CLOD morph
	bool	enforce_LoD_constraints;	//!< CLOD morph
	bool	highlight_LOD_transitions;	//!< CLOD debug
	bool	enable_vertex_colors;	//!< CLOD debug

public:
	mxDECLARE_CLASS(RrTerrainSettings, CStruct);
	mxDECLARE_REFLECTION;
	RrTerrainSettings()
	{
		distanceSplitFactor = 1.5f;
		debug_morph_factor = 0;
		ambient_occlusion_scale = 1.0f;
		ambient_occlusion_power = 1.0f;
		flat_shading = false;

		enable_geomorphing = true;
		enforce_LoD_constraints = false;//@TODO: REMOVE!
		highlight_LOD_transitions = false;
		enable_vertex_colors = false;
	}

	bool isAmbientOcclusionEnabled() const { return ambient_occlusion_scale > 0; }
};

/*
=======================================================================
	SKY MODEL
=======================================================================
*/
struct Sky_Parameters: CStruct
{
	//
	float	sunThetaDegrees;// [0..90]
	float	sunPhiDegrees;	// [0..360)
	float	skyTurbidity;

public:
	mxDECLARE_CLASS(Sky_Parameters, CStruct);
	mxDECLARE_REFLECTION;
	Sky_Parameters()
	{
		sunThetaDegrees = 60;
		sunPhiDegrees = 180;
		skyTurbidity = 4;
	}
};

/*
=======================================================================
	DEBUG TOOLS
=======================================================================
*/
struct DebugRenderFlags { enum Enum {
	DrawWireframe	= BIT(0),
	DrawModelAABBs	= BIT(1),
}; };
mxDECLARE_FLAGS( DebugRenderFlags::Enum, U32, DebugRenderFlagsT );


//struct DebugFlags {
//	enum Enum {
//		/// 
//		DrawModelsInWireframe = BIT(0),
//		DrawModelsBounds = BIT(0),
//	};
//};
//mxDECLARE_FLAGS( DebugFlags::Enum, U8, DebugFlagsT );

struct DebugSceneRenderMode { enum Enum {
	None,
	ShowDepthBuffer,
	// G-Buffer:
	ShowAlbedo,
	ShowAmbientOcclusion,
	ShowNormals,
	ShowSpecularColor,
}; };
mxDECLARE_ENUM( DebugSceneRenderMode::Enum, U8, DebugSceneRenderModeT );


/// settings for drawing thick wireframe lines
struct SolidWireSettings: CStruct
{
	V4f fill_color;
	V4f wire_color;
	V4f pattern_color;
	float line_width;
	float fade_distance;
	float pattern_period;
public:
	mxDECLARE_CLASS(SolidWireSettings, CStruct);
	mxDECLARE_REFLECTION;
	SolidWireSettings()
	{
		this->SetDefauls();
	}
	void SetDefauls()
	{
		line_width = 0.5;
		fade_distance = 50;
		pattern_period = 1.5;
		fill_color = V4f::set( 0.1, 0.2, 0.4, 1 );
		wire_color = V4f::set( 1, 1, 1, 1 );
		pattern_color = V4f::set( 1, 1, 0.5, 1 );
	}
};


struct TbRendererDebugSettings: CStruct
{
	DebugRenderFlagsT		flags;
	DebugSceneRenderModeT	scene_render_mode;

	SolidWireSettings	wireframe;

public:
	mxDECLARE_CLASS(TbRendererDebugSettings, CStruct);
	mxDECLARE_REFLECTION;
	TbRendererDebugSettings()
	{
		flags.raw_value = 0;
		scene_render_mode = DebugSceneRenderMode::None;
	}
};



/*
=======================================================================
	ANIMATION
=======================================================================
*/

struct RrAnimationFlags
{
	enum Enum
	{
		unused = BIT(0),
	};
};
mxDECLARE_FLAGS( RrAnimationFlags::Enum, U8, RrAnimationFlags8 );

/// 
struct RrAnimationSettings: CStruct
{
	RrAnimationFlags8	flags;

	/// animation speed, [0..2], 0 == all animations are paused!
	float	animation_speed_multiplier;

public:
	mxDECLARE_CLASS(RrAnimationSettings, CStruct);
	mxDECLARE_REFLECTION;

	RrAnimationSettings()
	{
		flags.raw_value = 0;
		animation_speed_multiplier = 1;
	}

	void FixBadValues()
	{
		animation_speed_multiplier = clampf( animation_speed_multiplier, minValue(), maxValue() );
	}

	static float minValue() {
		return 0;
	}
	static float maxValue() {
		return 2;
	}
};

struct RrExplosionSettings: CStruct
{
	F32	time_to_live_seconds;
	F32	radius;
	F32	emission_rate_particles_per_second;
	F32	particle_life_span_seconds;
	
	float g_ParticleVel;
	float particle_buoyancy_vel;	// add some velocity in the up direction, because of buoyancy
	float g_fParticleMinSize;
	float g_fParticleMaxSize;

public:
	mxDECLARE_CLASS(RrExplosionSettings, CStruct);
	mxDECLARE_REFLECTION;
	RrExplosionSettings();
};


struct RrGameSpecificSettings: CStruct
{
	// http://imaginaryblend.com/2018/10/16/weapon-fov/
	float	weapon_FoV_Y_in_degrees;	// default = 70 degrees

	/// the near clip plane used for rendering first-person weapon models
	float near_clip_plane_for_weapon_depth_hack;

	float far_clip_plane_for_weapon_depth_hack;

	bool use_infinite_far_clip_plane;
	bool	draw_fp_weapons_last;

public:
	mxDECLARE_CLASS(RrGameSpecificSettings, CStruct);
	mxDECLARE_REFLECTION;
	RrGameSpecificSettings();
};

/*
=======================================================================
	GLOBAL SETTINGS
=======================================================================
*/


/// Renderer settings that can be changed dynamically, at run-time.
struct RrGlobalSettings: CStruct
{
	RrGameSpecificSettings		game_specific;

	//=== Testing & Debugging ===
	TbRendererDebugSettings	debug;

	//=== Scene-Wide Settings
	V3f		sceneAmbientColor;

	float	PBR_metalness;
	float	PBR_roughness;

	//=== Large Scene Rendering
	double	floating_origin_threshold;

	//=== Shadow rendering
	RrShadowSettings	shadows;

	//=== Global Illumination
	VXGI::RuntimeSettings			vxgi;
	VXGI::IrradianceFieldSettings	ddgi;


	bool 	drawModels;	//!< true by default
	bool 	debug_DrawModelBounds;	//!< debugging
	bool 	debug_draw_shadow_frustums;	//!< debugging

	float	camera_near_clip;
	float	camera_far_clip;

	//=== Low-level
	bool	enable_Fullscreen;	//!< 
	bool	enable_VSync;	//!< 

	//=== Multi-threading
	//bool	use_tasking;	//!< TRUE to use the tasking path (for frustum culling, render command generation, etc.)
	bool	multithreaded_culling;
	bool	multithreaded_command_buffer_generation;

	//=== Voxel Terrain settings
	RrTerrainSettings	terrain;	//!< voxel terrain settings


	RrAnimationSettings		animation;

	RrExplosionSettings		explosions;


	//=== Post-Processing

	bool 	enableHDR;	//!< Enable High Dynamic Range (HDR) rendering?
	HDR_Parameters	HDR;

	bool 	enableBloom;	//!< we can have Bloom without HDR
	Bloom_Parameters	bloom;

	bool 	enableFXAA;	//!< Enable Fast Approximate Anti-Aliasing (FXAA)?

	bool 	enableGlobalFog;

	bool 	enableBlendedOIT;

	//=== Testing & Debugging



	//!< for creating nice screenshots with white backgrounds
	bool	whitepaper_quality;

	Sky_Parameters	sky;

public:
	mxDECLARE_CLASS(RrGlobalSettings, CStruct);
	mxDECLARE_REFLECTION;
	RrGlobalSettings();

	void FixBadValues()
	{
		shadows.FixBadValues();
		vxgi.FixBadValues();
	}
};

#pragma pack (pop)

}//namespace Rendering
