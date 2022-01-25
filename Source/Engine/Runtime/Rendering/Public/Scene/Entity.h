/*
=============================================================================
	File:	Renderer.h
	Desc:	High-level renderer interface.
=============================================================================
*/
#pragma once


namespace Rendering {


/// types of renderable objects
enum ERenderEntity
{
	RE_Bad = 0,	//!< used to pad object array for SIMD

	/// Generic mesh instance
	RE_MeshInstance,

	/// idTech4 (Doom 3) models
	RE_idRenderModel,

	/// A very simple static (non-skinned) model
	RE_NwModel,

	//RE_StaticModel,
	//RE_SkinnedModel,
	//RE_Vegetation,
	//RE_FogVolume,
	//RE_Cloud,
	//RE_Imposter,
	RE_Light,

	RE_DynamicPointLight,	// TbDynamicPointLight

	RE_Particles,	// VisibleParticlesT

	RE_VoxelTerrainChunk,

	RE_BurningFire,

	RE_MAX
};

mxDECLARE_16BIT_HANDLE(HRenderEntity);



struct RenderEntityBase
{
	// MUST BE EMPTY!
};


}//namespace Rendering
