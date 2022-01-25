#include <Shared/nw_shared_globals.h>

StructuredBuffer< float4 > t_sourceBuffer;

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

// Linearize the given 2D address + sample index into our flat framebuffer array
uint GetFramebufferSampleAddress(uint2 coords, uint sampleIndex)
{
    // Major ordering: Row (x), Col (y), MSAA sample
    return (sampleIndex * g_framebuffer_dimensions.y + coords.y) * g_framebuffer_dimensions.x + coords.x;
}

float4 PixelShaderMain( in VS_Output inputs ) : SV_Target
{
	return t_sourceBuffer[GetFramebufferSampleAddress(inputs.position.xy, 0)];
}
