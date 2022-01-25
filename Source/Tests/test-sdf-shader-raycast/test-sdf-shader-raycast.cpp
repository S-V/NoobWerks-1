// Renders a Signed Distance Field (SDF) using ray marching in a shader.
#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Text/String.h>

const bool SCREENSHOTS_FOR_PAPERS = 1;

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization.h>
#include <Core/Memory.h>
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

#include <Physics/Physics_Simulator.h>
//#pragma comment( lib, "Physics.lib" )

#include "test-sdf-shader-raycast.h"

#include <Scripting/Scripting.h>
#include <Scripting/FunctionBinding.h>
// Lua G(L) crap
#undef G

const char* PATH_TO_SETTINGS = "TestApp_RaycastSDF.AppSettings.son";
const char* PATH_TO_SAVEFILE = "TestApp_RaycastSDF.saved.son";

mxDEFINE_CLASS(AtmosphereParameters);
mxBEGIN_REFLECTION(AtmosphereParameters)
	mxMEMBER_FIELD(f3SunDirection),
	mxMEMBER_FIELD(fSunIntensity),
	mxMEMBER_FIELD(fInnerRadius),
	mxMEMBER_FIELD(fOuterRadius),
	mxMEMBER_FIELD(f3RayleighSctr),
	mxMEMBER_FIELD(fRayleighHeight),
	mxMEMBER_FIELD(f3MieSctr),
	mxMEMBER_FIELD(fMieHeight),
	mxMEMBER_FIELD(fMie),
mxEND_REFLECTION;

mxDEFINE_CLASS(TestApp_RaycastSDF::AppSettings);
mxBEGIN_REFLECTION(TestApp_RaycastSDF::AppSettings)
	mxMEMBER_FIELD(atmosphere),
mxEND_REFLECTION;

ERet TestApp_RaycastSDF::Initialize( const EngineSettings& _settings )
{
	{
		WindowsDriver::Settings	options;
		options.window_width = _settings.window_width;
		options.window_height = _settings.window_height;
		mxDO(WindowsDriver::Initialize(options));
	}
	{
		GL::Settings settings;
		settings.window = WindowsDriver::GetNativeWindowHandle();
		settings.cmd_buf_size = mxMiB(_settings.Gfx_Cmd_Buf_Size_MiB);
		mxDO(GL::Initialize(settings));
	}

	mxDO(DemoApp::Initialize(_settings));

	{
		VertexDescription	vertexDescription;
		AuxVertex::BuildVertexDescription( vertexDescription );
		HInputLayout hAuxVertexFormat = GL::CreateInputLayout(vertexDescription);
		mxDO(m_debugDraw.Initialize( hAuxVertexFormat ));
	}

	m_debugAxes.BuildGeometry();

	return ALL_OK;
}
void TestApp_RaycastSDF::Shutdown()
{
	m_debugDraw.Shutdown();

	DemoApp::Shutdown();

	GL::Shutdown();

	WindowsDriver::Shutdown();
}
void TestApp_RaycastSDF::Tick( const InputState& inputState )
{
	DemoApp::Tick(inputState);
	camera.Update(inputState.deltaSeconds);
}
ERet TestApp_RaycastSDF::Draw( const InputState& inputState )
{
	const float aspect_ratio = float(inputState.window.width) / inputState.window.height;
	const float near_clip = 1;
	const float far_clip = 9000;
	const float halfFoVY = mxPI_DIV_4;	// 45 degrees

	const M44f viewMatrix = camera.BuildViewMatrix();
	const M44f projectionMatrix = Matrix_Perspective( halfFoVY*2, aspect_ratio, near_clip, far_clip );
	const M44f viewProjectionMatrixT = Matrix_Transpose(Matrix_Multiply(viewMatrix, projectionMatrix));	// transposed


	GL::RenderContext* renderContext = GL::GetMainContext();
	{
		GL::ViewState	viewState;
		{
			viewState.Clear();
			viewState.targetCount = 1;
			viewState.colorTargets[0].SetDefault();
			viewState.depthTarget.SetNil();
			viewState.viewport.x = 0;
			viewState.viewport.y = 0;
			viewState.viewport.width = inputState.window.width;
			viewState.viewport.height = inputState.window.height;
			viewState.flags = GL::ClearAll;
			mxZERO_OUT(viewState.clearColors);
			viewState.depth = 1.0f;
			viewState.stencil = 0;
			GL::SetViewParameters( ViewID_FrameStart, viewState );
		}
		{
			viewState.Clear();
			viewState.targetCount = 1;
			viewState.colorTargets[0].SetDefault();
			viewState.depthTarget.SetNil();
			viewState.viewport.x = 0;
			viewState.viewport.y = 0;
			viewState.viewport.width = inputState.window.width;
			viewState.viewport.height = inputState.window.height;
			viewState.flags = 0;
			GL::SetViewParameters( ViewID_TestShader, viewState );
			GL::SetViewParameters( ViewID_DebugLines, viewState );
			GL::SetViewParameters( ViewID_ImGUI, viewState );
		}
	}

	{
		TbRenderState * state_NoCull = nil;
		mxDO(Resources::Load(state_NoCull, MakeAssetID("nocull"), &m_sceneData));

		TbShader* shader;
		mxDO(Resources::Load(shader, MakeAssetID("test_raycast_sdf"), &m_sceneData));
		{
			// XYZ - camera position in world space, W - far plane
			HUniform u_cameraPosition = shader->FindUniform( "u_cameraPosition" );
			V4f v_cameraPosition = V4f::Set( camera.m_eyePosition, far_clip );
			shader->SetUniform( u_cameraPosition, &v_cameraPosition );

			// XYZ - camera look-to direction, W - aspect ratio
			HUniform u_cameraForward = shader->FindUniform( "u_cameraForward" );
			V4f v_cameraForward = V4f::Set( camera.m_lookDirection, aspect_ratio );
			shader->SetUniform( u_cameraForward, &v_cameraForward );

			// XYZ - camera 'Right' vector, W - tan of half vertical FoV
			HUniform u_cameraRight = shader->FindUniform( "u_cameraRight" );
			V4f v_cameraRight = V4f::Set( camera.m_rightDirection, tanf(halfFoVY) );
			shader->SetUniform( u_cameraRight, &v_cameraRight );

			// camera 'Up' vector
			HUniform u_cameraUp = shader->FindUniform( "u_cameraUp" );
			V4f v_cameraUp = V4f::Set( camera.m_upDirection, 0 );
			shader->SetUniform( u_cameraUp, &v_cameraUp );
		}
		FxSlow_Commit(renderContext, GL::ViewID(ViewID_FrameStart), shader, GL::SortKey(0));


		const U32 firstCommandOffset = renderContext->GetCurrentPutPointer();
		mxDO(DrawFullScreenTriangle( renderContext, *state_NoCull, shader, 0 ));
		renderContext->SubmitCommands(
			firstCommandOffset,
			GL::ViewID(ViewID_TestShader),
			GL::SortKey(0)
			);
	}


	{
		TbShader* shader;
		mxDO(Resources::Load(shader, MakeAssetID("auxiliary_rendering")));
		m_debugDraw.SetShader(shader);
		mxDO(FxSlow_SetUniform(shader,"u_worldViewProjection",&viewProjectionMatrixT));
		FxSlow_Commit( renderContext, ViewID_DebugLines, shader, 0 );

		TbRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("nocull")));
		//mxDO(Resources::Load(renderState, MakeAssetID("default")));
		m_debugDraw.SetStates( *renderState );
		m_debugDraw.SetViewID( ViewID_DebugLines );

		DrawGizmo(
			m_debugAxes,
			Matrix_Identity(),//Matrix_Translation(V4f::Set(VX::CHUNK_SIZE*10.f,1)),//
			camera.m_eyePosition, m_debugDraw);
	}
	m_debugDraw.Flush();


	DemoApp::BeginGUI( GL::SortKey(ViewID_ImGUI) );
	{
		ImGui_DrawPropertyGrid(
			&camera,
			mxCLASS_OF(camera),
			"Camera"
			);
		ImGui_DrawPropertyGrid(
			&m_settings.atmosphere,
			mxCLASS_OF(m_settings.atmosphere),
			"Atmosphere"
		);
		ImGui::SliderFloat("SunDirX", &m_settings.atmosphere.f3SunDirection.x, -1, 1);
		ImGui::SliderFloat("SunDirY", &m_settings.atmosphere.f3SunDirection.y, -1, 1);
		ImGui::SliderFloat("SunDirZ", &m_settings.atmosphere.f3SunDirection.z, -1, 1);
		m_settings.atmosphere.f3SunDirection = V3_Normalized( m_settings.atmosphere.f3SunDirection );
	}
	DemoApp::EndGUI();

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
		TestApp_RaycastSDF app;

		FlyingCamera& camera = app.camera;
		camera.m_eyePosition = V3_Set(0, -100, 20);
		camera.m_movementSpeed = 10;
		camera.m_strafingSpeed = camera.m_movementSpeed;
		camera.m_rotationSpeed = 0.5;
		camera.m_invertYaw = true;
		camera.m_invertPitch = true;

		SON::LoadFromFile(PATH_TO_SAVEFILE,camera);
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

		SON::SaveToFile(camera, PATH_TO_SAVEFILE);
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
