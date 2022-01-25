/*
=============================================================================
	Graphics model used for rendering.
=============================================================================
*/
#pragma once

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Scene/Entity.h>


namespace Rendering
{

/// Model behaviour.
enum EModelFlags
{
	MF_Unused = BIT(0)
};
mxDECLARE_FLAGS( EModelFlags, U32, BModelFlags );

mxPREALIGN(16) struct BoneMatrix
{
	union {
		Vector4	r[3];	// laid out as 3 rows in memory
		V4f		v[3];
	};
};

mxDECLARE_STRUCT(BoneMatrix);
mxDECLARE_POD_TYPE(BoneMatrix);

///	MeshInstance is a graphics model used for rendering;
///	it basically represents an instance of a renderable mesh in a scene,
/// i.e. several models can share the same mesh.
///
mxPREALIGN(16) class MeshInstance: CStruct, NonCopyable
{
public:
	TPtr< NwMesh >			mesh;			//!< The mesh used for rendering.
	TPtr< RrTransform >		transform;		//!< A pointer to the local-to-world transform.
	TBuffer< M44f >			joint_matrices;	//!< Current-pose object-space joint matrices for skinning.

	TArray< Material* >	materials;		//!< The graphics material for each submesh.

	BModelFlags					_flags;
	U32							_hash;	//!< used for draw-call storing; the idea is that similar models will have similar hashes

	// non-serializable fields:
	HRenderEntity			render_entity_handle;	//!< handle to the AABB proxy for frustum culling

	//
	Material *	z__materials_storage[nwRENDERER_NUM_RESERVED_SUBMESHES];

public:
	mxDECLARE_CLASS( MeshInstance, CStruct );
	mxDECLARE_REFLECTION;
	MeshInstance();

	ERet setupFromMesh(
		NwMesh* _mesh,
		NwClump & _storage,
		const M44f& _localToWorld = g_MM_Identity
		, Material* fallback_material = nil
		);

	static ERet ñreateFromMesh(
		MeshInstance *&new_model_
		, NwMesh* mesh
		, NwClump & storage
		, const M44f* local_to_world_transform = nil
		, Material* override_material = nil
	);
};

}//namespace Rendering


namespace Rendering
{
	ERet submitModel(
		const MeshInstance& mesh_instance
		, const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		, const TbPassMaskT allowed_passes_mask = ~0
		);

	ERet submitModelWithCustomTransform(
		const MeshInstance& mesh_instance
		, const M44f& local_to_world_transform
		, const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		, const TbPassMaskT allowed_passes_mask = ~0
		);

	ERet renderMeshInstances(
		const RenderCallbackParameters& parameters
		);
}//namespace Rendering
