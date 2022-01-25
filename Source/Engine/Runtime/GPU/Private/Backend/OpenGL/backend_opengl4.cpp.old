#include "Graphics/Graphics_PCH.h"
#pragma hdrstop

#if LLGL_Driver == LLGL_Driver_OpenGL_4plus

#include <Base/Template/THandleManager.h>
#include <typeinfo.h>	// typeid()
#include <Graphics/Device.h>
#include "Driver_OpenGL4.h"

#include "Base/System/Windows/ptWindowsOS.h"

namespace llgl
{
	struct DriverGL4
	{
		DeviceContext	primaryContext;

		// OpenGL context
		WGLContext		context;

		// all created graphics resources
		THandleManager< DepthStencilStateGL4 >	depthStencilStates;
		THandleManager< RasterizerStateGL4 >	rasterizerStates;
		THandleManager< SamplerStateGL4 >		samplerStates;
		THandleManager< BlendStateGL4 >			blendStates;
		THandleManager< ColorTargetGL4 >		colorTargets;
		THandleManager< DepthTargetGL4 >		depthTargets;
		THandleManager< VertexFormatGL >	vertexFormats;
		THandleManager< TextureGL4 >	textures;
		THandleManager< BufferGL4 >	buffers;
		THandleManager< ShaderGL4 >	shaders;
		THandleManager< ProgramGL4 >	programs;

		GLint	binaryFormats[MAX_BINARY_FORMATS];
		GLint	numBinaryFormats;
	};

	mxDECLARE_PRIVATE_DATA( DriverGL4, gDriverData );

	//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#define me	mxGET_PRIVATE_DATA( DriverD3D11, gDriverData )
	//-----------------------------------------------------------------------

	ERet driverInitialize( const void* context )
	{
		mxINITIALIZE_PRIVATE_DATA( gDriverData );

#if (mxPLATFORM == mxPLATFORM_WINDOWS)
		HWND hWnd = (HWND) context;
		mxDO(me.context.Setup(hWnd));
#else
#		error Unsupported platform!
#endif

		::glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &me.numBinaryFormats );
		mxASSERT(me.numBinaryFormats <= mxCOUNT_OF(me.binaryFormats));
		::glGetIntegerv( GL_PROGRAM_BINARY_FORMATS, &me.binaryFormats[0] );

		return ALL_OK;
	}
	void driverShutdown()
	{
		TDestroyLiveObjects(me.depthStencilStates);
		TDestroyLiveObjects(me.rasterizerStates);
		TDestroyLiveObjects(me.samplerStates);
		TDestroyLiveObjects(me.blendStates);
		TDestroyLiveObjects(me.colorTargets);
		TDestroyLiveObjects(me.depthTargets);
		TDestroyLiveObjects(me.vertexFormats);
		TDestroyLiveObjects(me.textures);
		TDestroyLiveObjects(me.buffers);
		TDestroyLiveObjects(me.shaders);
		TDestroyLiveObjects(me.programs);

		me.context.Close();
		mxSHUTDOWN_PRIVATE_DATA( gDriverData );
	}
	HContext driverGetMainContext()
	{
		HContext handle = { &me.primaryContext };
		return handle;
	}
	HInputLayout driverCreateInputLayout( const VertexDescription& desc, const char* name )
	{
		const HInputLayout handle = { me.vertexFormats.Alloc() };
		VertexFormatGL& vertexFormat = me.vertexFormats[ handle.id ];

		vertexFormat.attribsMask = 0;
		vertexFormat.attribCount = desc.attribCount;
		vertexFormat.streamCount = 1;

		for( UINT attribIndex = 0; attribIndex < desc.attribCount; attribIndex++ )
		{
			const VertexElement& elemDesc = desc.attribsArray[ attribIndex ];
			mxASSERT(elemDesc.inputSlot == 0);
			const AttributeType::Enum attribType = (AttributeType::Enum) elemDesc.type;

			VertexAttribGL& attrib = vertexFormat.attribs[ attribIndex ];
			attrib.dataType		= ConvertAttribTypeGL(attribType);
			attrib.dimension	= elemDesc.dimension + 1;
			attrib.semantic		= elemDesc.semantic;
			attrib.offset		= desc.attribOffsets[attribIndex];
			attrib.stream		= elemDesc.inputSlot;
			attrib.stride		= desc.streamStrides[elemDesc.inputSlot];
			attrib.normalize	= elemDesc.normalized;

			vertexFormat.attribsMask |= (1UL << attrib.semantic);
			vertexFormat.streamCount = largest(vertexFormat.streamCount, attrib.stream+1);
		}

		return handle;
	}
	void driverDeleteInputLayout( HInputLayout handle )
	{
		VertexFormatGL& vertexFormat = me.vertexFormats[ handle.id ];
		mxUNUSED(vertexFormat);
		me.vertexFormats.Free( handle.id );
	}

	HTexture driverCreateTexture( const void* data, UINT size )
	{
		HTexture handle = { me.textures.Alloc() };
		TextureGL4& newTexture = me.textures[ handle.id ];
		newTexture.Create( data, size );
		return handle;
	}
	HTexture driverCreateTexture2D( const Texture2DDescription& txInfo, const void* imageData )
	{
		HTexture textureHandle = { me.textures.Alloc() };
		TextureGL4& newTexture = me.textures[ textureHandle.id ];
		newTexture.Create( txInfo, imageData );
		return textureHandle;
	}
	HTexture driverCreateTexture3D( const Texture3DDescription& txInfo, const Memory* initialData )
	{
		HTexture handle = { me.textures.Alloc() };
		UNDONE;
		return handle;
	}
	void driverDeleteTexture( HTexture handle )
	{
		TextureGL4& texture = me.textures[ handle.id ];
		texture.Destroy();
		me.textures.Free( handle.id );
	}

	HColorTarget driverCreateColorTarget( const ColorTargetDescription& rtInfo )
	{
		UNDONE;
		HColorTarget h;
		return h;
	}
	void driverDeleteColorTarget( HColorTarget rt )
	{
		UNDONE;
	}

	HDepthTarget driverCreateDepthTarget( const DepthTargetDescription& dtInfo )
	{
		UNDONE;
		HDepthTarget h;
		return h;
	}
	void driverDeleteDepthTarget( HDepthTarget dt )
	{
		UNDONE;
	}

	HDepthStencilState driverCreateDepthStencilState( const DepthStencilDescription& dsInfo )
	{
		const HDepthStencilState handle = { me.depthStencilStates.Alloc() };

		DepthStencilStateGL4& newDSS = me.depthStencilStates[ handle.id ];
		newDSS.depthFunction = ConvertComparisonFunctionGL(dsInfo.depthFunction);
		newDSS.enableDepthTest = dsInfo.enableDepthTest;
		newDSS.enableDepthWrite = dsInfo.enableDepthWrite;
		newDSS.enableStencil = dsInfo.enableStencil;
		newDSS.stencilReadMask = dsInfo.stencilReadMask;
		newDSS.stencilWriteMask = dsInfo.stencilWriteMask;
		newDSS.enableTwoSidedStencil = false;

		newDSS.front.stencilFunction = ConvertComparisonFunctionGL(dsInfo.frontFace.stencilFunction);
		newDSS.front.stencilPassOp = ConvertStencilOpGL(dsInfo.frontFace.stencilPassOp);
		newDSS.front.stencilFailOp = ConvertStencilOpGL(dsInfo.frontFace.stencilFailOp);
		newDSS.front.depthFailOp = ConvertStencilOpGL(dsInfo.frontFace.depthFailOp);

		newDSS.back.stencilFunction = ConvertComparisonFunctionGL(dsInfo.backFace.stencilFunction);
		newDSS.back.stencilPassOp = ConvertStencilOpGL(dsInfo.backFace.stencilPassOp);
		newDSS.back.stencilFailOp = ConvertStencilOpGL(dsInfo.backFace.stencilFailOp);
		newDSS.back.depthFailOp = ConvertStencilOpGL(dsInfo.backFace.depthFailOp);

		return handle;
	}
	HRasterizerState driverCreateRasterizerState( const RasterizerDescription& rsInfo )
	{
		const HRasterizerState handle = { me.rasterizerStates.Alloc() };

		RasterizerStateGL4& newRS = me.rasterizerStates[ handle.id ];
		newRS.fillMode = ConvertFillModeGL(rsInfo.fillMode);
		newRS.cullMode = ConvertCullModeGL(rsInfo.cullMode);
		newRS.enableDepthClip = rsInfo.enableDepthClip;

		return handle;
	}
	/*
	GL_TEXTURE_MIN_FILTER:

	GL_NEAREST - no filtering, no mipmaps
	GL_LINEAR - filtering, no mipmaps
	GL_NEAREST_MIPMAP_NEAREST - no filtering, sharp switching between mipmaps
	GL_NEAREST_MIPMAP_LINEAR - no filtering, smooth transition between mipmaps
	GL_LINEAR_MIPMAP_NEAREST - filtering, sharp switching between mipmaps
	GL_LINEAR_MIPMAP_LINEAR - filtering, smooth transition between mipmaps

	So:
	GL_LINEAR is bilinear
	GL_LINEAR_MIPMAP_NEAREST is bilinear with mipmaps
	GL_LINEAR_MIPMAP_LINEAR is trilinear
	*/
	static void GetMinMagFilterGL( TextureFilter::Enum e, GLenum &minFilter, GLenum &magFilter )
	{
		switch( e ) {
		case TextureFilter::Min_Mag_Mip_Point :
			minFilter = GL_NEAREST_MIPMAP_NEAREST;
			magFilter = GL_NEAREST;
			break;
		case TextureFilter::Min_Mag_Point_Mip_Linear :
			minFilter = GL_NEAREST_MIPMAP_LINEAR;
			magFilter = GL_NEAREST;
			break;
		case TextureFilter::Min_Point_Mag_Linear_Mip_Point :
			minFilter = GL_NEAREST_MIPMAP_NEAREST;
			magFilter = GL_NEAREST;
			break;
		case TextureFilter::Min_Point_Mag_Mip_Linear :
			minFilter = GL_NEAREST_MIPMAP_LINEAR;
			magFilter = GL_LINEAR;
			break;
		case TextureFilter::Min_Linear_Mag_Mip_Point :
			minFilter = GL_LINEAR_MIPMAP_NEAREST;
			magFilter = GL_NEAREST;
			break;
		case TextureFilter::Min_Linear_Mag_Point_Mip_Linear :
			minFilter = GL_LINEAR_MIPMAP_LINEAR;
			magFilter = GL_NEAREST;
			break;
		case TextureFilter::Min_Mag_Linear_Mip_Point :
			minFilter = GL_LINEAR_MIPMAP_NEAREST;
			magFilter = GL_LINEAR;
			break;
		case TextureFilter::Min_Mag_Mip_Linear :
			minFilter = GL_LINEAR_MIPMAP_LINEAR;
			magFilter = GL_LINEAR;
			break;
		case TextureFilter::Anisotropic :
			minFilter = GL_LINEAR_MIPMAP_LINEAR;
			magFilter = GL_LINEAR;
			break;
		mxNO_SWITCH_DEFAULT;
		}
	}

	HSamplerState driverCreateSamplerState( const SamplerDescription& ssInfo )
	{
		GLuint	samplerStateGL = 0;
		glGenSamplers( 1, &samplerStateGL );

		GLenum	minFilterGL;
		GLenum	magFilterGL;
		GetMinMagFilterGL(ssInfo.filter, minFilterGL, magFilterGL);

		glSamplerParameteri( samplerStateGL, GL_TEXTURE_MIN_FILTER, minFilterGL );
		glSamplerParameteri( samplerStateGL, GL_TEXTURE_MAG_FILTER, magFilterGL );
		glSamplerParameteri( samplerStateGL, GL_TEXTURE_MAX_ANISOTROPY_EXT, ssInfo.maxAnisotropy );

		glSamplerParameteri( samplerStateGL, GL_TEXTURE_WRAP_S, ConvertAddressModeGL(ssInfo.addressU) );
		glSamplerParameteri( samplerStateGL, GL_TEXTURE_WRAP_T, ConvertAddressModeGL(ssInfo.addressV) );
		glSamplerParameteri( samplerStateGL, GL_TEXTURE_WRAP_R, ConvertAddressModeGL(ssInfo.addressW) );

        glSamplerParameterfv( samplerStateGL, GL_TEXTURE_BORDER_COLOR, ssInfo.borderColor );

		glSamplerParameterf( samplerStateGL, GL_TEXTURE_MIN_LOD, ssInfo.minLOD );
        glSamplerParameterf( samplerStateGL, GL_TEXTURE_MAX_LOD, ssInfo.maxLOD );
        glSamplerParameterf( samplerStateGL, GL_TEXTURE_LOD_BIAS, ssInfo.mipLODBias );

		GLenum compareModeGL = (ssInfo.comparison != ComparisonFunc::Never) ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE;
        glSamplerParameteri( samplerStateGL, GL_TEXTURE_COMPARE_MODE, compareModeGL);
        glSamplerParameteri( samplerStateGL, GL_TEXTURE_COMPARE_FUNC, ConvertComparisonFunctionGL(ssInfo.comparison));

		const HSamplerState handle = { me.samplerStates.Alloc() };
		SamplerStateGL4& newSS = me.samplerStates[ handle.id ];
		newSS.m_id = samplerStateGL;

		return handle;
	}
	HBlendState driverCreateBlendState( const BlendDescription& bsInfo )
	{
		const HBlendState handle = { me.blendStates.Alloc() };

		BlendStateGL4& newBlendState = me.blendStates[ handle.id ];
		newBlendState.enableBlending = bsInfo.enableBlending;
		newBlendState.sourceFactor = ConvertBlendFactorGL(bsInfo.color.sourceFactor);
		newBlendState.destinationFactor = ConvertBlendFactorGL(bsInfo.color.destinationFactor);
		newBlendState.blendOperation = ConvertBlendOperationGL(bsInfo.color.operation);
		newBlendState.enableAlphaToCoverage = GL_FALSE;
		newBlendState.enableIndependentBlend = GL_FALSE;

		return handle;
	}

	void driverDeleteDepthStencilState( HDepthStencilState ds )
	{
		me.depthStencilStates.Free( ds.id );
	}
	void driverDeleteRasterizerState( HRasterizerState rs )
	{	
		me.rasterizerStates.Free( rs.id );
	}
	void driverDeleteSamplerState( HSamplerState ss )
	{
		me.samplerStates[ ss.id ].Destroy();
		me.samplerStates.Free( ss.id );
	}
	void driverDeleteBlendState( HBlendState bs )
	{
		me.blendStates.Free( bs.id );
	}

	HBuffer driverCreateBuffer( EBufferType type, const void* data, UINT size )
	{
		const HBuffer handle = { me.buffers.Alloc() };
		BufferGL4& newBuffer = me.buffers[ handle.id ];
		newBuffer.Create(type, size, data );
		return handle;
	}
	void driverDeleteBuffer( HBuffer handle )
	{
		BufferGL4& buffer = me.buffers[ handle.id ];
		buffer.Destroy();
		me.buffers.Free( handle.id );
	}

	HShader driverCreateShader( EShaderType shaderType, const void* compiledCode, UINT bytecodeLength )
	{
		const HShader handle = { me.shaders.Alloc() };
		const GLchar* sourceCode = (GLchar*) compiledCode;
		const GLint sourceLength = (GLint) bytecodeLength;
		me.shaders[ handle.id ].Create( shaderType, sourceCode, sourceLength );
		return handle;
	}
	void driverDeleteShader( HShader handle )
	{
		me.shaders[ handle.id ].Destroy();
		me.shaders.Free( handle.id );
	}

	HProgram driverCreateProgram( const ProgramDescription& pd )
	{
		const HProgram handle = { me.programs.Alloc() };
		ProgramGL4& program = me.programs[ handle.id ];
		program.Create(pd);
		return handle;
	}
	void driverDeleteProgram( HProgram handle )
	{
		me.programs[ handle.id ].Destroy();
		me.programs.Free( handle.id );
	}
	HResource driverGetShaderResource( HBuffer br )
	{
		HResource handle = { LLGL_NULL_HANDLE };
		UNDONE;
		return handle;
	}
	HResource driverGetShaderResource( HTexture tx )
	{
		HResource handle = { tx.id };
		return handle;
	}
	HResource driverGetShaderResource( HColorTarget rt )
	{
		HResource handle = { LLGL_NULL_HANDLE };
		UNDONE;
		return handle;
	}
	HResource driverGetShaderResource( HDepthTarget rt )
	{
		HResource handle = { LLGL_NULL_HANDLE };
		UNDONE;
		return handle;
	}

	void driverSubmitFrame( CommandBuffer & commands, UINT size )
	{
		me.primaryContext.EndFrame();

		// The function glFlush sends the command buffer to the graphics hardware.
		// It blocks until commands are submitted to the hardware
		// but does not wait for the commands to finish executing.
		glFlush();__GL_CHECK_ERRORS;

		me.context.Present();
	}

	BufferGL4::BufferGL4()
	{
		m_name = GL_NULL_HANDLE;
		m_type = 0;
		m_size = 0;
		m_dynamic = 0;
	}
	ERet BufferGL4::Create( EBufferType type, UINT size, const void* data )
	{
		glGenBuffers( 1, &m_name );__GL_CHECK_ERRORS;
		m_type = ConvertBufferTypeGL(type);
		m_size = size;

		m_dynamic = (data == NULL);
		m_dynamic |= (type == Buffer_Uniform);	// Uniform buffers are always dynamic.

		glBindBuffer( m_type, m_name );__GL_CHECK_ERRORS;
		glBufferData( m_type, size, data, m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW );__GL_CHECK_ERRORS;
		glBindBuffer( m_type, 0 );__GL_CHECK_ERRORS;

		DBGOUT("Create buffer: %u (%u bytes)\n", m_name, size);

		return ALL_OK;
	}
	void BufferGL4::Destroy()
	{
		if( m_name != GL_NULL_HANDLE )
		{
			glDeleteBuffers( 1, &m_name );__GL_CHECK_ERRORS;
			m_name = GL_NULL_HANDLE;
		}
		m_size = 0;
		m_type = 0;
		m_dynamic = 0;
	}
	void BufferGL4::BindAndUpdate( UINT32 offset, const void* data, UINT32 size )
	{
		mxASSERT(m_dynamic);
		mxASSERT(m_size >= size);
		__GL_CALL(glBindBuffer( m_type, m_name ));
		if( m_size == size )
		{
			// Replacing the whole buffer can help the driver to avoid pipeline stalls
			__GL_CALL(glBufferData( m_type, size, data, GL_DYNAMIC_DRAW ));
		}
		else
		{
			__GL_CALL(glBufferSubData( m_type, offset, size, data ));
		}
	}

	ShaderGL4::ShaderGL4()
	{
		m_id = GL_NULL_HANDLE;
		m_hash = 0;
	}
	void ShaderGL4::Create( EShaderType type, const GLchar* sourceCode, GLint sourceLength )
	{
		const GLenum shaderTypeGL = ConvertShaderTypeGL( type );
		m_id = glCreateShader( shaderTypeGL );__GL_CHECK_ERRORS;
		//DBGOUT("glCreateShader(): %u\n", m_id);
		DBGOUT("Create %s : %u\n", g_shaderTypeName[type].buffer, m_id);
		//@TODO: use glShaderBinary
		__GL_CALL(glShaderSource( m_id, 1, &sourceCode, &sourceLength ));
		__GL_CALL(glCompileShader( m_id ));

		int status;
		__GL_CALL(glGetShaderiv( m_id, GL_COMPILE_STATUS, &status ));

		if( status != GL_TRUE )
		{
			int loglen;
			__GL_CALL(glGetShaderiv( m_id, GL_INFO_LOG_LENGTH, &loglen ));
			if( loglen > 1 )
			{
				ScopedStackAlloc	stackAlloc(gCore.frameAlloc);
				char* log = (char*) stackAlloc.Alloc( loglen );
				__GL_CALL(glGetShaderInfoLog( m_id, loglen, &loglen, log ));
				ptERROR("Failed to compile shader: %s\n", log);
			}
			else {
				ptERROR("Failed to compile shader, no log available\n");
			}
			this->Destroy();
		}

		m_hash = MurmurHash32(sourceCode, sourceLength);
	}
	void ShaderGL4::Destroy()
	{
		if( m_id != GL_NULL_HANDLE )
		{
			__GL_CALL(glDeleteShader(m_id));
			m_id = GL_NULL_HANDLE;
		}
		m_hash = 0;
	}

	static UINT64 GetProgramHash( const ProgramDescription& pd )
	{
		const ShaderGL4& vertexShader = me.shaders[ pd.shaders[ShaderVertex].id ];
		const ShaderGL4& fragmentShader = me.shaders[ pd.shaders[ShaderFragment].id ];

		UINT64 hash;
		hash = UINT64(fragmentShader.m_hash);
		hash |= UINT64(vertexShader.m_hash) << 32UL;
		return hash;
	}

	ProgramGL4::ProgramGL4()
	{
		m_id = GL_NULL_HANDLE;
	}
	ERet ProgramGL4::Create( const ProgramDescription& pd )
	{
		const ProgramBindingsOGL* bindings = pd.bindings;

		//TODO: save the binaries at the first run and then use the binary blob
		// instead of recompiling the shaders and relinking the shader program.

		m_id = glCreateProgram();__GL_CHECK_ERRORS;

		//DBGOUT("glCreateProgram(): %u\n", m_id);
	
		bool bLoadedCachedProgram = false;

#if LLGL_SUPPORT_BINARY_GL_PROGRAMS
		const UINT64 programHash = GetProgramHash( pd );

		const GLint binaryProgramFormat = (me.numBinaryFormats > 0) ? me.binaryFormats[0] : 0;
		const bool bUseCachedProgramBinaries = (binaryProgramFormat != 0);

		if( bUseCachedProgramBinaries )
		{
			const GLsizei programBinarySize = g_client->cacheReadSize( programHash );
			if( programBinarySize > 0 )
			{
				ScopedStackAlloc	tempAlloc(gCore.frameAlloc);
				void* programBinary = tempAlloc.Alloc( programBinarySize );

				if( g_client->cacheRead( programHash, programBinary, programBinarySize ) )
				{
					__GL_CALL(glProgramBinary( m_id, binaryProgramFormat, programBinary, programBinarySize ));
					bLoadedCachedProgram = true;
				}
			}
		}
#endif // #if LLGL_SUPPORT_BINARY_GL_PROGRAMS

		if( !bLoadedCachedProgram )
		{
#if LLGL_SUPPORT_BINARY_GL_PROGRAMS
			if( bUseCachedProgramBinaries )
			{
				// Indicate to the implementation the intention of the application to retrieve the program's binary representation with glGetProgramBinary.
				// The implementation may use this information to store information that may be useful for a future query of the program's binary.
				__GL_CALL(glProgramParameteri( m_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE ));
			}
#endif // #if LLGL_SUPPORT_BINARY_GL_PROGRAMS

			TStaticList< GLuint, ShaderTypeCount >	attachedShaders;

			for( UINT shaderType = 0; shaderType < ShaderTypeCount; shaderType++ )
			{
				const HShader shaderHandle = pd.shaders[ shaderType ];
				if( shaderHandle.IsValid() )
				{
					const ShaderGL4& shader = me.shaders[ shaderHandle.id ];
					__GL_CALL(glAttachShader( m_id, shader.m_id ));
					DBGOUT("glAttachShader( %u, %u );\n", m_id, shader.m_id);
					attachedShaders.Add( shader.m_id );
				}
			}


			// Bind vertex attributes to attribute variables in the vertex shader.
			// NOTE: this must be done before linking the program, but after attaching the vertex shader.

			if( bindings )
			{
				for( UINT attribIndex = 0; attribIndex < VertexAttribute::Count; attribIndex++ )
				{
					if( bindings->activeVertexAttributes & (1UL << attribIndex) )
					{
						__GL_CALL(glBindAttribLocation( m_id, (GLuint)attribIndex, g_attributeNames[attribIndex] ));
					}
				}
			}
			else
			{
				for( UINT attribIndex = 0; attribIndex < VertexAttribute::Count; attribIndex++ )
				{
					__GL_CALL(glBindAttribLocation( m_id, (GLuint)attribIndex, g_attributeNames[attribIndex] ));
				}
			}


			__GL_CALL(glLinkProgram( m_id ));


			//for( UINT i = 0; i < attachedShaders.Num(); i++ )
			//{
			//	__GL_CALL(glDetachShader( m_id, attachedShaders[ i ] ));
			//}


			int logLength = 0;
			__GL_CALL(glGetProgramiv( m_id, GL_INFO_LOG_LENGTH, &logLength ));
			if( logLength > 1 )
			{
				ScopedStackAlloc	stackAlloc(gCore.frameAlloc);
				char* log = (char*) stackAlloc.Alloc( logLength );
				__GL_CALL(glGetProgramInfoLog( m_id, logLength, &logLength, log ));
				ptPRINT("%s\n", log);
			}

			int linkStatus = GL_FALSE;
			__GL_CALL(glGetProgramiv( m_id, GL_LINK_STATUS, &linkStatus ));

			if( linkStatus != GL_TRUE )
			{
				this->Destroy();
				return ERR_LINKING_FAILED;
			}

#if LLGL_VALIDATE_PROGRAMS
			__GL_CALL(glValidateProgram( m_id ));

			int validStatus;
			__GL_CALL(glGetProgramiv( m_id, GL_VALIDATE_STATUS, &validStatus ));

			if( validStatus != GL_TRUE )
			{
				this->Destroy();
				return ERR_VALIDATION_FAILED;
			}
#endif // LLGL_VALIDATE_PROGRAMS


#if LLGL_SUPPORT_BINARY_GL_PROGRAMS
			// Get the binary representation of the program object's compiled and linked executable source.
			if( bUseCachedProgramBinaries )
			{
				GLint	programBinarySize;
				__GL_CALL(glGetProgramiv( m_id, GL_PROGRAM_BINARY_LENGTH, &programBinarySize ));
				if( programBinarySize > 0 )
				{
					ScopedStackAlloc	tempAlloc(gCore.frameAlloc);
					void* programBinary = tempAlloc.Alloc( programBinarySize );

					__GL_CALL(glGetProgramBinary( m_id, programBinarySize, NULL/*length*/, (GLenum*)&binaryProgramFormat, programBinary ));

					g_client->cacheWrite( programHash, programBinary, programBinarySize );
				}
			}
#endif // #if LLGL_SUPPORT_BINARY_GL_PROGRAMS

		}//if( !bLoadedCachedProgram )


		// Specify binding points.
		// This sets state in the program (which is why you shouldn't be doing it every frame).

		if( bindings )
		{
			// This ("bind-to-modify") is necessary before calling glUniform*.
			__GL_CALL(glUseProgram( m_id ));

			// Assign binding points to the uniform blocks.
			for( UINT iUBO = 0; iUBO < bindings->cbuffers.Num(); iUBO++ )
			{
				const CBufferBindingOGL& binding = bindings->cbuffers[ iUBO ];

				const GLuint uniformBlockIndex = glGetUniformBlockIndex( m_id, binding.name.ToPtr() );__GL_CHECK_ERRORS;
				if( uniformBlockIndex != GL_INVALID_INDEX )
				{
					__GL_CALL(glUniformBlockBinding( m_id, uniformBlockIndex, binding.slot ));
				}
			}
			// Assign texture unit indices to the shader samplers.
			for( UINT iSampler = 0; iSampler < bindings->samplers.Num(); iSampler++ )
			{
				const SamplerBindingOGL& binding = bindings->samplers[ iSampler ];

				const GLint samplerLocation = glGetUniformLocation( m_id, binding.name.ToPtr() );__GL_CHECK_ERRORS;
				if( samplerLocation != -1 )
				{
					__GL_CALL(glUniform1i( samplerLocation, binding.slot ));
				}
			}

			__GL_CALL(glUseProgram( 0 ));
		}

		if(MX_DEBUG)
		{
			DBGOUT("Created program: %u (VS: %u, PS: %u)\n", m_id, pd.shaders[ShaderVertex].id, pd.shaders[ShaderFragment].id);
		}

		return ALL_OK;
	}
	void ProgramGL4::Destroy()
	{
		if( m_id != GL_NULL_HANDLE )
		{
			__GL_CALL(glDeleteProgram(m_id));
			m_id = GL_NULL_HANDLE;
		}
	}

	TextureGL4::TextureGL4()
	{
		m_id = GL_NULL_HANDLE;
		m_type = TextureType::TEXTURE_2D;
		m_numMips = 0;
	}
	void TextureGL4::Create( const void* data, UINT size )
	{
		const TextureHeader& header = *(TextureHeader*) data;
		const void* imageData = mxAddByteOffset(data, sizeof(TextureHeader));

		const PixelFormatT textureFormat = (PixelFormat::Enum) header.format;

		TextureImage image;
		image.data		= imageData;
		image.size		= header.size;
		image.width		= header.width;
		image.height	= header.height;
		image.depth		= header.depth;
		image.format	= textureFormat;
		image.numMips	= header.numMips;
		image.isCubeMap	= false;

		this->CreateInternal(image, GL_TEXTURE_2D);
	}
	void TextureGL4::Create( const Texture2DDescription& txInfo, const void* imageData )
	{
		TextureImage image;
		image.data		= imageData;
		image.size		= CalculateTextureSize(txInfo.width, txInfo.height, txInfo.format, txInfo.numMips);
		image.width		= txInfo.width;
		image.height	= txInfo.height;
		image.depth		= 1;
		image.format	= txInfo.format;
		image.numMips	= txInfo.numMips;
		image.isCubeMap	= false;

		this->CreateInternal(image, GL_TEXTURE_2D);
	}
	void TextureGL4::Create( const Texture3DDescription& txInfo, const Memory* data )
	{
		UNDONE;
	}
	void TextureGL4::Destroy()
	{
		if( m_id != GL_NULL_HANDLE )
		{
			__GL_CALL(glDeleteTextures( 1, &m_id ));
			m_id = GL_NULL_HANDLE;
		}
	}
	void TextureGL4::CreateInternal( const TextureImage& _image, GLenum _target )
	{
		mxASSERT(_target == GL_TEXTURE_2D);

		const PixelFormat::Enum textureFormat = _image.format;

		const bool blockCompressed = PixelFormat::IsCompressed(textureFormat);

		const GLenum internalFormat = gs_textureFormats[ textureFormat ].internalFormat;
		const GLenum format = gs_textureFormats[ textureFormat ].format;
		const GLenum type = gs_textureFormats[ textureFormat ].type;

		MipLevel	mips[ LLGL_MAX_TEXTURE_MIP_LEVELS ];
		mxASSERT(_image.numMips <= mxCOUNT_OF(mips));
		ParseMipLevels( _image, 0, mips, mxCOUNT_OF(mips) );

		__GL_CALL(glGenTextures( 1, &m_id ));
		__GL_CALL(glBindTexture( _target, m_id ));

		__GL_CALL(glTexParameteri( _target, GL_TEXTURE_BASE_LEVEL, 0 ));
		__GL_CALL(glTexParameteri( _target, GL_TEXTURE_MAX_LEVEL, 0 ));
#if 1
		// http://www.opengl.org/wiki/Common_Mistakes :
		// Better code would be to use texture storage functions (if you have OpenGL 4.2 or ARB_texture_storage) to allocate the texture's storage, then upload with glTexSubImage2D?:
		__GL_CALL(glTexStorage2D( _target, _image.numMips, internalFormat, _image.width, _image.height ));

		for( UINT lodIndex = 0; lodIndex < _image.numMips; lodIndex++ )
		{
			const MipLevel& mip = mips[ lodIndex ];

			if( blockCompressed )
			{
				__GL_CALL(glCompressedTexSubImage2D(
					_target,
					lodIndex,
					0,
					0,
					mip.width,
					mip.height,
					format,
					mip.size,
					mip.data
				));
			}
			else
			{
				__GL_CALL(glTexSubImage2D(
					_target,
					lodIndex,
					internalFormat,
					mip.width,
					mip.height,
					0,
					format,
					type,
					mip.data
				));
			}
		}
#else
		for( UINT lodIndex = 0; lodIndex < _image.numMips; lodIndex++ )
		{
			const MipLevel& mip = mips[ lodIndex ];
			if( blockCompressed )
			{
				__GL_CALL(glCompressedTexImage2D(
					target,
					lodIndex,
					internalFormat,
					mip.width,
					mip.height,
					0,
					mip.size,
					mip.data
				));
			}
			else
			{
				__GL_CALL(glTexImage2D(
					target,
					lodIndex,
					internalFormat,
					mip.width,
					mip.height,
					0,
					format,
					type,
					mip.data
				));
			}
		}
#endif
		__GL_CALL(glBindTexture( _target, 0 ));

		m_type = TextureType::TEXTURE_2D;
		m_numMips = _image.numMips;

		DBGOUT("Create texture : %u\n", m_id);
	}

	DeviceContext::DeviceContext()
	{
		this->ResetState();
	}
	DeviceContext::~DeviceContext()
	{

	}
	void DeviceContext::Initialize()
	{
		me.context.Activate();
	}
	void DeviceContext::Shutdown()
	{
	}
	void DeviceContext::ResetState()
	{
		m_currentBlendState.SetNil();
		m_currentRasterizerState.SetNil();
		m_currentDepthStencilState.SetNil();
		m_currentStencilReference = 0;

		m_currentProgram.SetNil();

		m_currentInputLayout.SetNil();
		m_enabledAttribsMask = 0;

		m_currentIndexBuffer.SetNil();
	}
	void DeviceContext::SubmitView( const ViewState& view )
	{
		const UINT numRenderTargets = view.targetCount;
		mxASSERT( numRenderTargets == 0 );

		const float* rgba = view.clearColors[0];
		glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);

		glClearDepth(view.depth);
		glClearStencil(view.stencil);

		GLbitfield clearFlagsGL = 0;
		if( view.flags & ClearColor0 ) {
			clearFlagsGL |= GL_COLOR_BUFFER_BIT;
		}
		if( view.flags & ClearDepth ) {
			clearFlagsGL |= GL_DEPTH_BUFFER_BIT;
		}
		if( view.flags & ClearStencil ) {
			clearFlagsGL |= GL_STENCIL_BUFFER_BIT;
		}

		if( clearFlagsGL ) {
			glClear(clearFlagsGL);
		}

		// Define the mapping of the near and far clipping planes to the window coordinates.
		//glDepthRangef( -1.0f, 1.0f );

		const Viewport& vp = view.viewport;
		glViewport( vp.x, vp.y, vp.width, vp.height );

		//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}
	void DeviceContext::UpdateBuffer( HBuffer handle, UINT32 start, const void* data, UINT32 size )
	{
		BufferGL4& bufferGL = me.buffers[ handle.id ];
		bufferGL.BindAndUpdate(start, data, size);
	}
	void DeviceContext::UpdateTexture2( HTexture handle, const void* data, UINT32 size )
	{
		UNDONE;
	}
	void* DeviceContext::MapBuffer( HBuffer _handle, UINT32 _start, EMapMode _mode, UINT32 _length )
	{
		BufferGL4& bufferGL = me.buffers[ _handle.id ];
		const GLenum target = bufferGL.m_type;
		glBindBuffer(target, bufferGL.m_name);__GL_CHECK_ERRORS;
		const GLbitfield flags = ConvertMapMode(_mode);
		void* mappedData = glMapBufferRange((GLenum)target, (GLintptr)_start, (GLsizeiptr)_length, flags);__GL_CHECK_ERRORS;
		return mappedData;
	}
	void DeviceContext::UnmapBuffer( HBuffer _handle )
	{
		BufferGL4& bufferGL = me.buffers[ _handle.id ];
		const GLenum target = bufferGL.m_type;
		glBindBuffer(target, bufferGL.m_name);__GL_CHECK_ERRORS;
		glUnmapBuffer(target);__GL_CHECK_ERRORS;
	}
	////void CopyResource( source, destination );
	//void GenerateMips( HColorTarget target );
	//void ResolveTexture( HColorTarget target );
	//void ReadPixels( HColorTarget source, void *destination, UINT32 bufferSize );
	void DeviceContext::SetRasterizerState( HRasterizerState rasterizerState )
	{
		if( rasterizerState != m_currentRasterizerState )
		{
			m_currentRasterizerState = rasterizerState;

			const RasterizerStateGL4& rasterizerStateGL = me.rasterizerStates[ rasterizerState.id ];

			if( rasterizerStateGL.cullMode != GL_NONE ) {
				glEnable(GL_CULL_FACE);
				glCullFace(rasterizerStateGL.cullMode);
			} else {
				glDisable(GL_CULL_FACE);
			}

			glPolygonMode(GL_FRONT_AND_BACK, rasterizerStateGL.fillMode);

			// Depth clamping turns off camera near/far plane clipping altogether.
			// Instead, the depth of each fragment is clamped to the [-1, 1] range in NDC space.
			if( rasterizerStateGL.enableDepthClip ) {
				glDisable(GL_DEPTH_CLAMP);
			} else {
				glEnable(GL_DEPTH_CLAMP);
			}
		}
	}
	void DeviceContext::SetDepthStencilState( HDepthStencilState depthStencilState, UINT8 stencilReference )
	{
		if( depthStencilState != m_currentDepthStencilState || stencilReference != m_currentStencilReference )
		{
			m_currentDepthStencilState = depthStencilState;
			m_currentStencilReference = stencilReference;

			const DepthStencilStateGL4& depthStencilStateGL = me.depthStencilStates[ depthStencilState.id ];

			// Set depth states.
			if( depthStencilStateGL.enableDepthTest )
			{
				glEnable(GL_DEPTH_TEST);
				glDepthMask(depthStencilStateGL.enableDepthWrite);
				glDepthFunc(depthStencilStateGL.depthFunction);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
			// Set stencil states.
			if( depthStencilStateGL.enableStencil )
			{
				UNDONE;
				glEnable(GL_STENCIL_TEST);
				glStencilMask(depthStencilStateGL.stencilWriteMask);
				if( depthStencilStateGL.enableTwoSidedStencil )
				{
					UNDONE;
				}
			} else {
				glDisable(GL_STENCIL_TEST);
			}
		}
	}
	void DeviceContext::SetBlendState( HBlendState blendState, const float* blendFactor /*= NULL*/, UINT32 sampleMask /*= ~0*/ )
	{
		if( blendState != m_currentBlendState )
		{
			m_currentBlendState = blendState;

			const BlendStateGL4& blendStateGL = me.blendStates[ blendState.id ];

			if( blendStateGL.enableBlending ) {
				glEnable(GL_BLEND);
			} else {
				glDisable(GL_BLEND);
			}
			mxUNDONE;
		}
	}
	void SetTexture( UINT8 _stage, HResource _texture, HSamplerState _sampler )
	{
		mxASSERT( _texture.IsValid() );
		mxASSERT( _sampler.IsValid() );

		const TextureGL4& textureGL = me.textures[ _texture.id ];
		glActiveTexture( GL_TEXTURE0 + _stage );__GL_CHECK_ERRORS;
		glBindTexture( GL_TEXTURE_2D, textureGL.m_id );__GL_CHECK_ERRORS;

		const SamplerStateGL4& samplerGL = me.samplerStates[ _sampler.id ];
		glBindSampler( _stage, samplerGL.m_id );__GL_CHECK_ERRORS;
	}
	void SetCBuffer( UINT8 _slot, HBuffer _buffer )
	{
		mxASSERT( _buffer.IsValid() );
		const BufferGL4& bufferGL = me.buffers[ _buffer.id ];
		glBindBufferBase( GL_UNIFORM_BUFFER, _slot, bufferGL.m_name );__GL_CHECK_ERRORS;
	}
	void DeviceContext::SubmitBatch( const DrawCall& batch )
	{
		// Bind shader program.
		if( batch.program != m_currentProgram )
		{
			m_currentProgram = batch.program;

			const ProgramGL4& program = me.programs[ batch.program.id ];
			__GL_CALL(glUseProgram(program.m_id));
		}

		for( UINT iCB = 0; iCB < mxCOUNT_OF(batch.CBs); iCB++ )
		{
			const HBuffer handle = batch.CBs[ iCB ];
			if( handle.IsValid() )
			{
				const BufferGL4& bufferGL = me.buffers[ handle.id ];
				__GL_CALL(glBindBufferBase( GL_UNIFORM_BUFFER, iCB, bufferGL.m_name ));
			}
		}
		for( UINT iSR = 0; iSR < mxCOUNT_OF(batch.SRs); iSR++ )
		{
			const HResource handle = batch.SRs[ iSR ];
			if( handle.IsValid() )
			{
				const TextureGL4& textureGL = me.textures[ handle.id ];
				__GL_CALL(glActiveTexture(GL_TEXTURE0 + iSR));
				__GL_CALL(glBindTexture( GL_TEXTURE_2D, textureGL.m_id ));
			}
		}
		for( UINT iSS = 0; iSS < mxCOUNT_OF(batch.SSs); iSS++ )
		{
			const HSamplerState handle = batch.SSs[ iSS ];
			if( handle.IsValid() )
			{
				const SamplerStateGL4& samplerGL = me.samplerStates[ handle.id ];
				__GL_CALL(glBindSampler( iSS, samplerGL.m_id ));
			}
		}


		m_currentInputLayout = batch.inputLayout;

		UINT32 newAttribsMask = 0;

		if( batch.inputLayout.IsValid() )
		{
			const VertexFormatGL& vertexFormat = me.vertexFormats[ batch.inputLayout.id ];
			mxASSERT(vertexFormat.streamCount == 1);
			newAttribsMask = vertexFormat.attribsMask;

			const BufferGL4& bufferGL = me.buffers[ batch.VB[0].id ];
			__GL_CALL(glBindBuffer( GL_ARRAY_BUFFER, bufferGL.m_name ));
			for( UINT attribIndex = 0; attribIndex < vertexFormat.attribCount; attribIndex++ )
			{
				const VertexAttribGL& attrib = vertexFormat.attribs[ attribIndex ];
				__GL_CALL(glVertexAttribPointer( attrib.semantic, attrib.dimension, attrib.dataType, attrib.normalize, attrib.stride, (GLvoid*)attrib.offset ));
				__GL_CALL(glEnableVertexAttribArray( attrib.semantic ));
			}
		}
		else
		{
			glBindBuffer( GL_ARRAY_BUFFER, 0 );__GL_CHECK_ERRORS;
		}

		//const UINT32 changedAttribsMask = newAttribsMask ^ m_enabledAttribsMask;
		//if( changedAttribsMask )
		//{
		//	m_enabledAttribsMask = newAttribsMask;
		//	for( GLuint attribIndex = 0; attribIndex < VertexAttribute::Count; attribIndex++ )
		//	{
		//		if( newAttribsMask & (1UL << attribIndex) ) {
		//			glEnableVertexAttribArray( attribIndex );
		//		} else {
		//			glDisableVertexAttribArray( attribIndex );
		//		}
		//		__GL_CHECK_ERRORS;
		//	}
		//}

		// Change index buffer binding.
		if( m_currentIndexBuffer != batch.IB )
		{
			m_currentIndexBuffer = batch.IB;
			if( batch.IB.IsValid() )
			{
				const BufferGL4& indexBuffer = me.buffers[ batch.IB.id ];
				__GL_CALL(glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer.m_name ));
			}
			else
			{
				__GL_CALL(glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ));
			}
		}

		// Execute the draw call.
		const GLenum primitiveTypeGL = ConvertPrimitiveTypeGL(Topology::Enum(batch.topology));
		if( batch.indexCount ) {
			const GLenum indexTypeGL = batch.b32bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
			__GL_CALL(glDrawElements(
				primitiveTypeGL,
				batch.indexCount,
				indexTypeGL,
				(void*)(uintptr_t)(batch.startIndex*2)mxWHY("<= startIndex*2")
			));
		} else {
			__GL_CALL(glDrawArrays(
				primitiveTypeGL,
				batch.baseVertex,
				batch.vertexCount
			));
		}
	}
	void DeviceContext::EndFrame()
	{
		for( GLuint attribIndex = 0; attribIndex < VertexAttribute::Count; attribIndex++ )
		{
			glDisableVertexAttribArray( attribIndex );
		}
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glUseProgram( 0 );
		for( GLuint iUB = 0; iUB < LLGL_MAX_BOUND_UNIFORM_BUFFERS; iUB++ )
		{
			glBindBufferBase( GL_UNIFORM_BUFFER, iUB, 0 );
		}
		for( GLuint iTU = 0; iTU < LLGL_MAX_BOUND_SHADER_TEXTURES; iTU++ )
		{
			glActiveTexture( GL_TEXTURE0 + iTU );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		for( GLuint iSS = 0; iSS < LLGL_MAX_BOUND_SHADER_SAMPLERS; iSS++ )
		{
			glBindSampler( iSS, 0 );
		}
		__GL_CHECK_ERRORS;
		this->ResetState();
	}

}//namespace llgl

#endif // LLGL_Driver == LLGL_Driver_OpenGL_4plus

mxNO_EMPTY_FILE

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
