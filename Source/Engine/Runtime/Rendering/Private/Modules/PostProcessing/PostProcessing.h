#pragma once

#include <Core/Assets/AssetManagement.h>
//#include <Renderer/resources/Texture.h>
#include <Rendering/Public/Globals.h>

namespace Rendering
{

/// Screen space ambient obscurance effect
struct AmbientObscurance: CStruct
{
	/// Degree of darkness produced by the effect.
	float intensity;

	/// Radius of sample points, which affects extent of darkened areas.
	float	radius;	//!< should always be > 0

	/// Number of sample points, which affects quality and performance.
	enum SampleCount { Lowest, Low, Medium, High, Custom };
	/// Number of sample points, which affects quality and performance.
	SampleCount sampleCount;
        
	/// Halves the resolution of the effect to increase performance.
	bool downsampling;


public:
	mxDECLARE_CLASS( AmbientObscurance, CStruct );
	mxDECLARE_REFLECTION;
	AmbientObscurance();

	ERet Initialize( NwClump * _storage );
	void Shutdown();

	ERet Render(
		NGpu::NwRenderContext & context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		NwClump * _storage
	);

public:
};


struct HDR_Renderer: CStruct
{
	enum { LUMINANCE_RT_SIZE = 128 };//512 };//1024

	// Float format will change the bandwidth needed.
	//static const NwDataFormat::Enum LUMINANCE_RT_FORMAT = NwDataFormat::R32F;
	static const NwDataFormat::Enum LUMINANCE_RT_FORMAT = NwDataFormat::R16F;

#if 0
	// these render targets hold gray-scale values
	HColorTarget	initialLuminance;	//!< HDR scene color -> gray-scale luminance
	HColorTarget	adaptedLuminance[2];//!< luminance values from previous frames
	UINT			iCurrLuminanceRT;	//!< index of current RT with average luminance

public:
	mxDECLARE_CLASS( HDR_Renderer, CStruct );
	mxDECLARE_REFLECTION;
	HDR_Renderer();

	ERet Initialize( Clump * _storage );
	void Shutdown();

	ERet Render(
		HContext context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		Clump * _storage
	);

public:
#endif
};

}//namespace Rendering
