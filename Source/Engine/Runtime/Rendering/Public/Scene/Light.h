// Light classes
#pragma once

#include <Rendering/Public/Scene/Entity.h>

/*
=======================================================================

	Dynamic lights

=======================================================================
*/

namespace Rendering
{

///	ELightType - enumerates all allowed light types.
enum ELightType
{
	//<= Global lights start here
	Light_Directional = 0,	// e.g.: sun light

	//<= Local lights start here
	//Light_Local,
	Light_Point,	// e.g.: light from explosion
	Light_Spot,		// e.g.: flash light

	//LT_Line,		// e.g.: laser beams

	//<= User-defined light types start here
	//Light_UserType0,
	NumLightTypes,

	LIGHT_TYPE_BITS = 3
};
mxDECLARE_ENUM( ELightType, U32, LightType );

struct LightFlags {
	enum Enum {
		// Is the light enabled?
		//Enabled	= BIT(0),
		// Does the light cast dynamic shadows? (Set if the light causes the affected geometry to cast shadows.)
		Shadows	= BIT(1),

		Dummy1	= BIT(2),
		Dummy2	= BIT(3),
		Dummy3	= BIT(4),
		//Dummy4	= BIT(5),
		//Dummy5	= BIT(6),
		//Dummy6	= BIT(7),
		//Dummy7	= BIT(8),
		//Dummy8	= BIT(9),
		//Dummy9	= BIT(10),
		//Dummy10	= BIT(11),

		//Default	= Enabled|Shadows
		Default	= 0
	};
};
mxDECLARE_FLAGS( LightFlags::Enum, U32, FLightFlags );

#if 0
/*
-----------------------------------------------------------------------------
	RrDirectionalLight
	represents a directional (infinite) light source.
-----------------------------------------------------------------------------
*/
mxREMOVE_THIS
struct RrDirectionalLight: CStruct
{
	V3f			direction;	//!< normalized light direction in world space
	FLightFlags	flags;

	V3f			color;		//!< diffuse color
	F32			pad0;
	//RGBAf		m_backColor;

	/// Flat ambient lighting color.
	V3f			ambientColor;	//!< ambient color is usually equal to the sky color
	F32			pad1;

	/// Directional light cascaded shadow parameters.
	V4f			cascadeSplits;	//!< PSSM/CSM cascade splits
	float		shadowDepthBias;
	float		shadowFadeDistance;

public:
	mxDECLARE_CLASS( RrDirectionalLight, CStruct );
	mxDECLARE_REFLECTION;
	RrDirectionalLight();
};
#endif

/*
-----------------------------------------------------------------------------
	RrLocalLight

	represents a localized dynamic light source in a scene;
	this structure should be as small as possible
-----------------------------------------------------------------------------
*/
mxPREALIGN(16) struct RrLocalLight: CStruct
{
	//V4f	position_radius;	// xyz = light position in view-space, z = light range
	//V4f	diffuse_falloff;	// rgb = diffuse color, a = falloff

	// common light data
	V3f		position;	//!< xyz = light position in view-space, z = light range
	float	radius;
	V3f		color;	//!< rgb = diffuse color, a = falloff
	float	brightness;	//!<

	LightType		m_lightType;	//!< ELightType
	FLightFlags		m_flags;

	// spot light data
	V3f		m_spotDirection;	//!< normalized axis direction in world space
	/// x = cosTheta (cosine of half inner cone angle), y = cosPhi (cosine of half outer cone angle)
	V2f		m_spotAngles;

	/// nudge a bit to reduce self-shadowing (there'll be shadow acne on all surfaces without this bias)
	F32		m_shadowDepthBias;

	//// for light flares
	//NwTexture::Ref	m_billboard;
	//F32				m_billboardSize;

	//// spot light projector
	//NwTexture::Ref	m_projector;
	F32		m_projectorIntensity;

	HRenderEntity	render_entity_handle;	// for visibility culling

public:
	mxDECLARE_CLASS( RrLocalLight, CStruct );
	mxDECLARE_REFLECTION;
	RrLocalLight();

	/// returns world-space position
	const V3f& GetOrigin() const;
	void SetOrigin( const V3f& newPos );

	/// range of influence
	FLOAT GetRadius() const;
	void SetRadius( FLOAT newRadius );

	// Direction that the light is pointing in world space.
	void SetDirection( const V3f& newDirection );
	const V3f& GetDirection() const;

	/// Sets the apex angles for the spot light which determine the light's angles of illumination.
	/// theta - angle, in radians, of a spotlight's inner cone - that is, the fully illuminated spotlight cone.
	/// This value must be in the range from 0 through the value specified by Phi.
	///
	void SetInnerConeAngle( FLOAT theta );
	FLOAT GetInnerConeAngle() const;

	/// phi - angle, in radians, defining the outer edge of the spotlight's outer cone. Points outside this cone are not lit by the spotlight.
	/// This value must be between 0 and pi.
	///
	void SetOuterConeAngle( FLOAT phi );
	FLOAT GetOuterConeAngle() const;

	/// Set projective texture blend factor ('factor' must be in range [0..1]).
	void SetProjectorIntensity( FLOAT factor = 0.5f );
	FLOAT GetProjectorIntensity() const;

	void SetShadowDepthBias( FLOAT f );
	FLOAT GetShadowDepthBias() const;

	FLOAT CalcBottomRadius() const;

	bool DoesCastShadows() const;
	void SetCastShadows( bool bShadows );

	//bool IntersectsSphere( const Sphere& sphere ) const;

	M44f BuildSpotLightViewMatrix() const;
	M44f BuildSpotLightProjectionMatrix() const;
};

#if 0
TPtr< WorldMatrixCM >	m_transform;	// pointer to local-to-world transform

/*
-----------------------------------------------------------------------------
	RrLocalLight

	represents a localized dynamic light source in a scene;

	this structure should be as small as possible
-----------------------------------------------------------------------------
*/
mxALIGN_16(struct RrLocalLight): public rxLight
{
	// common light data
	V4f			m_position;		// light position in world-space
	V4f			m_diffuseColor;	// rgb - color, a - light intensity
	V4f			m_radiusInvRadius;	// x - light radius, y - inverse light range
	V4f			m_specularColor;
	// spot light data
	V4f			m_spotDirection;	// normalized axis direction in world space
	// x = cosTheta (cosine of half inner cone angle), y = cosPhi (cosine of half outer cone angle)
	V4f			m_spotAngles;



	LightFlags		m_flags;
	LightType		m_lightType;	// ELightType



public:
	mxDECLARE_CLASS(RrLocalLight,rxLight);
	mxDECLARE_REFLECTION;

	RrLocalLight();

	bool EnclosesView( const rxSceneContext& sceneContext );



	mxIMPLEMENT_COMMON_PROPERTIES(RrLocalLight);
};

//
//	rxPointLight - is a point source.
//
//	The light has a position in space and radiates light in all directions.
//


//
//	rxSpotLight - is a spotlight source.
//
//	This light is like a point light,
//	except that the illumination is limited to a cone.
//	This light type has a direction and several other parameters
//	that determine the shape of the cone it produces.
//


/*
-----------------------------------------------------------------------------
	SDynLight

	represents a visible dynamic light source which can cast a shadow;

	this structure should be as small as possible
-----------------------------------------------------------------------------
*/
mxALIGN_16(struct SDynLight)
{
	V4f		posAndRadius;	// xyz = light position in view-space, z = light range
	V4f		diffuseAndInvRadius;	// rgb = diffuse color, a = inverse range
	V4f		falloff;	// x - falloff start, y - falloff width
	float4x4	viewProjection;
};
#endif


/// represents a directional (infinite) light source.
struct RrDirectionalLight: CStruct
{
	V3f			direction;	//!< normalized light direction in world space
	F32			pad0;

	V3f			color;		//!< diffuse color
	F32			pad1;
	//RGBAf		m_backColor;

	/// Flat ambient lighting color.
	V3f			ambientColor;	//!< ambient color is usually equal to the sky color
	F32			pad2;

	/// Directional light cascaded shadow parameters.

public:
	mxDECLARE_CLASS( RrDirectionalLight, CStruct );
	mxDECLARE_REFLECTION;
	RrDirectionalLight();

	bool isOk() const
	{
		return direction.isNormalized();
	}
};


}//namespace Rendering
