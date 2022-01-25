#pragma once

#define USE_JQ (0)






#include <ImGui/imgui.h>

#include <Base/Base.h>
#include <Base/Util/LogUtil.h>
#include <Base/Util/FPSTracker.h>
#include <Core/Core.h>

#if USE_JQ
//#include <jq/jq.h>
#else
#include <Core/JobSystem.h>
#endif





#include <Driver/Driver.h>
#include <Driver/Windows/ConsoleWindow.h>
#include <Graphics/graphics_device.h>
#include <Renderer/Pipelines/TiledDeferred.h>
#include <Renderer/Pipelines/Forward+.h>
#include <Renderer/Light.h>
#include <Renderer/Mesh.h>
#include <Renderer/Model.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/TxTSupport/TxTSerializers.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/DemoFramework/ImGUI_Renderer.h>
#include <Utility/DemoFramework/DemoFramework.h>
#include <Developer/EditorSupport/DevAssetFolder.h>

#include <Renderer/Pipelines/TiledCommon.h>

//#include <Renderer/Shaders/__shader_pre.h>
//#include <Renderer/Shaders/HLSL/_noise.h>
//#include <Renderer/Shaders/__shader_post.h>









struct cbStarField
{
	float	threshold;
	float	padding[3];
};

struct StarfieldSettings : public CStruct
{
	float	probability_threshold;
public:
	mxDECLARE_CLASS( StarfieldSettings, CStruct );
	mxDECLARE_REFLECTION;
	StarfieldSettings();
};





struct ProcTexGenApp : Demos::DemoApp
{
	Clump	m_clump;

	RendererGlobals		m_rendererGlobals;

	GUI_Renderer		m_gui_renderer;

	StarfieldSettings	m_settings;



	HTexture	m_dynamicTexture;

	int	m_texWidth, m_texHeight;

	TArray<R8G8B8A8>	m_temp;




	TSimpleMovingAverage<64>	m_fps;

	bool	m_enableMultithreading;
	int		m_numJobs;	// 0 == numWorkerThreads

	bool	m_useCPU;

public:
	ProcTexGenApp()
	{
		m_enableMultithreading = true;
		m_numJobs = 0;
		m_useCPU = true;
	}

	ERet Setup();

	void Shutdown();

	void Update(const double deltaSeconds) override
	{
		mxPROFILE_SCOPE("Update()");
		DemoApp::Update(deltaSeconds);
	}

	ERet BeginRender( const InputState& _state );

	ERet EndRender()
	{
		m_gui_renderer.EndFrame();
		return ALL_OK;
	}
};

float fract( float x )
{
	return x - Float_Floor(x);
}

float randomf( const V2F4& p )
{
	return fract(sin(
		V2F4_Dot(p,V2F4_Set(419.2,371.9))) * 833458.57832);
}
#if 0
/// The output is pseudo-random, in the range [0,1].
V2F4 random2f(V2F4 p)
{
	V2F4 q = V2F4_Set(
		V2F4_Dot(p,V2F4_Set(127.1,311.7)),
		V2F4_Dot(p,V2F4_Set(269.5,183.3))
	);
	return fract(V2F4_Replicate(sin(q))*V2F4_Replicate(43758.5453));
}
#endif

Vector4 ProceduralStarField(
							const StarfieldSettings& settings,
						 //const V4F4& SV_position,
						 const V2F4& UV
						 )
{
	Vector4 color = V4F4_Zero();

	// Sky Background Color
	color = Vector4_MultiplyAdd( V4F4_Set( 0.1, 0.2, 0.4, 1 ), Vector4_Replicate(UV.y), color );

	// Stars
	// Note: Choose fThreshhold in the range [0.99, 0.9999].
	// Higher values (i.e., closer to one) yield a sparser starfield.
//float fThreshhold = 0.97;
float fThreshhold = settings.probability_threshold;

	//float StarVal = Noise2d( UV );
	float StarVal = randomf( UV );

	//V2F4 rand2 = random2f(UV);
	//StarVal = rand2.x;

	if ( StarVal >= fThreshhold )
	{
		StarVal = pow( abs((StarVal - fThreshhold)/(1.0 - fThreshhold)), 9.0 );

		color = Vector4_Add( color, Vector4_Replicate(StarVal) );

		//if( rand2.y > 0.99 ) {
		//	color.r = 1;
		//}
	}

	return color;
}






#if USE_JQ

	//struct ThreadContext {};
	typedef uint64_t JobHandle;

#else

	struct MyJobData
	{
		const StarfieldSettings* settings;
		R8G8B8A8* imagePtr;
		int	startRow;
		int	numRows;
		int imageWidth;
		int imageHeight;
	};
	void MyJobFunction( const TbJobData& data, const ThreadContext& context )
	{
		const MyJobData* jobData = (MyJobData*) data.blob;

		const int W = jobData->imageWidth;
		const int H = jobData->imageHeight;
		const float invW = 1.0f / W;
		const float invH = 1.0f / H;

		for( int y = jobData->startRow; y < jobData->startRow + jobData->numRows; y++ )
		{
			const float y01 = y * invH;

			for( int x = 0; x < W; x++ )
			{
				const int i = y*W + x;

				const float x01 = x * invW;

				Vector4 vColor = ProceduralStarField(
					*jobData->settings,
					V2F4_Set(x01, y01)
				);

				//RGBAf color(x01, y01, 0, 1);
				RGBAf *color = (RGBAf*) &vColor;

				jobData->imagePtr[i].asU32 = color->ToInt32();
			}
		}
	}
#endif
