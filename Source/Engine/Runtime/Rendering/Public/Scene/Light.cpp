#include <Base/Base.h>
#pragma hdrstop
#include <Base/Math/Matrix/M44f.h>
#include <GPU/Public/graphics_config.h>
#include <Rendering/Public/Scene/Light.h>


namespace Rendering
{
mxBEGIN_REFLECT_ENUM(LightType)
	mxREFLECT_ENUM_ITEM(Directional, Light_Directional),
	mxREFLECT_ENUM_ITEM(Point, Light_Point),
	mxREFLECT_ENUM_ITEM(Spot, Light_Spot),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( FLightFlags )
//mxREFLECT_BIT( Enabled, LightFlags::Enabled ),
mxREFLECT_BIT( Shadows, LightFlags::Shadows ),
mxREFLECT_BIT( Dummy1, LightFlags::Dummy1 ),
mxREFLECT_BIT( Dummy2, LightFlags::Dummy2 ),
mxREFLECT_BIT( Dummy3, LightFlags::Dummy3 ),
//mxREFLECT_BIT( LightFlags, Dummy4 ),
//mxREFLECT_BIT( LightFlags, Dummy5 ),
//mxREFLECT_BIT( LightFlags, Dummy6 ),
//mxREFLECT_BIT( LightFlags, Dummy7 ),
//mxREFLECT_BIT( LightFlags, Dummy8 ),
//mxREFLECT_BIT( LightFlags, Dummy9 ),
//mxREFLECT_BIT( LightFlags, Dummy10 ),
mxEND_FLAGS

#if 0
mxDEFINE_CLASS( RrDirectionalLight );
mxBEGIN_REFLECTION( RrDirectionalLight )
	mxMEMBER_FIELD(color),
	mxMEMBER_FIELD(ambientColor),
	mxMEMBER_FIELD(direction),
	mxMEMBER_FIELD(flags),
	mxMEMBER_FIELD(cascadeSplits),
	mxMEMBER_FIELD(shadowDepthBias),
	mxMEMBER_FIELD(shadowFadeDistance),	
mxEND_REFLECTION
RrDirectionalLight::RrDirectionalLight()
{
	color = V3_SetAll(1);
	ambientColor = V3_SetAll(0.1f);
	direction = V3_Set(0, 0, -1);	// down

	//cascadeSplits = V4f::set( 0.125f, 0.25f, 0.5f, 1.0f );
	cascadeSplits = V4f::set( 0.05f, 0.2f, 0.5f, 1.0f );

	shadowDepthBias = 0.001f;
	shadowFadeDistance = 500.0f;

	flags = LightFlags::Default;
}
#endif
/*
-----------------------------------------------------------------------------
	RrLocalLight
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(RrLocalLight);
mxBEGIN_REFLECTION(RrLocalLight)
	//mxMEMBER_FIELD( position_radius ),
	//mxMEMBER_FIELD( diffuse_falloff ),
	mxMEMBER_FIELD( position ),
	mxMEMBER_FIELD( radius ),
	mxMEMBER_FIELD( color ),
	mxMEMBER_FIELD( brightness ),

	mxMEMBER_FIELD( m_flags ),
	mxMEMBER_FIELD( m_lightType ),
mxEND_REFLECTION

RrLocalLight::RrLocalLight()
{
	//position_radius = V4f::set(0,0,0,1);
	//diffuse_falloff = V4f::set(1,1,1,1);
	position = V3_Set(0,0,0);
	radius = 1.0f;
	color = V3_Set(1,1,1);
	brightness = 1;

	m_flags = LightFlags::Default;
	m_lightType = ELightType::Light_Point;

	//m_shadowDepthBias = -0.0005f;
	m_shadowDepthBias = -0.0001f;	//<- for lights with large radius
	//m_shadowDepthBias = 0;

//	m_billboardSize = 1.0f;
	m_projectorIntensity = 0.5f;
}

const V3f& RrLocalLight::GetOrigin() const
{
	return position;
}

void RrLocalLight::SetOrigin( const V3f& newPos )
{
	position = newPos;
}

FLOAT RrLocalLight::GetRadius() const
{
	return radius;
}

void RrLocalLight::SetRadius( FLOAT newRadius )
{
	mxASSERT(newRadius > 0.0f);
	radius = newRadius;
	//const FLOAT invRadius = 1.0f / newRadius;
	//m_radiusInvRadius = XMVectorSet( newRadius, invRadius, 0.0f, 0.0f );
}

void RrLocalLight::SetDirection( const V3f& newDirection )
{
	m_spotDirection = newDirection;
}

const V3f& RrLocalLight::GetDirection() const
{
	return m_spotDirection;
}

void RrLocalLight::SetInnerConeAngle( FLOAT theta )
{
	mxASSERT2( 0.0f < theta && theta < this->GetOuterConeAngle(), "spotlight inner angle must be in range [0..outerAngle]" );
	FLOAT cosTheta = mmCos( theta * 0.5f );
	m_spotAngles.x = cosTheta;
}

FLOAT RrLocalLight::GetInnerConeAngle() const
{
	FLOAT cosTheta = m_spotAngles.x;
	FLOAT innerAngle = mmACos(cosTheta) * 2.0f;
	return innerAngle;
}

void RrLocalLight::SetOuterConeAngle( FLOAT phi )
{
	mxASSERT2( 0.0f < phi && phi < mxPI, "spotlight outer angle must be in range [0..PI]" );
	FLOAT cosPhi = mmCos( phi * 0.5f );
	m_spotAngles.y = cosPhi;
	//FLOAT	innerAngle = this->GetInnerConeAngle();
	//innerAngle = minf( innerAngle, phi-0.001f );
	//this->SetInnerConeAngle(innerAngle);
}

FLOAT RrLocalLight::GetOuterConeAngle() const
{
	FLOAT cosTheta = m_spotAngles.y;
	FLOAT outerAngle = 2.0f * mmACos( cosTheta );
	return outerAngle;
}

FLOAT RrLocalLight::CalcBottomRadius() const
{
	FLOAT cosPhi = m_spotAngles.y;
	FLOAT halfOuterAngle = mmACos( cosPhi );
	FLOAT bottomRadius = this->GetRadius() * mmTan( halfOuterAngle );
	return bottomRadius;
}

void RrLocalLight::SetProjectorIntensity( FLOAT factor )
{
	m_projectorIntensity = factor;
}

FLOAT RrLocalLight::GetProjectorIntensity() const
{
	return m_projectorIntensity;
}

void RrLocalLight::SetShadowDepthBias( FLOAT f )
{
	m_shadowDepthBias = f;
}

FLOAT RrLocalLight::GetShadowDepthBias() const
{
	return m_shadowDepthBias;
}

bool RrLocalLight::DoesCastShadows() const
{
	return m_flags & LightFlags::Shadows;
}

void RrLocalLight::SetCastShadows( bool bShadows )
{
	setbit_cond( m_flags.raw_value, bShadows, LightFlags::Shadows );
}

M44f RrLocalLight::BuildSpotLightViewMatrix() const
{
	const V3f lightPositionWS = position;
	const V3f lightDirectionWS = m_spotDirection;

	// calculate right and up vectors for building a view matrix
    V3f RightDirection = V3_Cross( lightDirectionWS, V3_UP );
	float len = V3_Length( RightDirection );
	// prevent singularities when light direction is nearly parallel to 'up' direction
	if( len < 1e-3f ) {
		RightDirection = V3_RIGHT;	// light direction is too close to the UP vector
	} else {
		RightDirection /= len;
	}

	const V3f UpDirection = V3_Cross( RightDirection, lightDirectionWS );

	const M44f	lightViewMatrix = M44_BuildView(
		RightDirection,
		lightDirectionWS,
		UpDirection,
		lightPositionWS
	);

	return lightViewMatrix;
}

M44f RrLocalLight::BuildSpotLightProjectionMatrix() const
{
	FLOAT fovY = this->GetOuterConeAngle();
	FLOAT aspect = 1.0f;
	FLOAT farZ = this->GetRadius();
	FLOAT nearZ = minf( 0.1f, farZ * 0.5f );
	const M44f	projection_matrix = M44_Perspective( fovY, aspect, nearZ, farZ );
	return projection_matrix;
}


#if 0
//---------------------------------------------------------------------------

static
mxFORCEINLINE
FASTBOOL Point_Light_Encloses_Eye( const RrLocalLight& light, const rxSceneContext& view )
{
	mxOPTIMIZE("vectorize:");
#if 0
	FLOAT distanceSqr = (light.GetOrigin() - view.GetOrigin()).LengthSqr();
	return distanceSqr < squaref( light.GetRadius() + view.nearZ + 0.15f );
#else
	//FLOAT distance = (light.GetOrigin() - view.GetOrigin()).LengthFast();
	//return distance < light.GetRadius() + view.nearZ;
	V4f	lightVec = XMVectorSubtract( light.m_position, view.invViewMatrix.r[3] );
	V4f	lightVecSq = XMVector3LengthSq( lightVec );
	float	distanceSq = XMVectorGetX( lightVecSq );
	return distanceSq < squaref( light.GetRadius() + view.nearClip );
#endif
}

static
mxFORCEINLINE
FASTBOOL Spot_Light_EnclosesView( const RrLocalLight& light, const rxSceneContext& view )
{
#if 0

	V3f  origin( this->GetOrigin() );	// cone origin

	V3f  dir( GetDirection() );	// direction of cone axis
	//mxASSERT( dir.IsNormalized() );

	FLOAT t = dir * ( point - origin );

	if ( t < 0 || t > GetRange() ) {
		return FALSE;
	}

	FLOAT r = t * ( bottomRadius * GetInvRange() );	// cone radius at closest point

	// squared distance from the point to the cone axis
	FLOAT distSq = (( origin + dir * t ) - point).LengthSqr();

	return ( distSq < squaref(r) );

#else

	V3f  origin( light.GetOrigin() );

	V3f  d( view.GetEyePosition() - origin );

	FLOAT sqrLength = d.LengthSqr();

	d *= InvSqrt( sqrLength );

	FLOAT x = d * light.GetDirection();

	FLOAT cosPhi = as_vec4(light.m_spotAngles).y;

	return ( x > 0.0f )
		&& ( x >= cosPhi )
		&& ( sqrLength < squaref(light.GetRadius()) );

#endif
}

/*
-----------------------------------------------------------------------------
	RrLocalLight
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(RrLocalLight);
mxBEGIN_REFLECTION(RrLocalLight)
	mxMEMBER_FIELD( m_position )
	mxMEMBER_FIELD( m_diffuseColor )
	mxMEMBER_FIELD( m_radiusInvRadius )
	mxMEMBER_FIELD( m_specularColor )

	mxMEMBER_FIELD( m_spotDirection )
	mxMEMBER_FIELD( m_spotAngles )

	mxMEMBER_FIELD( m_shadowDepthBias )



	mxMEMBER_FIELD( m_billboard )
	mxMEMBER_FIELD( m_billboardSize )

	mxMEMBER_FIELD2( m_projector,	Projector_Texture,	Field_NoDefaultInit )
	mxMEMBER_FIELD( m_projectorIntensity )
mxEND_REFLECTION


RrLocalLight::RrLocalLight()
{
	m_position = g_XMIdentityR3;
	m_diffuseColor = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
	m_radiusInvRadius = XMVectorSet( 1.0f, 1.0f, 0.0f, 0.0f );
	m_specularColor = g_XMZero;

	m_spotDirection = XMVectorSet( 0.0f, -1.0f, 0.0f, 0.0f );
	this->SetOuterConeAngle(90.0f);
	this->SetInnerConeAngle(45.0f);

	//m_shadowDepthBias = -0.0005f;
	m_shadowDepthBias = -0.0001f;	//<- for lights with large radius

	m_flags = LF_All;
	m_lightType = ELightType::Light_Point;

	m_billboardSize = 1.0f;
	m_projectorIntensity = 0.5f;
}

bool RrLocalLight::EnclosesView( const rxSceneContext& sceneContext )
{
	switch( m_lightType )
	{
	case Light_Point :
		return Point_Light_Encloses_Eye( *this, sceneContext );

	case Light_Spot :
		return Spot_Light_EnclosesView( *this, sceneContext );

	default:
		Unreachable;
	}
	return false;
}


static
bool Point_Light_Intersects_Sphere( const RrLocalLight& light, const Sphere& sphere )
{
	// LengthSqr( centerA - centerB ) <= Sqr(radiusA + radiusB)
	return (as_vec3(light.m_position) - sphere.center).LengthSqr() <= squaref(light.GetRadius() + sphere.Radius);
}

static
bool Spot_Light_Intersects_Sphere( const RrLocalLight& light, const Sphere& sphere )
{
	return Point_Light_Intersects_Sphere(light,sphere);
}

bool RrLocalLight::IntersectsSphere( const Sphere& sphere ) const
{
	switch( m_lightType )
	{
	case Light_Point :
		return Point_Light_Intersects_Sphere( *this, sphere );

	case Light_Spot :
		return Spot_Light_Intersects_Sphere( *this, sphere );

	default:
		Unreachable;
	}
	return false;
}
#endif


mxDEFINE_CLASS( RrDirectionalLight );
mxBEGIN_REFLECTION( RrDirectionalLight )
	mxMEMBER_FIELD(color),
	mxMEMBER_FIELD(ambientColor),
	mxMEMBER_FIELD(direction),
mxEND_REFLECTION
RrDirectionalLight::RrDirectionalLight()
{
	color = V3_SetAll(1);
	ambientColor = V3_Zero();
	direction = -V3_UP;
}

}//namespace Rendering
