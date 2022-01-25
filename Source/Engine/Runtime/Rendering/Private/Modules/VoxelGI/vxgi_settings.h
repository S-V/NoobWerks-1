// Volumetric scene representation for quick occlusion testing and GI effects on GPU.
#pragma once


namespace Rendering
{
namespace VXGI
{


struct VolumeTextureSettings: CStruct
{
	U8	dim_log2;

public:
	mxDECLARE_CLASS(VolumeTextureSettings, CStruct);
	mxDECLARE_REFLECTION;
	VolumeTextureSettings()
	{
		dim_log2 = 0;
		//dim_log2 = 5;	// 32^3 cells by default
	}

	UINT GetResolution() const
	{
		return 1 << dim_log2;
	}

	bool IsValid() const
	{
		return dim_log2 > 0;
	}

	bool equals( const VolumeTextureSettings& other ) const
	{ return structsBitwiseEqual( *this, other ); }
};



/// all cascades are not exactly centered
/// A common setting VXGI and Light Probe Grid.
struct CascadeSettings: CStruct
{
	enum { MAX_CASCADES = 4 };

	//!< [0..4]
	U32		num_cascades;

	/// 
	F32	cascade_radius[MAX_CASCADES];

	/// max distance for camera movement before re-centering the cascade
	F32	cascade_max_delta_distance[MAX_CASCADES];

public:
	mxDECLARE_CLASS(CascadeSettings, CStruct);
	mxDECLARE_REFLECTION;

	CascadeSettings()
	{
		num_cascades = 0;
		TSetStaticArray( cascade_radius, 0.0f );
		TSetStaticArray( cascade_max_delta_distance, 0.0f );
	}

	bool IsValid() const
	{
		return num_cascades > 0;
	}

	void FixBadValues()
	{
		num_cascades = Clamp(num_cascades, (U32)1, (U32)MAX_CASCADES);
	}
};




struct RuntimeFlags
{
	enum Enum
	{
		//
		enable_GI = BIT(0),
		enable_AO = BIT(1),
	};
};
mxDECLARE_FLAGS( RuntimeFlags::Enum, U32, RuntimeFlagsT );


struct DebugFlags
{
	enum Enum
	{
		/// slow!
		dbg_revoxelize_each_frame = BIT(0),

		/// stop the nested grids from following the camera
		dbg_freeze_cascades = BIT(1),

		///
		dbg_draw_cascade_bounds = BIT(2),

		/// draw semi-transparent boxes over the main scene
		dbg_show_voxels = BIT(3),

		dbg_show_voxels_cascade0 = BIT(11),
		dbg_show_voxels_cascade1 = BIT(12),
		dbg_show_voxels_cascade2 = BIT(13),
		dbg_show_voxels_cascade3 = BIT(14),
		dbg_show_voxels_cascade4 = BIT(15),
		dbg_show_voxels_cascade5 = BIT(16),
		dbg_show_voxels_cascade6 = BIT(17),
		dbg_show_voxels_cascade7 = BIT(18),
	};
};
mxDECLARE_FLAGS( DebugFlags::Enum, U32, DebugFlagsT );


struct DebugSettings: CStruct
{
	DebugFlagsT	flags;

	U8		mip_level_to_show;	// 0 - most detailed

public:
	mxDECLARE_CLASS(DebugSettings, CStruct);
	mxDECLARE_REFLECTION;
	DebugSettings()
	{
		flags.raw_value = 0;
		mip_level_to_show = 0;
	}
};



struct RuntimeSettings: CStruct//RrOffsettedCascadeSettings
{
	RuntimeFlagsT	flags;

	DebugSettings	debug;

	//
	VolumeTextureSettings	radiance_texture_settings;

	CascadeSettings	cascades;

	// 
	float	indirect_light_multiplier;
	float	ambient_occlusion_scale;

public:
	mxDECLARE_CLASS(RuntimeSettings, CStruct);
	mxDECLARE_REFLECTION;
	RuntimeSettings()
	{
		flags.raw_value = 0;
		indirect_light_multiplier = 1;
		ambient_occlusion_scale = 1;
	}

	bool IsValid() const
	{
		return radiance_texture_settings.IsValid()
			&& cascades.IsValid()
			;
	}

	bool equals( const RuntimeSettings& other ) const
	{ return structsBitwiseEqual( *this, other ); }

	void FixBadValues()
	{
		radiance_texture_settings.dim_log2 = Clamp(
			(int)radiance_texture_settings.dim_log2
			, 4	// 16^3 min
			, 8	// 256^3 max
			);

		cascades.FixBadValues();

		//
		indirect_light_multiplier = Clamp( indirect_light_multiplier, 0.0f, 10.0f );
		ambient_occlusion_scale = Clamp( ambient_occlusion_scale, 0.0f, 10.0f );
	}

	bool isEnabled() const
	{
		return flags & RuntimeFlags::enable_GI;
	}
};

/*
=======================================================================
	LIGHT PROBES / IRRADIANCE FIELD SETTINGS
=======================================================================
*/
struct IrradianceFieldFlags
{
	enum Enum
	{
		//
		enable_diffuse_GI = BIT(0),

		//
		dbg_draw_grid_bounds		= BIT(4),
		dbg_visualize_light_probes	= BIT(5),
		dbg_freeze_ray_directions	= BIT(6),
		dbg_visualize_rays			= BIT(7),

		dbg_skip_updating_depth_probes = BIT(8),
	};
};
mxDECLARE_FLAGS( IrradianceFieldFlags::Enum, U32, IrradianceFieldFlagsT );

/// Diffuse Global Illumination based on Light Probes
/// See:
/// Precomputed Light Field Probes [2017]
/// Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields [2019]
///
struct IrradianceFieldSettings: CStruct
{
	IrradianceFieldFlagsT	flags;

	CubeCRf		bounds;

	float		dbg_viz_light_probe_scale;

	U8			probe_grid_dim_log2;




	// Maximum distance a probe ray may travel.
	float           probe_max_ray_distance;

    // Controls the influence of new rays when updating each probe. A value close to 1 will
    // very slowly change the probe textures, improving stability but reducing accuracy when objects
    // move in the scene. Values closer to 0.9 or lower will rapidly react to scene changes,
    // but will exhibit flickering.
    float           probe_hysteresis;

	// Exponent for depth testing. A high value will rapidly react to depth discontinuities, 
	// but risks causing banding.
	float           probe_distance_exponent;

    // Irradiance blending happens in post-tonemap space
    float           probe_irradiance_encoding_gamma;




	// A threshold ratio used during probe radiance blending that determines if a large lighting change has happened.
	// If the max color component difference is larger than this threshold, the hysteresis will be reduced.
	float           probe_change_threshold;

	// A threshold value used during probe radiance blending that determines the maximum allowed difference in brightness
	// between the previous and current irradiance values. This prevents impulses from drastically changing a
	// texel's irradiance in a single update cycle.
	float           probe_brightness_threshold;

	// Bias values for Indirect Lighting
	float           view_bias;
	float           normal_bias;







	// We encode the spherical diffuse irradiance
	// (in R11G11B10F format at 6x6 octahedral resolution),
	// spherical distance and squared distances to the nearest
	// geometry (both in RG16F format at 16x16 octahedral resolution)

	///// Side length of one face 
	//U8	irradianceOctResolution;
	//U8	depthOctResolution;


	/** Number of rays emitted each frame for each probe in the scene */
	/// You should try to prevent raysPerProbe from ever being set
	/// to something that isn't 32 (lowest end), or a multiple of 64 (better perf).
	//U16             irradianceRaysPerProbe;

public:
	mxDECLARE_CLASS(IrradianceFieldSettings, CStruct);
	mxDECLARE_REFLECTION;
	IrradianceFieldSettings()
	{
		flags.raw_value = 0;

		bounds.center = CV3f(0);
		bounds.radius = 50;

		dbg_viz_light_probe_scale = 1;

		probe_grid_dim_log2 = 2;	// 4^3 probes by default


		probe_max_ray_distance = 1e27f;
		probe_hysteresis = 0.97f;
		probe_distance_exponent = 50.f;
		probe_irradiance_encoding_gamma = 5.f;



		probe_change_threshold = 0.25f;
		probe_brightness_threshold = 0.10f;
		view_bias = 0.1f;
		normal_bias = 0.1f;



		//irradianceOctResolution = 6;
		//depthOctResolution = 16;

		//irradianceRaysPerProbe = 64;
	}

	UINT probeGridResolution() const
	{
		return 1 << probe_grid_dim_log2;
	}

	UInt3 probeGridDimensions() const
	{
		return UInt3( probeGridResolution() );
	}

	UINT numProbes() const
	{
		return ToCube( probeGridResolution() );
	}

	bool IsValid() const
	{
		return probe_grid_dim_log2 > 0;
	}

	void FixBadValues()
	{
		probe_grid_dim_log2 = Clamp(
			(int)probe_grid_dim_log2
			, 1	// 2^3 min
			, 8	// 256^3 max
			);
	}

	bool equals( const IrradianceFieldSettings& other ) const
	{ return structsBitwiseEqual( *this, other ); }
};

}//namespace VXGI
}//namespace Rendering
