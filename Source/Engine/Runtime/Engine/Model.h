// A static (i.e. not skinned) mesh instance
// with physics (a collision object or rigid body).
// e.g. a barrel, a desk, a gibs/debris item.
#pragma once

#include <Core/Assets/AssetReference.h>

#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Globals.h>	// Rendering::HRenderEntity
#include <Rendering/Public/Core/Mesh.h>

#include <Engine/ForwardDecls.h>



struct NwModelDef: NwSharedResource
{
	TResPtr< Rendering::NwMesh >	mesh;

	//
	TResPtr< NwShaderEffect >	shader;

	// optional:
	TResPtr< NwTexture >	base_map;
	TResPtr< NwTexture >	normal_map;

public:
	mxDECLARE_CLASS(NwModelDef, NwSharedResource);
	mxDECLARE_REFLECTION;
	NwModelDef();

	ERet LoadAssets(
		NwClump& storage_clump
		);

	static const SerializationMethod SERIALIZATION_METHOD = Serialize_as_Text;
};


/// A static (i.e. not skinned) mesh instance
/// with a simple physics model (a collision object or rigid body).
///
struct NwModel: CStruct
{
	// Visuals
	TResPtr< Rendering::NwMesh >			mesh;
	TPtr< Rendering::RrTransform >			transform;

	//
	TResPtr< NwTexture >		base_map;
	TResPtr< NwTexture >		normal_map;

	//
	TResPtr< NwShaderEffect >	shader_fx;

	/// handle to the AABB proxy for frustum culling
	Rendering::HRenderEntity			world_proxy_handle;

	// scales down and rotates the graphics model to match the rigid body
	M44f		visual_to_physics_transform;

	// Physics
	TPtr< NwRigidBody >		rigid_body;

public:
	mxDECLARE_CLASS(NwModel,CStruct);
	mxDECLARE_REFLECTION;

	NwModel();

	static ERet CreateFromDef(
		NwModel *&new_model_
		, const TResPtr< NwModelDef >& model_def
		, NwClump& storage_clump
		, const M44f* initial_transform = nil
		);

	const AABBf getAabbInWorldSpace() const
	{
		return mesh->local_aabb
			.transformed(transform->local_to_world)
			;
	}
};


namespace NwModel_
{
	enum COL_REPR
	{
		COL_BOX,
		COL_SPHERE,
	};

	struct PhysObjParams
	{
		float	friction;
		float	restitution;

		bool	is_static;

	public:
		PhysObjParams()
		{
			friction = 0.5f;
			restitution = 0.5f;

			is_static = false;
		}
	};

	ERet Create(
		NwModel *&new_model_

		, const AssetID& model_def_id
		, NwClump & storage

		, const M44f& visual_to_physics_transform
		, Rendering::SpatialDatabaseI& spatial_database

		, NwPhysicsWorld& physics_world
		, const int rigid_body_type // PHYS_OBJ_TYPE
		, const M44f& local_to_world_transform
		, const COL_REPR col_repr = COL_BOX

		, const V3f& initial_linear_velocity = CV3f(0)
		, const V3f& initial_angular_velocity = CV3f(0)

		, const PhysObjParams& phys_obj_params = PhysObjParams()
	);

	void Delete(
		NwModel *& old_model
		, NwClump & storage
		, Rendering::SpatialDatabaseI& spatial_database
		, NwPhysicsWorld& physics_world
		);
}//namespace

namespace NwModel_
{
	void UpdateGraphicsTransformsFromRigidBodies(
		NwClump & clump
		, Rendering::SpatialDatabaseI& spatial_database
		);

}//namespace

namespace NwModel_
{
	void RegisterRenderCallback(
		Rendering::RenderCallbacks & render_callbacks
		);

	//
	ERet RenderInstances(
		const Rendering::RenderCallbackParameters& parameters
		);

	// manual rendering (e.g. for player weapon model which is always visible)
	ERet RenderInstance(
		const NwModel& render_model
		, const M44f& local_to_world_matrix
		, const Rendering::NwCameraView& scene_view
		, const Rendering::RrGlobalSettings& renderer_settings
		, NGpu::NwRenderContext & render_context
	);

}//namespace


namespace SpatialDatabaseI_
{
	void Add_NwModel(
		Rendering::SpatialDatabaseI* spatial_database
		, NwModel* render_model
		);

	void Remove_NwModel(
		Rendering::SpatialDatabaseI* spatial_database
		, NwModel* render_model
		);

}//namespace
