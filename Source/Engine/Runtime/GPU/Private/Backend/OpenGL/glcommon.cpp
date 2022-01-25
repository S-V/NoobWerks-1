#if LLGL_Driver_Is_OpenGL

#include <glew/glew.h>
#include <glew/wglew.h>
#include "glcommon.h"

namespace NGpu
{

#if MX_DEBUG
	const char* GLErrorToChars( int errorCode )
	{
		switch( errorCode )
		{
		case GL_NO_ERROR :			return "No errors";
		case GL_INVALID_ENUM :		return "Invalid Enum";
		case GL_INVALID_VALUE :		return "Invalid Value";
		case GL_INVALID_OPERATION :	return "Invalid Operation";
		case GL_STACK_OVERFLOW :	return "Stack Overflow";
		case GL_STACK_UNDERFLOW :	return "Stack Underflow";
		case GL_OUT_OF_MEMORY :		return "Out of Memory";
		}
		return "Unknown error";
	}
#else
	const char* GLErrorToChars( int errorCode )
	{
		return "";
	}
#endif

	// advanced OpenGL debugging utils by Andon M. Coleman
	// swiped from http://stackoverflow.com/questions/18064988/rendering-issue-with-different-computers/18067245#18067245

	const char* ETB_GL_DEBUG_SOURCE_STR (GLenum source)
	{
		static const char* sources [] = {
			"API",   "Window System", "Shader Compiler", "Third Party", "Application",
			"Other", "Unknown"
		};

		int str_idx =
			smallest( source - GL_DEBUG_SOURCE_API, sizeof (sources) / sizeof (const char *) );

		return sources [str_idx];
	}

	const char* ETB_GL_DEBUG_TYPE_STR (GLenum type)
	{
		static const char* types [] = {
			"Error",       "Deprecated Behavior", "Undefined Behavior", "Portability",
			"Performance", "Other",               "Unknown"
		};

		int str_idx =
			smallest( type - GL_DEBUG_TYPE_ERROR, sizeof (types) / sizeof (const char *) );

		return types [str_idx];
	}

	const char* ETB_GL_DEBUG_SEVERITY_STR (GLenum severity)
	{
		static const char* severities [] = {
			"High", "Medium", "Low", "Unknown"
		};

		int str_idx =
			smallest( severity - GL_DEBUG_SEVERITY_HIGH, sizeof (severities) / sizeof (const char *) );

		return severities [str_idx];
	}

	DWORD ETB_GL_DEBUG_SEVERITY_COLOR (GLenum severity)
	{
		static DWORD severities [] = {
			0xff0000ff, // High (Red)
			0xff00ffff, // Med  (Yellow)
			0xff00ff00, // Low  (Green)
			0xffffffff  // ???  (White)
		};

		int col_idx =
			smallest( severity - GL_DEBUG_SEVERITY_HIGH, sizeof (severities) / sizeof (DWORD) );

		return severities [col_idx];
	}

	void My_OpenGL_Error_Callback(
		GLenum        source,
		GLenum        type,
		GLuint        id,
		GLenum        severity,
		GLsizei       length,
		const GLchar* message,
		GLvoid*       userParam
	)
	{
		//eTB_ColorPrintf (0xff00ffff, "OpenGL Error:\n");
		//eTB_ColorPrintf (0xff808080, "=============\n");

		//eTB_ColorPrintf (0xff6060ff, " Object ID: ");
		//eTB_ColorPrintf (0xff0080ff, "%d\n", id);

		//eTB_ColorPrintf (0xff60ff60, " Severity:  ");
		//eTB_ColorPrintf ( ETB_GL_DEBUG_SEVERITY_COLOR   (severity),
		//	"%s\n",
		//	ETB_GL_DEBUG_SEVERITY_STR (severity) );

		//eTB_ColorPrintf (0xffddff80, " Type:      ");
		//eTB_ColorPrintf (0xffccaa80, "%s\n", ETB_GL_DEBUG_TYPE_STR     (type));

		//eTB_ColorPrintf (0xffddff80, " Source:    ");
		//eTB_ColorPrintf (0xffccaa80, "%s\n", ETB_GL_DEBUG_SOURCE_STR   (source));

		//eTB_ColorPrintf (0xffff6060, " Message:   ");
		//eTB_ColorPrintf (0xff0000ff, "%s\n\n", message);

		//// Force the console to flush its contents before executing a breakpoint
		//eTB_FlushConsole ();

		//// Trigger a breakpoint in gDEBugger...
		//glFinish ();

		//// Trigger a breakpoint in traditional debuggers...
		//eTB_CriticalBreakPoint ();

		ptPRINT("OpenGL Error:\n");
		ptPRINT("=============\n");

		ptPRINT(" Object ID: ");
		ptPRINT("%d\n", id);

		ptPRINT(" Severity:  ");
		ptPRINT("%s\n", ETB_GL_DEBUG_SEVERITY_STR (severity));

		ptPRINT(" Type:      ");
		ptPRINT("%s\n", ETB_GL_DEBUG_TYPE_STR     (type));

		ptPRINT(" Source:    ");
		ptPRINT("%s\n", ETB_GL_DEBUG_SOURCE_STR   (source));

		ptPRINT(" Message:   ");
		ptPRINT("%s\n\n", message);

		// Trigger a breakpoint in gDEBugger...
		glFinish ();

		// Trigger a breakpoint in traditional debuggers...
		DebugBreak();
	}

	const char* g_attributeNames[NwAttributeUsage::Count] =
	{
		"a_position",

		"a_color0",
		"a_color1",

		"a_normal",
		"a_tangent",
		"a_bitangent",

		"a_texCoord0",
		"a_texCoord1",
		"a_texCoord2",
		"a_texCoord3",
		"a_texCoord4",
		"a_texCoord5",
		"a_texCoord6",
		"a_texCoord7",

		"a_boneIndices",
		"a_boneWeights",
	};

	NwAttributeUsage::Enum GetAttribIdByName( const char* attribName )
	{
		for( UINT i = 0; i < NwAttributeUsage::Count; i++ ) {
			if( !strcmp(g_attributeNames[i], attribName) ) {
				return (NwAttributeUsage::Enum) i;
			}
		}
		ptERROR("Unknown vertex attribute: '%s'\n", attribName);
		return (NwAttributeUsage::Enum) -1;
	}

	GLenum ConvertBufferTypeGL( EBufferType bufferType )
	{
		switch( bufferType )
		{
		case EBufferType::Buffer_Uniform :	return GL_UNIFORM_BUFFER;
		case EBufferType::Buffer_Vertex :	return GL_ARRAY_BUFFER;
		case EBufferType::Buffer_Index :	return GL_ELEMENT_ARRAY_BUFFER;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertAttribTypeGL( NwAttributeType::Enum attribType )
	{
		switch(attribType)
		{
		case NwAttributeType::Byte :		return GL_BYTE;
		case NwAttributeType::UByte :		return GL_UNSIGNED_BYTE;
		case NwAttributeType::Short :		return GL_SHORT;
		case NwAttributeType::UShort :	return GL_UNSIGNED_SHORT;
		//case AttributeType::Int :		return GL_INT;
		//case AttributeType::UInt :		return GL_UNSIGNED_INT;
		case NwAttributeType::Half :		return GL_HALF_FLOAT_ARB;
		case NwAttributeType::Float :		return GL_FLOAT;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertShaderTypeGL( EShaderType shaderType )
	{
		switch( shaderType )
		{
		case NwShaderType::Vertex :		return GL_VERTEX_SHADER;
		case NwShaderType::Geometry :	return GL_GEOMETRY_SHADER;
		case NwShaderType::Pixel :	return GL_FRAGMENT_SHADER;		
		default:				ptERROR("Unsupported shader type!\n");
		}
		return -1;
	}

	GLenum ConvertComparisonFunctionGL( NwComparisonFunc::Enum depthFunc )
	{
		switch(depthFunc)
		{
		case NwComparisonFunc::Always :			return GL_ALWAYS;
		case NwComparisonFunc::Never :			return GL_NEVER;
		case NwComparisonFunc::Less :				return GL_LESS;
		case NwComparisonFunc::Equal :			return GL_EQUAL;
		case NwComparisonFunc::Greater :			return GL_GREATER;
		case NwComparisonFunc::Not_Equal :		return GL_NOTEQUAL;
		case NwComparisonFunc::Less_Equal :		return GL_LEQUAL;
		case NwComparisonFunc::Greater_Equal :	return GL_GEQUAL;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertStencilOpGL( NwStencilOp::Enum stencilOp )
	{
		switch(stencilOp)
		{
		case NwStencilOp::KEEP :		return GL_KEEP;
		case NwStencilOp::ZERO :		return GL_ZERO;
		case NwStencilOp::INCR_WRAP :	return GL_INCR_WRAP;
		case NwStencilOp::DECR_WRAP :	return GL_DECR_WRAP;
		case NwStencilOp::REPLACE :	return GL_REPLACE;
		case NwStencilOp::INCR_SAT :	return GL_INCR;
		case NwStencilOp::DECR_SAT :	return GL_DECR;
		case NwStencilOp::INVERT :	return GL_INVERT;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertFillModeGL( NwFillMode::Enum fillMode )
	{
		switch(fillMode)
		{
		case NwFillMode::Solid :		return GL_FILL;
		case NwFillMode::Wireframe :	return GL_LINE;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}
	GLenum ConvertCullModeGL( NwCullMode::Enum cullMode )
	{
		switch(cullMode)
		{
		case NwCullMode::None :		return GL_NONE;
		case NwCullMode::Back :		return GL_BACK;
		case NwCullMode::Front :		return GL_FRONT;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertAddressModeGL( NwTextureAddressMode::Enum addressMode )
	{
		switch(addressMode)
		{
		case NwTextureAddressMode::Wrap :		return GL_REPEAT;
		case NwTextureAddressMode::Clamp :	return GL_CLAMP_TO_EDGE;
			mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertBlendFactorGL( NwBlendFactor::Enum blendFactor )
	{
		switch(blendFactor)
		{
		case NwBlendFactor::ZERO				:	return GL_ZERO;
		case NwBlendFactor::ONE				:	return GL_ONE;

		case NwBlendFactor::SRC_COLOR			:	return GL_SRC_COLOR;
		case NwBlendFactor::DST_COLOR			:	return GL_DST_COLOR;
		case NwBlendFactor::SRC_ALPHA			:	return GL_SRC_ALPHA;
		case NwBlendFactor::DST_ALPHA			:	return GL_DST_ALPHA;
		case NwBlendFactor::SRC1_COLOR		:	return GL_SRC1_COLOR;
		case NwBlendFactor::SRC1_ALPHA		:	return GL_SOURCE1_ALPHA;
		case NwBlendFactor::BLEND_FACTOR		:	return GL_CONSTANT_COLOR;

		case NwBlendFactor::INV_SRC_COLOR		:	return GL_ONE_MINUS_SRC_COLOR;
		case NwBlendFactor::INV_DST_COLOR		:	return GL_ONE_MINUS_DST_COLOR;
		case NwBlendFactor::INV_SRC_ALPHA		:	return GL_ONE_MINUS_SRC_ALPHA;
		case NwBlendFactor::INV_DST_ALPHA		:	return GL_ONE_MINUS_DST_ALPHA;
		case NwBlendFactor::INV_SRC1_COLOR	:	return GL_ONE_MINUS_SRC1_COLOR;
		case NwBlendFactor::INV_SRC1_ALPHA	:	return GL_TRIANGLE_STRIP;
		case NwBlendFactor::INV_BLEND_FACTOR	:	return GL_ONE_MINUS_SRC1_ALPHA;

		case NwBlendFactor::SRC_ALPHA_SAT		:	return GL_SRC_ALPHA_SATURATE;

		mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertBlendOperationGL( NwBlendOp::Enum blendOperation )
	{
		switch(blendOperation)
		{
		case NwBlendOp::MIN			:	return GL_MIN;
		case NwBlendOp::MAX			:	return GL_MAX;
		case NwBlendOp::ADD			:	return GL_FUNC_ADD;
		case NwBlendOp::SUBTRACT		:	return GL_FUNC_SUBTRACT;
		case NwBlendOp::REV_SUBTRACT	:	return GL_FUNC_REVERSE_SUBTRACT;
		mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLenum ConvertPrimitiveTypeGL( NwTopology::Enum primitive_topology )
	{
		switch(primitive_topology)
		{
		case NwTopology::PointList :		return GL_POINTS;
		case NwTopology::LineList :		return GL_LINES;
		case NwTopology::LineStrip :		return GL_LINE_STRIP;
		case NwTopology::TriangleList :	return GL_TRIANGLES;
		case NwTopology::TriangleStrip :	return GL_TRIANGLE_STRIP;
		case NwTopology::TriangleFan :	return GL_TRIANGLE_FAN;
		mxNO_SWITCH_DEFAULT;
		}
		return -1;
	}

	GLuint CreateBufferGL( EBufferType type, UINT size, const void* data )
	{
		GLuint bufferId = 0;
		glGenBuffers( 1, &bufferId );__GL_CHECK_ERRORS;
		GLenum typeGL = ConvertBufferTypeGL(type);
		glBindBuffer( typeGL, bufferId );__GL_CHECK_ERRORS;
		bool dynamic = (data != NULL);
		glBufferData( typeGL, size, data, dynamic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );__GL_CHECK_ERRORS;
		glBindBuffer( typeGL, 0 );__GL_CHECK_ERRORS;
		return bufferId;
	}

	GLbitfield ConvertMapMode( EMapMode mapMode )
	{
		switch(mapMode)
		{
		case Map_Read :					return GL_MAP_READ_BIT;
		case Map_Write :				return GL_MAP_WRITE_BIT;
		case Map_Read_Write :			return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		case Map_Write_Discard :		return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
		case Map_Write_DiscardRange :	return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
		mxNO_SWITCH_DEFAULT;
		}
		return 0;
	}

	const char* UniformTypeToChars( GLenum uniformType )
	{
		switch(uniformType)
		{
		case GL_FLOAT :				return "float";
		case GL_FLOAT_VEC2 :		return "vec2";
		case GL_FLOAT_VEC3 :		return "vec3";
		case GL_FLOAT_VEC4 :		return "vec4";
		case GL_DOUBLE :			return "double";
		case GL_DOUBLE_VEC2 :		return "dvec2";
		case GL_DOUBLE_VEC3 :		return "dvec3";
		case GL_DOUBLE_VEC4 :		return "dvec4";
		case GL_INT :				return "int";
		case GL_INT_VEC2 :			return "ivec2";
		case GL_INT_VEC3 :			return "ivec3";
		case GL_INT_VEC4 :			return "ivec4";
		case GL_UNSIGNED_INT :		return "uint";
		case GL_UNSIGNED_INT_VEC2 :	return "uivec2";
		case GL_UNSIGNED_INT_VEC3 :	return "uivec3";
		case GL_UNSIGNED_INT_VEC4 :	return "uivec4";
		case GL_BOOL :				return "bool";
		case GL_BOOL_VEC2 :			return "bvec2";
		case GL_BOOL_VEC3 :			return "bvec3";
		case GL_BOOL_VEC4 :			return "bvec4";
		case GL_FLOAT_MAT2 :		return "mat2";
		case GL_FLOAT_MAT3 :		return "mat3";
		case GL_FLOAT_MAT4 :		return "mat4";
		case GL_FLOAT_MAT2x3 :		return "mat2x3";
		case GL_FLOAT_MAT2x4 :		return "mat2x4";
		case GL_FLOAT_MAT3x2 :		return "mat3x2";
		case GL_FLOAT_MAT3x4 :		return "mat3x4";
		case GL_FLOAT_MAT4x2 :		return "mat4x2";
		case GL_FLOAT_MAT4x3 :		return "mat4x3";
		case GL_DOUBLE_MAT2 :		return "dmat2";
		case GL_DOUBLE_MAT3 :		return "dmat3";
		case GL_DOUBLE_MAT4 :		return "dmat4";
		case GL_DOUBLE_MAT2x3 :		return "dmat2x3";
		case GL_DOUBLE_MAT2x4 :		return "dmat2x4";
		case GL_DOUBLE_MAT3x2 :		return "dmat3x2";
		case GL_DOUBLE_MAT3x4 :		return "dmat3x4";
		case GL_DOUBLE_MAT4x2 :		return "dmat4x2";
		case GL_DOUBLE_MAT4x3 :		return "dmat4x3";
		case GL_SAMPLER_1D :		return "sampler1D";
		case GL_SAMPLER_2D :		return "sampler2D";
		case GL_SAMPLER_3D :		return "sampler3D";
		case GL_SAMPLER_CUBE :		return "samplerCube";
		case GL_SAMPLER_1D_SHADOW :	return "sampler1DShadow";
		case GL_SAMPLER_2D_SHADOW :	return "sampler2DShadow";
		case GL_SAMPLER_1D_ARRAY :	return "sampler1DArray";
		case GL_SAMPLER_2D_ARRAY :	return "sampler2DArray";
		case GL_SAMPLER_1D_ARRAY_SHADOW :		return "sampler1DArrayShadow";
		case GL_SAMPLER_2D_ARRAY_SHADOW :		return "sampler2DArrayShadow";
		case GL_SAMPLER_2D_MULTISAMPLE :		return "sampler2DMS";
		case GL_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "sampler2DMSArray";
		case GL_SAMPLER_CUBE_SHADOW :		return "samplerCubeShadow";
		case GL_SAMPLER_BUFFER :			return "samplerBuffer";
		case GL_SAMPLER_2D_RECT :			return "sampler2DRect";
		case GL_SAMPLER_2D_RECT_SHADOW :	return "sampler2DRectShadow";
		case GL_INT_SAMPLER_1D :			return "isampler1D";
		case GL_INT_SAMPLER_2D :			return "isampler2D";
		case GL_INT_SAMPLER_3D :			return "isampler3D";
		case GL_INT_SAMPLER_CUBE :			return "isamplerCube";
		case GL_INT_SAMPLER_1D_ARRAY :		return "isampler1DArray";
		case GL_INT_SAMPLER_2D_ARRAY :		return "isampler2DArray";
		case GL_INT_SAMPLER_2D_MULTISAMPLE :		return "isampler2DMS";
		case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "isampler2DMSArray";
		case GL_INT_SAMPLER_BUFFER :	return "isamplerBuffer";
		case GL_INT_SAMPLER_2D_RECT :	return "isampler2DRect";
		case GL_UNSIGNED_INT_SAMPLER_1D :	return "usampler1D";
		case GL_UNSIGNED_INT_SAMPLER_2D :	return "usampler2D";
		case GL_UNSIGNED_INT_SAMPLER_3D :	return "usampler3D";
		case GL_UNSIGNED_INT_SAMPLER_CUBE :	return "usamplerCube";
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY :	return "usampler2DArray";
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY :	return "usampler2DArray";
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE :		return "usampler2DMS";
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "usampler2DMSArray";
		case GL_UNSIGNED_INT_SAMPLER_BUFFER :	return "usamplerBuffer";
		case GL_UNSIGNED_INT_SAMPLER_2D_RECT :	return "usampler2DRect";
		case GL_IMAGE_1D :	return "image1D";
		case GL_IMAGE_2D :	return "image2D";
		case GL_IMAGE_3D :	return "image3D";
		case GL_IMAGE_2D_RECT :	return "image2DRect";
		case GL_IMAGE_CUBE :	return "imageCube";
		case GL_IMAGE_BUFFER :	return "imageBuffer";
		case GL_IMAGE_1D_ARRAY :	return "image1DArray";
		case GL_IMAGE_2D_ARRAY :	return "image2DArray";
		case GL_IMAGE_2D_MULTISAMPLE :	return "image2DMS";
		case GL_IMAGE_2D_MULTISAMPLE_ARRAY :	return "image2DMSArray";
		case GL_INT_IMAGE_1D :	return "iimage1D";
		case GL_INT_IMAGE_2D :	return "iimage2D";
		case GL_INT_IMAGE_3D :	return "iimage3D";
		case GL_INT_IMAGE_2D_RECT :	return "iimage2DRect";
		case GL_INT_IMAGE_CUBE :	return "iimageCube";
		case GL_INT_IMAGE_BUFFER :	return "iimageBuffer";
		case GL_INT_IMAGE_1D_ARRAY :	return "iimage1DArray";
		case GL_INT_IMAGE_2D_ARRAY :	return "iimage2DArray";
		case GL_INT_IMAGE_2D_MULTISAMPLE :	return "iimage2DMS";
		case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY :	return "iimage2DMSArray";
		case GL_UNSIGNED_INT_IMAGE_1D :	return "uimage1D";
		case GL_UNSIGNED_INT_IMAGE_2D :	return "uimage2D";
		case GL_UNSIGNED_INT_IMAGE_3D :	return "uimage3D";
		case GL_UNSIGNED_INT_IMAGE_2D_RECT :	return "uimage2DRect";
		case GL_UNSIGNED_INT_IMAGE_CUBE :	return "uimageCube";
		case GL_UNSIGNED_INT_IMAGE_BUFFER :	return "uimageBuffer";
		case GL_UNSIGNED_INT_IMAGE_1D_ARRAY :	return "uimage1DArray";
		case GL_UNSIGNED_INT_IMAGE_2D_ARRAY :	return "uimage2DArray";
		case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE :	return "uimage2DMS";
		case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY :	return "uimage2DMSArray";
		case GL_UNSIGNED_INT_ATOMIC_COUNTER :	return "atomic_uint";
			mxNO_SWITCH_DEFAULT;
		}
		return mxSTRING_Unknown;
	}

	mxSTOLEN("VSShaderLib: https://github.com/lighthouse3d/VSL/blob/master/VSL/source/vsShaderLib.cpp");
	UINT UniformTypeSize( GLenum uniformType )
	{
		switch(uniformType)
		{
		case GL_FLOAT : return sizeof(float);
        case GL_FLOAT_VEC2 : return sizeof(float)*2;
        case GL_FLOAT_VEC3 : return sizeof(float)*3;
        case GL_FLOAT_VEC4 : return sizeof(float)*4;

        case GL_DOUBLE : return sizeof(double);
        case GL_DOUBLE_VEC2 : return sizeof(double)*2;
        case GL_DOUBLE_VEC3 : return sizeof(double)*3;
        case GL_DOUBLE_VEC4 : return sizeof(double)*4;

        case GL_SAMPLER_1D : return sizeof(int);
        case GL_SAMPLER_2D : return sizeof(int);
        case GL_SAMPLER_3D : return sizeof(int);
        case GL_SAMPLER_CUBE : return sizeof(int);
        case GL_SAMPLER_1D_SHADOW : return sizeof(int);
        case GL_SAMPLER_2D_SHADOW : return sizeof(int);
        case GL_SAMPLER_1D_ARRAY : return sizeof(int);
        case GL_SAMPLER_2D_ARRAY : return sizeof(int);
        case GL_SAMPLER_1D_ARRAY_SHADOW : return sizeof(int);
        case GL_SAMPLER_2D_ARRAY_SHADOW : return sizeof(int);
        case GL_SAMPLER_2D_MULTISAMPLE : return sizeof(int);
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY : return sizeof(int);
        case GL_SAMPLER_CUBE_SHADOW : return sizeof(int);
        case GL_SAMPLER_BUFFER : return sizeof(int);
        case GL_SAMPLER_2D_RECT : return sizeof(int);
        case GL_SAMPLER_2D_RECT_SHADOW : return sizeof(int);
        case GL_INT_SAMPLER_1D : return sizeof(int);
        case GL_INT_SAMPLER_2D : return sizeof(int);
        case GL_INT_SAMPLER_3D : return sizeof(int);
        case GL_INT_SAMPLER_CUBE : return sizeof(int);
        case GL_INT_SAMPLER_1D_ARRAY : return sizeof(int);
        case GL_INT_SAMPLER_2D_ARRAY : return sizeof(int);
        case GL_INT_SAMPLER_2D_MULTISAMPLE : return sizeof(int);
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY : return sizeof(int);
        case GL_INT_SAMPLER_BUFFER : return sizeof(int);
        case GL_INT_SAMPLER_2D_RECT : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_1D : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_2D : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_3D : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_CUBE : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_BUFFER : return sizeof(int);
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT : return sizeof(int);
        case GL_BOOL : return sizeof(int);
        case GL_INT : return sizeof(int);
        case GL_BOOL_VEC2 : return sizeof(int)*2;
        case GL_INT_VEC2 : return sizeof(int)*2;
        case GL_BOOL_VEC3 : return sizeof(int)*3;
        case GL_INT_VEC3 : return sizeof(int)*3;
        case GL_BOOL_VEC4 : return sizeof(int)*4;
        case GL_INT_VEC4 : return sizeof(int)*4;

        case GL_UNSIGNED_INT : return sizeof(int);
        case GL_UNSIGNED_INT_VEC2 : return sizeof(int)*2;
        case GL_UNSIGNED_INT_VEC3 : return sizeof(int)*2;
        case GL_UNSIGNED_INT_VEC4 : return sizeof(int)*2;

        case GL_FLOAT_MAT2 : return sizeof(float)*4;
        case GL_FLOAT_MAT3 : return sizeof(float)*9;
        case GL_FLOAT_MAT4 : return sizeof(float)*16;
        case GL_FLOAT_MAT2x3 : return sizeof(float)*6;
        case GL_FLOAT_MAT2x4 : return sizeof(float)*8;
        case GL_FLOAT_MAT3x2 : return sizeof(float)*6;
        case GL_FLOAT_MAT3x4 : return sizeof(float)*12;
        case GL_FLOAT_MAT4x2 : return sizeof(float)*8;
        case GL_FLOAT_MAT4x3 : return sizeof(float)*12;
        case GL_DOUBLE_MAT2 : return sizeof(double)*4;
        case GL_DOUBLE_MAT3 : return sizeof(double)*9;
        case GL_DOUBLE_MAT4 : return sizeof(double)*16;
        case GL_DOUBLE_MAT2x3 : return sizeof(double)*6;
        case GL_DOUBLE_MAT2x4 : return sizeof(double)*8;
        case GL_DOUBLE_MAT3x2 : return sizeof(double)*6;
        case GL_DOUBLE_MAT3x4 : return sizeof(double)*12;
		case GL_DOUBLE_MAT4x2 : return sizeof(double)*8;
		case GL_DOUBLE_MAT4x3 : return sizeof(double)*12;
			mxNO_SWITCH_DEFAULT;
		}
		return 0;
	}
#if 0
	BaseUniformType GetBaseUniformType( GLenum uniformType )
	{
		switch(uniformType)
		{
		case GL_FLOAT :
		case GL_FLOAT_VEC2 :
		case GL_FLOAT_VEC3 :
		case GL_FLOAT_VEC4 :
			return BUT_FLOAT;
		case GL_DOUBLE :
		case GL_DOUBLE_VEC2 :
		case GL_DOUBLE_VEC3 :
		case GL_DOUBLE_VEC4 :
			return BUT_DOUBLE;
		case GL_INT :
		case GL_INT_VEC2 :
		case GL_INT_VEC3 :
		case GL_INT_VEC4 :
		case GL_UNSIGNED_INT :
		case GL_UNSIGNED_INT_VEC2 :
		case GL_UNSIGNED_INT_VEC3 :
		case GL_UNSIGNED_INT_VEC4 :
			return BUT_DOUBLE;
		case GL_BOOL :	return "bool";
		case GL_BOOL_VEC2 :	return "bvec2";
		case GL_BOOL_VEC3 :	return "bvec3";
		case GL_BOOL_VEC4 :	return "bvec4";
		case GL_FLOAT_MAT2 :	return "mat2";
		case GL_FLOAT_MAT3 :	return "mat3";
		case GL_FLOAT_MAT4 :	return "mat4";
		case GL_FLOAT_MAT2x3 :	return "mat2x3";
		case GL_FLOAT_MAT2x4 :	return "mat2x4";
		case GL_FLOAT_MAT3x2 :	return "mat3x2";
		case GL_FLOAT_MAT3x4 :	return "mat3x4";
		case GL_FLOAT_MAT4x2 :	return "mat4x2";
		case GL_FLOAT_MAT4x3 :	return "mat4x3";
		case GL_DOUBLE_MAT2 :	return "dmat2";
		case GL_DOUBLE_MAT3 :	return "dmat3";
		case GL_DOUBLE_MAT4 :	return "dmat4";
		case GL_DOUBLE_MAT2x3 :	return "dmat2x3";
		case GL_DOUBLE_MAT2x4 :	return "dmat2x4";
		case GL_DOUBLE_MAT3x2 :	return "dmat3x2";
		case GL_DOUBLE_MAT3x4 :	return "dmat3x4";
		case GL_DOUBLE_MAT4x2 :	return "dmat4x2";
		case GL_DOUBLE_MAT4x3 :	return "dmat4x3";
		case GL_SAMPLER_1D :	return "sampler1D";
		case GL_SAMPLER_2D :	return "sampler2D";
		case GL_SAMPLER_3D :	return "sampler3D";
		case GL_SAMPLER_CUBE :	return "samplerCube";
		case GL_SAMPLER_1D_SHADOW :	return "sampler1DShadow";
		case GL_SAMPLER_2D_SHADOW :	return "sampler2DShadow";
		case GL_SAMPLER_1D_ARRAY :	return "sampler1DArray";
		case GL_SAMPLER_2D_ARRAY :	return "sampler2DArray";
		case GL_SAMPLER_1D_ARRAY_SHADOW :	return "sampler1DArrayShadow";
		case GL_SAMPLER_2D_ARRAY_SHADOW :	return "sampler2DArrayShadow";
		case GL_SAMPLER_2D_MULTISAMPLE :	return "sampler2DMS";
		case GL_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "sampler2DMSArray";
		case GL_SAMPLER_CUBE_SHADOW :	return "samplerCubeShadow";
		case GL_SAMPLER_BUFFER :	return "samplerBuffer";
		case GL_SAMPLER_2D_RECT :	return "sampler2DRect";
		case GL_SAMPLER_2D_RECT_SHADOW :	return "sampler2DRectShadow";
		case GL_INT_SAMPLER_1D :	return "isampler1D";
		case GL_INT_SAMPLER_2D :	return "isampler2D";
		case GL_INT_SAMPLER_3D :	return "isampler3D";
		case GL_INT_SAMPLER_CUBE :	return "isamplerCube";
		case GL_INT_SAMPLER_1D_ARRAY :	return "isampler1DArray";
		case GL_INT_SAMPLER_2D_ARRAY :	return "isampler2DArray";
		case GL_INT_SAMPLER_2D_MULTISAMPLE :	return "isampler2DMS";
		case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "isampler2DMSArray";
		case GL_INT_SAMPLER_BUFFER :	return "isamplerBuffer";
		case GL_INT_SAMPLER_2D_RECT :	return "isampler2DRect";
		case GL_UNSIGNED_INT_SAMPLER_1D :	return "usampler1D";
		case GL_UNSIGNED_INT_SAMPLER_2D :	return "usampler2D";
		case GL_UNSIGNED_INT_SAMPLER_3D :	return "usampler3D";
		case GL_UNSIGNED_INT_SAMPLER_CUBE :	return "usamplerCube";
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY :	return "usampler2DArray";
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY :	return "usampler2DArray";
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE :	return "usampler2DMS";
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY :	return "usampler2DMSArray";
		case GL_UNSIGNED_INT_SAMPLER_BUFFER :	return "usamplerBuffer";
		case GL_UNSIGNED_INT_SAMPLER_2D_RECT :	return "usampler2DRect";
		case GL_IMAGE_1D :	return "image1D";
		case GL_IMAGE_2D :	return "image2D";
		case GL_IMAGE_3D :	return "image3D";
		case GL_IMAGE_2D_RECT :	return "image2DRect";
		case GL_IMAGE_CUBE :	return "imageCube";
		case GL_IMAGE_BUFFER :	return "imageBuffer";
		case GL_IMAGE_1D_ARRAY :	return "image1DArray";
		case GL_IMAGE_2D_ARRAY :	return "image2DArray";
		case GL_IMAGE_2D_MULTISAMPLE :	return "image2DMS";
		case GL_IMAGE_2D_MULTISAMPLE_ARRAY :	return "image2DMSArray";
		case GL_INT_IMAGE_1D :	return "iimage1D";
		case GL_INT_IMAGE_2D :	return "iimage2D";
		case GL_INT_IMAGE_3D :	return "iimage3D";
		case GL_INT_IMAGE_2D_RECT :	return "iimage2DRect";
		case GL_INT_IMAGE_CUBE :	return "iimageCube";
		case GL_INT_IMAGE_BUFFER :	return "iimageBuffer";
		case GL_INT_IMAGE_1D_ARRAY :	return "iimage1DArray";
		case GL_INT_IMAGE_2D_ARRAY :	return "iimage2DArray";
		case GL_INT_IMAGE_2D_MULTISAMPLE :	return "iimage2DMS";
		case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY :	return "iimage2DMSArray";
		case GL_UNSIGNED_INT_IMAGE_1D :	return "uimage1D";
		case GL_UNSIGNED_INT_IMAGE_2D :	return "uimage2D";
		case GL_UNSIGNED_INT_IMAGE_3D :	return "uimage3D";
		case GL_UNSIGNED_INT_IMAGE_2D_RECT :	return "uimage2DRect";
		case GL_UNSIGNED_INT_IMAGE_CUBE :	return "uimageCube";
		case GL_UNSIGNED_INT_IMAGE_BUFFER :	return "uimageBuffer";
		case GL_UNSIGNED_INT_IMAGE_1D_ARRAY :	return "uimage1DArray";
		case GL_UNSIGNED_INT_IMAGE_2D_ARRAY :	return "uimage2DArray";
		case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE :	return "uimage2DMS";
		case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY :	return "uimage2DMSArray";
		case GL_UNSIGNED_INT_ATOMIC_COUNTER :	return "atomic_uint";
			mxNO_SWITCH_DEFAULT;
		}
	}
#endif

	ERet MakeShader( EShaderType type, const GLchar* sourceCode, GLint sourceLength, GLuint &shader )
	{
		const GLenum shaderTypeGL = ConvertShaderTypeGL( type );
		shader = glCreateShader( shaderTypeGL );
		mxTODO("use glShaderBinary");
		glShaderSource( shader, 1, &sourceCode, &sourceLength );
		glCompileShader( shader );

		int status;
		glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

		if( status != GL_TRUE )
		{
			int loglen;
			glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &loglen );
			if( loglen > 1 )
			{
				StackAllocator	stackAlloc(gCore.frameAlloc);
				char* log = (char*) stackAlloc.Alloc( loglen );
				glGetShaderInfoLog( shader, loglen, &loglen, log );
				ptERROR("Failed to compile shader: %s\n", log);
			}
			else {
				ptERROR("Failed to compile shader, no log available\n");
			}
			glDeleteShader( shader );
			shader = 0;
			return ERR_COMPILATION_FAILED;
		}

		return ALL_OK;
	}

	ERet CreateVertexShader( const GLchar* sourceCode, GLint sourceLength, GLuint &shader )
	{
		return MakeShader(NwShaderType::Vertex, sourceCode, sourceLength, shader);
	}
	ERet CreateFragmentShader( const GLchar* sourceCode, GLint sourceLength, GLuint &shader )
	{
		return MakeShader(NwShaderType::Pixel, sourceCode, sourceLength, shader);
	}


	ERet MakeProgram( GLuint vertexShader, GLuint fragmentShader, GLuint &program )
	{
		program = glCreateProgram();
		glAttachShader( program, vertexShader );
		glAttachShader( program, fragmentShader );

		// Bind vertex attributes to attribute variables in the vertex shader.
		// NOTE: this must be done before linking the program.

		for( UINT attribIndex = 0; attribIndex < NwAttributeUsage::Count; attribIndex++ )
		{
			const GLuint semantics = NwAttributeUsage::Position + attribIndex;
			glBindAttribLocation( program, semantics, g_attributeNames[semantics] );__GL_CHECK_ERRORS;
		}

		glLinkProgram( program );

		glDetachShader( program, vertexShader );
		glDetachShader( program, fragmentShader );

		int linkStatus;
		glGetProgramiv( program, GL_LINK_STATUS, &linkStatus );

		if( linkStatus != GL_TRUE )
		{
			int loglen;
			glGetProgramiv( program, GL_INFO_LOG_LENGTH, &loglen );
			if( loglen > 1 )
			{
				StackAllocator	stackAlloc(gCore.frameAlloc);
				char* log = (char*) stackAlloc.Alloc( loglen );
				glGetProgramInfoLog( program, loglen, &loglen, log );
				ptERROR("Failed to link program: %s\n", log);
			}
			else {
				ptERROR("Failed to link program, no log available\n");
			}			
			glDeleteProgram( program );
			program = 0;
			return ERR_LINKING_FAILED;
		}

#if LLGL_VALIDATE_PROGRAMS
		glValidateProgram( program );

		int validStatus;
		glGetProgramiv( program, GL_VALIDATE_STATUS, &validStatus );

		if( validStatus != GL_TRUE )
		{
			glDeleteProgram( program );
			program = 0;
			return ERR_VALIDATION_FAILED;
		}
#endif // LLGL_VALIDATE_PROGRAMS

		return ALL_OK;
	}

	const TextureFormatGL gs_textureFormats[NwDataFormat::MAX] =
	{
		{ GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,            GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,            GL_ZERO,                        false }, // BC1
		{ GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,            GL_ZERO,                        false }, // BC2
		{ GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,            GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,            GL_ZERO,                        false }, // BC3
		{ GL_COMPRESSED_LUMINANCE_LATC1_EXT,           GL_COMPRESSED_LUMINANCE_LATC1_EXT,           GL_ZERO,                        false }, // BC4
		{ GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,     GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,     GL_ZERO,                        false }, // BC5
		//{ GL_ETC1_RGB8_OES,                            GL_ETC1_RGB8_OES,                            GL_ZERO,                        false }, // ETC1
		//{ GL_COMPRESSED_RGB8_ETC2,                     GL_COMPRESSED_RGB8_ETC2,                     GL_ZERO,                        false }, // ETC2
		//{ GL_COMPRESSED_RGBA8_ETC2_EAC,                GL_COMPRESSED_RGBA8_ETC2_EAC,                GL_ZERO,                        false }, // ETC2A
		//{ GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_ZERO,                        false }, // ETC2A1
		//{ GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,          GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,          GL_ZERO,                        false }, // PTC12
		//{ GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,          GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,          GL_ZERO,                        false }, // PTC14
		//{ GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,         GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,         GL_ZERO,                        false }, // PTC12A
		//{ GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,         GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,         GL_ZERO,                        false }, // PTC14A
		//{ GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,         GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,         GL_ZERO,                        false }, // PTC22
		//{ GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,         GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,         GL_ZERO,                        false }, // PTC24
		{ GL_ZERO,                                     GL_ZERO,                                     GL_ZERO,                        true  }, // Unknown
		//{ GL_LUMINANCE,                                GL_LUMINANCE,                                GL_UNSIGNED_BYTE,               true  }, // L8
		//{ GL_RGBA,                                     GL_RGBA,                                     GL_UNSIGNED_BYTE,               true  }, // BGRA8
		//{ GL_RGBA16,                                   GL_RGBA,                                     GL_UNSIGNED_BYTE,               true  }, // RGBA16
		//{ GL_RGBA16F,                                  GL_RGBA,                                     GL_HALF_FLOAT,                  true  }, // RGBA16F
		//{ GL_RGB565,                                   GL_RGB,                                      GL_UNSIGNED_SHORT_5_6_5,        true  }, // R5G6B5
		//{ GL_RGBA4,                                    GL_RGBA,                                     GL_UNSIGNED_SHORT_4_4_4_4,      true  }, // RGBA4

		////??? the same as BGRA8
		{ GL_RGBA8,                                    GL_RGBA,                                     GL_UNSIGNED_BYTE,               true  }, // RGBA8

		//{ GL_RGB5_A1,                                  GL_RGBA,                                     GL_UNSIGNED_SHORT_5_5_5_1,      true  }, // RGB5A1
		//{ GL_RGB10_A2,                                 GL_RGBA,                                     GL_UNSIGNED_INT_2_10_10_10_REV, true  }, // R10G10B10A2_UNORM
		//{ GL_R32F,                                     GL_LUMINANCE,                                GL_FLOAT,                       true  }, // R32f
	};
}//namespace NGpu

WGLContext::WGLContext()
{
	m_hWnd = NULL;
	m_hDC = NULL;
	m_hRC = NULL;
}
ERet WGLContext::Setup( HWND hWnd )
{
	DBGOUT("Creating an OpenGL context...\n");

	m_hWnd = hWnd;

	m_hDC = ::GetDC( hWnd );

	PIXELFORMATDESCRIPTOR pfd =
	{   
		sizeof(PIXELFORMATDESCRIPTOR),  // size
		1,                          // version
		PFD_SUPPORT_OPENGL |        // OpenGL window
		PFD_DRAW_TO_WINDOW |        // render to window
		PFD_DOUBLEBUFFER,           // support double-buffering
		PFD_TYPE_RGBA,              // color type
		32,                         // preferred color depth
		0, 0, 0, 0, 0, 0,           // color bits (ignored)
		0,                          // no alpha buffer
		0,                          // alpha bits (ignored)
		0,                          // no accumulation buffer
		0, 0, 0, 0,                 // accum bits (ignored)
		16,                         // depth buffer
		0,                          // no stencil buffer
		0,                          // no auxiliary buffers
		PFD_MAIN_PLANE,             // main layer
		0,                          // reserved
		0, 0, 0,                    // no layer, visible, damage masks
	};

	int pixelFormat = ::ChoosePixelFormat( m_hDC, &pfd );
	chkRET_X_IF_NOT(pixelFormat != 0, ERR_UNKNOWN_ERROR);

	//::DescribePixelFormat( m_hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd );
	//DBGOUT("Pixel format:\n"
	//	"\tiPixelType %d\n"
	//	"\tcColorBits %d\n"
	//	"\tcAlphaBits %d\n"
	//	"\tcDepthBits %d\n"
	//	"\tcStencilBits %d\n"
	//	, pfd.iPixelType
	//	, pfd.cColorBits
	//	, pfd.cAlphaBits
	//	, pfd.cDepthBits
	//	, pfd.cStencilBits
	//	);

	chkRET_X_IF_NOT(::SetPixelFormat( m_hDC, pixelFormat, &pfd ) != 0, ERR_UNKNOWN_ERROR);


#if 1
	
	m_hRC = ::wglCreateContext( m_hDC );

#else

	// Verify that this OpenGL implementation supports the required extensions.

	chkRET_X_IF_NOT( glewIsExtensionSupported("WGL_ARB_create_context"), ERR_UNSUPPORTED_FEATURE );

	// Create an OpenGL 4.x or 3.x rendering context.
	// The descending order of preference is:
	//  OpenGL 4.2
	//  OpenGL 4.1
	//  OpenGL 4.0
	//  OpenGL 3.3
	//  OpenGL 3.2
	//  OpenGL 3.1
	//  OpenGL 3.0 forward compatible

	const int attribListGL42[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	const int attribListGL41[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	const int attribListGL40[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	const int attribListGL33[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	const int attribListGL32[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	const int attribListGL31[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};
	const int attribListGL30[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};


	m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL42);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL41);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL40);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL33);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL32);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL31);
	//m_hRC = wglCreateContextAttribsARB(m_hDC, 0, attribListGL30);

	chkRET_X_IF_NIL( m_hRC, ERR_UNSUPPORTED_FEATURE );

#endif

	chkRET_X_IF_NOT(::wglMakeCurrent( m_hDC, m_hRC ) != 0, ERR_UNKNOWN_ERROR);

	// Experimental or pre-release drivers, however, might not report every available extension through the standard mechanism, in which case GLEW will report it unsupported.
	// To circumvent this situation, the glewExperimental global switch can be turned on by setting it to GL_TRUE before calling glewInit(), which ensures that all extensions with valid entry points will be exposed.
	glewExperimental = GL_TRUE;

	GLenum nGlewInitResult = glewInit();
	if( GLEW_OK != nGlewInitResult )
	{
		ptERROR("glewInit() failed: %s\n", glewGetErrorString(nGlewInitResult));
		return ERR_UNKNOWN_ERROR;
	}

	if( !GLEW_ARB_separate_shader_objects )
	{
		ptPRINT("OpenGL extension ARB_separate_shader_objects is not supported!\n");
		return ERR_UNSUPPORTED_FEATURE;
	}
	ptPRINT("OpenGL driver is initialized.\n");
	ptPRINT("Driver version: %s\n", glGetString(GL_VERSION));
	ptPRINT("Hardware: %s\n", glGetString(GL_RENDERER));
	ptPRINT("Vendor: %s\n", glGetString(GL_VENDOR));

	// Enable super verbose OpenGL debug output (glDebugMessageControl).
	glEnable                  (GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageControlARB  (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageCallbackARB ((GLDEBUGPROCARB)&NGpu::My_OpenGL_Error_Callback, NULL);


	GLint	maxVertexAttributes;
	::glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &maxVertexAttributes );
	DBGOUT("GL_MAX_VERTEX_ATTRIBS = %u", maxVertexAttributes );

	GLint	maxUniformBlockSizeInBytes;
	::glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSizeInBytes );
	DBGOUT("GL_MAX_UNIFORM_BLOCK_SIZE: %u bytes", maxUniformBlockSizeInBytes );

	GLint	maxVertexShaderUniformBlocks;
	GLint	maxGeometryShaderUniformBlocks;
	GLint	maxFragmentShaderUniformBlocks;
	::glGetIntegerv( GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxVertexShaderUniformBlocks );
	::glGetIntegerv( GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &maxGeometryShaderUniformBlocks );
	::glGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &maxFragmentShaderUniformBlocks );
	DBGOUT("CBs: VS=%u, GS=%u, PS=%u", maxVertexShaderUniformBlocks, maxGeometryShaderUniformBlocks, maxFragmentShaderUniformBlocks);

	GLint	maxTextureUnits;
	::glGetIntegerv( GL_MAX_TEXTURE_UNITS, &maxTextureUnits );
	DBGOUT("GL_MAX_TEXTURE_UNITS = %u", maxTextureUnits );

	GLint	maxVertexTextureUnits;
	::glGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUnits );
	DBGOUT("GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = %u", maxVertexTextureUnits );

	GLint	maxGeometryTextureUnits;
	::glGetIntegerv( GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &maxGeometryTextureUnits );
	DBGOUT("GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS = %u", maxGeometryTextureUnits );

	GLint	maxPixelTextureUnits;
	::glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &maxPixelTextureUnits );
	DBGOUT("GL_MAX_TEXTURE_IMAGE_UNITS = %u", maxPixelTextureUnits );

	GLint	maxCombinedTextureUnits;
	::glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits );
	DBGOUT("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = %u", maxCombinedTextureUnits );

	return ALL_OK;
}
void WGLContext::Close()
{
	::wglMakeCurrent( NULL, NULL );
	if( m_hRC != NULL )
	{
		::wglDeleteContext( m_hRC );
		m_hRC = NULL;
	}
	if( m_hDC != NULL )
	{
		::ReleaseDC( m_hWnd, m_hDC );
		m_hDC = NULL;
	}
	m_hWnd = NULL;
}
bool WGLContext::Activate()
{
	int ret = ::wglMakeCurrent( m_hDC, m_hRC );
	return ret == TRUE;
}
bool WGLContext::Present()
{
	::wglMakeCurrent( m_hDC, m_hRC );
	int ret = ::SwapBuffers( m_hDC );
	return ret == TRUE;
}
void WGLContext::GetRect(NwViewport &area)
{
	RECT	rect;
	::GetClientRect(m_hWnd, &rect);
	area.width = rect.right - rect.left;
	area.height = rect.bottom - rect.top;
	area.x = rect.left;
	area.y = rect.top;
}

#endif // LLGL_Driver_Is_OpenGL

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
