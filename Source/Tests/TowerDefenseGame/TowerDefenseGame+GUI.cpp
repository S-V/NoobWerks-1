#include <Base/Base.h>
#pragma once

#include <original_imgui/imgui.h>
#include <original_imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Core/CoreDebugUtil.h>

#include <Graphics/private/d3d_common.h>
#include <Graphics/DevTools/GraphicsDebugger.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/render_path.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain
#include <Renderer/private/RenderSystem.h>

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
#include <Voxels/Data/Chunking/vx5_GhostDataUpdate.h>


ImGui_DebugTextRenderer::ImGui_DebugTextRenderer()
{
	_text_labels.reserve(32);
}

void ImGui_DebugTextRenderer::drawText(
		const V3f& position,
		const RGBAf& color,
		const V3f* normal_for_culling,
		const float font_scale,	// 1 by default
		const char* fmt, ...
		)
{
	char	textBuffer[ 256 ];

	va_list argList;
	va_start( argList, fmt );
	_vsnprintf_s( textBuffer, sizeof(textBuffer), _TRUNCATE, fmt, argList );
	va_end( argList );

	textBuffer[ sizeof(textBuffer)-1 ] = '\0';

	TextLabel	new_vertex;
	{
		new_vertex.position = position;
		new_vertex.normal_for_culling = normal_for_culling ? *normal_for_culling : CV3f(0);
		new_vertex.font_scale = font_scale;
		new_vertex.color = color;
		Str::CopyS( new_vertex.text, textBuffer );
	}
	_text_labels.add( new_vertex );
}

bool ImGui_BeginFullScreenWindow()
{
	ImGui::SetNextWindowPos( ImVec2(0,0) );
	ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );

	return ImGui::Begin(
		"Dummy Fullscreen Window",
		nil,
		ImGuiWindowFlags_NoDecoration|
		ImGuiWindowFlags_NoTitleBar|
		ImGuiWindowFlags_NoResize|
		ImGuiWindowFlags_NoScrollbar|
		ImGuiWindowFlags_NoMove|
		ImGuiWindowFlags_NoCollapse|
		ImGuiWindowFlags_NoBackground|
		ImGuiWindowFlags_NoInputs
		);
}

mxFORCEINLINE
ImU32 ImGui_RGBA32_from_RGBAf( const RGBAf& color )
{
	return ImGui::ColorConvertFloat4ToU32(
		ImVec4( color.R, color.G, color.B, color.A )
		);
}

static void drawTextLabel(
						  const V3f& label_position_in_world_space
						  , const V3f& normal_for_culling
						  , const float label_font_scale
						  , const RGBAf& label_text_color
						  , const String& label_text
						  , const CameraMatrices& camera_matrices
						  , const ViewFrustum& view_frustum
						  , const InputState& input_state
						  , ImDrawList* draw_list
						  )
{
	//
	const V3f camera_to_label_dir = V3_Normalized( label_position_in_world_space - camera_matrices.origin );

	// "back face culling"

	const bool is_back_facing = V3_LengthSquared( normal_for_culling ) > 1e-3f
		? V3f::dot( normal_for_culling, camera_to_label_dir ) > 0
		: false
		;

	if( is_back_facing ) {
		return;
	}

	//
	if( view_frustum.PointInFrustum( label_position_in_world_space ) )
	{
		ImVec2 screen_space_label_position;
		const bool inside_viewport = ImGui_GetScreenSpacePosition(
			&screen_space_label_position,
			label_position_in_world_space,
			camera_matrices.view_projection_matrix,
			input_state.window.width,
			input_state.window.height
			);
		mxASSERT(inside_viewport);

		draw_list->AddText(
			nil	// const ImFont* font
			, draw_list->_Data->FontSize * label_font_scale // float font_size
			, screen_space_label_position
			, ImGui_RGBA32_from_RGBAf( label_text_color )
			, label_text.begin()
			, label_text.end()	// const char* text_end
			, 0	// float wrap_width
			, nil	// const ImVec4* cpu_fine_clip_rect
			);
	}
}

static void drawTextLabelsInDbgView(
									const VX::DebugView& dbgview
									, ImGui_DebugTextRenderer & imgui_text_renderer
									, const CameraMatrices& camera_matrices
									, const InputState& input_state
									, ImDrawList* draw_list
						   )
{
	ViewFrustum		view_frustum( camera_matrices.view_projection_matrix );

	//
	for( UINT i = 0; i < dbgview._text_labels.num(); i++ )
	{
		const VX::DebugView::TextLabel& label = dbgview._text_labels._data[ i ];

		drawTextLabel(
			label.position
			, label.normal_for_culling
			, label.font_scale
			, label.color
			, label.text
			, camera_matrices
			, view_frustum
			, input_state
			, draw_list
			);

	}//for each label

	//
	for( UINT i = 0; i < imgui_text_renderer._text_labels.num(); i++ )
	{
		const ImGui_DebugTextRenderer::TextLabel& label = imgui_text_renderer._text_labels._data[ i ];

		drawTextLabel(
			label.position
			, label.normal_for_culling
			, label.font_scale
			, label.color
			, label.text
			, camera_matrices
			, view_frustum
			, input_state
			, draw_list
			);

	}//for each label

	imgui_text_renderer._text_labels.empty();
}

void TowerDefenseGame::_drawImGui( const InputState& input_state, const CameraMatrices& camera_matrices )
{
	rmt_ScopedCPUSample(DrawGUI, 0);

	const bool draw_ImGui_debug_HUD = m_settings.showGUI;
	const bool draw_ImGui_gizmo = _editor_manipulator.isGizmoEnabled();
	const bool draw_TextLabels =
		!m_dbgView._text_labels.isEmpty()
		||
		!_imgui_text_renderer._text_labels.isEmpty()
		;

	const U32 passIndex = _render_system->getRenderPath().getPassDrawingOrder(mxHASH_STR("GUI"));
	DemoApp::begin_ImGUI( passIndex );

	//
	if( draw_ImGui_gizmo || draw_TextLabels )
	{
		if( draw_ImGui_gizmo )
		{
			_editor_manipulator.beginDrawing();

			_editor_manipulator.setGizmoOperation( m_settings.voxel_editor_tools.gizmo_operation );
		}

		//
		if( ImGui_BeginFullScreenWindow() )
		{
			if( draw_ImGui_gizmo )
			{
				_editor_manipulator.drawGizmoAndManipulate( camera_matrices, ImGui::GetIO() );
				m_settings.voxel_editor_tools.last_saved_gizmo_transform_of_mesh = _editor_manipulator.getTransformMatrix();
			}

			if( draw_TextLabels )
			{
				ImDrawList* draw_list = ImGui::GetWindowDrawList();

				drawTextLabelsInDbgView(
					m_dbgView
					, _imgui_text_renderer
					, camera_matrices
					, input_state
					, draw_list
					);
			}
		}
		ImGui::End();
	}

	//
	if( draw_ImGui_debug_HUD )
	{
		_ImGui_draw_VoxelToolsMenu( input_state, camera_matrices );

		_drawImGui_DebugHUD( input_state, camera_matrices );
	}

	//
	DemoApp::finish_ImGUI();
}

void TowerDefenseGame::_drawImGui_DebugHUD( const InputState& input_state, const CameraMatrices& camera_matrices )
{
	//
	ImGui::Checkbox("Show Scene Settings", &m_settings.dev_showSceneSettings);
	ImGui::Checkbox("Show Graphics Debugger", &m_settings.dev_showGraphicsDebugger);
	ImGui::Checkbox("Show Renderer Settings", &m_settings.dev_showRendererSettings);

	ImGui::Checkbox("Show Voxel Engine Settings", &m_settings.dev_showVoxelEngineSettings);
	ImGui::Checkbox("Show Voxel Engine Debug Menu", &m_settings.dev_showVoxelEngineDebugMenu);

	ImGui::Checkbox("Show Tweakables", &m_settings.dev_showTweakables);
	ImGui::Checkbox("Show Perf Stats", &m_settings.dev_showPerfStats);
	ImGui::Checkbox("Show Memory Usage", &m_settings.dev_showMemoryUsage);


	ImGui::SliderFloat("MdlOffsetX", &Dbg::g_mdl_offset.x, -999, 999);
	ImGui::SliderFloat("MdlOffsetY", &Dbg::g_mdl_offset.y, -999, 999);
	ImGui::SliderFloat("MdlOffsetZ", &Dbg::g_mdl_offset.z, -999, 999);







#if 0
	//ImGui::ShowTestWindow();
	{
		String512 buf;
		{
			//StringStream temp(buf);

			//bool bInside = false;
			//bool bHit = false;
			//				float thit;
			//				V3f hitNormal;

			//temp << "Camera pos: " << camera.m_eyePosition << ";"
			//	<< ", hit: " << bHit
			//	<< ", " << (bInside ? "inside" : "outside")
			//	<< "/n";



			ImGui::Checkbox("Show Wireframe", &Rendering::g_settings.drawModelWireframes);
			//ImGui::Checkbox("Flat Shading?", &m_settings.flatShading);
			//ImGui::Checkbox("Draw octree?", &m_settings.debugDrawOctree);
			ImGui::Checkbox("Draw 3rd person frustum?", &m_settings.draw_fake_camera);

			RrRuntimeSettings & rendererSettings = Rendering::g_settings;

			ImGui::Checkbox("Draw models?", &rendererSettings.drawModels);
			ImGui::Checkbox("Draw model bounds?", &rendererSettings.debug_DrawModelBounds);

			//ImGui::Checkbox("Draw shadows?", &m_settings.shadows);
			//if( m_settings.shadows ) {
			//	ImGui::Checkbox("Soft shadows?", &rendererSettings.enable_soft_shadows);
			//	ImGui::Checkbox("Show cascades?", &ShadowRenderer::r_dir_light_visualize_cascades.value);
			//}

			ImGui::Checkbox("Draw sky?", &m_settings.drawSky);

			ImGui::Checkbox("Draw crosshair?", &m_settings.drawCrosshair );
			ImGui::Checkbox("Draw gizmo?", &m_settings.drawGizmo);
			//				ImGui::Checkbox("Show BIH?", &m_settings.showBIH);
			//ImGui::InputFloat("Sub Sphere Radius", &m_settings.csg_subtract_sphere_radius);

			ImGui_DrawPropertyGrid(
				&m_settings.voxels_debug_viz,
				mxCLASS_OF(m_settings.voxels_debug_viz),
				"Debug Mesher"
				);
		}
		ImGui::Text(buf.c_str());

		{
			RrDirectionalLight* globalLight = m_runtime_clump.FindSingleInstance< RrDirectionalLight >();
			if( globalLight ) {
				ImGui::SliderFloat("SunDirX", &m_settings.sun_direction.x, -1, 1);
				ImGui::SliderFloat("SunDirY", &m_settings.sun_direction.y, -1, 1);
				ImGui::SliderFloat("SunDirZ", &m_settings.sun_direction.z, -1, 1);
				m_settings.sun_direction = V3_Normalized( m_settings.sun_direction );
				globalLight->direction = m_settings.sun_direction;

				//float luminance = RGBf(globalLight->color.x, globalLight->color.y, globalLight->color.z).Brightness();
				//ImGui::SliderFloat("SunLightBrightness", &globalLight->brightness, 0, 100);
				//globalLight->color *= luminance;
			}
		}

		ImGui::SliderFloat("ROILightBrightness", &m_fakeCameraLight->brightness, 0, 2);


#if VOXEL_TERRAIN5_WITH_PHYSICS
#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
		ImGui::Checkbox("Player.enable_collisions?", &m_settings.physics.enable_player_collisions);
		_player_controller.setCollisionsEnabled( m_settings.physics.enable_player_collisions );

		//
		//bool camera_gravity_enabled = m_settings.physics.gravity;
		ImGui::Checkbox("Player.enable_gravity?", &m_settings.physics.enable_player_gravity);
		_player_controller.setGravityEnabled( m_settings.physics.enable_player_gravity );
#endif
		//if( !camera_gravity_enabled != m_settings.physics.gravity )
		//{
		//	m_settings.physics.gravity = camera_gravity_enabled;
		//	_player_controller.setGravityEnabled( camera_gravity_enabled );
		//}
#endif


		ImGui::Checkbox("Fake camera pos?", &m_settings.use_fake_camera);
		if( m_settings.use_fake_camera )
		{
			ImGui::Text("Fake camera pos: %.3f, %.3f, %.3f",
				m_settings.fake_camera.origin.x, m_settings.fake_camera.origin.y, m_settings.fake_camera.origin.z);
			ImGui::InputFloat("Fake camera speed", &m_settings.fake_camera_speed);
		}

		ImGui::InputFloat("Movement speed", &camera.m_movementSpeed, 0.0f, 10.0f);
		ImGui::InputFloat("Acceleration", &camera.m_initialAccel, 0.0f, 10.0f);

		ImGui::InputFloat("AO_step_size(mul)", &m_settings.AO_step_size, 0.1f, 10.0f);
		ImGui::InputInt("AO_steps", &m_settings.AO_steps, 1, 16);

		ImGui::Checkbox("calc_vertex_pos_using_QEF", &m_settings.calc_vertex_pos_using_QEF);

		ImGui::InputInt("vertexProjSteps", &m_settings.vertexProjSteps, 1, 16);
		ImGui::InputFloat("vertexProjLambda", &m_settings.vertexProjLambda, 0.0f, 10.0f);

		ImGui::Checkbox("dbg_viz_SDF", &m_settings.dbg_viz_SDF);

		//ImGui::InputFloat("Strafing speed", &camera.m_strafingSpeed, 1.0f, 10.0f);

		//{
		//	VX5::Chunk* chunk = _voxel_world.m_chunkMap.FindRef(Int3_Set(0,0,0));
		//	if(ImGui::Begin("Octree Stats"))
		//	{
		//		ImGui::LabelText("Quads:", "%d", chunk->dbg->quads.num());
		//		ImGui::End();
		//	}
		//}

		if( ImGui::Begin("Stats") )
		{
			GL::BackEndCounters	graphics_perf_counters;
			GL::getLastFrameCounters( &graphics_perf_counters );
			//ImGui::Text("Triangles: %u", _voxel_world.m_stats.num_triangles_created);
			//ImGui::Text("Vertices: %u", _voxel_world.m_stats.num_vertices_created);
			ImGui::Text("Batches: %u", graphics_perf_counters.c_draw_calls);
			ImGui::Text("Triangles: %u", graphics_perf_counters.c_triangles);
			//				ImGui::Text("Vertices: %u", _voxel_world.m_stats.num_vertices_created);
			//ImGui::Text("Chunk Gen [msec]: %u", m_contouringStats.time_msec_chunkGeneration);
			//				ImGui::Text("Chunks queued: %u", _voxel_world.m_loadedRegion.unprocessed.num());
			//				ImGui::Text("Chunks dirty: %u", _voxel_world.m_chunksToReMesh.num());
			ImGui::Text("Octree nodes: %u", _voxel_world._chunk_node_pool.NumValidItems());
			//ImGui::Text("Octree nodes: %u", _voxel_world.m_blocks.NumValidItems()*8);
			//				ImGui::Text("VBs [KiB]: %.3f", _voxel_world.m_stats.memory_used_for_vertex_buffers/1024.0f);
			//				ImGui::Text("IBs [KiB]: %.3f", _voxel_world.m_stats.memory_used_for_index_buffers/1024.0f);
			ImGui::Text("FPS: %f", averageFramesPerSecond);
			//				ImGui::Text("GPU: %3.1lf ms", GL::g_BackEndStats.gpu.GPU_Time_Milliseconds);
			ImGui::Text("Slow tasks enqueued: %u", AtomicLoad(SlowTasks::NumPendingTasks()));
			ImGui::Text("Jobs finished last time: %u", Engine::g_job_sytem_stats.nNumFinished);

#if vx5_CFG_ENABLE_STATS
			const U32 numChunks = AtomicLoad( VX5::Testing::g_total_num_chunks );
			if( numChunks )
			{
				ImGui::Text("Average compression ratio: %.3f",
					(float)AtomicLoad( VX5::Testing::g_total_uncompressed_chunk_size ) / (float)AtomicLoad( VX5::Testing::g_total_compressed_chunk_size ));
			}
#endif
			ImGui::End();
		}

		if( ImGui::Begin("Renderer") )
		{
			ImGui_DrawPropertyGrid(
				&m_settings.renderer,
				mxCLASS_OF(m_settings.renderer),
				"Settings"
				);

			ImGui::Text("Objects visible: %u / %u (%u culled)", g_FrontEndStats.objects_visible, g_FrontEndStats.objects_total, g_FrontEndStats.objects_culled);
			ImGui::Text("Shadow casters rendered: %u", g_FrontEndStats.shadow_casters_rendered);
			//ImGui::Text("Meshes: %u", NwMesh::s_totalCreated);
			IF_DEBUG ImGui::Checkbox("Debug Break?", &g_DebugBreakEnabled);

#if 0
			m_visibleObjects.Reset();
			ViewFrustum	viewFrustum;
			viewFrustum.ExtractFrustumPlanes( view_projection_matrix );
			U64 startTimeMSec = mxGetTimeInMilliseconds();
			m_cullingSystem->FindVisibleObjects(viewFrustum, m_visibleObjects);
			U64 elapsedTimeMSec = mxGetTimeInMilliseconds() - startTimeMSec;
			ImGui::Text("Test Culling: %u visible (%u msec)", m_visibleObjects.count[RE_Model], elapsedTimeMSec);
#endif

			ImGui::End();
		}

		if( ImGui::Begin("Voxel World") )
		{
			//ImGui::Text("Chunks being generated: %u", AtomicLoad(_voxel_world.m_stats.chunks_being_generated));
			//ImGui::Text("Chunks being contoured: %u", AtomicLoad(_voxel_world.m_stats.chunks_being_contoured));

			ImGui::Text("Max Contour Requests: %u", _voxel_world.m_stats.max_chunk_contour_requests_per_frame);

			//ImGui::Checkbox("G_UPDATE_GHOST_CELLS?", &VX5::G_UPDATE_GHOST_CELLS);

			//ImGui::Checkbox("Simplify volumes?", &_voxel_world._settings.simplify_volume);
			//ImGui::InputFloat("Error threshold", &_voxel_world._settings.qef_threshold, 0.0f, 10.0f);
			//ImGui::Checkbox("Threaded generation?", &_voxel_world._settings.multithreaded_chunk_generation);
			//ImGui::Checkbox("Threaded contouring?", &_voxel_world._settings.multithreaded_chunk_contouring);
			//ImGui::Checkbox("Enable LoDs?", &_voxel_world._settings.debug_enable_LoDs);
			//ImGui::Checkbox("Freeze LoDs?", &_voxel_world._settings.debug_freeze_LoD);
			//ImGui::Checkbox("Freeze Octree?", &_voxel_world._settings.debug_freeze_world);
			ImGui::Checkbox("Disable Stitching?", &_voxel_world._settings.debug_disable_stitching);
			ImGui::Checkbox("Show LOD structure?", &_voxel_world._settings.debug_draw_LOD_structure);
			ImGui::Checkbox("Multithreaded editing?", &_voxel_world._settings.multithreaded_chunk_operations);

			ImGui::Text("World cells: %u", _voxel_world._chunk_node_pool.NumValidItems());
			ImGui::Text("Split jobs: %u, Merge jobs: %u, Remesh jobs: %u",
				_voxel_world._scene_tasks.num_running_split_jobs, _voxel_world._scene_tasks.num_running_merge_jobs, _voxel_world._scene_tasks.numRunningRemeshJobs.load());
			for( int iLoD = 0; iLoD < _voxel_world._octree._num_LoDs; iLoD++ ) {
				const VX::OctreeLoD& lod = _voxel_world._octree._LoDs[ iLoD ];
				ImGui::Text("LOD[%d]: %d cells", iLoD, lod.count );
			}
			//UInt4 P = _voxel_world.m_nodeContainingCamera.toCoordsAndLoD();
			//ImGui::Text("Viewer Node: %u, %u, %u, LoD=%u", P.x, P.y, P.z, P.w);
			mxUNDONE
#if 0
				VX5::Chunk* nodeContainingEye = _world.FindNodeContainingPoint( camera.m_eyePosition );
			if( nodeContainingEye ) {
				UInt4 P = nodeContainingEye->id.toCoordsAndLoD();
				ImGui::Text("Camera's Node: %u, %u, %u, LoD=%u", P.x, P.y, P.z, P.w);
				RrMeshInstance* model = (RrMeshInstance*) nodeContainingEye->mesh;
				if( model ) {
					//ImGui::InputFloat("Morph override", &model->m_morphOverride);
					ImGui::SliderFloat("Morph override", &model->m_morphOverride, -1.0f, 1.0f);
				}
			}
#endif

			if( ImGui::Button("Regenerate world") || (isInGameState && input_state.keyboard.held[KEY_R]) ) {
				_voxel_world.dbg_waitForAllTasks();
				if(mxSUCCEDED( this->rebuildBlobTreeFromLuaScript() )) {
					_voxel_world.dbg_regenerateWorld();
				}
			}
			if( ImGui::Button("Re-Mesh") ) {
				//					_voxel_world.DBG_Remesh_Existing_Chunks();
			}

			ImGui::End();
		}//Voxel World



		if( ImGui::Begin("Voxel Tool") )
		{
			ImGui_drawEnumComboBoxT( &m_settings.voxel_tool.current_tool, "Tool" );

			switch( m_settings.voxel_tool.current_tool )
			{
			case VX::VoxelToolType::CSG_Add_Shape:
				ImGui_DrawPropertyGridT( &m_settings.voxel_tool.csg_add
					, "CSG Add"
					, false /*create_windows*/ );
				break;

			case VX::VoxelToolType::CSG_Subtract_Shape:
				ImGui_DrawPropertyGridT( &m_settings.voxel_tool.csg_subtract
					, "CSG Subtract"
					, false /*create_windows*/ );
				break;

			case VX::VoxelToolType::CSG_Add_Model:
				break;

			case VX::VoxelToolType::CSG_Subtract_Model:
				break;

			default:
				LogStream(LL_Warn),"Unhandled tool: ",m_settings.voxel_tool.current_tool;
			}

			ImGui::End();
		}




		if( ImGui::Begin("GI") )
		{
			if( ImGui::Button("Bake GI") ) {
				this->_gi_bakeAlbedoDensityVolumeTexture( camera_matrices );
			}

			ImGui_DrawPropertyGridT( &m_settings.renderer.gi
				, "GI"
				, false /*create_windows*/ );


			ImGui::InputFloat("position_bias",
				&Rendering::VoxelConeTracing::Get()._current_settings.position_bias,
				0, 100.0f
				);

			ImGui::End();
		}









	}
#endif


	if( m_settings.dev_showSceneSettings )
	{
		if( ImGui::Begin("Scene Settings") )
		{
			bool sunLightChanged = false;

			if( ImGui::Button("Copy Camera Look Dir to Sun Direction" ) )
			{
				m_settings.sun_light.direction = camera.m_lookDirection;
				sunLightChanged = true;
			}
			if( ImGui::Button("Reset Sun Direction" ) )
			{
				m_settings.sun_light.direction = -V3_UP;
				sunLightChanged = true;
			}

			{
				V3f newSunLightDir = m_settings.sun_light.direction;

				ImGui::SliderFloat("SunDirX", &newSunLightDir.x, -1, 1);
				ImGui::SliderFloat("SunDirY", &newSunLightDir.y, -1, 1);
				ImGui::SliderFloat("SunDirZ", &newSunLightDir.z, -1, 1);

				newSunLightDir = V3_Normalized( newSunLightDir );
				if( newSunLightDir != m_settings.sun_light.direction )
				{
					m_settings.sun_light.direction = newSunLightDir;
					sunLightChanged = true;
				}
			}

			//float luminance = RGBf(globalLight->color.x, globalLight->color.y, globalLight->color.z).Brightness();
			//ImGui::SliderFloat("SunLightBrightness", &globalLight->brightness, 0, 100);
			//globalLight->color *= luminance;

			if( sunLightChanged )
			{
				_render_world->setGlobalLight( m_settings.sun_light );
				_render_world->modifySettings( m_settings.renderer );
			}

			ImGui_DrawPropertyGrid(
				&m_settings,
				mxCLASS_OF(m_settings),
				"App Settings"
				);
		}
		ImGui::End();
	}

	if( m_settings.dev_showRendererSettings )
	{
		if( ImGui::Begin("Renderer") )
		{
			ImGui::SliderFloat(
				"Animation Speed"
				, &m_settings.renderer.animation.animation_speed_multiplier
				, RrAnimationSettings::minValue()
				, RrAnimationSettings::maxValue()
				);

			//
			if( ImGui::Button("Regenerate Light Probes" ) )//zzz
			{
				((Rendering::NwRenderSystem&)TbRenderSystemI::Get())._test_need_to_relight_light_probes = true;
			}

			//
			const U64 rendererSettingsHash0 = m_settings.renderer.computeHash();

			ImGui_DrawPropertyGrid(
				&m_settings.renderer,
				mxCLASS_OF(m_settings.renderer),
				"Settings"
				);

			const U64 rendererSettingsHash1 = m_settings.renderer.computeHash();
			if( rendererSettingsHash0 != rendererSettingsHash1 )
			{
				DEVOUT("Renderer Settings changed");
				_render_world->modifySettings( m_settings.renderer );
			}

			ImGui::Text("Objects visible: %u / %u (%u culled)", g_FrontEndStats.objects_visible, g_FrontEndStats.objects_total, g_FrontEndStats.objects_culled);
			ImGui::Text("Shadow casters rendered: %u", g_FrontEndStats.shadow_casters_rendered);
			//ImGui::Text("Meshes: %u", NwMesh::s_totalCreated);
			IF_DEBUG ImGui::Checkbox("Debug Break?", &g_DebugBreakEnabled);

#if 0
			m_visibleObjects.Reset();
			ViewFrustum	viewFrustum;
			viewFrustum.ExtractFrustumPlanes( view_projection_matrix );
			U64 startTimeMSec = mxGetTimeInMilliseconds();
			m_cullingSystem->FindVisibleObjects(viewFrustum, m_visibleObjects);
			U64 elapsedTimeMSec = mxGetTimeInMilliseconds() - startTimeMSec;
			ImGui::Text("Test Culling: %u visible (%u msec)", m_visibleObjects.count[RE_Model], elapsedTimeMSec);
#endif
		}
		ImGui::End();

		_render_system->applySettings( m_settings.renderer );
	}


	//
	if( m_settings.dev_showVoxelEngineSettings )
	{
		if( ImGui::Begin("Voxel Engine Settings") )
		{
			//
			if( ImGui::Button("Save and Apply") )
			{
				this->saveSettingsToFiles();
				m_settings.fast_noise.applyTo( _fast_noise );
			}
			//
			const bool isInGameState = ( _game_state_manager.CurrentState() == GameStates::main );
			if( ImGui::Button("Regenerate world") || (isInGameState && input_state.keyboard.held[KEY_R]) ) {
				_voxel_world.dbg_waitForAllTasks();
				if(mxSUCCEDED( this->rebuildBlobTreeFromLuaScript() )) {
					_voxel_world.dbg_regenerateWorld();
				}
			}

			//
			ImGui_DrawPropertyGrid(
				&_voxel_world._settings,
				mxCLASS_OF(_voxel_world._settings),
				"Voxel Engine Settings"
				);

			//
			if( ImGui::Button("Reset Tool Settings" ) )
			{
				m_settings.dev_voxel_tools.resetAllToDefaultSettings(
					VX5::getChunkSizeAtLoD(0)
					);
			}

			ImGui_DrawPropertyGrid(
				&m_settings.dev_voxel_tools,
				mxCLASS_OF(m_settings.dev_voxel_tools),
				"Voxel Tools Settings"
				);

			//
			if( ImGui::BeginChild("Chunk Debugging") )
			{
				if( ImGui::Button("Remesh current chunk" ) )
				{
					_voxel_world.dbg_RemeshChunk( getCameraPosition() );
				}
			}

			ImGui::EndChild();

			//
		}
		ImGui::End();
	}

	//
	if( m_settings.dev_showVoxelEngineDebugMenu )
	{
		_ImGui_drawVoxelEngineDebugSettings(
			m_settings.voxel_engine_debug_state
			, input_state, camera_matrices
			);
	}


	//
	if( m_settings.dev_showTweakables )
	{
		ImGui_DrawTweakablesWindow();
	}

#if MX_ENABLE_MEMORY_TRACKING && MX_DEBUG_MEMORY
	if( m_settings.dev_showMemoryUsage )
	{
		ImGui_DrawMemoryAllocatorsTree( (ProxyAllocator*) & MemoryHeaps::global() );
	}
#endif // MX_ENABLE_MEMORY_TRACKING







	if( m_settings.dev_showGraphicsDebugger )
	{
		TSpan< Testbed::Graphics::DebuggerTool::Surface > surfaces =
			Testbed::Graphics::DebuggerTool::getSurfaces()
			;

		if( ImGui::Begin("Graphics Debugger") )
		{
			for( UINT i = 0; i < surfaces.num(); i++ )
			{
				Testbed::Graphics::DebuggerTool::Surface & surface = surfaces[i];

				ImGui::Checkbox( surface.name.c_str(), &surface.display );

				//
			}
		}
		ImGui::End();

		//

		for( UINT i = 0; i < surfaces.num(); i++ )
		{
			Testbed::Graphics::DebuggerTool::Surface & surface = surfaces[i];

			if( surface.display )
			{
				ImGui::SetNextWindowSize(ImVec2(256,256));

				if( ImGui::Begin( surface.name.c_str(), &surface.display, ImGuiWindowFlags_NoScrollbar ) )
				{
					ImGuiWindow* Window = ImGui::GetCurrentWindow();

					ImGui::Image( &surface.handle, Window->Size );
				}
				ImGui::End();
			}
		}
	}

	if( m_settings.dev_showPerfStats )
	{
		if( ImGui::Begin("Stats") )
		{
			//
			//ImGui::Text("Eye position: %f, %f, %f",
			//	camera_matrices.origin.x, camera_matrices.origin.y, camera_matrices.origin.z
			//	);
			ImGui::Text("Eye position X: %f", camera_matrices.origin.x);
			ImGui::Text("Eye position Y: %f", camera_matrices.origin.y);
			ImGui::Text("Eye position Z: %f", camera_matrices.origin.z);

			//
			const CubeMLd& world_cube = _voxel_world._octree._world_bounds;
			ImGui::Text("World size: %f; origin: (%f, %f, %f)",
				world_cube.side_length, world_cube.min_corner.x, world_cube.min_corner.y, world_cube.min_corner.z
				);

			//
			GL::BackEndCounters	graphics_perf_counters;
			GL::getLastFrameCounters( &graphics_perf_counters );
			//ImGui::Text("Triangles: %u", _voxel_world.m_stats.num_triangles_created);
			//ImGui::Text("Vertices: %u", _voxel_world.m_stats.num_vertices_created);
			ImGui::Text("Batches: %u", graphics_perf_counters.c_draw_calls);
			ImGui::Text("Triangles: %u", graphics_perf_counters.c_triangles);
			//				ImGui::Text("Vertices: %u", _voxel_world.m_stats.num_vertices_created);
			//ImGui::Text("Chunk Gen [msec]: %u", m_contouringStats.time_msec_chunkGeneration);
			//				ImGui::Text("Chunks queued: %u", _voxel_world.m_loadedRegion.unprocessed.num());
			//				ImGui::Text("Chunks dirty: %u", _voxel_world.m_chunksToReMesh.num());
			//ImGui::Text("Octree nodes: %u", _voxel_world.m_nodes.NumValidItems());
			//ImGui::Text("Octree nodes: %u", _voxel_world.m_blocks.NumValidItems()*8);
			//				ImGui::Text("VBs [KiB]: %.3f", _voxel_world.m_stats.memory_used_for_vertex_buffers/1024.0f);
			//				ImGui::Text("IBs [KiB]: %.3f", _voxel_world.m_stats.memory_used_for_index_buffers/1024.0f);

			{
				//	const double frameEndTimeSeconds = mxGetTimeInSeconds();
				//	const float frameAverageTimeSeconds = m_FPS_counter.add( frameEndTimeSeconds - frameStartTimeSeconds );

				//const float frameAverageTimeSeconds = m_FPS_counter.add( input_state.deltaSeconds );
				const float lastFrameTimeSeconds = microsecondsToSeconds( Testbed::realTimeElapsed() );
				const float frameAverageTimeSeconds = m_FPS_counter.add( lastFrameTimeSeconds );
				const float averageFramesPerSecond = 1.0f / (frameAverageTimeSeconds + 1e-5f);
				ImGui::Text("FPS: %f", averageFramesPerSecond);
			}

			//				ImGui::Text("GPU: %3.1lf ms", GL::g_BackEndStats.gpu.GPU_Time_Milliseconds);
			ImGui::Text("Slow tasks enqueued: %u", AtomicLoad(SlowTasks::NumPendingTasks()));
			ImGui::Text("Jobs finished last time: %u", Engine::g_job_sytem_stats.nNumFinished);



#if vx5_CFG_ENABLE_STATS
			const U32 numChunks = AtomicLoad( VX5::Testing::g_total_num_chunks );
			if( numChunks )
			{
				ImGui::Text("Average compression ratio: %.3f",
					(float)AtomicLoad( VX5::Testing::g_total_uncompressed_chunk_size ) / (float)AtomicLoad( VX5::Testing::g_total_compressed_chunk_size ));
			}
#endif
		}
		ImGui::End();
	}




	if( m_settings.voxels_debug_viz.drawTextLabels && _last_camera_matrices.has_value() )
	{
		const CameraMatrices& last_camera_matrices = *_last_camera_matrices;

		//const M44f viewMatrix = camera.BuildViewMatrix();
		//const M44f projectionMatrix = M44_Perspective(sceneView.halfFoVY*2, sceneView.aspect_ratio,
		//	sceneView.near_clip, sceneView.far_clip);
		//const M44f view_projection_matrix = M44_Multiply(viewMatrix, projectionMatrix);

		//
		ViewFrustum		view_frustum( last_camera_matrices.view_projection_matrix );

#if 0
		for( int i = 0; i < _dbg_pts.labelled_points.num(); i++ )
		{
			const DbgLabelledPoint& point = _dbg_pts.labelled_points[i];

			if( view_frustum.PointInFrustum( point.position_in_world_space ) )
			{
				ImVec2 windowPos;
				const bool inside_viewport = ImGui_GetWindowPosition(
					windowPos,
					point.position_in_world_space,
					last_camera_matrices.view_projection_matrix,
					input_state.window.width,
					input_state.window.height
					);
				mxASSERT(inside_viewport);

				const ImFont* font = ImGui::GetFont();
				const ImVec2 text_size = font->CalcTextSizeA(
					font->FontSize, FLT_MAX, 0,
					point.text_label.begin(),
					point.text_label.end()
					);

				//
				char	buf[32];
				sprintf_s( buf, "POINT_%d", i );
				if(ImGui::Begin(
					buf,
					nil,
					text_size,
					1.0f,
					ImGuiWindowFlags_NoDecoration|
					ImGuiWindowFlags_NoMove|
					ImGuiWindowFlags_NoCollapse|
					ImGuiWindowFlags_NoBackground|
					ImGuiWindowFlags_NoFocusOnAppearing|
					ImGuiWindowFlags_NoInputs
					))
				{
					ImGui::Text(point.text_label.c_str());
					ImGui::SetWindowPos(windowPos);
				}
				ImGui::End();
			}
		}//for
#endif
	}

	//ImGui::ShowMetricsWindow();

	m_settings.renderer.fixBadSettings();
}

/*
----------------------------------------------------------

	TowerDefenseGame::_ImGui_draw_VoxelToolsMenu

----------------------------------------------------------
*/
void TowerDefenseGame::_ImGui_draw_VoxelToolsMenu(
	const InputState& input_state, const CameraMatrices& camera_matrices
	)
{
	if( ImGui::Begin("Voxel Tools Menu") )
	{
		ImGui::Checkbox( "Mesh Tool", &m_settings.voxel_editor_tools.place_mesh_tool_is_enabled );
		_editor_manipulator.setGizmoEnabled(
			m_settings.voxel_editor_tools.place_mesh_tool_is_enabled
			);

		//
		if( ImGui::TreeNodeEx( "Mesh Tool Settings", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui_drawEnumComboBoxT(
				&m_settings.voxel_editor_tools.gizmo_operation
				, "Gizmo Operation"
				);

			//
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(
				_editor_manipulator.getTransformMatrix().a
				, matrixTranslation, matrixRotation, matrixScale
				);

			//float v_speed = 1.0f;
			//bool translation_changed = ImGui::DragFloat3("Trn", matrixTranslation, v_speed);
			//bool rotation_changed = ImGui::DragFloat3("Rot", matrixRotation, v_speed, -180.0f, +180.0f );
			//bool scale_changed = ImGui::DragFloat3("Scl", matrixScale, v_speed, 1e-3f, 1e3f);

			//if( translation_changed || rotation_changed || scale_changed )
			//{
			//	ImGuizmo::RecomposeMatrixFromComponents(
			//		matrixTranslation, matrixRotation, matrixScale
			//		, _editor_manipulator.getTransformMatrix().a
			//		);
			//}

			bool translation_changed = ImGui::InputFloat3( "Trn", matrixTranslation );
			bool rotation_changed = ImGui::InputFloat3( "Rot", matrixRotation );
			bool scale_changed = ImGui::InputFloat3( "Scl", matrixScale );

			if( translation_changed || rotation_changed || scale_changed )
			{
				M44f	new_transform;
				ImGuizmo::RecomposeMatrixFromComponents(
					matrixTranslation, matrixRotation, matrixScale
					, new_transform.a
					);

				_editor_manipulator.setLocalToWorldTransform( new_transform );
				m_settings.voxel_editor_tools.last_saved_gizmo_transform_of_mesh = new_transform ;
			}

			//
			ImGui::TreePop();
		}

		//
		if( ImGui::Button("ADD") )
		{
			_voxel_world.addMesh(
				_stamped_mesh
				, _editor_manipulator.getTransformMatrix()
				, VoxMat_Snow
				);
		}

		//
		if( ImGui::Button("SUB") )
		{
			UNDONE;
			//_voxel_world.addMesh(
			//	_stamped_mesh
			//	, _editor_manipulator.getTransformMatrix()
			//	, VoxMat_Snow
			//	);
		}
	}
	ImGui::End();
}

/*
----------------------------------------------------------

	TowerDefenseGame::_ImGui_drawVoxelEngineDebugSettings

----------------------------------------------------------
*/
void TowerDefenseGame::_ImGui_drawVoxelEngineDebugSettings(
	VoxelEngineDebugState & voxel_engine_debug_state
	, const InputState& input_state, const CameraMatrices& camera_matrices
	)
{
	if( ImGui::Begin("Voxel Engine Debug Menu") )
	{
		//
		VX::ChunkID containing_chunk_id = VX::ChunkID(0,0,0,0);

		const VX::NodeBase* enclosing_node = _voxel_world._octree.findEnclosingChunk(
			V3d::fromXYZ( camera_matrices.origin )
			, containing_chunk_id
			);

		//
		if( ImGui::TreeNodeEx( "Current Chunk's Coords", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			const char* current_chunk_coords_label_name = "<-- Current chunk's coords: ";

			if( enclosing_node ) {
				const VX5::ChunkNode* chunk = (VX5::ChunkNode*) enclosing_node;
				const bool chunk_is_busy = chunk->state != VX5::Chunk_Ready;

				String128	tmp;
				StringStream	stream(tmp);
				stream << containing_chunk_id;
				ImGui::Text(
					"%s, busy?: %d",
					tmp.c_str(), (int)chunk_is_busy
					);
			} else {
				ImGui::Text("OUT OF WORLD BOUNDS");
			}

			//
			if( ImGui::Button("Clear Debug Lines") ) {
				m_dbgView.clearAll();
			}

			ImGui::TreePop();
		}

		//
		if( enclosing_node )
		{
			if( ImGui::TreeNodeEx( "Chunk Debugging", ImGuiTreeNodeFlags_DefaultOpen ) )
			{
				if( ImGui::Button("Remesh Current Chunk" ) )
				{
					VX5::dbg_remesh_chunk(
						c_cast( VX5::ChunkNode* ) enclosing_node
						, containing_chunk_id
						, _voxel_world
						);
				}

				//
				if( ImGui::Button("Show Chunk's Texture Info" ) )
				{
					VX5::dbg_showChunkTextureInfo(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Edges" ) )
				{
					VX5::dbg_showChunkEdges(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Cells" ) )
				{
					VX5::dbg_showChunkCells(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Voxels" ) )
				{
					VX5::dbg_showChunkVoxels(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Cells Overlapping Neighbors' Meshes" ) )
				{
					VX5::dbg_show_Cells_overlapping_Meshes_of_Neighboring_Chunks(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Octree" ) )
				{
					VX5::dbg_show_Chunk_Octree(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Print Chunk's Octree" ) )
				{
					VX5::dbg_print_Chunk_Octree(
						containing_chunk_id
						, _voxel_world
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Check Ghost Cells Synced with Neighbors" ) )
				{
					VX5::dbg_check_ghost_cells_are_synced_with_neighbors(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						);
				}

				ImGui::TreePop();
			}

			//
			if( ImGui::TreeNodeEx( "Ghost Cells Visualization", ImGuiTreeNodeFlags_DefaultOpen ) )
			{
				VoxelEngineDebugState::GhostCellsVisualization & ghost_cells_visualization = voxel_engine_debug_state.ghost_cells;

				if( ImGui::Button("Show Ghost Cells of Current Chunk" ) )
				{
					VX5::dbg_show_Ghost_Cells_of_Chunk_with_ID(
						containing_chunk_id
						, _voxel_world
						, m_dbgView
						, MemoryHeaps::temporary()
						, ghost_cells_visualization.show_cached_coarse_cells
						, ghost_cells_visualization.show_ghost_cells
						, ghost_cells_visualization.show_cells_overlapping_maximal_neighbors_mesh
						);
				}
				ImGui::TreePop();
			}

			//
			if( ImGui::Button("Show Chunk's Octree External/Ghost Cells" ) )
			{
				const VX5::MooreNeighborsMask27 which_neighbors_to_show = {
					voxel_engine_debug_state.show_ghost_cells_in_these_neighbors_mask.raw_value
				};
				VX5::dbg_show_Cells_located_inside_Neighboring_Chunks(
					containing_chunk_id
					, which_neighbors_to_show
					, _voxel_world
					, m_dbgView
					, MemoryHeaps::temporary()
					);
			}
		}//if


		//
		if( ImGui::TreeNodeEx( "Level Of Detail", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			VoxelEngineDebugState::LevelOfDetailStitchingState & LoD_stitching = voxel_engine_debug_state.LoD_stitching;

			//
			if( ImGui::Button("Copy Coords From Current Chunk" ) )
			{
				LoD_stitching.debug_chunk_id.setFromChunkID( containing_chunk_id );
			}

			//
			if( ImGui::Button("Show Bounds For Selecting Cells From Coarse Neighbors" ) )
			{
				VX5::dbg_showBounds_for_SelectingCells_from_CoarseNeighbors(
					LoD_stitching.debug_chunk_id.asChunkId()
					, _voxel_world
					, m_dbgView
					, LoD_stitching.mask_to_show_bounds_only_for_specified_coarse_neighbors
					, LoD_stitching.show_bounds_for_C0_continuity_only
					);
			}

			//
			if( ImGui::Button("Show Cached Cells of Coarse Neighbors" ) )
			{
				VX5::dbg_show_Cached_Cells_of_Coarse_Neighbors(
					LoD_stitching.debug_chunk_id.asChunkId()
					, _voxel_world
					, m_dbgView
					, LoD_stitching.top_level_octants_mask_in_seam_octree
					, LoD_stitching.show_coarse_cells_needed_for_C0_continuity
					, LoD_stitching.show_coarse_cells_needed_for_C1_continuity
					);
			}

			ImGui::TreePop();
		}//


		//
		ImGui_DrawPropertyGrid(
			&voxel_engine_debug_state,
			mxCLASS_OF(voxel_engine_debug_state),
			"Voxel Engine Debug Settings"
			);

		//
		if( ImGui::TreeNodeEx( "Chunk Debugging", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			if( ImGui::Button("Remesh current chunk" ) )
			{
				_voxel_world.dbg_RemeshChunk( getCameraPosition() );
			}

			ImGui::TreePop();
		}


		//
		if( ImGui::Button("Regenerate world") )//|| (isInGameState && input_state.keyboard.held[KEY_R]) )
		{
			_voxel_world.dbg_waitForAllTasks();
			if(mxSUCCEDED( this->rebuildBlobTreeFromLuaScript() )) {
				_voxel_world.dbg_regenerateWorld();
			}
		}
	}
	ImGui::End();
}




bool dbg_enableMeshDebuggingForChunk( const VX::ChunkID& chunk_id )
{
#if vx5_CFG_DEBUG_MESHING

	return true;

	//return chunk_id == VX::ChunkID( 5,1,4, 2 )
	//	|| chunk_id == VX::ChunkID( 2, 1, 2, 3 )
	//	;

	//return chunk_id == VX::ChunkID( 4,3,4, 2 )
	//	|| chunk_id == VX::ChunkID( 2, 2, 2, 3 )
	//	;

	//return chunk_id ==
	//	TowerDefenseGame::Get().m_settings.voxel_engine_debug_state.LoD_stitching.debug_chunk_id.asChunkId()
	//	;

	return chunk_id.getLoD() == 2 || chunk_id.getLoD() == 3
		;

#else
	return false;
#endif
}
