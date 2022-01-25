// Order-Independent Transparency (OIT) Using Fragment Linked Lists
UNFINISHED
#include "nw_shared_globals.h"
#include "_VS.h"
#include "_transform.h"
#include "_lighting.h"


[earlydepthstencil]	// prevent UAV-writes if the pixel is occluded by opaque geometry
#if 0
 
row_major float4x4	u_worldViewProjection;	// for transforming positions
row_major float4x4	u_worldView;	// for transforming normals
float4				u_colorWithAlpha;
float4				u_lightDirection;
float4				u_lightDiffuseColor;

Texture2D		t_diffuseAlpha;
SamplerState	s_diffuseAlpha;

Texture2D		t_normalMap;
SamplerState	s_normalMap;

Texture2D		t_specularMap;
SamplerState	s_specularMap;

Texture2D		t_refractionMap;
SamplerState	s_refractionMap;


struct VSOut
{
	float4 position : SV_Position;
	
#if bLit
	float3	positionVS : Position;
	float3	normalVS : Normal;	
#endif

#if bTextured
	float2	texCoord : TexCoord0;
#endif
};

void OIT_VS_Draw_Transparent( in DrawVertex _inputs, out VSOut _outputs )
{
	const float4 localPosition = float4(_inputs.position, 1.0f);
	_outputs.position = mul( u_worldViewProjection, localPosition );

#if bLit
	_outputs.positionVS = mul( u_worldView, localPosition ).xyz;
	const float3 localNormal = UnpackVertexNormal(_inputs.normal);
	_outputs.normalVS = mul( u_worldView, float4(localNormal, 0) ).xyz;
#endif

#if bTextured
	_outputs.texCoord = _inputs.texCoord;
#endif
}

float4 OIT_PS_Draw_Transparent( in VSOut _inputs ) : SV_Target
{
	float4	pixelColor = u_colorWithAlpha;

	float	specularIntensity = 1;

#if bTextured
	float4 textureColor = t_diffuseAlpha.Sample( s_diffuseAlpha, _inputs.texCoord.xy ).rgba;
	pixelColor *= textureColor;
	specularIntensity = t_specularMap.Sample( s_specularMap, _inputs.texCoord.xy ).r;
#endif
	
#if bLit
	const float3 normalVS = normalize( _inputs.normalVS );
	const float3 L = -u_lightDirection.xyz;
	pixelColor.rgb += Phong_DiffuseLighting( normalVS, L, u_lightDiffuseColor.rgb );

	const float3 specularColor = float3(0.1, 0.3, 1);
	pixelColor.rgb += BlinnSpecularIntensity( normalVS, -_inputs.positionVS, L, 5 ) * specularIntensity;
#endif

	return pixelColor;
}
#endif
