#include "proc-tex-gen.h"
#include <Engine/Engine.h>

#include <Remotery/Remotery.h>
#pragma comment( lib, "Remotery.lib" )



#if MICROPROFILE_ENABLED
	//#if __cplusplus < 201103L
	//#define MICROPROFILE_NOCXX11
	//#endif

	#define MICROPROFILE_IMPL
#endif

#include <microprofile/microprofile.h>

#if MICROPROFILE_ENABLED
	#define MICROPROFILEUI_IMPL
	//#define MICROPROFILE_WEBSERVER 0
	#define MICROPROFILE_TEXT_HEIGHT 12
	#define MICROPROFILE_TEXT_WIDTH 7
	#include <microprofile/microprofileui.h>
#endif


#if 0
// define GPU queries as NULL
uint32_t MicroProfileGpuInsertTimeStamp() { return 0; }
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey) { return 0; }
uint64_t MicroProfileTicksPerSecondGpu() { return 1; }
int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu) { return 0; }
#endif // 0

#if MICROPROFILE_ENABLED

// UI functions
static ImDrawList*  g_pImDraw = 0;
static ImVec2       g_DrawPos;
void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters)
{
    g_pImDraw->AddText( ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ), nColor, pText, pText + nNumCharacters );
}

void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType boxType )
{
    switch( boxType )
    {
    case MicroProfileBoxTypeBar:
    {
        uint32_t cul = nColor; 
        uint32_t cur = ( nColor & 0x00FFFFFF ) + 0xFF000000; 
        uint32_t clr = ( nColor & 0x00FFFFFF ) + 0x50000000; 
        uint32_t cll = ( nColor & 0x00FFFFFF ) + 0x50000000; 
        g_pImDraw->AddRectFilledMultiColor(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), cul, cur, clr, cll );
        if( nX1 - nX > 5 )
        {
            g_pImDraw->AddRect(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), 0x50000000 );
        }
        break;
    }
    case MicroProfileBoxTypeFlat:
        g_pImDraw->AddRectFilled(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), nColor );
        break;
    default:
        assert(false);
    }
}

void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
    for( uint32_t vert = 0; vert + 1 < nVertices; ++vert )
    {
        uint32_t i = 2*vert;
        ImVec2 posA( pVertices[i] + g_DrawPos.x, pVertices[i+1] + g_DrawPos.y );
        ImVec2 posB( pVertices[i+2] + g_DrawPos.x, pVertices[i+3] + g_DrawPos.y );
        g_pImDraw->AddLine( posA, posB, nColor );
    }
}
#endif




#if USE_JQ
#define JQ_IMPL
#include <jq/jq.h>
#else
#include <Core/JobSystem.h>
#endif







#include <Graphics/private/d3d_common.h>

#include <Renderer/Shader.h>
#include <Developer/TypeDatabase/TypeDatabase.h>







void SoftwarePixelShader(const StarfieldSettings& _settings, int _width, int _height, R8G8B8A8* _image, int _startRow, int _numRows)
{
	//MICROPROFILE_SCOPE(SWPixelShader);
	MICROPROFILE_SCOPEI("JQ", "a", 0);

	const float invW = 1.0f / _width;
	const float invH = 1.0f / _height;

	int lastRow = _startRow + _numRows;
	mxASSERT(lastRow <= _height);
	lastRow = smallest(lastRow, _height - 1);

	for (int y = _startRow; y < lastRow; y++)
	{
		const float y01 = y * invH;

		for (int x = 0; x < _width; x++)
		{
			const int i = y*_width + x;

			const float x01 = x * invW;

			Vector4 vColor = ProceduralStarField(
				_settings,
				V2F4_Set(x01, y01)
				);

			RGBAf *color = (RGBAf*)&vColor;
			_image[i].asU32 = color->ToInt32();
		}
	}
}










mxDEFINE_CLASS( StarfieldSettings );
mxBEGIN_REFLECTION( StarfieldSettings )
	mxMEMBER_FIELD( probability_threshold ),
mxEND_REFLECTION
StarfieldSettings::StarfieldSettings()
{
	probability_threshold = 0.9f;
}

ERet ProcTexGenApp::Setup()
{
	//SON::LoadFromFile(SETTINGS_FILE_NAME,g_settings);
	const char* pathToAssets = gUserINI->GetString("path_to_compiled_assets");
	mxDO(DemoApp::__Initialize(pathToAssets));

	mxDO(Rendering::Static_InitializeGlobals());

	Clump* commonRendererResources;
	mxDO(GetAsset(commonRendererResources, MakeAssetID("common")));
	m_rendererGlobals.Initialize(ClumpList::g_loadedClumps);


	mxDO(m_gui_renderer.Initialize());

	// Main game mode
	stateMgr.PushState( &mainMode );
//	WindowsDriver::SetRelativeMouseMode(true);



	{
		const InputState& state = WindowsDriver::GetState();

		m_texWidth = state.window.width;
		m_texHeight = state.window.height;
		//m_texWidth = 1024;
		//m_texHeight = 512;

		Texture2DDescription	desc;
		desc.format = DataFormat::RGBA8;
		desc.width = m_texWidth;
		desc.height = m_texHeight;
		desc.numMips = 1;
		//desc.dynamic = true;

		m_dynamicTexture = llgl::CreateTexture(desc);
	}


	return ALL_OK;
}

void ProcTexGenApp::Shutdown()
{
	m_gui_renderer.Shutdown();

	m_rendererGlobals.Shutdown(ClumpList::g_loadedClumps);

	DemoApp::__Shutdown();
}

ERet ProcTexGenApp::BeginRender( const InputState& _state )
{
	mxPROFILE_SCOPE("Render()");

	const float avgSecondsPerFrame = m_fps.Add(_state.deltaSeconds);
	const float avgFramesPerSecond = 1.0f / avgSecondsPerFrame;

	const int screenWidth = _state.window.width;
	const int screenHeight = _state.window.height;

	const float aspect_ratio = (float)screenWidth / (float)screenHeight;
	const float FoV_Y_degrees = DEG2RAD(60);
	const float near_clip	= 1.0f;
	const float far_clip	= 1000.0f;

	const M44F4 viewMatrix = Matrix_Identity();
	const M44F4 projectionMatrix = Matrix_Perspective(FoV_Y_degrees, aspect_ratio, near_clip, far_clip);
	const M44F4 viewProjectionMatrix = Matrix_Multiply(viewMatrix, projectionMatrix);
	const M44F4 worldMatrix = Matrix_Identity();
	const M44F4 worldViewProjectionMatrix = Matrix_Multiply(worldMatrix, viewProjectionMatrix);

	SceneView sceneView;
	sceneView.viewMatrix = viewMatrix;
	sceneView.projectionMatrix = projectionMatrix;
	sceneView.viewProjectionMatrix = viewProjectionMatrix;
	sceneView.worldSpaceCameraPos = V3_Zero();
	sceneView.viewportWidth = screenWidth;
	sceneView.viewportHeight = screenHeight;
	sceneView.nearClip = near_clip;
	sceneView.farClip = far_clip;

	RendererGlobals & globals = RendererGlobals::Get();

	{
		llgl::ViewState	viewState;
		{
			viewState.Reset();
			viewState.colorTargets[0].SetDefault();
			viewState.targetCount = 1;
			viewState.flags = llgl::ClearAll;
		}
		llgl::SetView(llgl::GetMainContext(), viewState);
	}

	{
		G_PerFrame cbPerFrame;
		ShaderGlobals::SetPerFrameConstants(
			_state.totalSeconds, _state.deltaSeconds,
			_state.mouse.x, _state.mouse.y,
			0!=(_state.mouse.buttons & BIT(MouseButton_Left)), 0!=(_state.mouse.buttons & BIT(MouseButton_Right)),
			&cbPerFrame
		);
		llgl::UpdateBuffer(llgl::GetMainContext(), globals.cbPerFrame, sizeof(cbPerFrame), &cbPerFrame);
	}
	{
		G_PerCamera cbPerView;
		M44F4 inverseProjectionMatrix;
		ShaderGlobals::SetPerViewConstants(
			sceneView.viewMatrix,
			sceneView.projectionMatrix,
			sceneView.nearClip, sceneView.farClip,
			sceneView.viewportWidth, sceneView.viewportHeight,
			sceneView.worldSpaceCameraPos,
			&cbPerView,
			&inverseProjectionMatrix
		);
		llgl::UpdateBuffer(llgl::GetMainContext(), globals.cbPerCamera, sizeof(cbPerView), &cbPerView);
	}


	int numRealJobs = 0;
	int numWorkerThreads = 0;


	if(m_useCPU)
	{
		m_temp.SetNum( m_texWidth * m_texHeight );

		float invWidth = 1.0f / m_texWidth;
		float invHeight = 1.0f / m_texHeight;

		if( m_enableMultithreading )
		{
#if USE_JQ
			numWorkerThreads = JqGetNumWorkers();
#else
			numWorkerThreads = JobMan::NumWorkerThreads();
#endif

			numRealJobs = (m_numJobs != 0) ? m_numJobs : numWorkerThreads;

			const int rowsPerJob = m_texHeight / numRealJobs;

			enum { MAX_JOBS = 16 };

			TLocalArray< JobHandle, MAX_JOBS >	spawnedJobs;

			numRealJobs = smallest(numRealJobs, MAX_JOBS);


			int currRow = 0;
			int currJob = 0;
			while( currRow < m_texHeight )
			{
				const int rowsToProcess = smallest( rowsPerJob, m_texHeight-currRow );
				{
					String64 jobName;
					Str::SPrintF(jobName, "Job_%d", currJob++);

#if USE_JQ
					const JobHandle hJob = JqAdd(
						[=]( int a, int b )
						{
							SoftwarePixelShader(
								m_settings,
								m_texWidth,
								m_texHeight,
								m_temp.ToPtr(),
								currRow,
								rowsToProcess
							);
						},
						0, -1, -1
					);
					spawnedJobs.Add(hJob);
#else
					TbJobData jobData;
					MyJobData *myData = (MyJobData*) jobData.blob;
					myData->settings = &m_settings;
					myData->imagePtr = m_temp.ToPtr();
					myData->startRow = currRow;
					myData->numRows = rowsToProcess;
					myData->imageWidth = m_texWidth;
					myData->imageHeight = m_texHeight;

					const JobHandle hJob = JobMan::Add(jobData, &MyJobFunction, JobPriority_Normal, jobName.c_str());
					spawnedJobs.Add(hJob);
#endif
				}
				currRow += rowsToProcess;
			}

//JobMan::DebugDump(LogStream(LL_Debug));

#if USE_JQ
			JqWaitAll(
				spawnedJobs.ToPtr(),
				spawnedJobs.Num()
			);
#else
			JobMan::WaitForAll();
			//JobMan::WaitForAll(
			//	spawnedJobs.ToPtr(),
			//	spawnedJobs.Num()
			//);
#endif
		}
		else
		{
			SoftwarePixelShader(
				m_settings,
				m_texWidth,
				m_texHeight,
				m_temp.ToPtr(),
				0,
				m_texHeight
			);
			//#if USE_JQ

			//MyJobData myData;
			//myData.settings = &m_settings;
			//myData.imagePtr = m_temp.ToPtr();
			//myData.startRow = 0;
			//myData.numRows = m_texHeight;
			//myData.imageWidth = m_texWidth;
			//myData.imageHeight = m_texHeight;
		}

		llgl::UpdateTexture2D(
			llgl::GetMainContext(),
			m_dynamicTexture,
			0,
			0, 0,
			m_texWidth, m_texHeight,
			m_temp.ToPtr()
		);

		TbShader* shader;
		rrDO(GetAsset(shader, MakeAssetID("screen_copy"), &m_clump));
		{
			FxSamplerState* pointSampler;
			mxDO(GetByName(ClumpList::g_loadedClumps, "Point", pointSampler));
			mxDO(FxSlow_SetSampler(shader, "s_sourceTexture", pointSampler->handle));

			mxDO(FxSlow_SetInput(shader, "t_sourceTexture", llgl::AsInput(m_dynamicTexture)));
		}
		mxDO(ARenderLoop::DrawFullScreenTriangle(shader));
	}
	else // !m_useCPU
	{
		TbShader* shader;
		rrDO(GetAsset(shader, MakeAssetID("procedural_starfield.shader"), &m_clump));
		{
			cbStarField params;
			params.threshold = m_settings.probability_threshold;
			mxDO(FxSlow_UpdateUBO(llgl::GetMainContext(), shader, "cbStarfield", &params, sizeof(params)));
		}
		mxDO(ARenderLoop::DrawFullScreenTriangle(shader));
	}




	AClientState* currentState = stateMgr.GetCurrentState();
	//if( currentState != &mainMode )
	{
		m_gui_renderer.UpdateInputs();
	}

	m_gui_renderer.BeginFrame();
	{
		ImGui::Checkbox("Use CPU", &m_useCPU);
		if( m_useCPU )
		{
			ImGui::Checkbox("Toggle threading", &m_enableMultithreading);
			if( m_enableMultithreading )
			{
				ImGui::InputInt("Num.Jobs", &m_numJobs);
				ImGui::Text("Using %d jobs (%d workers)", numRealJobs, numWorkerThreads);
			}
		}

		ImGui::Text("Average FPS: %d", (int)avgFramesPerSecond);
		ImGui_DrawPropertyGrid(&m_settings, mxCLASS_OF(m_settings), "Settings");
	}

	return ALL_OK;
}

ERet MyEntryPoint( const String& root_folder )
{
	ProcTexGenApp	app;

	{
		InputBindings& input = app.m_bindings;
		mxDO(input.handlers.Reserve(64));
		mxDO(input.contexts.Reserve(4));

		{
			String512 path_to_key_bindings( root_folder );
			Str::AppendS( path_to_key_bindings, "Config/input_mappings.son" );
			mxDO(SON::LoadFromFile( path_to_key_bindings.c_str(), input.contexts ));
		}
	}


	WindowsDriver::Settings	options;
	//options.window_width = 1440;
	//options.window_height = 1024;
	//options.window_width = 1280;
	//options.window_height = 1024;
	options.window_width = 1024;
	options.window_height = 768;
	options.window_x = 50;
	options.window_y = 50;
	mxDO(WindowsDriver::Initialize(options));


	llgl::Settings settings;
	settings.window = WindowsDriver::GetNativeWindowHandle();
	mxDO(llgl::Initialize(settings));


	mxDO(app.Setup());


	InputState inputState;
	while( WindowsDriver::BeginFrame( &inputState, &app ) )
	{
//#if MICROPROFILE_ENABLED
//		MICROPROFILE_SCOPE(MainLoop);
//#endif

		app.Dev_ReloadChangedAssets();

		if( inputState.hasFocus )
		{
			app.Update(inputState.deltaSeconds);
		}
		else
		{
			//DBGOUT("[%d] Lost focus", inputState.frameNumber);
			mxSleepMilliseconds(100);
		}

		app.BeginRender(inputState);

#if MICROPROFILE_ENABLED
		{
			MicroProfileFlip();

            ImGui::SetNextWindowSize(ImVec2(inputState.window.width,inputState.window.height), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiSetCond_FirstUseEver);
			if( ImGui::Begin("Microprofile") )
			{
				g_pImDraw = ImGui::GetWindowDrawList();
				g_DrawPos = ImGui::GetCursorScreenPos();

				ImVec2 sizeForMicroDraw = ImGui::GetContentRegionAvail();
				ImGui::InvisibleButton("canvas", sizeForMicroDraw);
				if (ImGui::IsItemHovered())
				{
					MicroProfileMouseButton(ImGui::GetIO().MouseDown[0], ImGui::GetIO().MouseDown[1]);
				}
				else
				{
					MicroProfileMouseButton(0, 0);
				}
				MicroProfileMousePosition((uint32_t)(ImGui::GetIO().MousePos.x - g_DrawPos.x), (uint32_t)(ImGui::GetIO().MousePos.y - g_DrawPos.y), (int)ImGui::GetIO().MouseWheel);
				MicroProfileDraw(sizeForMicroDraw.x, sizeForMicroDraw.y);
				ImGui::End();
			}
		}
#endif // MICROPROFILE_ENABLED

		app.EndRender();

		llgl::NextFrame();

		mxSleepMilliseconds(1);

		mxPROFILE_INCREMENT_FRAME_COUNTER;
	}

	app.Shutdown();

	WindowsDriver::Shutdown();


	return ALL_OK;
}

int main(int /*_argc*/, char** /*_argv*/)
{
	EngineSettings settings;
	Scoped_SetupEngine	setupEngine(settings);


#if MICROPROFILE_ENABLED
	// Set up Microprofile
	MicroProfileToggleDisplayMode();
	MicroProfileInitUI();
	MicroProfileOnThreadCreate("Main");
#endif

#if USE_JQ
	JqStart(8-2);
#endif

	//EdTypeDatabase	typeDb;
	//typeDb.PopulateFromTypeRegistry();
	//SON::SaveToFile(typeDb, "R:/engine_types.son");

	// Create the main instance of Remotery.
	// You need only do this once per program.
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);


	String512	root_folder;
	GetRootFolder(root_folder);

	MyEntryPoint(root_folder);


	// Destroy the main instance of Remotery.
	rmt_DestroyGlobalInstance(rmt);

#if USE_JQ
	JqStop();
#endif
	return 0;
}
