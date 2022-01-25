// Sky rendering.
#pragma once

#include <Core/Assets/AssetManagement.h>
//#include <Renderer/resources/Texture.h>
#include <Rendering/Public/Globals.h>

namespace Rendering
{
ERet DrawSkybox_TriStrip_NoVBIB(
	NGpu::NwRenderContext & render_context,
	const NwRenderState32& states,
	const HProgram program
	);

ERet renderSkybox_Analytic(
	NGpu::NwRenderContext & context
	, const NGpu::LayerID viewId
);

ERet renderSkybox_EnvironmentMap(
	NGpu::NwRenderContext & context
	, const NGpu::LayerID viewId
	, const AssetID& environmentMapId
);

struct SkyDome: CStruct
{
	TPtr< NwTexture >	m_skyTexture;	//!< sky texture
	HBuffer	m_VB;
	HBuffer	m_IB;
	U32	m_numVertices;
	U32	m_numIndices;

public:
	mxDECLARE_CLASS( SkyDome, CStruct );
	mxDECLARE_REFLECTION;
	SkyDome();

	ERet Initialize( const AssetID& _textureId, NwClump * _storage );
	void Shutdown();

	ERet Render(
		NGpu::NwRenderContext & context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		const AssetID& _textureId,
		NwClump * _storage
	);

public:
};

//namespace SkyModel
struct SkyModel_ClearSky: CStruct
{
public:
	mxDECLARE_CLASS( SkyModel_ClearSky, CStruct );
	mxDECLARE_REFLECTION;
	SkyModel_ClearSky();

	ERet Render(
		NGpu::NwRenderContext & context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		NwClump * _storage
	);
};


struct SkyModel_Preetham: CStruct
{
	struct Parameters
	{
		V3f A, B, C, D, E;
		V3f Z;
	};

public:
	mxDECLARE_CLASS( SkyModel_Preetham, CStruct );
	mxDECLARE_REFLECTION;
	SkyModel_Preetham();

	ERet Initialize( const AssetID& _textureId, NwClump * _storage );
	void Shutdown();

	ERet Render(
		NGpu::NwRenderContext & context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		NwClump * _storage
	);

	static Parameters compute(float sunTheta, float turbidity, float normalizedSunY);
};

struct SkyModel_Hosek: CStruct
{
	struct Parameters {
		V3f A, B, C, D, E, F, G, H, I;
		V3f Z;
	};

public:
	mxDECLARE_CLASS( SkyModel_Hosek, CStruct );
	mxDECLARE_REFLECTION;
	SkyModel_Hosek();

	ERet Initialize( const AssetID& _textureId, NwClump * _storage );
	void Shutdown();

	static Parameters compute(float sunTheta, float turbidity, float normalizedSunY);
};

struct Planet_Atmosphere: CStruct
{
//	float m_fRadius;		//!< The radius of the planet
//	float m_fMaxHeight;		//!< The max height offset from that radius
//	float m_fAtmosphereRadius;
	float m_fInnerRadius;	//!< The inner (planetary) radius
	float m_fOuterRadius;	//!< The outer (atmosphere) radius

/// R = the wavelength of the Red color channel, default = 650 nm
/// G = the wavelength of the Green color channel, default = 570 nm
/// B = the wavelength of the Blue color channel, default = 475 nm
	float m_fWavelength[3];

	float m_fESun;	//!< the Sun's brightness ESun
	float m_fKr;	//!< the Rayleigh scattering constant Kr
	float m_fKm;	//!< the Mie scattering constant Km
	float m_fG;		//!< the Mie phase assymetry constant g

	//TPtr< TbShader > m_shSkyFromSpace;
	//TPtr< TbShader > m_shSkyFromAtmosphere;
	//TPtr< TbShader > m_shGroundFromSpace;
	//TPtr< TbShader > m_shGroundFromAtmosphere;
	//TPtr< TbShader > m_shSpaceFromSpace;
	//TPtr< TbShader > m_shSpaceFromAtmosphere;

public:
	mxDECLARE_CLASS( Planet_Atmosphere, CStruct );
	mxDECLARE_REFLECTION;
	Planet_Atmosphere();

	ERet Initialize( const AssetID& _textureId, NwClump * _storage );
	void Shutdown();

	ERet Render(
		NGpu::NwRenderContext & context,
		const RenderPath& _path,
		const NwCameraView& _camera,
		NwClump * _storage
	);
};

}//namespace Rendering
