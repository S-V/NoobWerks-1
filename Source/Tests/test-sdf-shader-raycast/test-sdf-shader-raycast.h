// Renders a Signed Distance Field (SDF) using ray marching in a shader.
#pragma once

#include <xnamath.h>

#include <Renderer/Pipelines/TiledDeferred.h>
#include <Renderer/Pipelines/Simple.h>

#include <Utility/DemoFramework/DemoFramework.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/BVH.h>

struct AtmosphereParameters : CStruct {
	V3f		f3SunDirection;     //!< the direction to the sun (normalized)
	float	fSunIntensity;		//!< intensity of the sun, default = 22
	float	fInnerRadius;		//!< radius of the planet in meters, default = 6371e3
	float	fOuterRadius;		//!< radius of the atmosphere in meters, default = 6471e3
	V3f		f3RayleighSctr;		//!< Rayleigh scattering coefficient ("Beta R") in m^-1, default = ( 5.5e-6, 13.0e-6, 22.4e-6 )
	float	fRayleighHeight;	//!< Rayleigh scale height in meters, default = 8e3
	V3f		f3MieSctr;			//!< Mie scattering coefficient ("Beta M") in m^-1, default = 21e-6
	float	fMieHeight;			//!< Mie scale height in meters, default = 1.2e3
	/// The Mie phase function includes a term g (the Rayleigh phase function doesn't)
	/// which controls the anisotropy of the medium. Aerosols exhibit a strong forward directivity.
	float	fMie;	//!< Mie preferred scattering direction (aka "mean cosine"), default = 0.758
public:
	mxDECLARE_CLASS(AtmosphereParameters, CStruct);
	mxDECLARE_REFLECTION;
	AtmosphereParameters() {
		this->SetEarth();
	}
	void SetEarth() {
		this->fSunIntensity		= 20;		//!< intensity of the sun
		this->fInnerRadius		= 6371e3;	//!< radius of the planet in meters
		this->fOuterRadius		= 6471e3;	//!< radius of the atmosphere in meters

		/// Rayleigh scattering coefficient, default = ( 5.5e-6, 13.0e-6, 22.4e-6 )
		//this->f3RayleighSctr	= V3f(5.5e-6, 13.0e-6, 22.4e-6);	// original

		// The sea level scattering coefficients from "Efficient Rendering of Atmospheric Phenomena", by Riley et al. and "Precomputed Atmospheric Scattering" by Bruneton et al.
		this->f3RayleighSctr	= V3f(5.8e-6, 13.5e-6, 33.1e-6);	//

		this->fRayleighHeight	= 8e3;		//!< Rayleigh scale height in meters, default = 8e3
		this->f3MieSctr			= V3f(21e-6);	//!< Mie scattering coefficient, default = 21e-6
		this->fMieHeight		= 1.2e3;	//!< Mie scale height in meters, default = 1.2e3
		this->fMie				= 0.758;	//!< Mie preferred scattering direction (aka "mean cosine"), default = 0.758
	}
};

struct TestApp_RaycastSDF : Demos::DemoApp
{
	struct AppSettings : CStruct
	{
		AtmosphereParameters	atmosphere;

	public:
		mxDECLARE_CLASS(AppSettings, CStruct);
		mxDECLARE_REFLECTION;
		AppSettings()
		{
		}
	};

	AppSettings			m_settings;

	// Scene management
	PrimitiveBatcher	m_debugDraw;
	AxisArrowGeometry	m_debugAxes;
	Clump			m_sceneData;
	FlyingCamera	camera;

public:
	virtual ERet Initialize( const EngineSettings& _settings ) override;
	virtual void Shutdown() override;
	virtual void Tick( const InputState& inputState ) override;
	virtual ERet Draw( const InputState& inputState ) override;

private:
	enum EViewIDs {
		ViewID_FrameStart = 0,
		ViewID_TestShader,
		ViewID_DebugLines,
		ViewID_ImGUI,
	};
};
