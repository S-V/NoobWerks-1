// This file must be included by HLSL <-> C++ compatible headers.

//#ifdef __cplusplus
//#pragma once
//#endif

//#ifndef NW_SHADER_INTEROP_INCLUDED
//#define NW_SHADER_INTEROP_INCLUDED

#ifndef NW_SHADER_INTEROP_INCLUDED
#define NW_SHADER_INTEROP_INCLUDED
#endif

#ifdef __cplusplus

	#define CPP_PREALIGN_CBUFFER	__declspec(align(16))
	#define cbuffer					mxPREALIGN(16) struct
	#define row_major

	//NOTE: cannot use typedef - conflicts with MathGeoLib
	//typedef M44f	float4x4;
	//typedef V4f		float4;
	//typedef V3f		float3;
	//typedef UInt4		uint4;
	#define float4x4	M44f
	#define float3x4	M34f
	#define float4		V4f
	#define float3		V3f
	#define float2		V2f
	#define uint		U32
	#define uint3		UInt3
	#define uint4		UInt4

	#define CONST_METHOD	const

#else

	#define CPP_PREALIGN_CBUFFER
	#define CONST_METHOD

#endif


#ifdef __cplusplus
#	define DECLARE_CBUFFER( cbName, slot )		enum { cbName##_Index = slot }; CPP_PREALIGN_CBUFFER struct cbName
#   define CHECK_STRUCT_ALIGNMENT( struc )		mxSTATIC_ASSERT( sizeof(struc) % 16 == 0 )
#	define DECLARE_SAMPLER( samplerName, slot )	enum { samplerName##_Index = slot };
#	define CPP_CODE( code )		code
#	define HLSL_CODE( code )
#else // if HLSL
#	define DECLARE_CBUFFER( cbName, slot )		cbuffer cbName : register( b##slot )
#   define CHECK_STRUCT_ALIGNMENT( struc )
#	define DECLARE_SAMPLER( samplerName, slot )	SamplerState samplerName : register( s##slot )
#	define CPP_CODE( code )
#	define HLSL_CODE( code )	code
#endif


//#endif // NW_SHADER_INTEROP_INCLUDED
