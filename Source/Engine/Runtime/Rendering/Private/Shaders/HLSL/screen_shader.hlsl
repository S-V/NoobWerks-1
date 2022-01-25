#include "_screen_shader.h"

Texture2D		t_sourceTexture;
SamplerState	s_sourceTexture;

float4 PixelShaderMain( in VS_ScreenOutput inputs ) : SV_Target
{
	return t_sourceTexture.Sample( s_sourceTexture, inputs.texCoord.xy );
}
