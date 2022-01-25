#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/ShaderInterop.h>

#include <Physics/RigidBody.h>
#include <Physics/PhysicsWorld.h>

#include <Engine/Model.h>



mxDEFINE_CLASS(NwModelDef);
mxBEGIN_REFLECTION(NwModelDef)
	mxMEMBER_FIELD( mesh ),

	mxMEMBER_FIELD( shader ),

	mxMEMBER_FIELD( base_map ),
	mxMEMBER_FIELD( normal_map ),
mxEND_REFLECTION;
NwModelDef::NwModelDef()
{
}

ERet NwModelDef::LoadAssets(
	NwClump& storage_clump
	)
{
	mxDO(mesh.Load(&storage_clump));

	//
	if(!AssetId_IsValid(shader._id))
	{
		shader._setId(MakeAssetID("model"));
	}
	mxDO(shader.Load(&storage_clump));

	//
	if(AssetId_IsValid(base_map._id))
	{
		mxDO(base_map.Load(&storage_clump));
	}

	//
	if(AssetId_IsValid(normal_map._id))
	{
		mxDO(normal_map.Load(&storage_clump));
	}

	return ALL_OK;
}


mxDEFINE_CLASS(NwModel);
mxBEGIN_REFLECTION(NwModel)
mxEND_REFLECTION
NwModel::NwModel()
{
	mxTODO("make a class for forward decl and init'ed by default!");
	world_proxy_handle.SetNil();

	visual_to_physics_transform = M44_Identity();
}

ERet NwModel::CreateFromDef(
	NwModel *&new_model_
	, const TResPtr< NwModelDef >& model_def
	, NwClump& storage_clump
	, const M44f* initial_transform // = nil
	//, Material* override_material	// = nil
	)
{
	TResPtr< NwShaderEffect >	shader_fx(model_def->shader);
	mxDO(shader_fx.Load(&storage_clump));

	//
	NwModel* new_model;
	mxDO(storage_clump.New( new_model ));

	//
	new_model->mesh = model_def->mesh;

	new_model->shader_fx = shader_fx;

	//
	TPtr< Rendering::RrTransform > model_transform;
	mxDO(storage_clump.New( model_transform._ptr ));

	if( initial_transform ) {
		model_transform->setLocalToWorldMatrix( *initial_transform );
	} else {
		model_transform->setLocalToWorldMatrix( g_MM_Identity );
	}

	new_model->transform = model_transform;

	//
	new_model->base_map = model_def->base_map;
	new_model->normal_map = model_def->normal_map;

	//
	new_model_ = new_model;

	return ALL_OK;
}


namespace NwModel_
{

ERet Create(
	NwModel *&new_model_

	, const AssetID& model_def_id
	, NwClump & storage

	, const M44f& visual_to_physics_transform
	, Rendering::SpatialDatabaseI& spatial_database

	, NwPhysicsWorld& physics_world
	, const int rigid_body_type // PHYS_OBJ_TYPE
	, const M44f& local_to_world_transform
	, const COL_REPR col_repr /*= COL_BOX*/

	, const V3f& initial_linear_velocity /*= CV3f(0)*/
	, const V3f& initial_angular_velocity /*= CV3f(0)*/

	, const PhysObjParams& phys_obj_params /*= PhysObjParams()*/
)
{
	TResPtr< NwModelDef >	model_def(model_def_id);
	mxDO(model_def.Load(&storage));

	mxDO(model_def->LoadAssets(storage));

	//
	NwModel*	new_model_inst;
	mxDO(NwModel::CreateFromDef(
		new_model_inst
		, model_def
		, storage
		, &(visual_to_physics_transform * local_to_world_transform)
		));

	//
	SpatialDatabaseI_::Add_NwModel(
		&spatial_database
		, new_model_inst
		);

	//
	NwRigidBody* new_rigid_body;
	mxDO(storage.New(new_rigid_body, NwRigidBody::ALLOC_GRANULARITY));

	//
	const AABBf local_aabb = new_model_inst->mesh->local_aabb;
	const V3f col_shape_half_extent = V3f::mul(
		local_aabb.halfSize(),
		visual_to_physics_transform.GetScaling()
		);

	//
	new_model_inst->visual_to_physics_transform = visual_to_physics_transform;

	//
	btCollisionShape* col_shape = nil;
	
	switch(col_repr)
	{
	case COL_BOX:
		col_shape = new(new_rigid_body->_col_shape_buf) btBoxShape(
			toBulletVec(col_shape_half_extent)
			);
		break;

	case COL_SPHERE:
		col_shape = new(new_rigid_body->_col_shape_buf) btSphereShape(
			col_shape_half_extent.maxComponent()
			);
		break;
	}


	//
	const float mass = local_aabb.volume() * 100;

	//
	btVector3	localInertia;
	col_shape->calculateLocalInertia(mass, localInertia);

	btRigidBody* bt_rigid_body = new(new_rigid_body->_rigid_body_storage) btRigidBody(
		mass,
		nil,
		col_shape,
		localInertia
		);

	const btTransform bt_rb_initial_transform = Get_btTransform(
		local_to_world_transform
		);
	bt_rigid_body->setCenterOfMassTransform( bt_rb_initial_transform );
	bt_rigid_body->setLinearVelocity(toBulletVec(initial_linear_velocity));
	bt_rigid_body->setAngularVelocity(toBulletVec(initial_angular_velocity));

	//
	bt_rigid_body->setFriction(phys_obj_params.friction);
	bt_rigid_body->setRestitution(phys_obj_params.restitution);

	// objects fall too slowly with high linear damping
	bt_rigid_body->setDamping( 1e-2f, 1e-4f );

	//
	bt_rigid_body->setUserIndex(rigid_body_type);


	if(mxUNLIKELY(phys_obj_params.is_static))
	{
		bt_rigid_body->setCollisionFlags(
			bt_rigid_body->getCollisionFlags()
			| (
			btCollisionObject::CF_KINEMATIC_OBJECT
			|
			btCollisionObject::CF_STATIC_OBJECT
			)
			);
	}

	physics_world.m_dynamicsWorld.addRigidBody(
		bt_rigid_body
		);

	
	//
	new_rigid_body->model = new_model_inst;
	new_model_inst->rigid_body = new_rigid_body;

	//
	new_model_ = new_model_inst;

	return ALL_OK;
}

void Delete(
	NwModel *& old_model
	, NwClump & storage
	, Rendering::SpatialDatabaseI& spatial_database
	, NwPhysicsWorld& physics_world
	)
{
	physics_world.DeleteRigidBody(old_model->rigid_body);
	SpatialDatabaseI_::Remove_NwModel(&spatial_database, old_model);
	//
	//
	storage.delete_(old_model->transform._ptr);
	storage.delete_(old_model);
}

}//namespace


namespace NwModel_
{

static
void UpdateGraphicsTransformFromPhysics2(
										const btRigidBody& bt_rigid_body
										, NwModel& model
										, Rendering::SpatialDatabaseI& spatial_database
										)
{
	const btTransform& bt_rb_xform = bt_rigid_body.getCenterOfMassTransform();
	const M44f	rigid_body_local_to_world_matrix = M44f_from_btTransform( bt_rb_xform );

	//
	const AABBf	aabb_in_model_space = model.mesh->local_aabb;

	//
	const M44f	gfx_local_to_world_matrix
		= model.visual_to_physics_transform
		* rigid_body_local_to_world_matrix
		;

	model.transform->local_to_world = gfx_local_to_world_matrix;

	//
	spatial_database.UpdateEntityBounds(
		model.world_proxy_handle,
		AABBd::fromOther(aabb_in_model_space.transformed(gfx_local_to_world_matrix))
		);
}

void UpdateGraphicsTransformsFromRigidBodies(
	NwClump & clump
	, Rendering::SpatialDatabaseI& spatial_database
	)
{
	NwClump::Iterator< NwModel >	model_iterator( clump );
	while( model_iterator.IsValid() )
	{
		NwModel& model = model_iterator.Value();

		//
		btRigidBody & bt_rb = model.rigid_body->bt_rb();

		if(bt_rb.getActivationState() != ISLAND_SLEEPING)
		{
			UpdateGraphicsTransformFromPhysics2(
				bt_rb
				, model
				, spatial_database
				);
		}
		//
		model_iterator.MoveToNext();
	}//For each EnemyNPC.
}

}//namespace


namespace NwModel_
{

void RegisterRenderCallback(
	Rendering::RenderCallbacks & render_callbacks
	)
{
	render_callbacks.code[ Rendering::RE_NwModel ] = &RenderInstances;
	render_callbacks.data[ Rendering::RE_NwModel ] = nil;
}


ERet RenderInstance(
	const NwModel& model
	, const M44f& local_to_world_matrix
	, const Rendering::NwCameraView& scene_view
	, const Rendering::RrGlobalSettings& renderer_settings
	, NGpu::NwRenderContext & render_context
	)
{
	const Rendering::RenderPath& render_path = Rendering::Globals::GetRenderPath();

	//
	const float distance_to_near_clipping_plane = 0;
UNDONE;
		//Plane_PointDistance(
		//scene_view.near_clipping_plane,
		//M44_getTranslation(local_to_world_matrix)
		//);
	const U32 sort_key_from_depth = 0;
	//Rendering::Sorting::DepthToBits( distance_to_near_clipping_plane );

	//
	G_PerObject	*	cb_per_object = nil;	// always valid
	mxDO(render_context._command_buffer.Allocate( &cb_per_object ));

	Rendering::ShaderGlobals::copyPerObjectConstants(
		cb_per_object
		, local_to_world_matrix
		, scene_view.view_matrix
		, scene_view.view_projection_matrix
		, 0	// const UINT vxgiCascadeIndex
		);

	//
	const Rendering::NwMesh& mesh = *model.mesh;

	//
	NGpu::Cmd_Draw	draw_cmd;

	draw_cmd.input_layout = mesh.vertex_format->input_layout_handle;
	draw_cmd.primitive_topology = mesh.topology;
	draw_cmd.use_32_bit_indices = mesh.flags & Rendering::MESH_USES_32BIT_IB;

	draw_cmd.VB = mesh.geometry_buffers.VB->buffer_handle;
	draw_cmd.IB = mesh.geometry_buffers.IB->buffer_handle;

	//
	const NwShaderEffect& shader_fx = *model.shader_fx;
	const TSpan< const NwShaderEffect::Pass > fx_passes = shader_fx.getPasses();
	
	//
	const NGpu::LayerID draw_order = render_path.getPassInfo(fx_passes._data[0].name_hash)
		.draw_order;

	//
	const UINT num_passes = fx_passes._count;
	mxASSERT(num_passes == 1);
	const UINT pass_index = 0;
	//
	//for( UINT pass_index = 0; pass_index < num_passes; pass_index++ )
	{
		const NwShaderEffect::Pass& pass = fx_passes._data[ pass_index ];

		//if( BIT(pass.filter_index) & allowed_passes_mask )
		{
			NGpu::RenderCommandWriter	cmd_writer( render_context );
			{
				if(model.base_map.IsValid())
				{
					cmd_writer.setResource( 0, model.base_map->m_resource );
					cmd_writer.setSampler(0, BuiltIns::g_samplers[BuiltIns::DiffuseMapSampler]);
				}
				
				if(model.normal_map.IsValid()) {
					cmd_writer.setResource( 1, model.normal_map->m_resource );
					cmd_writer.setSampler(1, BuiltIns::g_samplers[BuiltIns::NormalMapSampler]);
				}
			}

			cmd_writer.SetRenderState( pass.render_state );

			const HProgram program_handle = pass.default_program_handle;
			draw_cmd.program = program_handle;

			// Ensure that the uniform buffers will be filled with model matrices.
			// The uniform buffers will be updated only when necessary.
			{
				Rendering::RenderSystemData& d = *Rendering::Globals::g_data;

				mxDO(cmd_writer.BindCBufferDataCopy(
					*cb_per_object
					, d._global_uniforms.per_model.slot
					, d._global_uniforms.per_model.handle
					, (MX_DEVELOPER ? "cb_per_object" : nil)
					));
			}

			//
			draw_cmd.base_vertex = 0;
			draw_cmd.vertex_count = mesh.vertex_count;
			draw_cmd.start_index = 0;
			draw_cmd.index_count = mesh.index_count;
			IF_DEBUG draw_cmd.src_loc = GFX_SRCFILE_STRING;

			cmd_writer.Draw( draw_cmd );

			cmd_writer.SubmitCommandsWithSortKey(
				NGpu::buildSortKey( draw_order, program_handle, sort_key_from_depth )
			nwDBG_CMD_SRCFILE_STRING
				);
		}//If pass is allowed
	}//For each pass.

	return ALL_OK;
}

ERet RenderInstances(
	const Rendering::RenderCallbackParameters& parameters
	)
{
	tbPROFILE_FUNCTION;

	NGpu::NwRenderContext & render_context = parameters.render_context;

	//
	const Rendering::NwCameraView& scene_view = parameters.scene_view;

	//
	const NwModel*const*const model_instances = (const NwModel**) parameters.entities._data[0];

	//
	for( int i = 0; i < parameters.entities._count; i++ )
	{
		const NwModel& model = *(model_instances[ i ]);

		//
		RenderInstance(
			model
			, model.transform->local_to_world
			, scene_view
			, parameters.renderer_settings
			, render_context
			);

	}//For each visible terrain chunk.

	return ALL_OK;
}

}//namespace


namespace SpatialDatabaseI_
{
	void Add_NwModel(
		Rendering::SpatialDatabaseI* spatial_database
		, NwModel* model
		)
	{
		mxASSERT(model->world_proxy_handle.IsNil());

		model->world_proxy_handle = spatial_database->AddEntity(
			model,
			Rendering::RE_NwModel,
			AABBd::fromOther(
				model->getAabbInWorldSpace()
			)
		);
	}

	void Remove_NwModel(
		Rendering::SpatialDatabaseI* spatial_database
		, NwModel* model
		)
	{
		mxASSERT(model->world_proxy_handle.IsValid());

		spatial_database->RemoveEntity(
			model->world_proxy_handle
			);

		model->world_proxy_handle.SetNil();
	}

}//namespace
