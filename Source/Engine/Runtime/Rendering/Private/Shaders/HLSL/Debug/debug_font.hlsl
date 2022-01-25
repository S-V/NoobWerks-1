Texture2D		t_font : register(t0);
SamplerState	s_font : register(s0);

//cbuffer cbData
//{
//	// xy = inverse viewport size, zy = inverse font texture size
//	float4	invScreenAndTextureSize;
//};

struct VSIn
{
	float4	xy_wh : TexCoord0;
	float4	tl_br : TexCoord1;	// UVs for top left and bottom right corners
};

struct GSIn
{
	float4	xy_wh : TexCoord0;
	float4	tl_br : TexCoord1;
};

struct PSIn
{
	float4	posH : SV_Position;
	float2	uv : TexCoord;
};

void VertexShaderMain( in VSIn input, out GSIn output )
{
	output.xy_wh = input.xy_wh;
	output.tl_br = input.tl_br;
}

[maxvertexcount(4)]
void GeometryShaderMain( point GSIn input[1], inout TriangleStream<PSIn> outStream )
{
	float2	pos = input[0].xy_wh.xy;
	float	width = input[0].xy_wh.z;
	float	height = input[0].xy_wh.w;
	float2	tl = input[0].tl_br.xy;
	float2	br = input[0].tl_br.zw;

	PSIn	topLeft;
	PSIn	topRight;
	PSIn	bottomLeft;
	PSIn	bottomRight;

	topLeft.posH = float4( pos.x,				pos.y,				0.0f, 	1.0f );
	topRight.posH = float4( pos.x + width,		pos.y,				0.0f, 	1.0f );
	bottomLeft.posH = float4( pos.x,			pos.y - height,		0.0f, 	1.0f );
	bottomRight.posH = float4( pos.x + width,	pos.y - height,		0.0f, 	1.0f );

	topLeft.uv = float2( tl.x, tl.y );
	topRight.uv = float2( br.x, tl.y );
	bottomLeft.uv = float2( tl.x, br.y );
	bottomRight.uv = float2( br.x, br.y );

	outStream.Append( topLeft );
	outStream.Append( topRight );
	outStream.Append( bottomLeft );
	outStream.Append( bottomRight );
}

float4 PixelShaderMain( in PSIn input ) : SV_Target
{
	float4 color = t_font.Sample( s_font, input.uv ).rgba;
	if( color.w < 1.0/255.0 ) {
		discard;
	}
	return color;
}
