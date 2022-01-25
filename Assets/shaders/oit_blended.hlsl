// Blended Order-Independent Transparency (OIT)
/*
References:
McGuire and Bavoil, Weighted Blended Order-Independent Transparency, Journal of Computer Graphics Techniques (JCGT), vol. 2, no. 2, 122–141, 2013. http://jcgt.org/published/0002/02/09.
http://casual-effects.blogspot.com/2014/03/weighted-blended-order-independent.html
http://casual-effects.blogspot.fr/2015/03/implemented-weighted-blended-order.html
*/

#define bFLAT_SHADING 1

#include "nw_shared_globals.h"
#include "_VS.h"
#include "_transform.h"
#include "_lighting.h"

//float4				u_colorWithAlpha;
#define u_colorWithAlpha	float4(0.4, 0.7, 0.6, 0.9)

#define bLit 1

static const float4	u_lightDirection = float4(-1,0,0, 0);
static const float4	u_lightDiffuseColor = float4( 1, 0, 0, 1 );

Texture2D		t_diffuseAlpha;
SamplerState	s_diffuseAlpha;

Texture2D		t_normalMap;
SamplerState	s_normalMap;

Texture2D		t_specularMap;
SamplerState	s_specularMap;

struct VSOut
{
	float4 position : SV_Position;	// clip-space position
	float3 positionVS : Position;	// view-space position
#if bLit
	float3 normalVS : Normal;		// view-space normal
#endif
#if bTextured
	float2 texCoord : TexCoord0;
#endif
};

void BlendedOIT_VS_Draw_Transparent( in DrawVertex _inputs, out VSOut _outputs )
{
	const float4 localPosition = float4(_inputs.position, 1.0f);
	_outputs.position = Pos_Local_To_Clip( localPosition );
	_outputs.positionVS = Pos_Local_To_View( localPosition ).xyz;

#if bLit	
	const float3 localNormal = UnpackVertexNormal( _inputs.normal );
	_outputs.normalVS = Dir_Local_To_View( localNormal ).xyz;
#endif

#if bTextured
	_outputs.texCoord = _inputs.texCoord;
#endif
}

[earlydepthstencil]	// prevent pixel shader execution if the pixel is occluded by opaque geometry
void BlendedOIT_PS_Draw_Transparent(
	in VSOut _inputs,
	out float4 _accumulation : SV_Target0,
	out float4 _revealage : SV_Target1
)
{
	float4	color = u_colorWithAlpha;


	float	specularIntensity = 1;

#if bTextured
	float4 textureColor = t_diffuseAlpha.Sample( s_diffuseAlpha, _inputs.texCoord.xy ).rgba;
	color *= textureColor;
	specularIntensity = t_specularMap.Sample( s_specularMap, _inputs.texCoord.xy ).r;
#endif

#if bLit
	#if bFLAT_SHADING
		// face normal in eye-space:
		// '-' because we use right-handed coordinate system
		const float3 viewSpaceNormal = normalize(-cross(ddx(_inputs.positionVS), ddy(_inputs.positionVS)));
	#else
		const float3 viewSpaceNormal = normalize( _inputs.viewSpaceNormal );
	#endif
	
	const float3 L = -u_lightDirection.xyz;
	color.rgb += Phong_DiffuseLighting( viewSpaceNormal, L, u_lightDiffuseColor.rgb );

	const float3 specularColor = float3(0.1, 0.3, 1);
	color.rgb += BlinnSpecularIntensity( viewSpaceNormal, -_inputs.positionVS, L, 5 ) * specularIntensity;
#endif



	const float alpha = color.a;
	const float depth = _inputs.position.z / _inputs.position.w;
	//const float3 premultipliedColor = color.rgb * color.a;

	// Insert your favorite weighting function here. The color-based factor
	// avoids color pollution from the edges of wispy clouds. The z-based
	// factor gives precedence to nearer surfaces.

//	const float weight = alpha * clamp( 0.03 / (1e-5 + pow(depth / 200.0, 5.0) ), 0.01, 3000.0 );

/*
	// RGB stores Sum[FragColor.rgb * FragColor.a * Weight(FragDepth)]
	// Alpha stores Sum[FragColor.a * Weight(FragDepth)]
	const float weight = clamp(
		pow( min(1.0, premultipliedColor.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - _inputs.positionVS.z * 0.9, 3.0)
		, 1e-2
		, 3e3
	);
	const float weight = clamp(0.03 / (1e-5 + pow(depth / 200.0, 5.0) ), 0.01, 3000.0);
*/

	const float weight = alpha * clamp(pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - _inputs.position.w * 0.9, 3.0), 1e-2, 3e3);


	// Switch to premultiplied alpha and weight
	_accumulation = float4( color.rgb * alpha, alpha ) * weight;

	_revealage = alpha;
}
