#pragma once

#if LLGL_Driver == LLGL_Driver_OpenGL_4plus

#include "Backend.h"
#include "glcommon.h"

namespace llgl
{
	struct DepthStencilSideGL4
	{
		GLenum		stencilFunction;
		GLenum		stencilPassOp;
		GLenum		stencilFailOp;
		GLenum		depthFailOp;
	public:
		DepthStencilSideGL4()
		{
			stencilFunction = GL_ALWAYS;
			stencilPassOp = GL_KEEP;
			stencilFailOp = GL_KEEP;
			depthFailOp = GL_KEEP;
		}
	};
	struct DepthStencilStateGL4
	{
		//UINT64		hash;
		GLboolean	enableDepthTest;
		GLboolean	enableDepthWrite;
		GLboolean	enableStencil;
		GLenum		depthFunction;
		UINT8		stencilReadMask;
		UINT8		stencilWriteMask;
		GLboolean	enableTwoSidedStencil;
		DepthStencilSideGL4	front;
		DepthStencilSideGL4	back;
	public:
		DepthStencilStateGL4()
		{
			depthFunction = GL_LESS;
			enableDepthTest = GL_FALSE;
			enableDepthWrite = GL_TRUE;
			enableStencil = GL_FALSE;
			stencilReadMask = ~0;
			stencilWriteMask = ~0;
		}
		void Destroy() {}
	};
	struct RasterizerStateGL4
	{
		//UINT64	hash;
		GLenum	cullMode;
		GLenum	fillMode;
		bool	enableDepthClip;
	public:
		RasterizerStateGL4()
		{
			cullMode = GL_NONE;
			fillMode = GL_FILL;
			enableDepthClip = true;
		}
		void Destroy() {}
	};
	struct SamplerStateGL4
	{
		//UINT64	hash;
		GLuint	m_id;
	public:
		SamplerStateGL4()
		{
			m_id = GL_NULL_HANDLE;
		}
		void Destroy()
		{
			glDeleteSamplers( 1, &m_id );
		}
	};
	struct BlendStateGL4
	{
		//UINT64			hash;
		GLboolean		enableBlending;
		GLenum			sourceFactor;
		GLenum			destinationFactor;
		GLenum			blendOperation;
		GLboolean		enableAlphaToCoverage;
		GLboolean		enableIndependentBlend;
	public:
		BlendStateGL4()
		{
			enableBlending = GL_FALSE;
			sourceFactor = GL_ONE;
			destinationFactor = GL_ZERO;
			blendOperation = GL_FUNC_ADD;
			enableAlphaToCoverage = GL_FALSE;
			enableIndependentBlend = GL_FALSE;
		}
		void Destroy() {}
	};

	// contains data necessary for glVertexAttribPointer()
	struct VertexAttribGL
	{
		GLenum		dataType;	// GL_FLOAT, GL_UNSIGNED_BYTE
		GLuint		dimension;	// vector dimension: 1,2,3,4
		GLuint		semantic;	// POSITION, COLOR, TEXCOORD3
		GLuint		stream;		// [0..LLGL_MAX_VERTEX_STREAMS)
		GLuint		stride;		// The size of a single vertex
		GLuint		offset;		// byte offset in vertex structure		
		GLboolean	normalize;	// GL_TRUE or GL_FALSE
	public:
		VertexAttribGL()
		{
			dataType = GL_FLOAT;
			dimension = 0;
			semantic = 0;
			stream = 0;
			stride = 0;
			offset = 0;
			normalize = GL_FALSE;
		}
	};

	struct VertexFormatGL
	{
		VertexAttribGL	attribs[LLGL_MAX_VERTEX_ATTRIBS];
		UINT32			attribsMask;	// enabled attributes mask
		UINT16			attribCount;
		UINT16			streamCount;
	public:
		VertexFormatGL()
		{
			attribsMask = 0;
			attribCount = 0;
			streamCount = 1;
		}
		void Destroy() {}
	};

	struct BufferGL4
	{
		GLuint		m_name;
		GLenum		m_type;	// GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
		UINT32		m_size;
		UINT32		m_dynamic;
	public:
		BufferGL4();
		ERet Create( EBufferType type, UINT size, const void* data );
		void Destroy();
		void BindAndUpdate( UINT32 offset, const void* data, UINT32 size );
	};

	struct ShaderGL4
	{
		GLuint		m_id;
		UINT32		m_hash;
	public:
		ShaderGL4();
		void Create( EShaderType type, const GLchar* sourceCode, GLint sourceLength );
		void Destroy();
	};

	struct ProgramGL4
	{
		GLuint		m_id;
	public:
		ProgramGL4();
		ERet Create( const ProgramDescription& pd );
		void Destroy();
	};

	struct TextureGL4
	{
		GLuint			m_id;
		TextureTypeT	m_type;
		UINT8			m_numMips;
	public:
		TextureGL4();
		void Create( const void* data, UINT size );
		void Create( const Texture2DDescription& txInfo, const void* imageData = NULL );
		void Create( const Texture3DDescription& txInfo, const Memory* data = NULL );
		void Destroy();
	private:
		void CreateInternal( const TextureImage& _image, GLenum _target );
	};

	struct ColorTargetGL4
	{
		GLuint		m_id;
		UINT16		m_width;
		UINT16		m_height;
		UINT8		m_format;	// DXGI_FORMAT
		UINT8		m_flags;
	public:
		ColorTargetGL4();
		void Create( const ColorTargetDescription& rtInfo );
		void Destroy() {}
	};
	struct DepthTargetGL4
	{
		GLuint		m_id;		// render buffer id
		UINT16		m_width;
		UINT16		m_height;
		UINT32		m_flags;
	public:
		DepthTargetGL4();
		void Create( const DepthTargetDescription& dtInfo );
		void Destroy() {}
	};

	class DeviceContext
	{
		HBlendState			m_currentBlendState;
		HRasterizerState	m_currentRasterizerState;
		HDepthStencilState	m_currentDepthStencilState;
		UINT8				m_currentStencilReference;

		HProgram			m_currentProgram;

		HInputLayout		m_currentInputLayout;
		UINT32				m_enabledAttribsMask;

		HBuffer				m_currentIndexBuffer;

	public:
		DeviceContext();
		~DeviceContext();

		void Initialize();
		void Shutdown();

		void ResetState();

		void SubmitView( const ViewState& view );

		void UpdateBuffer( HBuffer handle, UINT32 start, const void* data, UINT32 size );
		void UpdateTexture2( HTexture handle, const void* data, UINT32 size );
		void* MapBuffer( HBuffer _handle, UINT32 _start, EMapMode _mode, UINT32 _length );
		void UnmapBuffer( HBuffer _handle );
		//void CopyResource( source, destination );
		void GenerateMips( HColorTarget target );
		void ResolveTexture( HColorTarget target );
		void ReadPixels( HColorTarget source, void *destination, UINT32 bufferSize );

		void SetRasterizerState( HRasterizerState rasterizerState );
		void SetDepthStencilState( HDepthStencilState depthStencilState, UINT8 stencilReference );
		void SetBlendState( HBlendState blendState, const float* blendFactor = NULL, UINT32 sampleMask = ~0 );

		void SetTexture( UINT8 _stage, HResource _texture, HSamplerState _sampler );
		void SetCBuffer( UINT8 _slot, HBuffer _buffer );

		void SubmitBatch( const DrawCall& batch );

		void EndFrame();
	};
}//namespace llgl

#endif // LLGL_Driver == LLGL_Driver_OpenGL_4plus

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
