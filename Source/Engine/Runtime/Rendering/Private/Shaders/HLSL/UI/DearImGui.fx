/*
used for rendering debug UI with DearImGui: https://github.com/ocornut/imgui

%%
pass GUI
{
	vertex = VertexShaderMain
	pixel = PixelShaderMain

	// alpha-blending, no face culling, no depth testing
	state = GUI
}

samplers
{
	s_texture = BilinearSampler
}

%%
*/

struct VSIn // ImDrawVert
{
	float2 position : Position;
	float2 texCoord : TexCoord0;
	float4 color : Color0;	//autonormalized into [0..1] range
};
struct VSOut
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
	float4 color : Color0;
};

float4x4	u_transform;

Texture2D		t_texture;
SamplerState	s_texture;

void VertexShaderMain( in VSIn inputs, out VSOut outputs )
{
	outputs.position = mul(u_transform, float4(inputs.position, 0.0f, 1.0f));
	outputs.texCoord = inputs.texCoord;
	outputs.color = inputs.color;
}

float4 PixelShaderMain( in VSOut inputs ) : SV_Target
{
	float4 textureColor = t_texture.Sample( s_texture, inputs.texCoord.xy ).rgba;
	return textureColor * inputs.color;
}
