/*
=============================================================================
	idTech4 (Doom 3) render model
=============================================================================
*/
#pragma once

#include <Core/Assets/Asset.h>
#include <Rendering/BuildConfig.h>
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Globals.h>	// HRenderEntity
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/Entity.h>
#include <Rendering/Private/Modules/idTech4/idMaterial.h>


namespace Rendering
{
struct idRenderModelFlags
{
	enum Enum
	{
		// true if we must change the projection matrix when rendering this model
		// so that this model is not clipped by the near plane.
		// This is used to bring first-person weapon very close to the near clipping plane.
		NeedsWeaponDepthHack	= BIT(0),
	};
};
mxDECLARE_FLAGS( idRenderModelFlags::Enum, U32, idRenderModelFlagsT );


/// used for rendering MD5 models
mxPREALIGN(16) struct idRenderModel: CStruct, NonCopyable
{
	M44f					local_to_world;

	idMaterialPassFlagsT	flags;

	TResPtr< NwMesh >		mesh;			//!< The mesh used for rendering.

	TArray< idMaterial* >	materials;		//!< The graphics material for each submesh.

	/// Current-pose object-space joint matrices for skinning.
	TBuffer< M44f >			joint_matrices;

	// can be used in any way by shader or model generation
	idLocalShaderParameters	shader_parms;


	// non-serializable fields:

	/// handle to the AABB proxy for frustum culling
	HRenderEntity			world_proxy_handle;

	//
	idMaterial *	z__materials_storage[nwRENDERER_NUM_RESERVED_SUBMESHES];

public:
	mxDECLARE_CLASS(idRenderModel, CStruct);
	mxDECLARE_REFLECTION;
	idRenderModel();

	const AABBf getAabbInWorldSpace() const
	{
		return mesh->local_aabb
			.transformed(local_to_world)
			;
	}
};


namespace idRenderModel_
{
	ERet CreateFromMesh(
		idRenderModel *&new_model_
		, const TResPtr< NwMesh >& mesh
		, NwClump & clump
		);
}//namespace

namespace idRenderModel_
{
	void RegisterRenderCallback(
		RenderCallbacks & render_callbacks
		);

	//
	ERet RenderInstances(
		const RenderCallbackParameters& parameters
		);

	// manual rendering (e.g. for player weapon model which is always visible)
	ERet RenderInstance(
		const idRenderModel& render_model
		, const M44f& local_to_world_matrix
		, const NwCameraView& scene_view
		, const RrGlobalSettings& renderer_settings
		, NGpu::NwRenderContext & render_context

		// only if idRenderModel has the NeedsWeaponDepthHack flag set
		, const float custom_FoV_Y_in_degrees = 0
	);

}//namespace


#if 0
namespace SpatialDatabaseI_
{
	void Add_idRenderModel(
		SpatialDatabaseI* spatial_database
		, idRenderModel* render_model
		);

	void Remove_idRenderModel(
		SpatialDatabaseI* spatial_database
		, idRenderModel* render_model
		);

}//namespace
#endif

}//namespace Rendering
