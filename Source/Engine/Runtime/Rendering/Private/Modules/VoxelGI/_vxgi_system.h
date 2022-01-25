// Volumetric scene representation for quick occlusion testing and GI effects on GPU.
#pragma once

#include <Rendering/Public/Globals.h>	// NwCameraView, RenderPath
#include <Rendering/Public/Settings.h>	// RrVolumeTextureCommonSettings
#include <Rendering/Private/SceneRendering.h>

#include <Rendering/Private/Modules/VoxelGI/vxgi_brickmap.h>
#include <Rendering/Private/Modules/VoxelGI/vxgi_settings.h>

namespace Rendering
{
namespace VXGI
{


/// Contains the cascade's center and half-extent, in world space.
/// (see ClipMaps/Nested ClipBoxes in voxel terrain rendering)
struct Cascade: CubeCRf
{
};

/// see Global Illumination via Cascaded Voxel Cone Tracing
struct ClipMap: NonCopyable
{
	U16			num_cascades;	// 0 - disable shadow caster culling
	U16			dirty_cascades;

	Cascade		cascades[4];

	/// max distance for camera movement before re-centering the cascade
	F32			delta_distance[4];	//

public:
	ClipMap();

	void SetAllCascadesDirty() { dirty_cascades = ~0; }

	void OnCameraMoved( const V3f& new_position_in_world_space );

	void ApplySettings( const CascadeSettings& new_settings );
};


struct VolumeTextures
{
	struct DynamicGeom {
		/// Irradiance Cache
		/// Each voxel stores RGB - radiance color (injected light).
		/// Used for relighting diffuse GI probes
		/// and for (blurry) glossy reflections.
		HTexture	h_tex3d_lit_voxels;

		/// average opacity/density/occlusion (geometry)
		/// used for voxel-based AO and soft shadows
		HTexture	h_tex3d_opacity;
	} dynamic;

	//

	struct StaticGeom {

		/// Used for re-lighting the "lit voxels" texture above.
		/// Each voxel stores:
		/// - Albedo
		/// - Emissivity (optional)
		/// - Surface normal
		HTexture	h_tex3d_unlit_voxels;

		/// Used for re-lighting the "opacity" texture above.
		HTexture	h_tex3d_static_opacity;

	} static_geom;
};


///
class VoxelGrids: NonCopyable
{
public_internal:
	//
	static const DataFormatT VOXEL_RADIANCE_TEXTURE_FORMAT;
	static const DataFormatT VOXEL_DENSITY_TEXTURE_FORMAT;

	// G_VXGI
	HBuffer		cb_handle;	// constant buffer handle


	// During voxelization, the pixel shader writes voxel values into this buffer using atomic instructions.
	HBuffer		radiance_voxel_grid_buffer;

	///
	/// RGB - radiance color (injected light), A - average opacity/density/occlusion (geometry):
	HTexture	_radiance_opacity_texture[MAX_GI_CASCADES];
	//NOTE: could also use R10G10B10A2_UNORM: RGB - HDR color, A - flags (e.g. is voxel solid?)

	//// R8 - voxel density, updated on CPU
	//HTexture	_density_textures[MAX_GI_CASCADES];

	TPtr< BrickMap >	brickmap;
	TPtr< VoxelBVH >	voxel_BVH;


	//
	//VoxelizationCallbacks	_voxelization_callbacks;

	struct CPUStuff
	{
		ClipMap		clipmap;
	} cpu;

	//
	RuntimeSettings	settings;

public:
	VoxelGrids();
	~VoxelGrids();

	ERet Initialize( const RuntimeSettings& settings );
	void Shutdown();

	ERet ApplySettings( const RuntimeSettings& new_settings );

public:
	void OnRegionChanged(
		const AABBf& region_bounds_in_world_space
		);

	ERet UpdateAndReVoxelizeDirtyCascades(
		const SpatialDatabaseI& spatial_database
		, const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		, const RrGlobalSettings& renderer_settings
		);

	ERet DebugDrawIfNeeded(
		const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		);

private:
	ERet _UpdateShaderConstants();

	ERet _UpdateCascade(
		const U32 cascade_index
		, const NwCameraView& scene_view
		, const SpatialDatabaseI& spatial_database
		, NGpu::NwRenderContext & render_context
		, const RrGlobalSettings& renderer_settings
		);
		
public:
	void bindVoxelCascadeTexturesToShader(
		NwShaderEffect* technique
		) const;

public:
	ERet _Dbg_VizualizeVolumeTexture(
		const U32 cascade_index
		, const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		) const;

private:
	void _AllocateResources( const RuntimeSettings& settings );
	void _AllocateVoxelizationBuffer( const RuntimeSettings& settings );
	void _AllocateVoxelGridTextures( const RuntimeSettings& settings );

	void _ReleaseResources();
};

ERet RegisterCallbackForVoxelizingEntitiesOfType(
	const ERenderEntity entity_type
	, RenderCallback* render_callback
	, void* user_data
	);

}//namespace VXGI
}//namespace Rendering
