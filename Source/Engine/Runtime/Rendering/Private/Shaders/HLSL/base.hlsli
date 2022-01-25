// included by most shaders. contains common macros, types, functions, constants and variables

#ifndef __BASE_SHADER_CODE_HLSLI__
#define __BASE_SHADER_CODE_HLSLI__

#include <Common/constants.hlsli>


#define nwMAX_CB_SIZE			(65536)


//-------------------------------------------------------------
//	Markers (notes to the developer).
//-------------------------------------------------------------
//
//	Pros: bookmarks are not needed;
//		  easy to "find All References" and no comments needed.
//	Cons: changes to the source file require recompiling;
//		  time & data have to be inserted manually.
//
#define mxNOTE( message_string )
#define mxTODO( message_string )
#define mxREFACTOR( message_string )
#define mxBUG( message_string )
#define mxFIXME( message_string )
#define mxHACK( message_string )
#define mxOPTIMIZE( message_string )
#define mxWARNING( message_string )
#define mxREMOVE_THIS
#define mxUNDONE
#define mxSTOLEN( message_string )
#define mxUNSAFE
#define mxTIDY_UP_FILE
#define mxTEMP
// 'to be documented'
#define mxTBD

#define mxWHY( message_string )

//-------------------------------------------------------------------
//	Macros
//-------------------------------------------------------------------

// Avoids flow control constructs.
#define MX_UNROLL											[unroll]
// Gives preference to flow control constructs.
#define MX_LOOP												[loop]
// Performs branching by using control flow instructions like jmp and label.
#define MX_BRANCH											[branch]
// Performs branching by using the cnd instructions.
#define nwFLATTEN											[flatten]
// Executes the conditional part of an if statement when the condition is true for all threads on which the current shader is running.
#define MX_IFALL											[ifAll]
// Executes the conditional part of an if statement when the condition is true for any thread on which the current shader is running.
#define MX_IFANY											[ifAny]

//-------------------------------------------------------------------
//	Math helpers
//-------------------------------------------------------------------

#define m4x4_identity	float4x4( 1.0f, 0.0f, 0.0f, 0.0f,	\
								 0.0f, 1.0f, 0.0f, 0.0f,	\
								 0.0f, 0.0f, 1.0f, 0.0f,	\
								 0.0f, 0.0f, 0.0f, 1.0f )


float squaref( float x ) { return (x * x); }
float cubef( float x ) { return (x * x * x); }
float FourthPower( float x ) { float sq = x * x; return sq * sq; }

float sum( float3 v ) { return (v.x + v.y + v.z); }
float average3( float3 v ) { return (1.0f / 3.0f) * sum(v); }

float min3( in float3 v ) {
	return min( min( v.x, v.y ), v.z );
}
float max3( in float3 v ) {
	return max( max( v.x, v.y ), v.z );
}

float min4( in float4 v ) {
	return min( min(v.x, v.y), min(v.z, v.w) );
}
float max4( in float4 v ) {
	return max( max(v.x, v.y), max(v.z, v.w) );
}

int maxIndex3( in float3 v )
{
	return ( v.x > v.y )
		? ( ( v.x > v.z ) ? 0 : 2 )
		: ( ( v.y > v.z ) ? 1 : 2 );
}


/// Performs an arithmetic multiply/add operation on three values: m * a + b.
#define MAD(m, a, b)	mad(m, a, b)


/// [0..1] -> [-1..+1], useful for decoding signed values from UNORM textures
//#define Decode01_to_minus1plus1(X)	((X) * 2 - 1)
#define Decode01_to_minus1plus1(X)	MAD(X, 2, -1)



// Composes a floating point value with the magnitude of 'x' and the sign of 's'.
float CopySign(float x, float s)
{
    return (s >= 0) ? abs(x) : -abs(x);
}

// Returns -1 for negative numbers and 1 for positive numbers.
// 0 can be handled in 2 different ways.
// The IEEE floating point standard defines 0 as signed: +0 and -0.
// However, mathematics typically treats 0 as unsigned.
// Therefore, we treat -0 as +0 by default: FastSign(+0) = FastSign(-0) = 1.
// Note that the sign() function in HLSL implements signum, which returns 0 for 0.
float FastSign(float s)
{
    return CopySign(1, s);
}


mxSTOLEN( "Wicked engine, globals.hlsli" )
// Helpers:

// returns a random float in range (0, 1). seed must be >0!
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

// 2D array index to flattened 1D array index
inline uint FlattenIndex2D( uint2 coord, uint2 dim )
{
	return coord.x + coord.y * dim.x;
}
// flattened array index to 2D array index
inline uint2 UnravelIndex2D( uint idx, uint2 dim )
{
	return uint2(idx % dim.x, idx / dim.x);
}

/// 3D array index to flattened 1D array index
inline uint FlattenIndex3D( uint3 coord, uint3 dim )
{
	return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
}

// flattened array index to 3D array index
inline uint3 UnravelIndex3D( uint idx, uint3 dim )
{
	const uint z = idx / (dim.x * dim.y);
	idx -= (z * dim.x * dim.y);
	const uint y = idx / dim.x;
	const uint x = idx % dim.x;
	return uint3(x, y, z);
}

///
inline bool Is3DTextureCoordOutsideVolume(in float3 uvw)
{
	 // saturate instructions are free
	return any( uvw - saturate( uvw ) );
}


// Creates a unit cube triangle strip from just vertex ID (14 vertices)
// https://twitter.com/Donzanoid/status/616370134278606848
// https://www.gamedev.net/forums/topic/674733-vertex-to-cube-using-geometry-shader/?do=findComment&comment=5271097
// https://gist.github.com/XProger/66e53d69c7f861a7b2a2
// http://www.asmcommunity.net/forums/topic/?id=6284.
// Usage:
// for( uint cube_vertex_idx = 0; cube_vertex_idx < 14; cube_vertex_idx++ )
//    const float3 cube_vertex_pos_01 = getCubeCoordForVertexId( cube_vertex_idx );
//    ...
//
inline float3 getCubeCoordForVertexId( in uint vertexID )
{
	uint b = 1 << vertexID;
	return float3( (0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0 );
}


//-------------------------------------------------------------------
//	Screen space
//-------------------------------------------------------------------

/**
 * Aligns the clip space position so that it can be used as a texture coordinate.
 */
inline float2 ClipPosToTexCoords( in float2 clipPosition )
{
	// flip Y axis, [-1..+1] -> [0..1]
	//return float2( 0.5f, -0.5f ) * clipPosition + 0.5f;
	return float2(clipPosition.x, -clipPosition.y) * 0.5f + 0.5f;
}

/**
 * Aligns the [0,1] UV to match the view within the backbuffer.
 */
inline float2 TexCoordsToClipPos( in float2 texCoords )
{
	// [0..1] -> [-1..+1], flip Y axis
	return texCoords * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f );
}

/**
 * aligns the clip space position so that it can be used as a texture coordinate
 * to properly align in screen space
 */
inline float4 ScreenAlignedPosition( float4 screen_space_position )
{
	float4 result = screen_space_position;
	result.xy = (screen_space_position.xy / screen_space_position.w) * 0.5f + 0.5f;
	result.y = 1.0f - result.y;
	return result;
}



float3 UnpackVertexNormal( uint4 packedNormal )
{
	// [0..255] => [-1..+1]
	return packedNormal.xyz * (1.0f / 127.5f) - 1.0f;
}




#define FRUSTUM_CORNER_FAR_TOP_LEFT		0
#define FRUSTUM_CORNER_FAR_TOP_RIGHT	1
#define FRUSTUM_CORNER_FAR_BOTTOM_RIGHT	2
#define FRUSTUM_CORNER_FAR_BOTTOM_LEFT	3


// Calculates the angle between two vectors
//
inline float AngleBetween(in float3 dir0, in float3 dir1)
{
	return acos(dot(dir0, dir1));
}

float3x3 InvertMatrix3x3( in float3x3 mat )
{
	float det = determinant( mat );
	float3x3 T = transpose( mat );
	return float3x3(
		cross( T[1], T[2] ),
		cross( T[2], T[0] ),
		cross( T[0], T[1] )
		) / det;
}

// [ X, Y, Z, W ] => projection transform => homogeneous divide => [Px, Py, Pz, W ]
float4 ProjectPoint( in float4x4 transform, in float4 p )
{
	p = mul( transform, p );
    p.xyz /= p.w;
    return p;
}
float4 ProjectPoint3( in float4x4 transform, in float3 p )
{
	float4 result = mul( transform, float4( p, 1.0f ) );
    result.xyz /= result.w;
    return result;
}


//-------------------------------------------------------------------
//	GAUSSIAN BLUR
//-------------------------------------------------------------------

/// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight( int sampleDistance, float sigma )
{
	float g = 1.0f / sqrt( M_TWO_PI * sigma * sigma );
	return (g * exp(-(sampleDistance * sampleDistance) / (2.0f * sigma * sigma)));
}

/// Performs a gaussian blur in one direction
float4 GaussianBlur1D(
	in Texture2D _sourceTexture,
	in SamplerState _sampler,
	in float _invTextureSize,	
	in float2 texCoord,
	in float2 texScale,
	in int _radius,
	in float sigma
	)
{
    float4 color = 0;
	[unroll]
    for( int i = -_radius; i < _radius; i++ )
    {
		float weight = CalcGaussianWeight( i, sigma );
		texCoord += i * (_invTextureSize * texScale);
		float4 sample = _sourceTexture.Sample( _sampler, texCoord );
		color += sample * weight;
    }
    return color;
}


#endif // __BASE_SHADER_CODE_HLSLI__
