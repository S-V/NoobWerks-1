/*
=============================================================================
	Build configuration settings.
	These are compile time settings for the libraries.
=============================================================================
*/
#pragma once

#include <Base/Base.h>

#define LLGL_Driver_Direct3D_11		(1)
#define LLGL_Driver_OpenGL_4plus	(2)



//	Defines
#define LLGL_Driver		LLGL_Driver_Direct3D_11
// enable multithreaded draw call submission
#define LLGL_MULTITHREADED		(0)

#define LLGL_CONFIG_PROFILER	(0)


//=====================================================================
//	DEBUGGING
//=====================================================================

#define LLGL_CONFIG_ENABLE_INTERNAL_DEBUGGER	(MX_DEVELOPER)

/// Very slow!
#define GFX_CFG_DEBUG_COMMAND_BUFFER	(0)

// Constants
#define LLGL_DEBUG_LEVEL_HIGH	(3)	// Slow! Should be used for tracking down rare bugs.
#define LLGL_DEBUG_LEVEL_NORMAL	(2)
#define LLGL_DEBUG_LEVEL_LOW	(1)
#define LLGL_DEBUG_LEVEL_NONE	(0)

// 0..3
#if MX_DEBUG
	#define LLGL_DEBUG_LEVEL		(LLGL_DEBUG_LEVEL_NORMAL)
#else
	#define LLGL_DEBUG_LEVEL		(LLGL_DEBUG_LEVEL_NONE)
#endif

/// output debug info about every state creation/deletion
#define LLGL_VERBOSE	(0)


#define LLGL_ENABLE_PERF_HUD			(MX_DEVELOPER)


// Config (NOTE: don't make them too big to avoid data structures bloat)

#define LLGL_CREATE_RENDER_THREAD		(0)

#define LLGL_COMMAND_BUFFER_SIZE		(mxMiB(4))
#define LLGL_MAX_RENDER_COMMANDS		(65536)

//
#define LLGL_MAX_SHADER_COMMAND_BUFFER_SIZE	mxKiB(64)

//
#define LLGL_MAX_VERTEX_ATTRIBS			(8)
#define LLGL_MAX_TEXTURE_DIMENSIONS		(4096)
#define LLGL_MAX_TEXTURE_MIP_LEVELS		(14)
#define LLGL_MAX_TEXTURE_ARRAY_SIZE		(256)
#define LLGL_MAX_RENDER_TARGET_SIZE		(8192)
#define LLGL_MAX_BOUND_TARGETS			(8)

#define LLGL_NUM_VIEW_BITS				(6)
#define LLGL_MAX_VIEWS					(1 << LLGL_NUM_VIEW_BITS)

// -1 == invalid handle
#define LLGL_MAX_DEPTH_STENCIL_STATES	(256)
#define LLGL_MAX_RASTERIZER_STATES		(256)
#define LLGL_MAX_SAMPLER_STATES			(256)
#define LLGL_MAX_BLEND_STATES			(256)

#define LLGL_MAX_INPUT_LAYOUTS			(16)//max: (255)

#define LLGL_MAX_DEPTH_TARGETS			(16)//max: (255)
#define LLGL_MAX_COLOR_TARGETS			(64)//max: (255)

#define LLGL_MAX_SHADERS				(1024)
#define LLGL_MAX_PROGRAMS				(1024)
#define LLGL_MAX_TEXTURES				(4<<10)
#define LLGL_MAX_BUFFERS				(8<<10)

//
#define LLGL_CBUFFER_ALIGNMENT		(16)
#define LLGL_CBUFFER_ALIGNMENT_MASK	(LLGL_CBUFFER_ALIGNMENT-1)

#define LLGL_MIN_UNIFORM_BUFFER_SIZE		(16)
#define LLGL_MAX_UNIFORM_BUFFER_SIZE		(mxKiB(64))	// large size for skinning matrices
// TODO: move to config file
#define LLGL_TRANSIENT_VERTEX_BUFFER_SIZE	(mxMiB(32))
#define LLGL_TRANSIENT_INDEX_BUFFER_SIZE	(mxMiB(8))

#define LLGL_SUPPORT_BINARY_GL_PROGRAMS		(0)

#define LLGL_SUPPORT_GEOMETRY_SHADERS		(1)
#define LLGL_SUPPORT_TESSELATION_SHADERS	(1)

#define LLGL_MAX_BOUND_UNIFORM_BUFFERS	(8)		//!< D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT = 14 (2 slots are reserved)
#define LLGL_MAX_BOUND_SHADER_SAMPLERS	(16)	//!< D3D11_COMMONSHADER_SAMPLER_REGISTER_COUNT = 16
#define LLGL_MAX_BOUND_SHADER_TEXTURES	(32)	//!< D3D11_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT = 128
#define LLGL_MAX_BOUND_OUTPUT_BUFFERS	(8)		//!< D3D11_PS_CS_UAV_REGISTER_COUNT = 8

#define LLGL_MAX_VERTEX_STREAMS		(1)

/// for profiling and debugging
#define LLGL_MAX_VIEW_NAME_LENGTH				(32)

#define LLGL_Driver_Is_Direct3D	(LLGL_Driver == LLGL_Driver_Direct3D_11)
#define LLGL_Driver_Is_OpenGL	(LLGL_Driver == LLGL_Driver_OpenGL_4plus)

enum { VCACHE_SIZE = 24 };	// for testing

//=====================================================================
//	MINI-MATH
//=====================================================================

#if LLGL_Driver_Is_Direct3D
	#define M44_Perspective	M44_PerspectiveD3D
#endif

#if LLGL_Driver_Is_OpenGL
	#define M44_Perspective	M44_PerspectiveOGL
#endif


#if LLGL_Driver_Is_Direct3D
	#define NDC_NEAR_CLIP	(0.0f)
#endif

#if LLGL_Driver_Is_OpenGL
	#define NDC_NEAR_CLIP	(-1.0f)
#endif

/// Corners of the unit cube
/// in Direct3D's or OpenGL's Normalized Device Coordinates (NDC)
extern const V3f gs_NDC_Cube[8];

class ID3D11Device;
class ID3D11DeviceContext;

namespace NGpu
{

struct PlatformData
{
	struct D3D11
	{
		ID3D11Device* device;
		ID3D11DeviceContext* context;
	} d3d11;
};

}//namespace NGpu


#if LLGL_CONFIG_PROFILER
#	define LLGL_PROFILER_SCOPE(_name, _abgr) ProfilerScope BX_CONCATENATE(profilerScope, __LINE__)(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define LLGL_PROFILER_BEGIN(_name, _abgr) g_callback->profilerBeginLiteral(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define LLGL_PROFILER_END() g_callback->profilerEnd()
#	define LLGL_PROFILER_SET_CURRENT_THREAD_NAME(_name) mxNOOP()
#else
#	define LLGL_PROFILER_SCOPE( name, rgba ) mxNOOP()
#	define LLGL_PROFILER_BEGIN( name, rgba ) mxNOOP()
#	define LLGL_PROFILER_END() mxNOOP()
#	define LLGL_PROFILER_SET_CURRENT_THREAD_NAME(_name) mxNOOP()
#endif // LLGL_PROFILER_SCOPE

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
