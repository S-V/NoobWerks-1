// Diffuse Global Illumination based on Light Probes
// light-grid-voxel-based per pixel lighting
// (very similar to Volumetric Lightmapping)
#pragma once

//#include <Rendering/Public/Settings.h>	// IrradianceFieldSettings
//#include <Rendering/Private/Shaders/Shared/nw_shared_definitions.h>	// GPU_LIGHTPROBE_CUBEMAP_RES
//#include <Rendering/Private/Shaders/Shared/nw_shared_globals.h>	// SkyAmbientCube

#include <Rendering/Private/Modules/VoxelGI/vxgi_settings.h>	// IrradianceFieldSettings

class NwCameraView;
class VXGI_Cascade;


#define DEBUG_RAYMARCHING	(MX_DEVELOPER)


namespace Rendering
{
namespace VXGI
{

struct DDGIResources
{
	/** 
        Low resolution irradiance probes, R11G11B10F radiance.
        width = padded( probe_counts.X * probe_counts.Z ),
		height = padded( probe_counts.Y ).
    */
	HTexture	irradiance_probes_tex2d;

    /**
        Low resolution variance shadow-map style probes, RG16F.
        X channel is distance, Y channel is sum of squared distances.
        width = padded( probe_counts.X * probe_counts.Z ),
		height = padded( probe_counts.Y ).
    */
	HTexture	visibility_probes_tex2d;
};

///
class IrradianceField: TSingleInstance<IrradianceField>
{
public_internal:
	IrradianceFieldSettings	_settings;

	// G_DDGI
	HBuffer		_ddgi_cb_handle;

	//// Irradiance ray textures
	//HTexture	_irradiance_ray_origins_tex2d;
	//HTexture	_irradiance_ray_directions_tex2d;

	/// create two textures for ping-ponging, because we cannot RWTexture3D< float3 > in Direct3D 11
	DDGIResources	_pingpong[2];
	int				_curr_dst_tex_idx;

	Tuple2< U16 >	_irradiance_probes_tex2d_size;
	Tuple2< U16 >	_visibility_probes_tex2d_size;


	// Ray-traced scene radiance and probe depth, RGBA16F.
	// width = NUM_RAYS_PER_PROBE,
	// height = total probe count.
	HTexture		_raytraced_radiance_and_distance_tex2d;
	Tuple2< U16 >	_raytraced_radiance_and_distance_tex2d_size;


	//
#if DEBUG_RAYMARCHING
	HTexture		_raytraced_debug_tex2d;
	Tuple2< U16 >	_raytraced_debug_tex2d_size;
#endif

	//
	bool	_need_to_recompute_probes;

public:
	IrradianceField();
	~IrradianceField();

	ERet Initialize( const IrradianceFieldSettings& settings );
	void Shutdown();

	ERet ApplySettings( const IrradianceFieldSettings& new_settings );

	void SetDirty();

public:

	const DDGIResources& GetDstIrradianceProbes() const;
	const DDGIResources& GetPrevReadOnlyIrradianceProbes() const;

	// Generates and Traces Probe-Update Rays.
	ERet UpdateProbes(
		const NwCameraView& scene_view
		, const VoxelGrids& voxel_grids
		, const NwTime& time
		, NGpu::NwRenderContext & render_context
		);

	ERet DebugDrawIfNeeded(
		const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		);

public:
private:
	//ERet _DoApplyNewSettings( const IrradianceFieldSettings& new_settings );
	void _AllocateResources( const IrradianceFieldSettings& settings );
	void _ReleaseResources();

	///
	ERet _GenerateRaysAndUpdateShaderConstants(
		const NwTime& time
		);

	/// Traces rays and writes results into ray-probe G-Buffer.
	ERet _TraceRays(
		const VoxelGrids& voxel_grids
		, NGpu::NwRenderContext & render_context
		);

	ERet _UpdateAndBlendProbes(
		NGpu::NwRenderContext & render_context
		);
};

//
//class VoxelGrids;
//
//struct IrradianceFieldSettings {};
//
////enum { rrHL2_AMBIENT_CUBE_SIDES = 6 };
//
//struct AmbientCubeFaceFlags
//{
//	enum Enum
//	{
//		PosX = BIT(0),
//		NegX = BIT(1),
//		PosY = BIT(2),
//		NegY = BIT(3),
//		PosZ = BIT(4),
//		NegZ = BIT(5),
//	};
//};
//mxDECLARE_FLAGS( AmbientCubeFaceFlags::Enum, U8, AmbientCubeFaceFlagsT );
//	
//
//struct CubemapVectors
//{
//	// direction, 2 * tan( half_apex angle )
//	V4f		cones[GPU_NUM_CUBEMAP_FACES][GPU_LIGHTPROBE_CUBEMAP_RES][GPU_LIGHTPROBE_CUBEMAP_RES];
//
//	// 
//	float	weights[GPU_NUM_CUBEMAP_FACES];
//};

}//namespace VXGI
}//namespace Rendering
