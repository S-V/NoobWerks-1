#include "stdafx.h"
#pragma hdrstop

#if MX_DEVELOPER
#include <Core/Util/Tweakable.h>
#endif // MX_EDITOR

#include <Engine/WindowsDriver.h>	// InputState
#include <Engine/Engine.h>	// NwEngineSettings
#include <Graphics/graphics_shader_system.h>
#include <Renderer/Renderer.h>
#include <Renderer/Pipeline/RenderPipeline.h>
mxREFACTOR(":")
#include <Renderer/private/RenderSystem.h>
#include <Engine/Model.h>

#include "FPSGame.h"
#include "game_settings.h"
#include "game_world/game_world.h"
#include "game_player.h"
#include "game_rendering/game_renderer.h"


namespace
{
	static ProxyAllocator& rendererHeap() { return MemoryHeaps::renderer(); }
}


tbDEFINE_VERTEX_FORMAT(BillboardStarVertex);
mxBEGIN_REFLECTION( BillboardStarVertex )
	mxMEMBER_FIELD( sphere_in_view_space ),
	mxMEMBER_FIELD( velocity_and_life ),
mxEND_REFLECTION
void BillboardStarVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float4, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0)
		.end();
}

/*
-----------------------------------------------------------------------------
	MyGameRenderer
-----------------------------------------------------------------------------
*/
MyGameRenderer::MyGameRenderer()
{
	_is_drawing_debug_lines = false;
}

ERet MyGameRenderer::initialize(
								 const NwEngineSettings& engine_settings
								 , const RrRuntimeSettings& renderer_settings
								 , NwClump* scene_clump
								 )
{
	_scene_clump = scene_clump;

	//
	Rendering::initializeResourceLoaders( rendererHeap() );

	//
	_render_system = NwRenderSystemI::create(rendererHeap());
	mxDO(_render_system->initialize(
		engine_settings.window.width, engine_settings.window.height
		, renderer_settings
		, scene_clump
		));

	//
	mxREFACTOR("move to engine!");
	NwModel_::RegisterRenderCallback(_render_system->_render_callbacks);


	//
	mxDO(_render_system->_debug_renderer.reserveSpace( mxMiB(1), mxMiB(1), mxKiB(64) ));

	//
	_gizmo_renderer.BuildGeometry();

	mxDO(particle_renderer.Initialize(
		engine_settings
		, renderer_settings
		, scene_clump
		));

	return ALL_OK;
}

void MyGameRenderer::shutdown()
{
	particle_renderer.Shutdown();

	//
	_render_system->shutdown();
	NwRenderSystemI::destroy( _render_system );
	_render_system = nil;

	Rendering::shutdownResourceLoaders( rendererHeap() );

	//
	_scene_clump = nil;
}

void MyGameRenderer::beginFrame(
	const NwCameraView& camera_view
	, const RrRuntimeSettings& renderer_settings
	, const NwTime& game_time
	)
{
	_render_system->beginFrame(
		camera_view
		, renderer_settings
		, game_time
		, _atmosphere_rendering_parameters
		);

	//
	particle_renderer.AdvanceParticlesOncePerFrame(
		game_time
		);

	//
	burning_fires.UpdateOncePerFrame(
		game_time
		);

	explosions.UpdateOncePerFrame(game_time);

	//
	decals.AdvanceDecalsOncePerFrame(
		game_time
		);
}

ERet MyGameRenderer::beginDebugLineDrawing(
	const NwCameraView& camera_view
	, GL::NwRenderContext & render_context
	)
{
	const U32 passIndex = _render_system->getRenderPath()
		.getPassDrawingOrder(mxHASH_STR("DebugLines"));

	NwShaderEffect* shaderTech;
	mxDO(Resources::Load(shaderTech, MakeAssetID("auxiliary_rendering"), _scene_clump));
	_render_system->_debug_renderer.setTechnique(shaderTech);

	{
		GL::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			GL::buildSortKey( passIndex, 0 )	// first
			);

		const M44f view_projection_matrixT = M44_Transpose(camera_view.view_projection_matrix);
		mxDO(shaderTech->BindCBufferData(
			render_context._command_buffer,
			view_projection_matrixT
			));

		mxDO(shaderTech->pushShaderParameters( render_context._command_buffer ));
	}

	_render_system->_debug_renderer.SetViewID( passIndex );

	_is_drawing_debug_lines = true;

	return ALL_OK;
}

void MyGameRenderer::endDebugLineDrawing()
{
	_render_system->_debug_line_renderer.flush(
		_render_system->_debug_renderer
		);
	_render_system->_debug_renderer.Flush();
	_is_drawing_debug_lines = false;
}

ERet MyGameRenderer::drawWorld_Debug(
									  const MyGameWorld& game_world
									  , const ARenderWorld* render_world
									  , const NwCameraView& camera_view
									  , const MyGamePlayer& game_player
									  , const MyGameSettings& game_settings
									  , const RrRuntimeSettings& renderer_settings
									  , const NwTime& game_time
									  , GL::NwRenderContext & render_context
									  )
{
	mxASSERT(_is_drawing_debug_lines);

	//
	if(game_settings.ai.debug_draw_AI)
	{
		game_world.NPCs.DebugDrawAI(
			game_time
			);
	}

	//
#if 0
	game_player.Dbg_RenderFirstPersonView(
		camera_view
		, *this
		);
#endif

#if !GAME_CFG_RELEASE_BUILD
	// Crosshair
	if( game_settings.drawCrosshair )
	{
		_render_system->_debug_renderer.DrawSolidCircle(
			camera_view.eye_pos + camera_view.look_dir,
			camera_view.right_dir,
			camera_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
	}
#endif

	//
	if( game_settings.drawGizmo )
	{
		const NwFloatingOrigin floating_origin = render_world->getFloatingOrigin();


		// Draws a red point at the center of the world.
#if 0
		_render_system->_debug_renderer.DrawSolidCircle(
			-V3f::fromXYZ(floating_origin.position_in_world_space),
			camera_view.right_dir,
			camera_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
#endif


		// draw axes at the world origin

		DrawGizmo(
			_gizmo_renderer
			, M44_BuildTS(
				-V3f::fromXYZ(floating_origin.center_in_world_space)// - camera_view.eye_pos
				, 1
			)
			, camera_view.eye_pos
			, _render_system->_debug_renderer
			);
	}

	//
	if( game_settings.dbg_use_3rd_person_camera_for_terrain_RoI )
	{
		//
		const NwFloatingOrigin floating_origin = render_world->getFloatingOrigin();

		double radius = sqrt(( game_player.camera._eye_position - game_settings.dbg_3rd_person_camera_position ).lengthSquared()) * 0.03;

		_render_system->_debug_renderer.DrawSolidSphere(
			floating_origin.getRelativePosition( game_settings.dbg_3rd_person_camera_position )
			, radius
			, RGBAf::RED
			);
	}




#if GAME_CFG_WITH_PHYSICS
	if( game_settings.physics.debug_draw )
	{
		// VERY SLOW!!!
		btDynamicsWorld& bt_world = (btDynamicsWorld&) game_world.physics_world.m_dynamicsWorld;
		bt_world.debugDrawWorld();
		//dbg_drawRigidBodyFrames(_physics_world, _render_system->m_debugRenderer);
	}
#endif // GAME_CFG_WITH_PHYSICS



#if TEST_LOAD_RED_FACTION_MODEL

	{
		const M44f local_to_world = M44f::createUniformScaling(10)
			* M44f::createTranslation(CV3f(0,0,0)
			);

		//
		if( game_settings.rf1.draw_mesh_in_bind_pose )
		{
			game_world._RF_test_model.drawBindPoseMesh(
				_render_system->_debug_renderer
				, local_to_world
				, RGBAf::DARK_GREY
				);
		}

		//
		struct Foo: ADebugTextRenderer
		{
			virtual void drawText(
				const V3f& position,
				const RGBAf& color,
				const V3f* normal_for_culling,
				const float font_scale,	// 1 by default
				const char* fmt, ...
				) override
			{
			};
		} _imgui_text_renderer;

		game_world._RF_test_model.drawOriginalBones_Quat(
			_render_system->_debug_renderer
			, _imgui_text_renderer
			, camera_view
			, local_to_world
			, RGBAf::ORANGE
			, RGBAf::RED
			);

		// WRONG
		//game_world._RF_test_model.drawBindPoseSkeleton(
		//	_render_system->_debug_renderer
		//	, camera_view
		//	, local_to_world
		//	, RGBAf::ORANGE
		//	, RGBAf::RED
		//	);
		//	



		//
		if( game_world._RF_test_model.isSkinned()
			&&
			!game_world._RF_test_anim.bone_tracks.isEmpty() )
		{
			//


			if( game_settings.rf1.draw_animated_skeleton )
			{
				const float anim_time_ratio = clampf(game_settings.rf1.anim_time_ratio / 100.0f, 0.0f, 1.0f);

				if(1)
				{
					game_world._RF_test_model.drawAnimatedSkeleton_Quat2(
						_render_system->_debug_renderer
						, _imgui_text_renderer
						, camera_view
						, local_to_world
						, RGBAf::GREEN	// child_joint_color
						, RGBAf::ORANGE	// parent_joint_color

						, game_world._RF_test_anim

						, anim_time_ratio
						//, _RF_test_anim_playback_ctrl._time_ratio
						);
				}
				else
				{
					game_world._RF_test_model.drawAnimatedSkeleton_Mat(
						_render_system->_debug_renderer
						, _imgui_text_renderer
						, camera_view
						, local_to_world
						, RGBAf::GREEN	// child_joint_color
						, RGBAf::ORANGE	// parent_joint_color

						, game_world._RF_test_anim

						, anim_time_ratio
						//, _RF_test_anim_playback_ctrl._time_ratio
						);
				}
			}
		}// if the test RF mesh is skinned and the test animation is loaded






	}

#endif // TEST_LOAD_RED_FACTION_MODEL



#if 0

	{
		NwRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("default")));
		_render_system->_debug_renderer.SetStates( *renderState );
		_render_system->_debug_line_renderer.flush( _render_system->_debug_renderer );

		_render_system->_debug_renderer.Flush();
	}

	{
		NwRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("nocull")));
		_render_system->_debug_renderer.SetStates( *renderState );
	}

	if( game_settings.voxels_draw_debug_data )
	{
		m_dbgView.Draw(camera_view, _render_system->_debug_renderer, input_state, game_settings.voxels_debug_viz);
		this->_gi_debugDraw();
	}

	{
		NwRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("nocull")));
		_render_system->_debug_renderer.SetStates( *renderState );
	}

	if( game_settings.use_fake_camera && game_settings.draw_fake_camera )
	{
		const M44f fakeCamera_viewMatrix = M44_BuildView(
			game_settings.fake_camera.right_dir,
			game_settings.fake_camera.look_dir,
			game_settings.fake_camera.getUpDir(),
			game_settings.fake_camera.origin
			);
		const M44f fakeCamera_projectionMatrix = M44_Perspective(
			game_settings.fake_camera.half_FoV_Y*2,
			game_settings.fake_camera.aspect_ratio,
			game_settings.fake_camera.near_clip,
			game_settings.fake_camera.far_clip
			);
		const M44f fakeCamera_view_projection_matrix = M44_Multiply(
			fakeCamera_viewMatrix, fakeCamera_projectionMatrix
			);
		_render_system->_debug_renderer.DrawWireFrustum( fakeCamera_view_projection_matrix );
	}





	//

	if( _voxel_world._settings.debug_draw_LOD_structure )
	{
		_voxel_world.dbg_draw(
			camera_view,
			_render_system->_debug_renderer,
			input_state,
			0
			);
	}


	if( input_state.buttons.released[KEY_F3] )
	{
		game_settings.player_torchlight_params.enabled ^= 1;
	}
	if( game_settings.player_torchlight_params.enabled )
	{
		TbDynamicPointLight	player_torch_light;
		player_torch_light.position = camera._eye_position;
		player_torch_light.radius = game_settings.player_torchlight_params.radius;
		player_torch_light.color = game_settings.player_torchlight_params.color;
		_render_world->pushDynamicPointLight(player_torch_light);

		//m_torchlightSpot->SetOrigin( camera._eye_position - camera.m_upDirection * 3.0f );
		//m_torchlightSpot->SetDirection( camera.m_look_direction );
	}
#endif

	return ALL_OK;
}

ERet MyGameRenderer::Submit_Skybox()
{
	GL::NwRenderContext& render_context = GL::getMainRenderContext();

	const GL::LayerID viewId = _render_system->getRenderPath().getPassDrawingOrder(ScenePasses::Sky);

#if 1

	{
		NwShaderEffect* technique = nil;
		mxDO(Resources::Load( technique, MakeAssetID("procedural_skybox") ));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		//
		GL::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			GL::buildSortKey( viewId, pass0.default_program_handle, 0 )
			);

		mxDO(Rendering::DrawSkybox_TriStrip_NoVBIB(
			render_context
			, pass0.render_state
			, pass0.default_program_handle
		));
	}


#elif 0
	Rendering::renderSkybox_Analytic(
		GL::getMainRenderContext()
		, viewId
		);
#else
	Rendering::renderSkybox_EnvironmentMap(
		GL::getMainRenderContext()
		, viewId

		//, MakeAssetID("MarlinCube")// from ATI SDK 2006; NOTE: their Y - UP
		, MakeAssetID("space")	// from ATI SDK 2006;
		//, MakeAssetID("sky_cube")// from NVIDIA Direct3D SDK 11; NOTE: their Y - UP
		//, MakeAssetID("NYC_dusk")// from NVIDIA Direct3D SDK 11; Z is up, but visible seams
		);
#endif

	return ALL_OK;
}

/*
*/
static void scaleIntoCameraDepthRange(
								 const V3d& pos_in_world_space
								 , const MyGamePlayer& game_player
								 , const NwCameraView& camera_view
								 , double *object_scale_	// (0 < scale < 1]
								 , V3d *scaled_position_relative_to_camera_
								 )
{
	//
	const V3d position_relative_to_camera =
		pos_in_world_space - game_player.camera._eye_position
		;

	const double real_distance_from_camera = sqrt( position_relative_to_camera.lengthSquared() );

	//
	const double half_depth_range = camera_view.far_clip * 0.5;

	const double VERY_FAR_AWAY = 1e9;

	//

	double scaled_distance_from_camera
		= half_depth_range
		+ half_depth_range * (1.0 - exp((half_depth_range - real_distance_from_camera) / VERY_FAR_AWAY))
		;

	*object_scale_ = scaled_distance_from_camera / real_distance_from_camera;
	*scaled_position_relative_to_camera_ = position_relative_to_camera.normalized() * scaled_distance_from_camera;
}

ERet MyGameRenderer::submit_Star(
								  const NwStar& star
								  , const MyGamePlayer& game_player
								  , const NwCameraView& camera_view
								  , GL::NwRenderContext &render_context
								  )
{
#if MX_DEVELOPER
	{
		NwStar & mut_star = (NwStar&) star;
		HOT_VAR(mut_star.pos_in_world_space);
		HOT_VAR(mut_star.radius_in_meters);
	}
#endif

	//
	double	star_scale;
	V3d		scaled_position_relative_to_camera;
	scaleIntoCameraDepthRange(
		star.pos_in_world_space
		, game_player
		, camera_view
		, &star_scale
		, &scaled_position_relative_to_camera
		);

	//
	const V3f star_position_relative_to_camera = V3f::fromXYZ(scaled_position_relative_to_camera);

	//
	const RenderPath& render_path = _render_system->getRenderPath();
	const U32 scene_pass_index = render_path.getPassDrawingOrder( ScenePasses::VolumetricParticles );
UNDONE;
	//
	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, MakeAssetID("billboard_star") ));


	//
	const Rendering::NwRenderSystem& render_sys = (Rendering::NwRenderSystem&) *_render_system;
	mxDO(shader_effect->setResource(
		mxHASH_STR("DepthTexture"),
		GL::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
		));


	//
	const NwShaderEffect::Variant shader_variant = shader_effect->getDefaultVariant();

	//
	GL::RenderCommandWriter	gfx_cmd_writer( render_context );
	//
	gfx_cmd_writer.setRenderState( shader_variant.render_state );

	//
	mxDO(shader_effect->pushAllCommandsInto( render_context._command_buffer ));

	//
	NwTransientBuffer	transient_VB;

	const U32	max_vertices = 1;

	mxDO(GL::allocateTransientVertexBuffer(
		transient_VB
		, max_vertices
		, sizeof(BillboardStarVertex)
		));
	{
		BillboardStarVertex & star_vertex = *(BillboardStarVertex*) transient_VB.data;

		star_vertex.sphere_in_view_space = V4f::set(
			star_position_relative_to_camera
			, (float) ( star.radius_in_meters * star_scale )
			);
		star_vertex.velocity_and_life = V4f::set(CV3f(0), 1);
	}

	//
	GL::Cmd_Draw	draw_indexed_cmd;
	draw_indexed_cmd.setup( shader_variant.program );
	draw_indexed_cmd.input_layout = BillboardStarVertex::metaClass().input_layout_handle;
	draw_indexed_cmd.primitive_topology = NwTopology::PointList;
	draw_indexed_cmd.use_32_bit_indices = false;

	draw_indexed_cmd.VB = transient_VB.buffer_handle;
	draw_indexed_cmd.IB.setNil();

	draw_indexed_cmd.base_vertex = transient_VB.base_index;
	draw_indexed_cmd.vertex_count = max_vertices;
	draw_indexed_cmd.start_index = 0;
	draw_indexed_cmd.index_count = 0;

	//
	gfx_cmd_writer.DrawIndexed( draw_indexed_cmd );

	//
	gfx_cmd_writer.submitCommandsWithSortKey(
		GL::buildSortKey( scene_pass_index, shader_variant.program.id )
		);

	return ALL_OK;
}

//void MyGameRenderer::_updateAtmosphereRenderingParameters(
//	const NwCameraView& camera_view
//	, const RrRuntimeSettings& renderer_settings
//	, const MyGameSettings& game_settings
//	)
//{
//	_atmosphere_rendering_parameters.atmosphere = game_settings.test_atmosphere;
//
//	_atmosphere_rendering_parameters.relative_eye_position = V3f::fromXYZ( camera_view.eye_pos_in_world_space );
//}

ERet MyGameRenderer::Render_VolumetricParticlesWithShader(
	const AssetID& particle_shader_id
	, const int num_particles
	, ParticleVertex *&allocated_buffer_
	)
{
	mxASSERT(num_particles > 0);
	mxASSERT2(num_particles < MAX_UINT16, "Only 16-bit indices are supported!");
	//
	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, particle_shader_id ));

	const NwShaderEffect::Variant shader_effect_variant = shader_effect->getDefaultVariant();

	//
	const Rendering::NwRenderSystem& render_sys = (Rendering::NwRenderSystem&) Rendering::NwRenderSystem::Get();

	//
	GL::NwRenderContext & render_context = GL::getMainRenderContext();
	//
	const U32 scene_pass_index = _render_system->getRenderPath()
		.getPassDrawingOrder( ScenePasses::VolumetricParticles );


	//
	ParticleVertex *	allocated_particles;

	//
	NwTransientBuffer	vertex_buffer;
	mxDO(GL::allocateTransientVertexBuffer(
		vertex_buffer
		, num_particles
		, sizeof(allocated_particles[0])
		));

	//
	allocated_particles = static_cast< ParticleVertex* >( vertex_buffer.data );
	allocated_buffer_ = allocated_particles;

	//
	GL::NwPushCommandsOnExitingScope	applyShaderCommands(
		render_context,
		GL::buildSortKey( scene_pass_index, 0 )
		);

	//
	GL::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	mxDO(shader_effect->setResource(
		mxHASH_STR("DepthTexture"),
		GL::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
		));

	shader_effect->pushShaderParameters( render_context._command_buffer );

	//
	GL::Cmd_Draw	batch;
	batch.reset();
	{
		batch.states = shader_effect_variant.render_state;
		batch.program = shader_effect_variant.program;

		batch.inputs = GL::Cmd_Draw::EncodeInputs(
			ParticleVertex::metaClass().input_layout_handle,
			NwTopology::PointList,
			false	// 32-bit indices?
			);

		batch.VB = vertex_buffer.buffer_handle;

		batch.base_vertex = vertex_buffer.base_index;
		batch.vertex_count = num_particles;
		batch.start_index = 0;
		batch.index_count = 0;
	}
	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
	cmd_writer.draw( batch );

	return ALL_OK;
}
