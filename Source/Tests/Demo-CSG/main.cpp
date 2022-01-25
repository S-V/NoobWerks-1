#include <SDL/include/SDL_syswm.h>
#include <SDL/include/SDL_filesystem.h>
#include <bx/macros.h>

#include "xray_re/xr_ogf.h"

//#include <bgfx/include/bgfx.h>
//#include <bgfx/include/bgfxplatform.h>
#include <ImGui/imgui.h>

#include <FMOD/fmod.h>
#include <FMOD/fmod_errors.h>

#include <Base/Base.h>
#include <Base/Util/LogUtil.h>
#include <Core/Core.h>
#include <Driver/Driver.h>
#include <Driver/Windows/ConsoleWindow.h>
#include <Graphics/graphics_device.h>
#include <Renderer/Pipelines/TiledDeferred.h>
#include <Renderer/Light.h>
#include <Renderer/Mesh.h>
#include <Renderer/Geometry.h>
#include <Renderer/Shaders/__shader_globals_common.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/TxTSupport/TxTSerializers.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/DemoFramework/ImGUI_Renderer.h>
#include <Utility/DemoFramework/DemoFramework.h>
#include <Developer/EditorSupport/DevAssetFolder.h>
#include <Utility/Meshok/BSP.h>

#include <OGF/Fmesh.h>
#include "ModelLoader_md5.h"


ERet FMOD_RESULT_TO_ERet( FMOD_RESULT result )
{
	switch( result ) {
		case FMOD_OK :	return ALL_OK;
		default:
			return ERR_UNKNOWN_ERROR;
	}
}

#define mxCALL_FMOD( X )\
	mxMACRO_BEGIN\
		const FMOD_RESULT result = (X);\
		if( result != FMOD_OK )\
		{\
			ptERROR("FMOD error: '%s' (%d)\n", FMOD_ErrorString(result), result);\
			return FMOD_RESULT_TO_ERet(result);\
		}\
	mxMACRO_END



struct FMOD
{
	FMOD_SYSTEM *	system;

	FMOD_SOUND *	sound_shoot_loop;
	FMOD_SOUND *	sound_shoot_end;
	FMOD_CHANNEL *	channel_shoot_loop;
	FMOD_CHANNEL *	channel_shoot_end;

public:
	FMOD()
	{
		system = NULL;
		sound_shoot_loop = NULL;
		sound_shoot_end = NULL;
		channel_shoot_loop = NULL;
		channel_shoot_end = NULL;
	}
	ERet Initialize()
	{
		mxCALL_FMOD(FMOD_System_Create(&system));

		unsigned int      version;
		mxCALL_FMOD(FMOD_System_GetVersion(system, &version));

		if (version < FMOD_VERSION)
		{
			ptERROR("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
			return ERR_INCOMPATIBLE_VERSION;
		}

		mxCALL_FMOD(FMOD_System_Init(system, 32, FMOD_INIT_NORMAL, NULL));



		mxCALL_FMOD(FMOD_System_CreateSound(
			system,
			
			"D:/games/Project IGI [2000]/_unpacked/___orig/ak47_loop.wav",
			//"D:/games/Project IGI [2000]/_unpacked/___orig/mp5sd_loop.wav",
			//"D:/games/Project IGI [2000]/_unpacked/___orig/m16_loop.wav",

			FMOD_HARDWARE, 0, &sound_shoot_loop));
		mxCALL_FMOD(FMOD_Sound_SetMode(sound_shoot_loop, FMOD_LOOP_NORMAL));

		mxCALL_FMOD(FMOD_System_CreateSound(
			system,

			"D:/games/Project IGI [2000]/_unpacked/___orig/ak47_loop_e.wav",
			//"D:/games/Project IGI [2000]/_unpacked/___orig/mp5sd_loop_e.wav",
			//"D:/games/Project IGI [2000]/_unpacked/___orig/m16_loop_e.wav",

			FMOD_HARDWARE, 0, &sound_shoot_end));

		return ALL_OK;
	}
	void Shutdown()
	{
		FMOD_Sound_Release(sound_shoot_loop);
		sound_shoot_loop = NULL;
		channel_shoot_loop = NULL;

		FMOD_System_Close(system);
		FMOD_System_Release(system);
		system = NULL;
	}
	ERet Update()
	{
		mxCALL_FMOD(FMOD_System_Update(system));
		return ALL_OK;
	}

	ERet PlaySound_Shoot()
	{
//		mxCALL_FMOD(FMOD_System_PlaySound(system, FMOD_CHANNEL_FREE, sound_shoot_loop, 0, &channel_shoot_loop));
//		mxCALL_FMOD(FMOD_Channel_SetVolume(channel_shoot_loop, 2.0f));
		return ALL_OK;
	}
	ERet Stop_Shooting()
	{
		if( channel_shoot_loop )
		{
			mxCALL_FMOD(FMOD_Channel_SetVolume(channel_shoot_loop, 1.0f));
			mxCALL_FMOD(FMOD_Channel_Stop(channel_shoot_loop));

			mxCALL_FMOD(FMOD_System_PlaySound(system, FMOD_CHANNEL_FREE, sound_shoot_end, 0, &channel_shoot_end));

			channel_shoot_loop = NULL;
		}
		return ALL_OK;
	}
};




class MyMenuState : public MenuState {
public:
	MyMenuState()
	{
		//
	}
	virtual void Update( double deltaSeconds ) override
	{}
	virtual void RenderGUI() override
	{
		if(ImGui::Begin("Debug Visualization"))
		{
			//ImGui::Checkbox("Visualize vertex normals",&g_settings.drawVertexNormals);
			//ImGui::Checkbox("Draw meshes in wireframe",&g_settings.drawWireframeMesh);
			ImGui::End();
		}

		if(ImGui::Begin("Settings"))
		{
			//ImGui_DrawPropertyGrid(&g_settings,mxCLASS_OF(g_settings),"Settings");
			ImGui::End();
		}

		if( ImGui::Begin("Control Panel") )
		{
			//if(ImGui::Button("Rebuild"))
			//{
			//	DBGOUT("rebuilding octree!");
			//	g_rebuild_octree = true;
			//}
			//if(ImGui::Button("EXIT"))
			//{
			//	WindowsDriver::RequestExit();
			//}
			ImGui::End();
		}		

		//ImGui::ShowTestWindow();
	}
};

struct MyApp : Demos::DemoApp
{
	Clump *		m_scene_data;
	Clump *		m_renderer_data;

	FlyingCamera	camera;

	DeferredRenderer	m_deferredRenderer;
	//SimpleRenderer		m_render_system;

	BatchRenderer			m_batch_renderer;
	PrimitiveBatcher		m_prim_batcher;
	GUI_Renderer		m_gui_renderer;

	AxisArrowGeometry	m_gizmo_renderer;

	FMOD	sound;


//	MeshBuilder	m_meshBuffer;

	bool m_debugShowNormalsRT;

	V3F4			m_lastRayCastStart;
	V3F4			m_lastRayCastDirection;
//	RayCastResult	m_lastRayCastResult;

	V3F4 m_pointPosition;
	V3F4 m_closestPointOnMesh;

	MyMenuState	m_menuState;

	SceneStats	m_sceneStats;

	RrMesh *	m_mesh;
	RrLocalLight* m_localLight;

	bool m_isFiring;

	md5::Model	m_md5_model;

public:
	MyApp()
	{
		m_debugShowNormalsRT = false;
		m_isFiring = false;
	}
	ERet Setup()
	{
		//SON::LoadFromFile(SETTINGS_FILE_NAME,g_settings);
		const char* pathToAssets = gUserINI->GetString("path_to_compiled_assets");
		mxDO(DemoApp::__Initialize(pathToAssets));

m_renderer_data = NULL;
mxDO(GetAsset(m_renderer_data, MakeAssetID("deferred")));
//DemoUtil::DBG_PrintClump(*m_rendererData);


		mxDO(m_deferredRenderer.Initialize(m_renderer_data));
mxDO(m_batch_renderer.Initialize());
		mxDO(m_prim_batcher.Initialize(&m_batch_renderer));
		mxDO(m_gui_renderer.Initialize());

		mxDO(sound.Initialize());

		this->m_gizmo_renderer.BuildGeometry();

		mxDO(CreateScene());

		// Main game mode
		stateMgr.PushState( &mainMode );
		WindowsDriver::SetRelativeMouseMode(true);

		return ALL_OK;
	}
	void Shutdown()
	{
		SON::SaveClumpToFile(*m_scene_data, "R:/scene.son");

		sound.Shutdown();

		m_gui_renderer.Shutdown();
		m_prim_batcher.Shutdown();
m_batch_renderer.Shutdown();
		m_deferredRenderer.Shutdown();

		m_scene_data->Clear();
		delete m_scene_data;
		m_scene_data = NULL;

		DemoApp::__Shutdown();

		//dev_save_state(GameActions::dev_save_state,IS_Pressed,0);
	}

	ERet CreateScene()
	{
		mxPROFILE_SCOPE("CreateScene()");

		m_scene_data = new Clump();

		{
			{
				RrGlobalLight* globalLight;
				mxDO(m_scene_data->New(globalLight));
				globalLight->m_direction = V3_Set(0,0,-1);
				globalLight->m_color = V3_Set(0.2,0.4,0.3);
			}
			{
				RrLocalLight* localLight;
				mxDO(m_scene_data->New(localLight));
				localLight->position = V3_Set(0,0,500);
				localLight->radius = 2000;
				localLight->color = V3_Set(1,0,0);
				m_localLight = localLight;
			}
		}

		m_pointPosition = V3_Zero();
		m_closestPointOnMesh = V3_Zero();

#if(0)
		{
			const float S = 1000.0f;

			DrawVertex planeVertices[4] =
			{
				/* south-west */{ V3_Set( -S, -S, 0.0f ),  V2F_To_Half2(V2F_Set(    0, 1.0f )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
				/* north-west */{ V3_Set( -S,  S, 0.0f ),  V2F_To_Half2(V2F_Set(    0,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
				/* north-east */{ V3_Set(  S,  S, 0.0f ),  V2F_To_Half2(V2F_Set( 1.0f,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
				/* south-east */{ V3_Set(  S, -S, 0.0f ),  V2F_To_Half2(V2F_Set( 1.0f, 1.0f )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ) },
			};

			const UINT16 planeIndices[6] = {
				0, 2, 1, 0, 3, 2,
			};

			VertexDescription vertexDecl;
			BSP::Vertex::BuildVertexDescription(vertexDecl);

			HInputLayout inputLayout = llgl::CreateInputLayout(vertexDecl,"BSP");
			Rendering::g_inputLayouts[VTX_BSP] = inputLayout;

			RrMesh* mesh = NULL;
			mxDO(RrMesh::Create(
				mesh,
				m_scene_data,
				planeVertices, mxCOUNT_OF(planeVertices), sizeof(planeVertices),
				VTX_Draw,
				planeIndices, sizeof(planeIndices),
				Topology::TriangleList
			));

			RrModel* model = RrModel::Create( mesh, m_scene_data );

			RrMaterial*	defaultMaterial;
			mxDO(GetAsset( defaultMaterial, MakeAssetID("plastic.material"), m_scene_data ));
//			defaultMaterial->m_uniforms[0] = V4F_Replicate(1);
			model->m_batches.Add(defaultMaterial);

			m_mesh = mesh;
		}
#endif


	if(0)
	{
		mxDO(md5::LoadModel(
			//"R:/testbed/Assets/3rdparty/ETQW/gdf_machine_pistol/machine_pistol_world.md5mesh",
			"R:/testbed/Assets/3rdparty/ETQW/gdf_machine_pistol/view.md5mesh",
			m_md5_model
			));

		//ETQW::LoadMd5b(
		//	"R:/testbed/Assets/_test/gdf_machine_pistol/machine_pistol_world.md5b"
		//	);
	}


		const UINT32 currentTimeMSec = mxGetTimeInMilliseconds();
//		stats.Print(currentTimeMSec - startTimeMSec);

		return ALL_OK;
	}

	ERet LoadSceneFromCache()
	{
		mxPROFILE_SCOPE("LoadSceneFromCache()");
		UNDONE;
		return ALL_OK;
	}

	void Update(const double deltaSeconds) override
	{
		mxPROFILE_SCOPE("Update()");

		// update audio
		sound.Update();

		DemoApp::Update(deltaSeconds);

		camera.Update(deltaSeconds);

		float s, c;
		Float_SinCos(deltaSeconds, s, c);
		m_localLight->position.x = 2000 * s;
		m_localLight->position.y = 2000 * c;

		//dev_cast_ray(GameActions::dev_cast_ray,EInputState::IS_Pressed,0);


		//if( m_isFiring ) {
		//	sound.PlaySound_Shoot();
		//} else {
		//	sound.Stop_Shooting();
		//}
		//m_isFiring = false;
	}

	ERet BeginRender()
	{
		mxPROFILE_SCOPE("Render()");

		const InputState& inputState = WindowsDriver::GetState();

		const M44F4 viewMatrix = camera.BuildViewMatrix();

		const int screenWidth = inputState.window.width;
		const int screenHeight = inputState.window.height;

		const float aspect_ratio = (float)screenWidth / (float)screenHeight;
		const float FoV_Y_degrees = DEG2RAD(90);
		const float near_clip	= 10.0f;
		const float far_clip	= 10000.0f;

		const M44F4 projectionMatrix = Matrix_Perspective(FoV_Y_degrees, aspect_ratio, near_clip, far_clip);
		const M44F4 viewProjectionMatrix = Matrix_Multiply(viewMatrix, projectionMatrix);
		const M44F4 worldMatrix = Matrix_Identity();
		const M44F4 worldViewProjectionMatrix = Matrix_Multiply(worldMatrix, viewProjectionMatrix);


		SceneView view;
		view.viewMatrix = viewMatrix;
		view.projectionMatrix = projectionMatrix;
		view.viewProjectionMatrix = viewProjectionMatrix;
		view.worldSpaceCameraPos = camera.m_eyePosition;
		view.viewportWidth = screenWidth;
		view.viewportHeight = screenHeight;
		view.nearClip = near_clip;
		view.farClip = far_clip;

		llgl::ViewState	viewState;
		{
			viewState.Reset();
			viewState.colorTargets[0].SetDefault();
			viewState.targetCount = 1;
			viewState.flags = llgl::ClearAll;
		}
		llgl::SetView(llgl::GetMainContext(), viewState);

		if(0){
		mxDO(m_deferredRenderer.RenderScene(view, *m_scene_data, &m_sceneStats));
		}
		else
		{
			mxGPU_SCOPE(Test);

			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_renderer_data, "NoCulling"));


			//TbShader* shader;
			//mxDO(GetAsset(shader,MakeAssetID("full_screen_triangle.shader"),m_rendererData));

			//RrTexture* texture;
			//mxDO(GetAsset(texture,MakeAssetID("Foliage(DXT5).texture"),m_sceneData));
			//mxDO(FxSlow_SetResourceOBSOLETE(shader, "t_sourceTexture", texture->m_resource, Rendering::g_samplers[PointSampler]));
			//mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));
			//mxDO(m_render_system.DrawFullScreenTriangle(shader));



#if 0
			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("debug_draw_colored.shader"),m_rendererData));
			{
				G_PerObject	cbPerObject;
				cbPerObject.g_worldMatrix = Matrix_Identity();
				cbPerObject.g_worldViewMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewMatrix);
				cbPerObject.g_worldViewProjectionMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewProjectionMatrix);
				llgl::UpdateBuffer(llgl::GetMainContext(), m_render_system.m_hCBPerObject, sizeof(cbPerObject), &cbPerObject);

				mxDO(FxSlow_SetUniform( shader, "g_worldViewProjectionMatrix", &cbPerObject.g_worldViewProjectionMatrix ));
				mxDO(FxSlow_SetUniform( shader, "g_worldMatrix", &cbPerObject.g_worldMatrix ));
			}
			V4F4 color = V4F_Replicate(1);
			mxDO(FxSlow_SetUniform( shader, "g_color", &color ));
			mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));
#endif

			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("forward_lit_bumpmapped.shader"),m_renderer_data));
			{
				G_PerObject	cbPerObject;

				const M44F4 worldMatrix = Matrix_Identity();
				const M44F4 worldViewMatrix = Matrix_Multiply(worldMatrix, view.viewMatrix);
				const M44F4 worldViewProjectionMatrix = Matrix_Multiply(worldMatrix, view.viewProjectionMatrix);

				cbPerObject.g_worldMatrix2 = Matrix_Transpose( worldMatrix );
				cbPerObject.g_worldViewMatrix2 = Matrix_Transpose( worldViewMatrix );
				cbPerObject.g_worldViewProjectionMatrix2 = Matrix_Transpose( worldViewProjectionMatrix );

				llgl::UpdateBuffer(llgl::GetMainContext(), Rendering::tr.cbPerObject, sizeof(cbPerObject), &cbPerObject);
			}
			{
				HUniform u_pointLight = FxGetUniform(shader, "g_pointLight");
				if(u_pointLight.IsValid())
				{
					PointLight pointLight;
					pointLight.Pos_InvRadius = V4F4_Set(m_localLight->position, 1.0f/m_localLight->radius);
					pointLight.Color_Radius = V4F4_Set(m_localLight->color, m_localLight->radius);
					FxSetUniform(shader,u_pointLight,&pointLight);
				}
			}
			mxDO(FxSlow_SetUniform( shader, "g_worldCamPos", &view.worldSpaceCameraPos ));

			RrTexture* diffuseMap;
			mxDO(GetAsset(diffuseMap,MakeAssetID("stones_diffuse.texture"),m_scene_data));
			RrTexture* normalMap;
			mxDO(GetAsset(normalMap,MakeAssetID("stones_normals_height.texture"),m_scene_data));
			
			FxSamplerState* sampler = FindByName<FxSamplerState>(*m_renderer_data, "Bilinear");

			mxDO(FxSlow_SetResourceOBSOLETE(shader, "t_diffuseMap", diffuseMap->m_resource, sampler->handle));
			mxDO(FxSlow_SetResourceOBSOLETE(shader, "t_normalMap", normalMap->m_resource, sampler->handle));
			mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));


			llgl::Batch	batch;
			batch.Clear();

			m_deferredRenderer.SetGlobalUniformBuffers( &batch );
			FxApplyShaderStateOBSOLETE(batch, *shader);

			batch.inputLayout = Rendering::tr.inputLayouts[m_mesh->m_vertexFormat];
			batch.topology = m_mesh->m_topology;
			batch.VB[0] = m_mesh->m_vertexBuffer;
			batch.IB = m_mesh->m_indexBuffer;
			batch.flags = m_mesh->m_flags;
			batch.baseVertex = 0;
			batch.vertexCount = 4;
			batch.startIndex = 0;
			batch.indexCount = 6;
			llgl::Draw(llgl::GetMainContext(), batch);
		}

#if 0
		{
			mxGPU_SCOPE(Debug);

			//llgl::ViewState	viewState;
			//{
			//	viewState.Reset();
			//	viewState.colorTargets[0].SetDefault();
			//	viewState.targetCount = 1;
			//	viewState.flags = llgl::ClearAll;
			//}
			//llgl::SetView(llgl::GetMainContext(), viewState);

			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_renderer_data, "NoCulling"));


			//TbShader* shader;
			//mxDO(GetAsset(shader,MakeAssetID("full_screen_triangle.shader"),m_rendererData));

			//RrTexture* texture;
			//mxDO(GetAsset(texture,MakeAssetID("Foliage(DXT5).texture"),m_sceneData));
			//mxDO(FxSlow_SetResourceOBSOLETE(shader, "t_sourceTexture", texture->m_resource, Rendering::g_samplers[PointSampler]));
			//mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));
			//mxDO(m_render_system.DrawFullScreenTriangle(shader));



#if 0
			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("debug_draw_colored.shader"),m_rendererData));
			{
				G_PerObject	cbPerObject;
				cbPerObject.g_worldMatrix = Matrix_Identity();
				cbPerObject.g_worldViewMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewMatrix);
				cbPerObject.g_worldViewProjectionMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewProjectionMatrix);
				llgl::UpdateBuffer(llgl::GetMainContext(), m_render_system.m_hCBPerObject, sizeof(cbPerObject), &cbPerObject);

				mxDO(FxSlow_SetUniform( shader, "g_worldViewProjectionMatrix", &cbPerObject.g_worldViewProjectionMatrix ));
				mxDO(FxSlow_SetUniform( shader, "g_worldMatrix", &cbPerObject.g_worldMatrix ));
			}
			V4F4 color = V4F_Replicate(1);
			mxDO(FxSlow_SetUniform( shader, "g_color", &color ));
			mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));
#endif

			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("debug_draw_tangents.shader"),m_renderer_data));
			{
				G_PerObject	cbPerObject;
				cbPerObject.g_worldMatrix = Matrix_Identity();
				cbPerObject.g_worldViewMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewMatrix);
				cbPerObject.g_worldViewProjectionMatrix = Matrix_Multiply(cbPerObject.g_worldMatrix, view.viewProjectionMatrix);
				llgl::UpdateBuffer(llgl::GetMainContext(), m_deferredRenderer.m_hCBPerObject, sizeof(cbPerObject), &cbPerObject);
			}
			float g_lineLength = 10000;
			mxDO(FxSlow_SetUniform( shader, "g_lineLength", &g_lineLength ));
			mxDO(FxSlow_Commit(llgl::GetMainContext(), shader));



			llgl::Batch	batch;
			batch.Clear();

			batch.inputLayout = Rendering::g_inputLayouts[m_mesh->m_vertexFormat];
			batch.topology = Topology::PointList;//m_mesh->m_topology;

			batch.VB[0] = m_mesh->m_vertexBuffer;
			//batch.IB = m_mesh->m_indexBuffer;
			batch.flags = m_mesh->m_flags;

			m_deferredRenderer.SetGlobalUniformBuffers( &batch );
			FxApplyShaderStateOBSOLETE(batch, *shader);

			batch.baseVertex = 0;
			batch.vertexCount = 4;
			batch.startIndex = 0;
			batch.indexCount = 0;

			llgl::Draw(llgl::GetMainContext(), batch);

			//for( int iSubMesh = 0; iSubMesh < m_mesh->m_parts.Num(); iSubMesh++ )
			//{
			//	const RrSubmesh& submesh = m_mesh->m_parts[iSubMesh];

			//	batch.baseVertex = 0;//submesh.baseVertex;
			//	batch.vertexCount = 0;//submesh.vertexCount;
			//	batch.startIndex = submesh.startIndex;
			//	batch.indexCount = submesh.indexCount;

			//	llgl::Draw(llgl::GetMainContext(), batch);
			//}
		}
#endif

#if 0

		if(g_settings.drawWireframeMesh)
		{
			mxGPU_SCOPE(Wireframe);

			llgl::ViewState	viewState;
			{
				viewState.Reset();
				viewState.colorTargets[0].SetDefault();
				viewState.targetCount = 1;
				viewState.depthTarget.SetDefault();
			}
			llgl::SetView(llgl::GetMainContext(), viewState);

			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("solid_wireframe"),m_sceneData));
			{
				const SolidWireSettings& settings = g_settings.solidWire;
				mxDO(FxSlow_SetUniform(shader,"LineWidth",&settings.LineWidth));
				mxDO(FxSlow_SetUniform(shader,"FadeDistance",&settings.FadeDistance));
				mxDO(FxSlow_SetUniform(shader,"PatternPeriod",&settings.PatternPeriod));
				mxDO(FxSlow_SetUniform(shader,"WireColor",&settings.WireColor));
				mxDO(FxSlow_SetUniform(shader,"PatternColor",&settings.PatternColor));
			}
			mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));

			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_rendererData, "SolidWireframe"));
			m_render_system.DBG_Draw_Models_Wireframe(view, *m_sceneData, *shader);
		}
		if(g_settings.drawVertexNormals)
		{
			TbShader* shader;
			mxDO(GetAsset(shader,MakeAssetID("debug_draw_normals.shader"),m_sceneData));
			{
				float lineLength = 100;
				mxDO(FxSlow_SetUniform(shader,"g_lineLength",&lineLength));
			}
			mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));

			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_rendererData, "Default"));
			m_render_system.DBG_Draw_Models_Custom(view, *m_sceneData, *shader, Topology::PointList);
		}

		{
			mxGPU_SCOPE(DebugDrawOctree);
			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_rendererData, "NoCulling"));

			TbShader* shader;
			mxDO(GetByName(*m_rendererData, "auxiliary_rendering", shader));
			m_batch_renderer.SetShader(shader);
			mxDO(FxSlow_SetUniform(shader,"u_modelViewProjMatrix",&worldViewProjectionMatrix));
			mxDO(FxSlow_SetUniform(shader,"u_modelMatrix",&worldMatrix));
			mxDO(FxSlow_SetUniform(shader,"u_color",RGBAf::WHITE.ToPtr()));
			mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));



			m_batch_renderer.DrawLine(m_pointPosition, m_closestPointOnMesh, RGBAf::RED, RGBAf::RED);



			//OctCubeF octants[8];
			//GetChildOctants(worldCube,octants);
			//for( int i = 0; i < 8; i++ )
			//{
			//	AABB24 aabb;
			//	OctBoundsToAABB(octants[i], aabb);
			//	m_batch_renderer.DrawAABB(aabb.min_point, aabb.max_point, RGBAf::lightblue);
			//}

			if(g_settings.visualize_octree)
			{
				DebugDrawOctree(0,worldCube,0,0,m_batch_renderer);
			}

			// 'crosshair'
			m_batch_renderer.DrawCircle(
				camera.m_eyePosition + camera.m_lookDirection * 100,
				camera.m_rightDirection,
				camera.m_upDirection,
				RGBAf::RED,
				1, 8
			);

			if( m_lastRayCastResult.hitAnything )
			{
				m_batch_renderer.DrawLine( m_lastRayCastStart, m_lastRayCastResult.rayExit, RGBAf::WHITE, RGBAf::RED );
				m_batch_renderer.DrawCircle(m_lastRayCastResult.rayEntry, camera.m_rightDirection, camera.m_upDirection, RGBAf::YELLOW, 10, 8);
				m_batch_renderer.DrawCircle(m_lastRayCastResult.rayExit, camera.m_rightDirection, camera.m_upDirection, RGBAf::GREEN, 10, 8);
				//DrawHitLeaves( 0, worldBounds, m_batch_renderer );
				for( int i = 0; i < m_hitLeaves.Num(); i++ )
				{
					const DebugLeafInfo& leafInfo = m_hitLeaves[i];
					m_batch_renderer.DrawAABB(leafInfo.aabb.min_point, leafInfo.aabb.max_point, RGBAf::RED);
					m_batch_renderer.DrawCircle( leafInfo.xyz, camera.m_rightDirection, camera.m_upDirection, RGBAf::ORANGE, 20, 4 );
					m_batch_renderer.DrawLine( leafInfo.xyz, leafInfo.xyz + leafInfo.N * 100, RGBAf::WHITE, RGBAf::RED );
				}
			}
		}
#endif

		{
			mxGPU_SCOPE(Gizmos);
			mxDO(FxSlow_SetRenderState(llgl::GetMainContext(), *m_renderer_data, "NoCulling"));

			TbShader* shader;
			mxDO(GetAsset(shader, MakeAssetID("auxiliary_rendering.shader"), m_renderer_data));
			m_prim_batcher.SetShader(shader);
			mxDO(FxSlow_SetUniform(shader,"u_modelViewProjMatrix",&worldViewProjectionMatrix));
			mxDO(FxSlow_SetUniform(shader,"u_modelMatrix",&worldMatrix));
			mxDO(FxSlow_SetUniform(shader,"u_color",RGBAf::WHITE.ToPtr()));
			mxDO(FxSlow_Commit(llgl::GetMainContext(),shader));

			m_md5_model.Dbg_DrawMesh(m_prim_batcher, Matrix4::CreateScale(Vec3D(100)), RGBAf::WHITE);

			DrawGizmo(m_gizmo_renderer, worldMatrix, camera.m_eyePosition, m_prim_batcher);

#if 0
			const AABB24 worldAabb = GetSceneBounds();
			m_batch_renderer.DrawAABB(worldAabb.min_point, worldAabb.max_point, RGBAf::LIGHT_YELLOW_GREEN);

			m_batch_renderer.Flush();
#endif
		}

		m_prim_batcher.Flush();

		m_gui_renderer.UpdateInputs();

		m_gui_renderer.BeginFrame();
		{
			if(ImGui::Begin("Render Stats"))
			{
				ImGui::Text("Models: %d", m_sceneStats.models_rendered);
				ImGui::End();
			}
			ImGui_Window_Text_Float3(camera.m_eyePosition);

			ImGui::Text("m_rotationSpeed: %f", camera.m_rotationSpeed);
			ImGui::Text("m_pitchSensitivity: %f", camera.m_pitchSensitivity);
			ImGui::Text("m_yawSensitivity: %f", camera.m_yawSensitivity);
		}
		return ALL_OK;
	}

	ERet EndRender()
	{
		m_gui_renderer.EndFrame();
		return ALL_OK;
	}

	void attack1( GameActionID action, EInputState status, float value )
	{
		if( status == IS_Pressed || status == IS_HeldDown )
		{
			DBGOUT("Firing");
			sound.PlaySound_Shoot();
		}
		else
		{
			DBGOUT("Stop fire");
			sound.Stop_Shooting();
		}

		m_isFiring = true;//if( status == IS_Pressed || status == IS_HeldDown );
		//m_pointPosition = camera.m_eyePosition;
		//m_closestPointOnMesh = meshSDF.GetClosestPointTo(m_pointPosition);
		//LogStream(LL_Debug) << "Closest point: " << m_closestPointOnMesh << ", dist: " << V3_Length(m_pointPosition-m_closestPointOnMesh);
		//return;

		//RayCastResult result;
		//m_octree.CastRay(camera.m_eyePosition, camera.m_lookDirection, result, g_settings.dc);
		//if( result.hitLeaves.Num() > 0 )
		//{
		//	TLocalArray< DebugLeafInfo, 32 >	hitLeaves;
		//	DC::DebugGatherHitLeaves( result, m_octree, 0, GetSceneCube(), hitLeaves );
		//	const DebugLeafInfo& firstHit = hitLeaves.GetFirst();
		//	//int leafIndex = result.hitLeaves.GetFirst();
		//	CSG_Subtract( firstHit.xyz );
		//}
	}
	void attack2( GameActionID action, EInputState status, float value )
	{
		//RayCastResult result;
		//m_octree.CastRay(camera.m_eyePosition, camera.m_lookDirection, result, g_settings.dc);
		//if( result.hitLeaves.Num() > 0 )
		//{
		//	TLocalArray< DebugLeafInfo, 32 >	hitLeaves;
		//	DC::DebugGatherHitLeaves( result, m_octree, 0, GetSceneCube(), hitLeaves );
		//	const DebugLeafInfo& firstHit = hitLeaves.GetFirst();
		//	//int leafIndex = result.hitLeaves.GetFirst();
		//	CSG_Add( firstHit.xyz );
		//}
	}
	void dev_cast_ray( GameActionID action, EInputState status, float value )
	{
		////DBGOUT("Cast ray");
		//m_lastRayCastStart = camera.m_eyePosition;
		//m_lastRayCastDirection = camera.m_lookDirection;
		//m_octree.CastRay(camera.m_eyePosition, camera.m_lookDirection, m_lastRayCastResult, g_settings.dc);

		//m_hitLeaves.Empty();
		//DC::DebugGatherHitLeaves( m_lastRayCastResult, m_octree, 0, GetSceneCube(), m_hitLeaves );

		//if( m_hitLeaves.Num() > 0 )
		//{
		//	DBGOUT("%d hits",m_hitLeaves.Num());

		//	const DebugLeafInfo& firstHit = m_hitLeaves.GetFirst();
		//	LogStream(LL_Debug) << "firstHit.pos=" << firstHit.xyz;

		//	for( int i = 0; i < m_hitLeaves.Num(); i++ )
		//	{
		//		const DebugLeafInfo& leafInfo = m_hitLeaves[i];
		//		const DC::Leaf2& leaf = m_octree.m_leaves[ leafInfo.leafIndex ];
		//		LogStream(LL_Debug) << "Leaf[" << leafInfo.leafIndex << "]: pos=" << leafInfo.xyz << "; quantized: " << (int)leaf.xyz[0] << "," << (int)leaf.xyz[1] << "," << (int)leaf.xyz[2];
		//	}

		//	CSG_Subtract( firstHit.xyz );
		//}
		//else
		//{
		//	DBGOUT("no intersection");
		//}
	}
	void dev_toggle_menu( GameActionID action, EInputState status, float value ) override
	{
		ToggleState( &m_menuState );
	}
	void dev_reset_state( GameActionID action, EInputState status, float value ) override
	{
		camera.m_eyePosition = V3_Set(300,300,300);
	}
	void dev_save_state( GameActionID action, EInputState status, float value ) override
	{
		////SON::SaveToFile(g_settings,SETTINGS_FILE_NAME);
		//FileWriter file;
		//if(mxSUCCEDED(file.Open(OCTREE_SAVE_FILE_NAME))) {
		//	m_octree.Save(file);
		//}		
	}
	void dev_load_state( GameActionID action, EInputState status, float value ) override
	{
		////SON::LoadFromFile(SETTINGS_FILE_NAME,g_settings);
		//FileReader file;
		//if(mxSUCCEDED(file.Open(OCTREE_SAVE_FILE_NAME))) {
		//	m_octree.Load(file);
		//	DualContouring::OctStats stats;
		//	m_octree.Triangulate(m_meshBuffer,g_settings.dc,stats);
		//	CreateMesh(m_meshBuffer, m_mesh);
		//}	
	}
	void take_screenshot( GameActionID action, EInputState status, float value )
	{
		CalendarTime	localTime( CalendarTime::GetCurrentLocalTime() );

		String64	timeOfDay;
		GetTimeOfDayString( timeOfDay, localTime.hour, localTime.minute, localTime.second );

		String128	timeStamp;
		Str::SPrintF(timeStamp, "%d.%d.%d - %s",
			localTime.year, localTime.month, localTime.day, timeOfDay.c_str());

		String256	fileName("screenshots/");
		Str::Append(fileName, timeStamp);
		Str::AppendS(fileName, ".jpg");

		DBGOUT("Saving screenshot to file '%s'", fileName.c_str());
		llgl::SaveScreenshot( fileName.c_str() );
	}
};

ERet MyEntryPoint( const String& root_folder )
{
	MyApp	app;

	FlyingCamera& camera = app.camera;
	camera.m_eyePosition = V3_Set(0, -100, 20);
	camera.m_movementSpeed = 1000;
	camera.m_strafingSpeed = 1000;
	camera.m_rotationSpeed = 0.1;
	camera.m_invertYaw = true;
	camera.m_invertPitch = true;
UNDONE;
//	SON::LoadFromFile(gUserINI->GetString("path_to_camera_state"),camera);

	{
		InputBindings& input = app.m_bindings;

		{
			String512 path_to_key_bindings( root_folder );
			Str::AppendS( path_to_key_bindings, "Config/input_mappings.son" );
			mxDO(SON::LoadFromFile( path_to_key_bindings.c_str(), input.contexts ));
		}

		input.handlers.Add( ActionBindingT(GameActions::move_forward, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_forward,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_back, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_back,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_left, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_left,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_right, mxBIND_MEMBER_FUNCTION(FlyingCamera,strafe_right,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_up, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_up,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::move_down, mxBIND_MEMBER_FUNCTION(FlyingCamera,move_down,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::rotate_pitch, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_pitch,camera)) );
		input.handlers.Add( ActionBindingT(GameActions::rotate_yaw, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_yaw,camera)) );
	}



	WindowsDriver::Settings	options;
	options.window_x = 900;
	options.window_y = 30;
//	options.create_OpenGL_context = true;
	mxDO(WindowsDriver::Initialize(options));

	//WindowsDriver::SetRelativeMouseMode(true);


	llgl::Settings settings;
	settings.window = WindowsDriver::GetNativeWindowHandle();
	llgl::Initialize(settings);


	mxDO(app.Setup());



	CPUTimer timer;

	InputState inputState;
	while( WindowsDriver::BeginFrame( &inputState, &app ) )
	{
//		DBGOUT("DT: %G",inputState.deltaSeconds);

		//inputState.deltaSeconds = timer.GetTimeMilliseconds() * 1e-3;

		app.Dev_ReloadChangedAssets();

		if( inputState.hasFocus )
		{
			//inputState.deltaSeconds *= 100000;
			// 
			//DBGOUT("ticks: %u, seconds: %G",ticksEl_,_state.deltaSeconds);
			app.Update(inputState.deltaSeconds);
		}

		app.BeginRender();

		{

			ImGui::Text("dt: %.3f", inputState.deltaSeconds);
		}

		app.EndRender();

		//{
		//	llgl::ViewState	viewState;
		//	viewState.Reset();
		//	viewState.colorTargets[0].SetDefault();
		//	viewState.targetCount = 1;
		//	viewState.flags = llgl::ClearAll;
		//	llgl::SetView(llgl::GetMainContext(), viewState);
		//}

		//app.m_gui_renderer.BeginFrame();

		//ImGui_Window_Text_Float3(app.camera.m_eyePosition);
		//{
		//	LogStream(LL_Debug) << app.camera.m_eyePosition;
		//}

		//ImGui::ShowTestWindow();

		//app.m_gui_renderer.EndFrame();

		llgl::NextFrame();

		mxSleepMilliseconds(15);

		mxPROFILE_INCREMENT_FRAME_COUNTER;
	}

	app.Shutdown();

	WindowsDriver::Shutdown();


	SON::SaveToFile(camera,gUserINI->GetString("path_to_camera_state"));


	return ALL_OK;
}


ERet CreateMeshFromOGF( xray_re::xr_ogf* ogf, Clump &scene )
{
	const bool hierarchical	= ogf->hierarchical();
	const bool skeletal		= ogf->skeletal();
	const bool animated		= ogf->animated();
	const bool progressive	= ogf->progressive();
	const bool versioned	= ogf->versioned();

	UNDONE;
	{
		const xray_re::xr_vbuf& vb = ogf->vb();
		const xray_re::xr_ibuf& ib = ogf->ib();

		if( vb.size() ) {
			//
		}
		if( ib.size() ) {
			//
		}
	}
	return ALL_OK;
}

// http://lodka3d.narod.ru/lodfileformat.html

namespace LODka
{
	union ChunkHdr_t
	{
		struct {
			U32	type;	// fourCC chunk type
			U32	size;	// size of data following this chunk
			U32	count;	// auxiliary parameter
		};
		char dbg_txt[12];
	};

	enum ChunkID
	{
		BLOCK_LOD1 = MCHAR4('L','O','D','1'),

		// subchunks
		___	CHUNK_SKELETON		= MCHAR4('S','K','L','1'),	// SKL1 - Bone hierarchy
		___	CHUNK_MESH_LIST		= MCHAR4('M','S','L','1'),	// MSL1 - Mesh list
		___	CHUNK_MESH_BUFFER	= MCHAR4('M','S','H','1'),	// MSH1 - Mesh
	};

	U32 FindChunk( IOStreamFILE & file, ChunkID type, ChunkHdr_t * _header = nil )
	{
		const size_t fileSize = file.Length();

		char	temp_buffer[4096];

		size_t current_offset = file.Tell();

		while( current_offset < fileSize )
		{
			ChunkHdr_t	chunkHdr;
			mxDO(file.Get(chunkHdr));
			current_offset += sizeof(chunkHdr);

			if( chunkHdr.type == type )
			{
				if( _header ) {
					*_header = chunkHdr;
				}
				return current_offset;
			}

			char dbgname[5] = {0};
			*(U32*)dbgname = chunkHdr.type;

			ptPRINT("Skipping chunk: type=%s, size=%d", dbgname, chunkHdr.size);
			mxDO(Stream_Skip(file, chunkHdr.size, temp_buffer, sizeof(temp_buffer)));
			current_offset += chunkHdr.size;
		}
		return 0;

	}

	struct Bone
	{
		String64	name;
		int			id;			// 0..32767
		int			parentId;	// -1..32767 (-1 if root)		
	};


	U32 CalculateSizeOfIndices( U32 indexCount )
	{
		U32 index_data_size = 0;
		if( indexCount <= 256 ) {
			index_data_size = indexCount * sizeof(U8);
		} else if( indexCount <= 65536 ) {
			index_data_size = indexCount * sizeof(U16);
		} else {// > 65536
			index_data_size = indexCount * sizeof(U32);
		}
		return index_data_size;
	}


	ERet Load_LODka( const char* filename )
	{
		IOStreamFILE	file;
		mxDO(file.Open( filename ));

		char	temp_buffer[4096] = {0};

		mxDO(file.Read( temp_buffer, 8 ));

		if( strcmp(temp_buffer, "LODka3D1") != 0 ) {
			return ERR_OBJECT_OF_WRONG_TYPE;
		}

		const U32 LOD1_offset = FindChunk( file, BLOCK_LOD1 );
		if( LOD1_offset )
		{
			TLocalArray< Bone, 64 >	bones;

			// SKL1
			ChunkHdr_t	chunkHeader;
			const U32 skeleton_offset = FindChunk( file, CHUNK_SKELETON, &chunkHeader );
			const bool is_animated = (skeleton_offset != 0);
			if( is_animated )
			{
				U8 skeleton_type = 0;
				mxDO(file.Get(skeleton_type));
				mxASSERT(skeleton_type == 1);

				bones.SetNum( chunkHeader.count );
				for( int boneIndex = 0; boneIndex < chunkHeader.count; boneIndex++ )
				{
					U16 boneId = 0;
					mxDO(file.Get(boneId));

					U16 parentId = 0;
					mxDO(file.Get(parentId));

					U32 name_len = 0;
					mxDO(file.Get(name_len));

					char name_buf[64] = {0};
					mxDO(file.Read(name_buf, name_len));
					//ptPRINT("Read bone '%s'", name_buf);
				}
			}

			const U32 bones_count = bones.Num();
			ptPRINT("Bones %u", bones_count);

			//CHUNK_MESH
			const U32 mesh_list_offset = FindChunk( file, CHUNK_MESH_LIST );
			if( mesh_list_offset )
			{
				const U32 mesh_list_offset = FindChunk( file, CHUNK_MESH_BUFFER );
				if( mesh_list_offset )
				{
					U32 mesh_name_len;
					mxDO(file.Get(mesh_name_len));

					char mesh_name_buf[64] = {0};
					mxDO(file.Read(mesh_name_buf, mesh_name_len));
					ptPRINT("Read mesh '%s'", mesh_name_buf);

					U8 has_skeleton = 0;
					mxDO(file.Get(has_skeleton));

					U8 mesh_mode = 0;
					mxDO(file.Get(mesh_mode));
					mxASSERT(mesh_mode == 3);

					U8 mesh_visible = 0;
					mxDO(file.Get(mesh_visible));


					U32 vertex_count = 0;
					mxDO(file.Get(vertex_count));
					mxDO(Stream_Skip(file, vertex_count*sizeof(V3F4), temp_buffer, sizeof(temp_buffer)));

					ptPRINT("Read %u vertices", vertex_count);

					U32 normals_count = 0;
					mxDO(file.Get(normals_count));
					mxDO(Stream_Skip(file, normals_count*sizeof(V3F4), temp_buffer, sizeof(temp_buffer)));

					mxASSERT(vertex_count == normals_count);


					U8 texture_layers = 0;
					mxDO(file.Get(texture_layers));

					for( int i = 0; i < texture_layers; i++ )
					{
						U32 texCoords_count = 0;
						mxDO(file.Get(texCoords_count));
//						mxASSERT(texCoords_count == vertex_count);
						mxDO(Stream_Skip(file, texCoords_count*sizeof(V2F), temp_buffer, sizeof(temp_buffer)));
					}


					U32 num_bone_indices = 0;
					mxDO(file.Get(num_bone_indices));
					mxASSERT(num_bone_indices == 0 || num_bone_indices == vertex_count);

					const U32 size_of_bone_indices = ( bones_count < 255 )
													? num_bone_indices * sizeof(U8)
													: num_bone_indices * sizeof(U16)
													;
					mxDO(Stream_Skip(file, size_of_bone_indices, temp_buffer, sizeof(temp_buffer)));


					U32 vertex_weights_present = 0;
					mxDO(file.Get(vertex_weights_present));
					mxASSERT(vertex_weights_present == 0 || vertex_weights_present == 1);

					if( vertex_weights_present == 1 )
					{
						U32 weights_per_vertex = 0;
						mxDO(file.Get(weights_per_vertex));
//						mxASSERT(weights_per_vertex > 1);

						U32 num_bone_weights = 0;
						mxDO(file.Get(num_bone_weights));

						const U32 size_of_bone_weights_data = ( bones_count <= 127 )
													? num_bone_weights * 5
													: num_bone_weights * 6
													;
						mxDO(Stream_Skip(file, size_of_bone_weights_data, temp_buffer, sizeof(temp_buffer)));
					}

					U32 num_face_groups = 0;
					mxDO(file.Get(num_face_groups));

					for( int faceGroupIndex = 0; faceGroupIndex < num_face_groups; faceGroupIndex++ )
					{
						U32 face_type = 0;
						mxDO(file.Get(face_type));
						mxASSERT(face_type == 2);

						U8 face_mode = 0;
						mxDO(file.Get(face_mode));
						mxASSERT(face_mode == 0);	// triangle list

						U32 face_name_len = 0;
						mxDO(file.Get(face_name_len));

						char face_name_buf[64] = {0};
						mxDO(file.Read(face_name_buf, face_name_len));
						ptPRINT("Read face '%s'", face_name_buf);

						U8 face_visible = 0;
						mxDO(file.Get(face_visible));


						U32 material_name_lengths[16/*MAX_TEXTURE_LAYERS*/];
						for( int i = 0; i < texture_layers; i++ )
						{
							U32 material_name_len = 0;
							mxDO(file.Get(material_name_len));
							material_name_lengths[i] = material_name_len;
						}

						String64 material_names[16/*MAX_TEXTURE_LAYERS*/];
						for( int i = 0; i < texture_layers; i++ )
						{
							char name[64] = {0};
							mxDO(file.Read(name, material_name_lengths[i]));
							ptPRINT("Material: '%s'", name);
							Str::CopyS( material_names[i], name );
						}

						U32 num_indices = 0;
						mxDO(file.Get(num_indices));
						mxASSERT(num_indices % 3 == 0);

						//CalculateSizeOfIndices
						U32 index_data_size = 0;
						if( num_indices <= 256 ) {
							index_data_size = num_indices * sizeof(U8);
						} else if( num_indices <= 65536 ) {
							index_data_size = num_indices * sizeof(U16);
						} else {// > 65536
							index_data_size = num_indices * sizeof(U32);
						}
						mxDO(Stream_Skip(file, index_data_size, temp_buffer, sizeof(temp_buffer)));


						U32 smoothing_groups = 0;
						mxDO(file.Get(smoothing_groups));

						mxDO(Stream_Skip(file, smoothing_groups*sizeof(U32), temp_buffer, sizeof(temp_buffer)));

						U32 num_normal_indices = 0;
						mxDO(file.Get(num_normal_indices));
						mxASSERT(num_normal_indices % 3 == 0);

						const U32 normal_indices_data_size = CalculateSizeOfIndices(num_normal_indices);
						mxDO(Stream_Skip(file, normal_indices_data_size, temp_buffer, sizeof(temp_buffer)));
					}

					//CHUNK_MESH
				}
			}
		}

		//mxDO(file.Seek(8));

		//const U32 MESH1_offset = FindChunk( file, CHUNK_MESH );
	

		return ALL_OK;
	}
}


namespace ETQW
{
	ERet ReadString( AReader& _reader, String &_string )
	{
		U32 len = 0;
		mxDO(_reader.Get( len ));

		mxDO(_string.Resize(len));

		mxDO(_reader.Read( _string.ToPtr(), len ));

		//_string.CapLength(len);

		return ALL_OK;
	}
	ERet LoadMd5b( const char* filename )
	{
		IOStreamFILE	file;
		mxDO(file.Open( filename ));

		char	temp_buffer[4096] = {0};

		// 0x01000000 0x01000000
		mxDO(file.Read( temp_buffer, 8 ));

		U32 numJoints = 0;
		mxDO(file.Get(numJoints));

		for ( int i = 0; i < numJoints; i++ )
		{
			String64 jointName;
			mxDO(ReadString( file, jointName ));

			ptPRINT("Joint[%d]: '%s'", i, jointName.c_str());

			I32 offset;
			mxDO(file.Get( offset ));
			if ( offset >= 0 ) {
				//joints[i].parent = joints.Ptr() + offset;
			} else {
				//joints[i].parent = NULL;
			}
		}

		return ALL_OK;
	}
}//namespace ETQW


int main(int /*_argc*/, char** /*_argv*/)
{
	SetupBaseUtil	setupBase;
	FileLogUtil		fileLog;

#if 0
	{
		LODka::Load_LODka(
			//"D:/research/demos/Душегубъ/Models/Weapons/AK74M/AK74M.lod"
			"R:/testbed/Assets/_test/AK74M.lod"
			);
	}

	{
		xray_re::xr_ogf* ogf = xray_re::xr_ogf::load_ogf(
			//"G:/Games/STALKER-Clear Sky/UEgamedata/meshes/dynamics/weapons/wpn_ak74/wpn_ak74.ogf"
			"G:/Games/STALKER-Clear Sky/UEgamedata/meshes/dynamics/el_tehnika/komp_klava.ogf"
			);
		ogf->to_object();
		CreateMeshFromOGF(ogf,Clump());


		//IOStreamFILE	file;
		//mxDO(file.Open(
		//	//"G:/Games/STALKER-Clear Sky/UEgamedata/meshes/dynamics/weapons/wpn_ak74/wpn_ak74.ogf"
		//	"G:/Games/STALKER-Clear Sky/UEgamedata/meshes/dynamics/el_tehnika/komp_klava.ogf"
		//));
		//
		//mxDO(LoadOGF(file));
	}

#endif

	SetupCoreUtil	setupCore;
	CConsole		consoleWindow;






	// Try to load the INI config first.
	String512	root_folder;
	{
		// Returns an absolute path in UTF-8 encoding to the application data directory.
		char *base_path = SDL_GetBasePath();
		Str::CopyS( root_folder, base_path );
		Str::StripLastFolders( root_folder, 2 );
		Str::NormalizePath( root_folder );
		SDL_free(base_path);
	}

	String512	path_to_INI_config;
	Str::Copy( path_to_INI_config, root_folder );
	Str::AppendS( path_to_INI_config, "Config/Engine.son" );

	String512	path_to_User_INI;
	Str::Copy( path_to_User_INI, root_folder );
	Str::AppendS( path_to_User_INI, "Config/client.son" );

	SON::TextConfig	engineINI;
	engineINI.LoadFromFile( path_to_INI_config.c_str() );
	gINI = &engineINI;

	SON::TextConfig	clientINI;
	clientINI.LoadFromFile( path_to_User_INI.c_str() );
	gUserINI = &clientINI;

	MyEntryPoint( root_folder );

	return 0;
}
