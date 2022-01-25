// Preprocessor clean-up.

#ifndef NW_SHADER_INTEROP_INCLUDED
#error __shader_interop_pre header must have been included first!
#endif

#ifdef __cplusplus

#	undef DECLARE_CB
#	undef DECLARE_SAMPLER
#	undef CPP_CODE
#	undef HLSL_CODE

#	undef cbuffer
#	undef row_major

#	undef float4x4
#	undef float3x4
#	undef float4
#	undef float3
#	undef float2
#	undef uint
#	undef uint4

#endif

#undef NW_SHADER_INTEROP_INCLUDED
