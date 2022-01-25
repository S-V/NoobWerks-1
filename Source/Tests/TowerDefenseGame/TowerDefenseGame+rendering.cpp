#include <Base/Base.h>
#pragma once

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Graphics/graphics_shader_system.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/render_path.h>
#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain
#include <Renderer/private/shader_uniforms.h>	// ShaderGlobals
#include <Renderer/scene/SkyModel.h>

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/Meshok/Octree.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/VoxelEngine/VoxelizedMesh.h>

#include "TowerDefenseGame.h"

namespace
{
	static ProxyAllocator& rendererHeap() { return MemoryHeaps::renderer(); }
}



#if TB_TEST_LOAD_GLTF_MODEL

static
ERet loadTextureViewIfNeeded(
							 NwTexture **texture_
							 , const Tb_TextureView& texture_view
							 , NwClump * asset_storage
							 , const GrTextureCreationFlagsT flags
							 )
{
	mxASSERT( texture_view.isValid() );

	const AssetKey	asset_key = AssetKey::make< NwTexture >( texture_view.id );

	if( NwTexture* existing_texture = (NwTexture*)Resources::find( asset_key ) ) {
		*texture_ = existing_texture;
		return ALL_OK;
	}

	//
	NwTexture* new_texture;
	mxDO(asset_storage->New( new_texture ));

	//
	const GL::Memory* mem_block = GL::makeRef( texture_view.data.raw(), texture_view.data.rawSize() );

	mxDO(new_texture->loadFromMemory( mem_block, flags ));

	//
	mxDO(Resources::insert( asset_key, new_texture, asset_storage ));

	*texture_ = new_texture;

	return ALL_OK;
}

static
ERet renderGltfMesh(
					const NwMesh& gltf_mesh
					, const CameraMatrices& camera_matrices
					, NwClump * asset_storage
					, const TArray< TRefPtr< TcMaterial > >& submesh_materials
					, bool use_deferred_path
					)
{
	TbShaderEffect* shader_effect;
	mxDO(Resources::Load( shader_effect, MakeAssetID("test_PBR"), asset_storage ));

	//
	const Rendering::NwRenderSystem& render_sys = (Rendering::NwRenderSystem&) Rendering::NwRenderSystem::Get();

	const RenderPath& render_path = render_sys.getRenderPath();

	GL::GfxRenderContext & render_context = GL::getMainRenderContext();

	//
	const U32 renderPassIndex = render_path.getPassDrawingOrder(
		use_deferred_path ? ScenePasses::Opaque_GBuffer : ScenePasses::Opaque_Forward
		);

	const TbShaderEffect::Pass* passes = shader_effect->getPasses();
	const TbShaderEffect::Pass& pass = use_deferred_path ? passes[0] : passes[1];

	//
	for( UINT iSubmesh = 0; iSubmesh < gltf_mesh._submeshes.num(); iSubmesh++ )
	{
		const TbSubmesh& submesh = gltf_mesh._submeshes[ iSubmesh ];

		//
		const TcMaterial* submesh_material = submesh_materials[ iSubmesh ];

		//
		rendering::SubmitCommandsInCurrentScope	applyShaderCommands(
			render_context,
			GL::buildSortKey( renderPassIndex, GFX_DRAW_CALL_SORT_KEY_BIAS )
			);

		//
		GL::CommandBufferWriter	cbwriter( render_context._command_buffer );

		//
		IF_DEVELOPER GL::Dbg::GpuScope	perfScope(
			render_context._command_buffer
			, RGBAf::ORANGE.ToRGBAi().u
			, "GLTF mesh: subset=%d", iSubmesh
			);

		//
		const M44f	world_matrix
			= M44_uniformScaling(50)
			* M44_RotationX( DEG2RAD(90) )
			* M44_buildTranslationMatrix(CV3f(0, 0, 10))
			//(V4f&) Quaternion_RotationNormal( CV3f(1,0,0), DEG2RAD(90) )
			;
		const M44f	world_view_matrix = world_matrix * camera_matrices.viewMatrix;
		const M44f	world_view_projection_matrix = world_view_matrix * camera_matrices.projectionMatrix;

//const M44f	world_view_matrix_T = M44_Transpose(world_view_matrix);
//const M44f	world_view_projection_matrix_T = M44_Transpose(world_view_projection_matrix);

//mxDO(shader_effect.setUniform( mxHASH_STR("world_view_matrix"), &world_view_matrix_T ));
//mxDO(shader_effect.setUniform( mxHASH_STR("world_view_projection_matrix"), &world_view_projection_matrix_T ));

		//
		{
			G_PerObject	*	cb_per_object;
			mxDO(cbwriter.allocateUpdateBufferCommandWithData(
				TbRenderSystemI::Get()._global_uniforms.per_model.handle
				, &cb_per_object
				));

			ShaderGlobals::copyPerObjectConstants(
				cb_per_object
				, world_matrix
				, camera_matrices.viewMatrix
				, camera_matrices.view_projection_matrix
				, 0	// const UINT vxgiCascadeIndex
				);
		}

		//
		mxTODO("how to avoid writing operator -> ()?")
		if( const Tb_pbr_metallic_roughness* pbr_metallic_roughness = submesh_material->pbr_metallic_roughness.operator->() )
		{
			{
				NwTexture *	base_color_texture;
				mxDO(loadTextureViewIfNeeded(
					&base_color_texture,
					pbr_metallic_roughness->base_color_texture,
					asset_storage,
					GrTextureCreationFlagsT(
						// apply sRGB -> Linear conversion after sampling
						GrTextureCreationFlags::sRGB /*| GrTextureCreationFlags::GENERATE_MIPS*/ |
						GrTextureCreationFlags::defaults
						)
					));

				//
				mxDO(shader_effect->setResource(
					mxHASH_STR("t_base_color"),
					base_color_texture->m_resource
					));
			}

			//
			{
				NwTexture *	metallic_roughness_texture;
				mxDO(loadTextureViewIfNeeded(
					&metallic_roughness_texture,
					pbr_metallic_roughness->metallic_roughness_texture,
					asset_storage,
					GrTextureCreationFlags::defaults
					));

				//
				mxDO(shader_effect->setResource(
					mxHASH_STR("t_roughness_metallic"),
					metallic_roughness_texture->m_resource
					));
			}
		}

		//
		if( submesh_material->normal_texture.isValid() )
		{
			NwTexture *	normal_texture;
			mxDO(loadTextureViewIfNeeded(
				&normal_texture,
				submesh_material->normal_texture,
				asset_storage,
				GrTextureCreationFlags::defaults
				));

			//
			mxDO(shader_effect->setResource(
				mxHASH_STR("t_normals"),
				normal_texture->m_resource
				));
		}


		//
		shader_effect->pushShaderParameters( render_context._command_buffer );

		//
		GL::Cmd_Draw	batch;
		batch.reset();
		{
			batch.states = *pass.render_state;
			batch.program = pass.defaultProgram;

			batch.inputs = GL::Cmd_Draw::EncodeInputs(
				gltf_mesh.vertex_format->input_layout_handle,
				gltf_mesh.m_topology,
				gltf_mesh.m_flags & MESH_USES_32BIT_IB
				);

			batch.VB = gltf_mesh.geometry_buffers.VB->buffer_handle;
			batch.IB = gltf_mesh.geometry_buffers.IB->buffer_handle;

			batch.base_vertex = 0;
			batch.vertex_count = gltf_mesh.m_numVertices;
			batch.start_index = submesh.start_index;
			batch.index_count = submesh.index_count;
		}
		IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
		cbwriter.draw( batch );
	}//

	return ALL_OK;
}

#endif // TB_TEST_LOAD_GLTF_MODEL



ERet TowerDefenseGame::initialize_Renderer( const EngineSettings& engine_settings )
{
	Rendering::initializeResourceLoaders( rendererHeap() );

	_render_system = TbRenderSystemI::create(rendererHeap());
	mxDO(_render_system->initialize(
		engine_settings.window_width, engine_settings.window_height
		, m_settings.renderer
		));

	_render_world = ARenderWorld::staticCreate(rendererHeap());
	_render_world->setGlobalLight( m_settings.sun_light );
	_render_world->modifySettings( m_settings.renderer );

	//
#if 1
	_render_system->m_renderCallbacks.code[RE_VoxelTerrainChunk] = &VoxelTerrainRenderer::renderVoxelTerrainChunks_DLoD;
	_render_system->m_renderCallbacks.data[RE_VoxelTerrainChunk] = &_voxel_terrain_renderer;
#else
	_render_system->m_renderCallbacks.code[RE_VoxelTerrainChunk] = CustomTerrainRenderer::renderVoxelTerrainChunks_DLoD_with_Stencil_CSG;
	_render_system->m_renderCallbacks.data[RE_VoxelTerrainChunk] = &_voxel_terrain_renderer;
#endif

	//
	_render_system->_shadow_render_callbacks.code[ RE_VoxelTerrainChunk ] = &VoxelTerrainRenderer::renderShadowDepthMap_DLoD;
	_render_system->_shadow_render_callbacks.data[ RE_VoxelTerrainChunk ] = &_voxel_terrain_renderer;

	//
	_render_system->_voxelization_callbacks.code[ RE_VoxelTerrainChunk ] = &VoxelTerrainRenderer::voxelizeChunksOnGPU_DLoD;
	_render_system->_voxelization_callbacks.data[ RE_VoxelTerrainChunk ] = &_voxel_terrain_renderer;

	//
	mxDO(_render_system->m_debugRenderer.reserveSpace( mxMiB(1), mxMiB(1), mxKiB(64) ));

	//
	skyBoxGeometry.Build();

	return ALL_OK;
}

void TowerDefenseGame::shutdown_Renderer()
{
	Rendering::shutdownResourceLoaders( rendererHeap() );
}

#if VOXEL_TERRAIN5_WITH_PHYSICS
void dbg_drawRigidBodyFrames( const Physics::TbPhysicsWorld& physicsWorld, TbPrimitiveBatcher & debugRenderer )
{
	const btDynamicsWorld& bullet_dynamics_world = physicsWorld.m_dynamicsWorld;
	const btCollisionObjectArray& col_objects = bullet_dynamics_world.getCollisionObjectArray();

	for (int i= 0; i < col_objects.size(); i++)
	{
		btCollisionObject* col_obj = col_objects[i];
		btRigidBody* body = btRigidBody::upcast(col_obj);

		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);

			const btMatrix3x3& basis = trans.getBasis();

			debugRenderer.drawFrame(
				fromBulletVec(trans.getOrigin())
				, fromBulletVec(basis.getColumn(0)), fromBulletVec(basis.getColumn(1)), fromBulletVec(basis.getColumn(2))
				);
		}
	}
}
#endif

///
ERet TowerDefenseGame::Draw( const InputState& input_state )
{
	rmt_ScopedCPUSample(Draw, 0);
	PROFILE;

	const bool isInGameState = ( _game_state_manager.CurrentState() == GameStates::main );

//	const double frameStartTimeSeconds = mxGetTimeInSeconds();
	const V3f region_of_interest_position = m_settings.use_fake_camera ? m_settings.fake_camera.origin : camera.m_eyePosition;

	CameraMatrices camera_matrices;
	camera_matrices.origin = camera.m_eyePosition;
	camera_matrices.right = camera.m_rightDirection;
	camera_matrices.look = camera.m_lookDirection;
	camera_matrices.near_clip = maxf( m_settings.renderer.camera_near_clip, 0.1f );
	camera_matrices.far_clip = m_settings.renderer.camera_far_clip;
	camera_matrices.viewportWidth = input_state.window.width;
	camera_matrices.viewportHeight = input_state.window.height;
	camera_matrices.recomputeDerivedMatrices();

	_last_camera_matrices = camera_matrices;

	//
	const M44f view_projection_matrixT = M44_Transpose(camera_matrices.view_projection_matrix);	// transposed

	GL::GfxRenderContext & render_context = GL::getMainRenderContext();

	//
	Viewport	viewport;
	viewport.x = 0;	viewport.y = 0;
	viewport.width = input_state.window.width;
	viewport.height = input_state.window.height;




	if( m_settings.terrain_roi_torchlight_params.enabled )
	{
		TbDynamicPointLight	test_light;
		test_light.position = region_of_interest_position;
		test_light.radius = m_settings.terrain_roi_torchlight_params.radius;
		test_light.color = m_settings.terrain_roi_torchlight_params.color;
		_render_world->pushDynamicPointLight(test_light);
	}




	mxDO(_render_system->render( _render_world, camera_matrices, m_settings.renderer ));


	_worldState.render( camera_matrices, render_context );


	if(_test_render_model != nil)
	{
		Rendering::submitModel( *_test_render_model, camera_matrices, render_context );
	}

	if(_voxel_editor_model != nil && m_settings.voxel_editor_tools.place_mesh_tool_is_enabled)
	{
		const M44f	local_to_world_transform = _editor_manipulator.getTransformMatrix();

		_voxel_editor_model->transform->setLocalToWorldMatrix( local_to_world_transform );

		const AABBf mesh_aabb_in_world_space = _stamped_mesh.local_AABB.transformed( local_to_world_transform );

		//
		_render_world->updateBoundingBox(
			_voxel_editor_model->m_proxy
			, mesh_aabb_in_world_space
			);

		Rendering::submitModel( *_voxel_editor_model, camera_matrices, render_context );
	}


	_drawSky();


	_game_player.renderFirstPersonView(
		camera_matrices
		, render_context
		, m_settings.weapon_def
		);


	if( m_settings.dbg_draw_game_world )
	{
		_worldState.dbg_draw(
			&Rendering::NwRenderSystem::shared().m_debugRenderer
			, m_settings.ai
			);
	}


#if VOXEL_TERRAIN5_WITH_PHYSICS
	if( m_settings.physics.dbg_draw_bullet_physics )
	{
		// VERY SLOW!!!
		_physics_world.m_dynamicsWorld.debugDrawWorld();
		//dbg_drawRigidBodyFrames(_physics_world, _render_system->m_debugRenderer);
	}
#endif




	if(1)
	{
		const U32 passIndex = _render_system->getRenderPath().getPassDrawingOrder(mxHASH_STR("DebugLines"));

		TbShaderEffect* shaderTech;
		mxDO(Resources::Load(shaderTech, MakeAssetID("auxiliary_rendering"), &m_runtime_clump));
		_render_system->m_debugRenderer.setTechnique(shaderTech);

		{
			rendering::SubmitCommandsInCurrentScope	applyShaderCommands(
				render_context,
				GL::buildSortKey( passIndex, 0 )	// first
				);
			mxDO(shaderTech->bindUniformBufferData(
				render_context._command_buffer,
				view_projection_matrixT
				));
			mxDO(shaderTech->pushShaderParameters( render_context._command_buffer ));
		}

		_render_system->m_debugRenderer.SetViewID( passIndex );

		{
			NwRenderState * renderState = nil;
			mxDO(Resources::Load(renderState, MakeAssetID("default")));
			_render_system->m_debugRenderer.SetStates( *renderState );
			_render_system->m_debug_line_renderer.flush( _render_system->m_debugRenderer );

			_render_system->m_debugRenderer.Flush();
		}

		{
			NwRenderState * renderState = nil;
			mxDO(Resources::Load(renderState, MakeAssetID("nocull")));
			_render_system->m_debugRenderer.SetStates( *renderState );
		}

		if( m_settings.voxels_draw_debug_data )
		{
			m_dbgView.Draw(camera_matrices, _render_system->m_debugRenderer, input_state, m_settings.voxels_debug_viz);
			this->_gi_debugDraw();
		}

		{
			NwRenderState * renderState = nil;
			mxDO(Resources::Load(renderState, MakeAssetID("nocull")));
			_render_system->m_debugRenderer.SetStates( *renderState );
		}

		if( m_settings.use_fake_camera && m_settings.draw_fake_camera )
		{
			const M44f fakeCamera_viewMatrix = M44_BuildView(
				m_settings.fake_camera.right,
				m_settings.fake_camera.look,
				m_settings.fake_camera.getUpDir(),
				m_settings.fake_camera.origin
			);
			const M44f fakeCamera_projectionMatrix = M44_Perspective(
				m_settings.fake_camera.half_FoV_Y*2,
				m_settings.fake_camera.aspect_ratio,
				m_settings.fake_camera.near_clip,
				m_settings.fake_camera.far_clip
			);
			const M44f fakeCamera_view_projection_matrix = M44_Multiply(
				fakeCamera_viewMatrix, fakeCamera_projectionMatrix
			);
			_render_system->m_debugRenderer.DrawWireFrustum( fakeCamera_view_projection_matrix );
		}

		// Crosshair
		if( m_settings.drawCrosshair )
		{
			_render_system->m_debugRenderer.DrawSolidCircle(
				camera.m_eyePosition + camera.m_lookDirection,
				camera.m_rightDirection,
				camera.m_upDirection,
				RGBAf::RED,
				0.01f
			);
		}
		if( m_settings.drawGizmo )
		{
			DrawGizmo(
				m_gizmo_renderer,
				M44_Identity(),//M44_buildTranslationMatrix(V4f::set(VX5::CHUNK_SIZE_CELLS*10.f,1)),//
				camera.m_eyePosition, _render_system->m_debugRenderer);
		}
	}



	//{
	//	NwMaterial* terrainMaterial = m_terrainMaterials.mat_terrain_DLOD;
	//	//const TbShader* terrainShader = terrainMaterial->m_program;
	//	//const FxShaderPass& pass0 = terrainShader->passes.getFirst();
	//	const TbShaderFeatureBitMask shader_program_index = 0
	//		//pass0.ComposeFeatureMask( mxHASH_STR("ENABLE_GEOMORPHING"), m_settings.renderer.terrain.enable_geomorphing )|
	//		//pass0.ComposeFeatureMask( mxHASH_STR("ENFORCE_BOUNDARY_CONSTRAINTS"), m_settings.renderer.terrain.enforce_LoD_constraints )|
	//		//pass0.ComposeFeatureMask( mxHASH_STR("HIGHLIGHT_LOD_TRANSITIONS"), m_settings.renderer.terrain.highlight_LOD_transitions )|
	//		//pass0.ComposeFeatureMask( mxHASH_STR("MULTIPLY_WITH_VERTEX_COLOR"), m_settings.renderer.terrain.enable_vertex_colors )
	//		;
	//	terrainMaterial->m_instance = shader_program_index;
	//}

	if( _voxel_world._settings.debug_draw_LOD_structure )
	{
		_voxel_world.dbg_draw(
			camera_matrices,
			_render_system->m_debugRenderer,
			input_state,
			0
			);
	}



	if( m_settings.use_fake_camera )
	{
		//if( DistanceBetween( region_of_interest_position, camera.m_eyePosition ) > 10.0f )	// don't block our view
		{
			//_render_system->m_debugRenderer.DrawSolidSphere( region_of_interest_position, 2.0f, RGBAf::RED );
			float radius = V3_Length(camera.m_eyePosition - m_settings.fake_camera.origin) * 0.03f;
			_render_system->m_debugRenderer.DrawSolidSphere( region_of_interest_position, radius, RGBAf::RED );
		}
	}

	if( input_state.buttons.released[KEY_F3] )
	{
		m_settings.player_torchlight_params.enabled ^= 1;
	}
	if( m_settings.player_torchlight_params.enabled )
	{
		TbDynamicPointLight	player_torch_light;
		player_torch_light.position = camera.m_eyePosition;
		player_torch_light.radius = m_settings.player_torchlight_params.radius;
		player_torch_light.color = m_settings.player_torchlight_params.color;
		_render_world->pushDynamicPointLight(player_torch_light);

		//m_torchlightSpot->SetOrigin( camera.m_eyePosition - camera.m_upDirection * 3.0f );
		//m_torchlightSpot->SetDirection( camera.m_lookDirection );
	}


	if( input_state.keyboard.held[KEY_F5] ) {
		mxDO(SON::SaveToFile(camera, "saved_camera_state.son"));
		//SON::SaveClumpToFile(m_runtime_clump, "R:/scene.son");
	}
	if( input_state.keyboard.held[KEY_F9] )
	{
		TempAllocator4096	temp_allocator( &MemoryHeaps::global() );
		mxDO(SON::LoadFromFile( "saved_camera_state.son", camera, temp_allocator ));
	}

#if 0
	// adjust engine_settings for demo
	if( input_state.buttons.released[KEY_F4] ) {
		m_settings.renderer.drawModels ^= 1;
	}
	// adjust engine_settings for demo
	if( input_state.buttons.released[KEY_F10] )
	{
		static bool bSLOW = true;
		if( bSLOW ) {
			// SLOW!
			m_settings.drawSky = true;
			m_settings.renderer.enableHDR = false;
			m_settings.renderer.enableGlobalFog = true;
			m_settings.renderer.enableShadows = true;      
		} else {
			m_settings.drawSky = false;
			m_settings.renderer.enableHDR = true;
			m_settings.renderer.enableGlobalFog = false;
			m_settings.renderer.enableShadows = true;
		}
		bSLOW ^= 1;
		Rendering::g_settings = m_settings.renderer;
	}
#endif

if( input_state.buttons.released[KEY_L] ) {
	m_settings.physics.enable_player_collisions ^= 1;
}



if( input_state.buttons.released[KEY_E] ) {
_voxel_world._settings.debug_draw_LOD_structure ^= 1;
}
if( isInGameState )
{
	if( (input_state.modifiers & KeyModifier_Ctrl) && input_state.keyboard.held[KEY_C] ) {
		m_settings.fake_camera = camera_matrices;
	}
	if( (input_state.modifiers & KeyModifier_Ctrl) && input_state.keyboard.held[KEY_M] ) {
		_voxel_world.dbg_RemeshAll();
	}
	if( (input_state.modifiers & KeyModifier_Ctrl) && input_state.keyboard.held[KEY_B] )
	{
		//_voxel_world.DBG_GenChunks(
		//	m_settings.use_fake_camera ? m_settings.fake_camera.origin : camera.m_eyePosition,
		//	16
		//	);
	}
	//if( (input_state.modifiers & KeyModifier_Ctrl) && input_state.keyboard.held[KEY_U] ) {
	//	_voxel_world.UpdateChunkAdjacencyMasks();
	//}

	VX::Dbg_MoveSpectatorCamera(camera, input_state);
	VX::Dbg_ControlTerrainCamera( m_settings.fake_camera.origin, m_settings.fake_camera_speed, input_state );	
}

//NOTE: always accepts input, even in editor/debug mode
VX::Dbg_ControlDebugView( m_dbgView, input_state );



if( input_state.keyboard.held[KEY_F11] )
{
	mxUNDONE
	//Engine::SaveScreenShot(
	//	EImageFileFormat::IFF_JPG
	//	//EImageFileFormat::IFF_PNG
	//);
}

	//if( isInGameState && (input_state.mouse.buttons & BIT(MouseButton_Right)) )
	//{
	//	m_dbgView.CastDebugRay( camera.m_eyePosition, camera.m_lookDirection );
	//}




	//this->_drawCurrentVoxelTool( m_settings.voxel_tool
	//	, camera_matrices
	//	, render_context
	//	, m_runtime_clump );





#if TB_TEST_LOAD_GLTF_MODEL
	renderGltfMesh( *gltf_mesh, camera_matrices, &m_runtime_clump, gltf_submesh_materials, m_settings.pbr_test_use_deferred );
#endif // TB_TEST_LOAD_GLTF_MODEL






#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	{
		const TbShaderEffect& effect = *_voxel_terrain_material_instance->material->effect;

		const UINT iProgramIndex = 0
			| effect.composeFeatureMask( mxHASH_STR("ENABLE_NORMAL_MAP"), m_settings.planet.enable_normal_mapping )
			;

		_voxel_terrain_material_instance->program = effect.getPasses()[0].programHandles[ iProgramIndex ];
	}
#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE


	_render_system->m_debugRenderer.Flush();

	this->_drawImGui( input_state, camera_matrices );

	return ALL_OK;
}

void TowerDefenseGame::_drawSky()
{
	const GL::GfxViewIndexT viewId = _render_system->getRenderPath().getPassDrawingOrder(ScenePasses::Sky);

	switch( m_settings.skyRenderingMode )
	{
	case DemoSkyRenderMode::None:
		break;

	case DemoSkyRenderMode::Skybox:
	case DemoSkyRenderMode::Skydome:
		Rendering::renderSkybox_EnvironmentMap(
			skyBoxGeometry
			, GL::getMainRenderContext()
			, viewId

			//, MakeAssetID("MarlinCube")// from ATI SDK 2006; NOTE: their Y - UP
			//, MakeAssetID("space")	// from ATI SDK 2006;
			//, MakeAssetID("sky_cube")// from NVIDIA Direct3D SDK 11; NOTE: their Y - UP
			, MakeAssetID("NYC_dusk")// from NVIDIA Direct3D SDK 11; Z is up, but visible seams
			);
		break;

	case DemoSkyRenderMode::Analytic:
	default:
		Rendering::renderSkybox_Analytic(
			skyBoxGeometry
			, GL::getMainRenderContext()
			, viewId
			);
	}
}

void TowerDefenseGame::_drawCurrentVoxelTool( const VX::SVoxelToolSettings_All& voxel_tool_settings
											   , const CameraMatrices& camera_matrices
											   , GL::GfxRenderContext & render_context
											   , NwClump & storage_clump )
{
	switch( voxel_tool_settings.current_tool )
	{
	case VX::VoxelToolType::CSG_Add_Shape:
		this->_drawVoxelTool_AddShape( voxel_tool_settings.csg_add, camera_matrices
			, render_context
			, storage_clump );
		break;

	case VX::VoxelToolType::CSG_Subtract_Shape:
		this->_drawVoxelTool_SubtractShape( voxel_tool_settings.csg_subtract, camera_matrices
			, render_context
			, storage_clump );
		break;

	case VX::VoxelToolType::CSG_Add_Model:
		break;

	case VX::VoxelToolType::CSG_Subtract_Model:
		break;

	case VX::VoxelToolType::Paint_Brush:
		break;

	case VX::VoxelToolType::SmoothGeometry:
		break;

	case VX::VoxelToolType::Grow:
		break;

	case VX::VoxelToolType::Shrink:
		break;

	default:
		LogStream(LL_Warn),"Unhandled tool: ",m_settings.voxel_tool.current_tool;
	}
}

static
void composeDrawMeshCommand( GL::Cmd_Draw *batch_
							, const NwMesh& mesh
							, const HProgram shader_program
							, const RenderState32& render_state
							)
{
	batch_->reset();
	batch_->states = render_state;

	//
	batch_->inputs = GL::Cmd_Draw::EncodeInputs(
		mesh.vertex_format->input_layout_handle,
		mesh.m_topology,
		mesh.m_flags & MESH_USES_32BIT_IB
		);

	//
	batch_->VB = mesh.geometry_buffers.VB->buffer_handle;
	batch_->IB = mesh.geometry_buffers.IB->buffer_handle;

	//
	batch_->base_vertex = 0;
	batch_->vertex_count = mesh.m_numVertices;
	batch_->start_index = 0;
	batch_->index_count = mesh.m_numIndices;

	//
	batch_->program = shader_program;
}

static
ERet drawVoxelBrushMesh( const V3f& brush_center, const float brush_radius
						, const VX::EVoxelBrushShapeType brush_type
						, const bool is_additive_brush	// specifies whether it adds or subtracts
						, const CameraMatrices& camera_matrices
						, GL::GfxRenderContext & render_context
						, NwClump & storage_clump
						, const TbRenderSystemI& renderSystem
						)
{
	TPtr< NwMesh >			unit_sphere_mesh;

	const char* voxel_brush_mesh_name = (brush_type == VX::VoxelBrushShape_Sphere)
		? "unit_sphere_ico"
		: "unit_cube"
		;

	mxDO(Resources::Load(
		unit_sphere_mesh._ptr
		,MakeAssetID(voxel_brush_mesh_name)
		,&storage_clump
		));

	//
	const U32 view_id = renderSystem.getRenderPath().getPassDrawingOrder(ScenePasses::Translucent);

	TbShader* shader;
	mxDO(Resources::Load(shader, MakeAssetID("material_voxel_tool_shape"), &storage_clump));

	//
	const FxShaderPass& pass0 = shader->passes.getFirst();

	const TbShaderFeatureBitMask shader_program_index = 0
		|pass0.ComposeFeatureMask( mxHASH_STR("bLit"), true )
		|pass0.ComposeFeatureMask( mxHASH_STR("bCSGAdd"), is_additive_brush )
		;

	//
	NwRenderState * render_state = nil;
	mxDO(Resources::Load( render_state, MakeAssetID("voxel_tool_brush"), &storage_clump ));


	//
	GL::CommandBufferWriter	cbwriter( render_context._command_buffer );

	//
	const U32 first_command_offset = render_context._command_buffer.currentOffset();

	// Update constant buffers.
	{
		G_PerObject	*	per_object_cb_data;
		mxDO(cbwriter.allocateUpdateBufferCommandWithData(
			renderSystem._global_uniforms.per_model.handle
			, &per_object_cb_data
			));


		const M44f local_to_world_matrix = M44_uniformScaling(brush_radius) * M44_buildTranslationMatrix(brush_center);

		ShaderGlobals::copyPerObjectConstants(
			per_object_cb_data
			, local_to_world_matrix
			, camera_matrices.viewMatrix
			, camera_matrices.view_projection_matrix
			, 0
			);
	}

	GL::Cmd_Draw	batch;

	composeDrawMeshCommand( &batch
		, *unit_sphere_mesh
		, shader->programs[shader_program_index]
		, *render_state
		);

	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;

	cbwriter.draw( batch );

	//
	render_context.submit(
		first_command_offset,
		GL::buildSortKey(view_id,
		GL::buildSortKey(batch.program, 0)
		)
		);

	return ALL_OK;
}

void TowerDefenseGame::_drawVoxelTool_AddShape( const VX::SVoxelToolSettings_CSG_Add& voxel_editor_tool_additive_brush
												 , const CameraMatrices& camera_matrices
												 , GL::GfxRenderContext & render_context
												 , NwClump & storage_clump )
{
	if( _voxel_tool_raycast_hit_anything )
	{
		drawVoxelBrushMesh( _last_pos_where_voxel_tool_raycast_hit_world, voxel_editor_tool_additive_brush.brush_radius_in_world_units
			, voxel_editor_tool_additive_brush.brush_shape_type, true /*is_additive_brush*/
			, camera_matrices, render_context, storage_clump, *_render_system );
	}
}


void TowerDefenseGame::_drawVoxelTool_SubtractShape( const VX::SVoxelToolSettings_CSG_Subtract& voxel_tool_subtract_shape
													  , const CameraMatrices& camera_matrices
													  , GL::GfxRenderContext & render_context
													  , NwClump & storage_clump )
{
	if( _voxel_tool_raycast_hit_anything )
	{
		drawVoxelBrushMesh( _last_pos_where_voxel_tool_raycast_hit_world, voxel_tool_subtract_shape.brush_radius_in_world_units
			, voxel_tool_subtract_shape.brush_shape_type, false /*is_additive_brush*/
			, camera_matrices, render_context, storage_clump, *_render_system );
	}
}
