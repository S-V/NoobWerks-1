// common OpenGL stuff
#pragma once

// Include GLEW. Always include it before gl.h and glfw.h.
#include <glew/glew.h>
#if MX_AUTOLINK
	#pragma comment( lib, "opengl32.lib" )
	#pragma comment( lib, "glu32.lib" )
	#pragma comment( lib, "glew.lib" )
#endif

#include <GPU/Public/graphics_device.h>

// The name zero is reserved by the NGpu; for some object types,
// zero names a default object of that type, and in others zero
// will never correspond to an actual instance of that object type.
// glGen* functions return 1 on first invocation, 0 can't be a valid NGpu object's name.
#define GL_NULL_HANDLE	(0)



// e.g. __GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer));
#if MX_DEBUG
	#define __GL_CALL( X )		{\
									X;\
									int ec = glGetError();\
									if( ec != GL_NO_ERROR ){\
										ptERROR("OpenGL error in '%s': %s (%d)\n", #X, NGpu::GLErrorToChars(ec), ec);\
									}\
								}
#else
	#define __GL_CALL( X )		(X)
#endif

// e.g. glActiveTexture(GL_TEXTURE0);__GL_CHECK_ERRORS;
#if MX_DEBUG
	#define __GL_CHECK_ERRORS	{\
									int ec = glGetError();\
									if( ec != GL_NO_ERROR ){\
										ptERROR("OpenGL error: %s (%d)\n", NGpu::GLErrorToChars(ec), ec);\
									}\
								}
#else
	#define __GL_CHECK_ERRORS	__noop
#endif

// e.g. if(FAILED_GL(glGetError())) ...
#define __GL_FAILED( X )		((X) != GL_NO_ERROR)



namespace NGpu
{
	const char* GLErrorToChars( int errorCode );

	void My_OpenGL_Error_Callback(
		GLenum        source,
		GLenum        type,
		GLuint        id,
		GLenum        severity,
		GLsizei       length,
		const GLchar* message,
		GLvoid*       userParam
	);

	extern const char* g_attributeNames[NwAttributeUsage::Count];

	NwAttributeUsage::Enum GetAttribIdByName( const char* attribName );

	// Maximum number of OpenGL shader program binary formats (vendor-specific).
	enum { MAX_BINARY_FORMATS = 3 };

	GLenum ConvertBufferTypeGL( EBufferType bufferType );
	GLenum ConvertAttribTypeGL( NwAttributeType::Enum attribType );

	GLenum ConvertShaderTypeGL( EShaderType shaderType );
	GLenum ConvertComparisonFunctionGL( NwComparisonFunc::Enum depthFunc );
	GLenum ConvertStencilOpGL( NwStencilOp::Enum stencilOp );

	GLenum ConvertFillModeGL( NwFillMode::Enum fillMode );
	GLenum ConvertCullModeGL( NwCullMode::Enum cullMode );

	GLenum ConvertAddressModeGL( NwTextureAddressMode::Enum addressMode );

	GLenum ConvertBlendFactorGL( NwBlendFactor::Enum blendFactor );
	GLenum ConvertBlendOperationGL( NwBlendOp::Enum blendOperation );

	GLenum ConvertPrimitiveTypeGL( NwTopology::Enum primitive_topology );

	GLuint CreateBufferGL( EBufferType type, UINT size, const void* data = NULL );
	GLbitfield ConvertMapMode( EMapMode mapMode );

	UINT UniformTypeSize( GLenum uniformType );

	const char* UniformTypeToChars( GLenum uniformType );

	enum BaseUniformType
	{
		BUT_INT,
		BUT_BOOL,
		BUT_FLOAT,
		BUT_DOUBLE,
	};
	BaseUniformType GetBaseUniformType( GLenum uniformType );
	//UINT GetBaseUniformType( GLenum uniformType );

	// enum values correspond to OpenGL ES 2.0 functions (glUniform*)
	enum UniformType
	{
		Uniform1iv,
		Uniform1fv,
		Uniform2fv,
		Uniform3fv,
		Uniform4fv,
		Uniform3x3fv,
		Uniform4x4fv,
		UniformTypeBits = 3
	};

	// creates and compiles a new shader
	ERet MakeShader( EShaderType type, const GLchar* sourceCode, GLint sourceLength, GLuint &shader );

	ERet CreateVertexShader( const GLchar* sourceCode, GLint sourceLength, GLuint &shader );
	ERet CreateFragmentShader( const GLchar* sourceCode, GLint sourceLength, GLuint &shader );

	// creates and links a new program
	ERet MakeProgram( GLuint vertexShader, GLuint fragmentShader, GLuint &program );

	struct TextureFormatGL
	{
		GLenum	internalFormat;
		GLenum	format;
		GLenum	type;
		bool	isSupported;
	};
	extern const TextureFormatGL gs_textureFormats[NwDataFormat::MAX];

}//namespace NGpu

class WGLContext
{
	HWND	m_hWnd;
	HDC		m_hDC;
	HGLRC	m_hRC;
public:
	WGLContext();

	ERet Setup( HWND hWnd );
	void Close();

	bool Activate();
	bool Present();

	void GetRect(NwViewport &area);
	bool IsFullscreen();
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
