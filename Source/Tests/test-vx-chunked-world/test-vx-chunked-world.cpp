#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Text/String.h>

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization.h>
//#include <Core/Tasking.h>
#include <Core/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/Meshok/DualContouring.h>
#include <Utility/TxTSupport/TxTConfig.h>

#include <Renderer/Geometry.h>
#include <Renderer/Material.h>

#include "test-vx-chunked-world.h"

#include <Scripting/Scripting.h>
#include <Scripting/FunctionBinding.h>
// Lua G(L) crap
#undef G


const char* PATH_TO_SETTINGS = "R:/m_settings2.son";

mxDEFINE_CLASS(AppSettings);
mxBEGIN_REFLECTION(AppSettings)
mxEND_REFLECTION;

ERet App_Test_Chunked_World::Initialize( const EngineSettings& _settings )
{
	{
		mxREFACTOR(:)
		WindowsDriver::Settings	options;
		options.window_width = _settings.window_width;
		options.window_height = _settings.window_height;
		options.create_OpenGL_context = _settings.UseOpenGL;
		mxDO(WindowsDriver::Initialize(options));
	}
	{
		GL::Settings settings;
		settings.window = WindowsDriver::GetNativeWindowHandle();
		settings.cmd_buf_size = _settings.Gfx_Cmd_Buf_Size_MiB;
		mxDO(GL::Initialize(settings));
	}

	mxDO(DemoApp::Initialize(_settings));

	mxDO(m_sceneRenderer.Initialize(_settings.window_width, _settings.window_height));

	mxDO(m_batch_renderer.Initialize());
	mxDO(m_prim_batcher.Initialize(&m_batch_renderer));

	this->m_gizmo_renderer.BuildGeometry();


	{
		RrMaterial* terrainMat = nil;
		//mxDO(Resources::Load(plasticMat, MakeAssetID("material_plastic"), &m_runtime_clump));
		//mxDO(Resources::Load(terrainMat, MakeAssetID("material_terrain"), &m_runtime_clump));
		mxDO(Resources::Load(terrainMat, MakeAssetID("material_terrain_plain"), &m_runtime_clump));
		m_voxelMaterial = terrainMat;
	}


	{
		TPtr<RrGlobalLight> globalLight;
		mxDO(m_runtime_clump.New(globalLight.Ptr));
		//globalLight->direction = V3_Normalized(V3_Set(1,1,-1));
		globalLight->direction = V3_Normalized(V3_Set(-0.4,+0.4,-1));

//globalLight->direction = V3_Set(1,0,0);

		globalLight->flags = LightFlags::Shadows;

		if(0)
			globalLight->color = V3_Set(0.6,0.7,0.7)* 0.1f;	// night
		else
			globalLight->color = V3_Set(0.6,0.7,0.7)* 1.1f;	// day
//globalLight->color = V3_Set(0,1,0);
	}
	//{
	//	TPtr<RrLocalLight> localLight;
	//	mxDO(m_runtime_clump.New(localLight.Ptr));
	//	localLight->position = V3_Set(50,50,1000);
	//	localLight->radius = 100;
	//	localLight->color = V3_Set(0.5,0.6,0.7);
	//}

	{
		mxDO(m_runtime_clump.New(m_torchlight.Ptr));
		m_torchlight->radius = 150;
		//m_torchlight->color = V3_Set(0.9,0.6,0.7);
		m_torchlight->color = V3_Set(0.6,0.8,0.7);
	}


	AHeap & scratch = Memory::Scratch();


//VX::ADC::SignedOctree	signedOctree( scratch );
//{
//	VX::HermiteData		hermiteData( scratch );
//	mxDO(hermiteData.Create( procGenSettings, _SDF, chunkOffset ));
//	mxDO(signedOctree.CreateFromHermiteData( hermiteData ));
//}


	ScopedTimer	timer;


#if 0
	{
		const int res = 32;
		const Int3 LDNI_resolution = Int3_Set( res, res, res );

		Meshok::LDNI::SolidBody::BuildOptions	buildOptions;
		buildOptions.bounds.mins = V3_Zero();
		//buildOptions.bounds.mins = V3_SetAll(-res);
		buildOptions.bounds.maxs = V3_SetAll(+res);
		buildOptions.resolution = LDNI_resolution;

		Meshok::TriMesh	sourceMesh(scratch);
		mxDO(sourceMesh.LoadFromFile(
			//"D:/sdk/NVIDIA Direct3D SDK 10/Media/Gargoyle/gargoyle.dae"

			//"R:/testbed/Assets/meshes/monkey.obj"	//! HOLES!
			//"G:/__MVC++/simpl/apple.ply"	//! HOLES! on North pole!
			//"R:/test_scene1.obj"

			//"R:/testbed/Assets/meshes/untitled.3ds"
			//"R:/testbed/Assets/meshes/cat.ply"
			//"R:/testbed/Assets/meshes/unit_cube.3ds"
//			"R:/testbed/Assets/meshes/cow.ply"
			//"R:/testbed/Assets/meshes/cylinder1.dae"
			//"G:/__MVC++/simpl/hind.ply"	//bad - holes!
			//"G:/__MVC++/simpl/cow.ply"
//			"D:/research/__test/meshes/teapot/teapot.obj"	//BAD!!!
			//"D:/research/__test/meshes/fandisk.obj"		//THE MESH CONTAINS HOLES
			//"D:/research/__test/meshes/fandisk_lite.obj"
			//"D:/research/__test/meshes/bolt.obj"
			//"D:/research/__test/meshes/skull.obj"
			//"D:/research/__test/meshes/stl(opencamlib)/sphere.stl"	//BAD EXAMPLE
			//"G:/__MVC++/_temp_dc_models/sphere8.obj"
			//"D:/research/__test/meshes/kitten.obj"	//HOLES!!!
			//"D:/research/__test/meshes/cactus-coarse.obj"
			//"D:/research/__test/meshes/evil-ape-skull.obj"
			//"D:/research/__test/meshes/dragon.obj"	//fine

			//"D:/research/__test/meshes/blender_monkey_difficult_topology.obj"
			"D:/research/__test/meshes/unit_cube.stl"
			//"D:/research/__test/meshes/rotated_cube1.obj"	//JAGGIEZ AND HOLES!

			//"R:/opencamlib/stl/spider.stl"	//holes with BVH if 4^3!
			//"D:/research/engines/Phyre Engine/Media/DonkeyTrader/arches_lods.dae"
			//"D:/research/__test/meshes/large/Parthenon.obj"	// VERY BAD WITH LDNI!
		));
		{
			const float meshScale = 0.8f;
			const V3F center = AABB_Center(buildOptions.bounds);
			const V3F sceneSize = AABB_FullSize(buildOptions.bounds);
			AABB newAabb(buildOptions.bounds);
			newAabb.mins += sceneSize * (1-meshScale) * 0.5f;
			newAabb.maxs -= sceneSize * (1-meshScale) * 0.5f;
			sourceMesh.FitInto(newAabb);
		}

		//Meshok::LDNI::SolidBody	body(scratch);
		Meshok::LDNI::SolidBody	& body = VX::DebugHelper::Get().debugLDNI;
		mxDO(body.Build( sourceMesh, buildOptions ));

//body.SubtractSphere(
//	V3_Multiply(V3_SetAll(res), V3_Set(0.2, 0.3, 0.3)), res*0.33f,
//	VX::DebugHelper::Get().debugEdges );

body.SubtractSphere( V3_Multiply(V3_SetAll(res), V3_Set(0.5, 0.3, 0.3)), res*0.33f );


		VX::LDNI_Sampler	volumeSampler( body, LDNI_resolution );



		//QEF_Solver_Simple	solver;
		//QEF_Solver_SVD	solver;
		QEF_Solver_SVD2		solver;
		//QEF_Solver_QR		solver;
		//QEF_Solver_SVD_Eigen	solver;

		Meshok::TriMesh	outputMesh(scratch);






		SDF::Cube	cube;
//		cube.center = AABB_Center(buildOptions.bounds);
		cube.extent = res * 0.28f;


		//SDF::CSG::CSG_Rotate	rotation;
		//rotation.O = &cube;
		//rotation.matrix = M33_RotationZ(45);

		SDF::Transform	transform;
		transform.O = &cube;

		transform.matrix = Matrix_RotationZ(45);
		transform.matrix = Matrix_Multiply(transform.matrix, Matrix_RotationX(45));
		Matrix_SetTranslation(&transform.matrix, AABB_Center(buildOptions.bounds));

		transform.matrix_inverse = Matrix_OrthoInverse(transform.matrix);



		VX::SDF_Sampler	volumeSampler2(
			transform//*SDF::Testing::GetTestScene(V3_From_XYZ(LDNI_resolution))//
			, LDNI_resolution );



		Meshok::DualContouring::Polygonize( volumeSampler, solver, outputMesh, VX::DebugHelper::Get(), scratch );


		{
			RrMesh *	mesh;
			m_runtime_clump.New(mesh);

			RrModel *	model;
			m_runtime_clump.New(model);

			M34F4 *	transform;
			mxDO(m_runtime_clump.New( transform ));
			*transform = M34_Pack( Matrix_Identity() );

			model->m_mesh = mesh;
			model->m_transform = transform;
			model->m_materialSet = m_voxelMaterials;

			VX::UploadRenderMesh( outputMesh, mesh, &m_runtime_clump, scratch );
		}
	}
#endif



	return ALL_OK;
}
void App_Test_Chunked_World::Shutdown()
{
//	m_jobMan.WaitforAllAndShutdown();

	SlowTasks::Shutdown();

	Rendering::Destroy_All_Buffers_In_Clump(&m_runtime_clump, true);

	m_prim_batcher.Shutdown();
	m_batch_renderer.Shutdown();
	m_sceneRenderer.Shutdown();

	DemoApp::Shutdown();

	GL::Shutdown();

	WindowsDriver::Shutdown();
}
void App_Test_Chunked_World::Tick( const InputState& inputState )
{
	DemoApp::Tick(inputState);

	camera.Update(inputState.deltaSeconds);

	m_torchlight->position = camera.m_eyePosition;
}
ERet App_Test_Chunked_World::Draw( const InputState& inputState )
{
//	const double frameStartTimeSeconds = mxGetTimeInSeconds();

	SceneView sceneView;
	sceneView.right = camera.m_rightDirection;
	sceneView.look = camera.m_lookDirection;
	sceneView.up = camera.m_upDirection;
	sceneView.origin = camera.m_eyePosition;
	sceneView.near_clip = 0.5;
	sceneView.far_clip = 2000;
	//sceneView.halfFoVY = FoV_Y_radians/2;
	//sceneView.aspect_ratio = aspect_ratio;
	sceneView.viewportWidth = inputState.window.width;
	sceneView.viewportHeight = inputState.window.height;

	const M44f viewMatrix = camera.BuildViewMatrix();
	const M44f projectionMatrix = Matrix_Perspective(sceneView.halfFoVY*2, sceneView.aspect_ratio,
		sceneView.near_clip, sceneView.far_clip);
	const M44f viewProjectionMatrixT = Matrix_Transpose(Matrix_Multiply(viewMatrix, projectionMatrix));	// transposed

	HContext renderContext = GL::GetMainContext();


	Viewport	viewport;
	viewport.x = 0;	viewport.y = 0;
	viewport.width = inputState.window.width;
	viewport.height = inputState.window.height;

	RendererStats	sceneStats;
	mxDO(Rendering::DrawScene(viewport, sceneView, m_runtime_clump, m_sceneRenderer, &sceneStats));

	//if( m_settings.render.drawSky ) {
	//	Rendering::DrawSkybox( renderContext, m_sceneRenderer.m_renderPath
	//		, MakeAssetID("sky-cubemap")
	//		, &m_runtime_clump );
	//}

	{
		const U32 passIndex = m_sceneRenderer.m_renderPath->FindPassIndex(ScenePasses::DebugLines);

		TbShader* shader;
		mxDO(Resources::Load(shader, MakeAssetID("auxiliary_rendering")));
		m_prim_batcher.SetShader(shader);
		const M44f u_worldViewProjection = Matrix_Transpose(projectionMatrix);
		mxDO(FxSlow_SetUniform(shader,"u_worldViewProjection",&viewProjectionMatrixT));
		FxSlow_Commit2( renderContext, passIndex, shader, 0 );

		m_prim_batcher.SetViewID( passIndex );

		// Crosshair
//		if( m_settings.render.drawCrosshair )
		{
			m_prim_batcher.DrawSolidCircle(
				camera.m_eyePosition + camera.m_lookDirection,
				camera.m_rightDirection,
				camera.m_upDirection,
				RGBAf::RED,
				0.01f
			);
		}
//		if( m_settings.render.drawGizmo )
		{
			DrawGizmo(
				m_gizmo_renderer,
				Matrix_Identity(),//Matrix_Translation(V4_Set(VX::CHUNK_SIZE*10.f,1)),//
				camera.m_eyePosition, m_prim_batcher);
		}
	}
	m_prim_batcher.Flush();

	//if( m_settings.render.showWireframe )
	//{
	//	rmt_ScopedCPUSample(DrawWireframe, 0);
	//	Demos::SolidWireSettings	solidWireSettings;
	//	if(SCREENSHOTS_FOR_PAPERS)
	//	{
	//		solidWireSettings.FillColor = V4_Set( 0, 0, 0, 1 );
	//		solidWireSettings.WireColor = V4_Set( 0, 0, 0, 1 );
	//		solidWireSettings.PatternColor = V4_Set( 0, 0, 0, 1 );
	//	}
	//	Demos::DrawModelsInWireframe(sceneView, m_runtime_clump, m_sceneRenderer.m_renderPath, renderContext, &m_runtime_clump, solidWireSettings);
	//}

	if( inputState.keyboard[KEY_F1] ) {
//		m_settings.render.showGUI ^= 1;
	}



//	const double frameEndTimeSeconds = mxGetTimeInSeconds();
//	const float frameAverageTimeSeconds = m_FPS_counter.Add( frameEndTimeSeconds - frameStartTimeSeconds );

	//const float frameAverageTimeSeconds = m_FPS_counter.Add( inputState.deltaSeconds );
	const float frameAverageTimeSeconds = m_FPS_counter.Add( Engine::g_lastFrameTimeSeconds );
	const float averageFramesPerSecond = 1.0f / (frameAverageTimeSeconds + 1e-5f);


//	if( m_settings.render.showGUI )
	{
		const U32 passIndex = m_sceneRenderer.m_renderPath->FindPassIndex(GetStaticStringHash("GUI"));
		DemoApp::BeginGUI( passIndex );
#if 0
		{
			String512 buf;
			{
				StringStream temp(buf);
				temp << "Camera pos: " << camera.m_eyePosition;
				ImGui::Checkbox("Show Wireframe", &m_settings.render.showWireframe);
				ImGui::Checkbox("Flat Shading?", &m_settings.render.flatShading);
				ImGui::Checkbox("Draw Voxel grid?", &m_settings.render.debugVoxels);
				ImGui::Checkbox("Draw Dual grid?", &m_settings.render.debugDrawDualGrid);
				ImGui::Checkbox("Draw Hermite data?", &m_settings.render.debugDrawEdges);
				ImGui::Checkbox("Draw clamped vertices?", &m_settings.render.debugDrawClampedVertices);
				ImGui::Checkbox("Draw sharp features?", &m_settings.render.debugDrawSharpFeatures);
				ImGui::Checkbox("Show auxiliary info?", &m_settings.render.debugDrawAuxiliary);
				ImGui::Checkbox("Draw octree?", &m_settings.render.debugDrawOctree);
				ImGui::Checkbox("Draw sky?", &m_settings.render.drawSky);
				ImGui::Checkbox("Draw crosshair?", &m_settings.render.drawCrosshair );
				ImGui::Checkbox("Draw gizmo?", &m_settings.render.drawGizmo);
				ImGui::Checkbox("Show BIH?", &m_settings.render.showBIH);

				if( ImGui::CollapsingHeader("LDNI") )
				{
					ImGui::Checkbox("Draw Axis X", &m_settings.render.showLDNIx);
					ImGui::Checkbox("Draw Axis Y", &m_settings.render.showLDNIy);
					ImGui::Checkbox("Draw Axis Z", &m_settings.render.showLDNIz);
				}
			}
			ImGui::Text(buf.c_str());

			{
				const F32 distance = SDF::Testing::GetTestScene(VX::CHUNK_SIZE)->DistanceAt(camera.m_eyePosition);
				ImGui::Text("Distance: %.4f", distance);
			}

			ImGui::InputFloat("Movement speed", &camera.m_movementSpeed, 1.0f, 10.0f);
			ImGui::InputFloat("Strafing speed", &camera.m_strafingSpeed, 1.0f, 10.0f);

			if( ImGui::Begin("Stats") )
			{
				//ImGui::Text("Triangles: %u", m_world.m_stats.num_triangles_created);
				//ImGui::Text("Vertices: %u", m_world.m_stats.num_vertices_created);
				ImGui::Text("Batches: %u", GL::g_PipelineStats.batches_submitted);
				ImGui::Text("Triangles: %u", GL::g_PipelineStats.triangles_rendered);
				ImGui::Text("Vertices: %u", m_world.m_stats.num_vertices_created);
				//ImGui::Text("Chunk Gen [msec]: %u", m_contouringStats.time_msec_chunkGeneration);
				ImGui::Text("Chunks queued: %u", m_world.m_loadedRegion.unprocessed.Num());
				ImGui::Text("Chunks dirty: %u", m_world.m_dirtyChunks.Num());

				ImGui::Text("Octree Gen [msec]: %u", m_contouringStats.time_msec_octreeBuilding);
				ImGui::Text("Octree Mem [KiB]: %u", m_contouringStats.mem_KiB_octreeLeaves);
				ImGui::Text("Octree Contour [msec]: %u", m_contouringStats.time_msec_octreeContouring);

				ImGui::Text("VBs [KiB]: %.3f", m_world.m_stats.memory_used_for_vertex_buffers/1024.0f);
				ImGui::Text("IBs [KiB]: %.3f", m_world.m_stats.memory_used_for_index_buffers/1024.0f);

				ImGui::Text("FPS: %f", averageFramesPerSecond);
				ImGui::Text("GPU: %3.1f ms", GL::g_PipelineStats.GPU_Time_Milliseconds);
				ImGui::Text("Slow tasks: %u", AtomicLoad(SlowTasks::NumPendingTasks()));
				ImGui::Text("Jobs finished: %u", Engine::g_job_sytem_stats.nNumFinished);
				ImGui::End();
			}

			if( ImGui::Begin("Voxel World") )
			{
				VX::ContouringSettings	newContouringSettings( m_settings.contour );

				ImGui::Checkbox("Simplify?", &m_settings.contour.simplify);
				ImGui::InputFloat("Error threshold", &m_settings.contour.threshold, 0.0f, 10.0f);
				ImGui::InputInt("Octree resolution", (int*)&m_settings.contour.resolution, 1, 1);
				if(m_settings.contour.resolution < 0 ) m_settings.contour.resolution = 1;
				if(m_settings.contour.resolution > VX::PMIC::NUM_CELLS ) m_settings.contour.resolution = VX::PMIC::NUM_CELLS/2;

				ImGui::Checkbox("Generate seams?", &m_settings.generateSeams);
				if( m_settings.generateSeams ) {
					m_settings.contour.crackPatching = VX::FillTheSeams;
				} else {
					m_settings.contour.crackPatching = VX::UglyCracks;
				}

				ImGui::Checkbox("Decimate meshes?", &m_settings.contour.decimateMeshes);
				ImGui::InputFloat("Decimation error", &m_settings.contour.decimationThreshold);

				ImGui::Checkbox("Threaded generation?", &m_world.m_settings.multithreaded_chunk_generation);
				ImGui::Checkbox("Threaded contouring?", &m_world.m_settings.multithreaded_chunk_contouring);

				ImGui::Checkbox("Enable LoDs?", &m_world.m_debug_enable_LoDs);
				ImGui::Checkbox("Freeze LoDs?", &m_world.m_debug_freeze_LoDs);

				if( ImGui::Button("Re-Mesh") ) {
					m_debugRemeshVoxelWorld = true;
				}
				ImGui::End();
			}
		}
#endif
		DemoApp::EndGUI();
	}

	return ALL_OK;
}

ERet MyEntryPoint()
{
	EngineSettings settings;
	{
		String256 path_to_engine_config;
		mxDO(SON::GetPathToConfigFile("engine_config.son", path_to_engine_config));
		mxDO(SON::LoadFromFile(path_to_engine_config.c_str(), settings));
	}

	{
		App_Test_Chunked_World app;

		FlyingCamera& camera = app.camera;
		camera.m_eyePosition = V3_Set(0, -100, 20);
		camera.m_movementSpeed = 10;
		camera.m_strafingSpeed = camera.m_movementSpeed;
		camera.m_rotationSpeed = 0.5;
		camera.m_invertYaw = true;
		camera.m_invertPitch = true;

		SON::LoadFromFile(settings.path_to_save_file.c_str(),camera);
		SON::LoadFromFile(PATH_TO_SETTINGS, app.m_settings);

		{
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_forward, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_forward,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_back, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_back,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_left, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_left,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_right, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_right,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_up, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_up,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::move_down, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_down,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::rotate_pitch, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_pitch,camera)) );
			app.m_StateMan.handlers.Add( ActionBindingT(GameActions::rotate_yaw, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_yaw,camera)) );
			//app.m_StateMan.handlers.Add( ActionBindingT(GameActions::editor_select, mxBIND_MEMBER_FUNCTION(TerrainApp,editor_select,app)) );
			//app.m_StateMan.handlers.Add( ActionBindingT(GameActions::editor_csg_add, mxBIND_MEMBER_FUNCTION(TerrainApp,editor_csg_add,app)) );
			//app.m_StateMan.handlers.Add( ActionBindingT(GameActions::editor_csg_subtract, mxBIND_MEMBER_FUNCTION(TerrainApp,editor_csg_subtract,app)) );
		}

		Engine::Run( &app, settings );

		SON::SaveToFile(camera, settings.path_to_save_file.c_str());

		SON::SaveToFile(app.m_settings, PATH_TO_SETTINGS);
	}

	return ALL_OK;
}

int main(int /*_argc*/, char** /*_argv*/)
{
//	TIncompleteType<sizeof(GL::ViewState)>();
	MyEntryPoint();
	return 0;
}
