#include <bx/debug.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Base/Util/LogUtil.h>
#include <ImGui/imgui.h>
#include <Engine/WindowsDriver.h>

#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_utilities.h>

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Scene/Light.h>

//#include <EffectCompiler2/Effect_Compiler.h>
#include <DemoFramework/DemoFramework.h>

#include <Core/Serialization/Text/TxTSerializers.h>
#include <Core/Serialization/Text/TxTReader.h>

namespace Demos
{

ERet DemoApp::Initialize( const NEngine::LaunchConfig & engine_settings )
{
	mxDO(_asset_folder.Initialize());
	mxDO(_asset_folder.Mount(engine_settings.path_to_compiled_assets.c_str()));

#if 0
	{
		m_StateMan.handlers.add( ActionBindingT(GameActions::attack1, mxBIND_MEMBER_FUNCTION(DemoApp,attack1,*this)) );
		m_StateMan.handlers.add( ActionBindingT(GameActions::attack2, mxBIND_MEMBER_FUNCTION(DemoApp,attack2,*this)) );
		m_StateMan.handlers.add( ActionBindingT(GameActions::attack3, mxBIND_MEMBER_FUNCTION(DemoApp,attack3,*this)) );

		m_StateMan.handlers.add( ActionBindingT(GameActions::take_screenshot, mxBIND_MEMBER_FUNCTION(DemoApp,take_screenshot,*this)) );

#if 0
		m_StateMan.handlers.add( ActionBindingT(GameActions::dev_toggle_console, mxBIND_MEMBER_FUNCTION(DemoApp,dev_toggle_console,*this)) );
		m_StateMan.handlers.add( ActionBindingT(GameActions::dev_toggle_menu, mxBIND_MEMBER_FUNCTION(DemoApp,dev_toggle_menu,*this)) );
#endif
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_reload_shaders, mxBIND_MEMBER_FUNCTION(DemoApp,dev_reload_shaders,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_reset_state, mxBIND_MEMBER_FUNCTION(DemoApp,dev_reset_state,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_save_state, mxBIND_MEMBER_FUNCTION(DemoApp,dev_save_state,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_load_state, mxBIND_MEMBER_FUNCTION(DemoApp,dev_load_state,*this)) );

		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_action0, mxBIND_MEMBER_FUNCTION(DemoApp,dev_toggle_raytrace,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_action1, mxBIND_MEMBER_FUNCTION(DemoApp,dev_do_action1,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_action2, mxBIND_MEMBER_FUNCTION(DemoApp,dev_do_action2,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_action3, mxBIND_MEMBER_FUNCTION(DemoApp,dev_do_action3,*this)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::dev_action4, mxBIND_MEMBER_FUNCTION(DemoApp,dev_do_action4,*this)) );

		//_game_state_manager.handlers.add( ActionBindingT(GameActions::exit_app, mxBIND_FREE_FUNCTION(request_exit)) );
		_game_state_manager.handlers.add( ActionBindingT(GameActions::exit_app, mxBIND_MEMBER_FUNCTION(DemoApp,request_exit,*this)) );
	}
#endif

	return ALL_OK;
}

void DemoApp::Shutdown()
{
	Resources::unloadAndDestroyAll();

	_asset_folder.Unmount();
	_asset_folder.Shutdown();
}

void DemoApp::Tick()
{
	this->dev_ReloadChangedAssetsIfNeeded();
}

#if 0

void DemoApp::ToggleState( AClientState* state )
{
	//bool grabsMouse = state->GrabsMouseInput();
	//bool isMainMode = ((state->GetStateMask() & BIT(GameStates::main)) != 0);
	//bool isGuiMode = !isMainMode;
	bool grabsMouse = state->NeedsRelativeMouseMode();
//	DBGOUT("New state grabs mouse: %d",grabsMouse);

	bool isGuiMode = !grabsMouse;

	bool allowModeSwitch = true;

	if( !isGuiMode )
	{
		// allow mode switch if we are in main game mode (e.g. fly, walk)
	}
	else
	{
		// If we're in GUI mode and interacting with a window, don't switch mode:
		if( ImGui::IsMouseHoveringAnyWindow() || ImGui::IsAnyItemActive() ) {
			allowModeSwitch = false;
		}
	}

	if( allowModeSwitch )
	{
		AClientState* previousState = stateMgr.GetCurrentState();
		mxASSERT_PTR(previousState);
		if( previousState != state ) {
			stateMgr.PushState( state );
		} else {
			stateMgr.PopState();
		}

		AClientState* currentState = stateMgr.GetCurrentState();
		if( currentState ) {
//			DBGOUT("%s -> %s", previousState->DbgGetName(), currentState->DbgGetName());
			WindowsDriver::SetRelativeMouseMode(currentState->NeedsRelativeMouseMode());
		}
	}
	else
	{
//		DBGOUT("allowModeSwitch = false, isGuiMode: %d",isGuiMode);
	}
}
#endif

DemoApp::DemoApp()
{
	struct MyExceptionCallback : AExceptionCallback
	{
		TLocalString< 2046 >	m_stackTrace;

		virtual void AddStackEntry( const char* text ) override
		{
			Str::AppendS( m_stackTrace, text );
		}
		virtual void OnException() override
		{
			if( m_stackTrace.length() )
			{
				mxGetLog().VWrite(LL_Error, m_stackTrace.raw(), m_stackTrace.length());
				::MessageBoxA(NULL, m_stackTrace.raw(), "ERROR", MB_OK);
			}
		}
	};
	static MyExceptionCallback myExceptionCallback;
	g_exceptionCallback = &myExceptionCallback;
}

DemoApp::~DemoApp()
{

}

// Input bindings
void DemoApp::attack1( GameActionID action, EInputState status, float value )
{

}
void DemoApp::attack2( GameActionID action, EInputState status, float value )
{

}
void DemoApp::attack3( GameActionID action, EInputState status, float value )
{

}
void DemoApp::take_screenshot( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_toggle_console( GameActionID action, EInputState status, float value )
{
	UNDONE;
	//ToggleState( &devConsole );
}
void DemoApp::dev_toggle_menu( GameActionID action, EInputState status, float value )
{
	//ToggleState( &menuState );
}
void DemoApp::dev_reset_state( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_reload_shaders( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_toggle_raytrace( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_save_state( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_load_state( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_do_action1( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_do_action2( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_do_action3( GameActionID action, EInputState status, float value )
{
}
void DemoApp::dev_do_action4( GameActionID action, EInputState status, float value )
{
}
void DemoApp::request_exit( GameActionID action, EInputState status, float value )
{
	G_RequestExit();
}

void DemoApp::dev_ReloadChangedAssetsIfNeeded()
{
#if MX_DEVELOPER
	AssetHotReloader callback;
	_asset_folder.ProcessChangedAssets(&callback);
#endif
}

}//namespace Demos

namespace Utilities
{

ERet preallocateObjectLists( NwClump & scene_data )
{
	scene_data.CreateObjectList(Rendering::NwGeometryBuffer::metaClass(), 1024);

	scene_data.CreateObjectList(Rendering::NwMesh::metaClass(), 1024);
	//scene_data.CreateObjectList(TbShader::metaClass(), 512);
	scene_data.CreateObjectList(Rendering::Material::metaClass(), 32);

	scene_data.CreateObjectList(Rendering::MeshInstance::metaClass(), 1024);
	scene_data.CreateObjectList(Rendering::RrTransform::metaClass(), 1024);

	scene_data.CreateObjectList(Rendering::RrLocalLight::metaClass(), 16);

	return ALL_OK;
}

	mxTEMP
#if 0
ERet CreatePlane(
	MeshInstance *&_model,
	Clump * _storage
	)
{
	const float S = 1000.0f;
	const V2f UV(10,10);
	DrawVertex planeVertices[4] =
	{
		/* south-west */{ V3_Set( -S, -S, 0.0f ),  V2_To_Half2(V2_Set(    0, UV.y )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ), UByte4(), UByte4() },
		/* north-west */{ V3_Set( -S,  S, 0.0f ),  V2_To_Half2(V2_Set(    0,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ), UByte4(), UByte4() },
		/* north-east */{ V3_Set(  S,  S, 0.0f ),  V2_To_Half2(V2_Set( UV.x,    0 )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ), UByte4(), UByte4() },
		/* south-east */{ V3_Set(  S, -S, 0.0f ),  V2_To_Half2(V2_Set( UV.x, UV.y )), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f, 1.0f ), UByte4(), UByte4() },
	};
	const UINT16 planeIndices[6] = {
		0, 2, 1, 0, 3, 2,
	};

	NwMesh* mesh = NULL;
	mxDO(NwMesh::Create(
		mesh,
		_storage,
		planeVertices, mxCOUNT_OF(planeVertices), sizeof(planeVertices),
		VTX_Draw,
		planeIndices, sizeof(planeIndices),
		Topology::TriangleList
	));
//	mesh->m_materialSet = materials;

	TPtr<MeshInstance> model;
	mxDO(_storage->New(model.Ptr));
//	model->m_materialSet = materials;

	TPtr<M34f> modelXform;
	mxDO(_storage->New(modelXform.Ptr));
	*modelXform = M34_Pack( M44_Identity() );

	model->m_mesh = mesh;
	model->m_transform = modelXform;

	_model = model;

	return ALL_OK;
}
#endif
}//namespace Utilities

namespace Meshok
{

ERet ComputeVertexNormals( const TriMesh& mesh, DrawVertex *vertices, AllocatorI & scratch )
{
//		DynamicArray< V3f >	faceNormals( scratch );
//		mxDO(faceNormals.setNum(this->triangles.num()));

	const UINT numVertices = mesh.vertices.num();

	DynamicArray< V3f >	vertexNormals( scratch );
	mxDO(vertexNormals.setCountExactly(numVertices));
	Arrays::setAll(vertexNormals, V3_Zero());

	for( UINT iFace = 0; iFace < mesh.triangles.num(); iFace++ )
	{
		const TriMesh::Triangle& face = mesh.triangles[ iFace ];
		const V3f a = mesh.vertices[ face.a[0] ].xyz;
		const V3f b = mesh.vertices[ face.a[1] ].xyz;
		const V3f c = mesh.vertices[ face.a[2] ].xyz;
		const V3f faceNormal = V3_Cross( b - a, c - a );
//			faceNormals[ iFace ] = faceNormal;

		vertexNormals[ face.a[0] ] += faceNormal;
		vertexNormals[ face.a[1] ] += faceNormal;
		vertexNormals[ face.a[2] ] += faceNormal;
	}

	for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		vertexNormals[ iVertex ] = V3_Normalized( vertexNormals[ iVertex ] );
		vertices[ iVertex ].N = PackNormal( vertexNormals[ iVertex ] );
	}
	return ALL_OK;
}

}//namespace Meshok
