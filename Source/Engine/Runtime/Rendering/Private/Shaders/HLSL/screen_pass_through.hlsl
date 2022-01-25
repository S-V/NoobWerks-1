Texture2D		t_sourceTexture;
SamplerState	s_sourceTexture;

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

float4 PixelShaderMain( in VS_Output inputs ) : SV_Target
{
	return t_sourceTexture.Sample( s_sourceTexture, inputs.texCoord.xy );
}
