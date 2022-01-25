#include "nw_shared_globals.h"
#include "_VS.h"
#include "_transform.h"

Texture2D		t_skyTexture;
SamplerState	s_skyTexture;	// trilinear

struct VS_to_PS
{
	float4 positionCS : SV_Position;
	float2 texCoord : TexCoord0;
};

void SkyDome_VS( in AuxVertex _inputs, out VS_to_PS _outputs )
{
	const float4 localPosition = float4(_inputs.position, 1.0f);
	_outputs.positionCS = Pos_Local_To_Clip( localPosition );
	_outputs.texCoord = _inputs.texCoord;
}

float3 SkyDome_PS( in VS_to_PS _inputs ) : SV_Target
{
	const float4 color = t_skyTexture.Sample( s_skyTexture, _inputs.texCoord ).rgba;
	return color.rgb;
}
