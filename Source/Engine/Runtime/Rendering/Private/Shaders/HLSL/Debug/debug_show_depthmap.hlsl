// a debugging aid for showing the contents of a shadow depth map
Texture2D		t_sourceTexture;
SamplerState	s_sourceTexture;
//float			inverseFarClip;

struct VS_Input
{
    float3 position : Position;
    float2 texCoord : TexCoord0;
};
struct VS_Output
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

VS_Output VertexShaderMain( in VS_Input input )
{
    VS_Output output;
    output.position = float4(input.position,1);
	output.texCoord = input.texCoord;
    return output;
}

float3 PixelShaderMain( in VS_Output inputs ) : SV_Target
{
	float rawDepth = t_sourceTexture.Sample( s_sourceTexture, inputs.texCoord.xy ).r;
	rawDepth = (1 - rawDepth) * 300;
	// draw as grayscale
	return float3(rawDepth, rawDepth, rawDepth);
}
