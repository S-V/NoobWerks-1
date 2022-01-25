#pragma once

#include <ImGui/imgui.h>

#include <Base/Base.h>
#include <Base/Util/LogUtil.h>
#include <Core/Core.h>
#include <Driver/Driver.h>
#include <Driver/Windows/ConsoleWindow.h>
#include <Graphics/graphics_device.h>
#include <Renderer/Pipelines/TiledDeferred.h>
#include <Renderer/Pipelines/Forward+.h>
#include <Renderer/Light.h>
#include <Renderer/Geometry.h>
#include <Renderer/Shaders/__shader_globals_common.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/TxTSupport/TxTSerializers.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/DemoFramework/ImGUI_Renderer.h>
#include <Utility/DemoFramework/DemoFramework.h>
#include <Developer/EditorSupport/DevAssetFolder.h>

#include <Renderer/Pipelines/TiledCommon.h>


inline int FindClosestVertex(
					  const V3F& eye,
					  const V3F& dir,
					  const TArray< DrawVertex >& vertices,
					  const float maxDistToLine = 10
					  )
{
	float distToEye = BIG_NUMBER;
	int minDistIndex = -1;
	for( int i = 0; i < vertices.Num(); i++ )
	{
		const V3F& p = vertices[i].xyz;
		const V3F cp = Ray_ClosestPoint(eye, dir, p);
		const float distToLine = Distance( cp, p );
		if( distToLine > maxDistToLine ) {
			continue;
		}

		const float distSq = V3_LengthSquared( cp - eye );
		if( distSq < distToEye )
		{
			distToEye = distSq;
			minDistIndex = i;
		}
	}
	if( minDistIndex != -1 ) {
		LogStream(LL_Debug) << "Found " << minDistIndex
			<< ", UV : " << Half2_To_V2F(vertices[minDistIndex].uv)
			//<< vertices[minDistIndex].xyz
			;
	}
	return minDistIndex;
}








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
	TPtr< Clump >		m_scene_data;

	DeferredRenderer		m_deferredRenderer;
	//ForwardPlusRenderer		m_forwardPlusRenderer;
	//TiledDeferredRenderer	m_tiledDeferredRenderer;

	TPtr< ARenderer >	m_currentRenderer;

	MyBatchRenderer		m_batch_renderer;
	PrimitiveBatcher	m_prim_batcher;

	AxisArrowGeometry	m_gizmo_renderer;



	FlyingCamera	camera;



	MyMenuState	m_menuState;

	SceneStats	m_sceneStats;

	RrMesh *	m_mesh;
	TPtr<RrLocalLight> m_localLight;

	TPtr< RrMesh >	m_cubeMesh;


	//HTexture	m_CS_output;
	//HBuffer	m_CS_output;
	HBuffer		m_UBO_number_of_structures;
	//HBuffer		m_UBO_number_of_debug_lines_readback;

	bool debug_renderTargets;

	bool	animateLights;

	Fonts::SpriteFont	m_debugFont;
	Fonts::FontRenderer	m_fontRenderer;


	SceneView	m_debugFrustumView;
	bool		m_debugDrawFrustum;
	bool		m_debugSaveFrustum;

#if DEBUG_EMIT_POINTS
	TArray<DebugPoint>	m_debugPoints;
#endif
	//HTexture	m_cubemapTexture;


TArray< DrawVertex >	m_vertices;
TArray< int >	m_indices;
int m_closestVertexIndex;

public:
	MyApp()
	{
		animateLights = false;

		debug_renderTargets = true;
		m_debugDrawFrustum = false;
		m_debugSaveFrustum = false;

		m_closestVertexIndex = -1;
	}

	ERet Initialize( const EngineSettings& _settings );

	void Shutdown();

	ERet CreateScene();

	ERet LoadSceneFromCache()
	{
		mxPROFILE_SCOPE("LoadSceneFromCache()");
		UNDONE;
		return ALL_OK;
	}

	virtual void Tick( const InputState& inputState ) override
	{
		mxPROFILE_SCOPE("Update()");

		__super::Tick(inputState);

		camera.Update(inputState.deltaSeconds);

		if(m_localLight != nil)
		{
			//float s, c;
			//Float_SinCos(deltaSeconds, s, c);
			//m_localLight->position.x = 2000 * s;
			//m_localLight->position.y = 2000 * c;
			m_localLight->position = camera.m_eyePosition;
		}

		//dev_cast_ray(GameActions::dev_cast_ray,EInputState::IS_Pressed,0);


		//if( m_isFiring ) {
		//	sound.PlaySound_Shoot();
		//} else {
		//	sound.Stop_Shooting();
		//}
		//m_isFiring = false;
	}

	ERet BeginRender(const double deltaSeconds);

	ERet EndRender()
	{
		__super::EndGUI();
		return ALL_OK;
	}

	virtual void attack1( GameActionID action, EInputState status, float value ) override
	{
		m_closestVertexIndex = FindClosestVertex(camera.m_eyePosition, camera.m_lookDirection, m_vertices);
	}
	void dev_reset_state( GameActionID action, EInputState status, float value ) override
	{
		camera.m_eyePosition = V3_Set(-5,-60,42);
		camera.m_lookDirection = V3_Normalized(V3_Negate(camera.m_eyePosition));
	}
	void dev_save_state( GameActionID action, EInputState status, float value ) override
	{
		m_debugSaveFrustum = true;
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


