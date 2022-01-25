// platform-independent 'low-level' shader compiler interface
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Text/FileInclude.h>
#include <Graphics/Public/graphics_formats.h>

#define USE_D3D_SHADER_COMPILER	(LLGL_Driver_Is_Direct3D)
#define USE_OGL_SHADER_COMPILER	(LLGL_Driver_Is_OpenGL)

//#define REFLECT_GLOBAL_CONSTANT_BUFFERS		(0)

namespace Shaders
{
	//=====================================================
	//		SHADER REFLECTION
	//=====================================================
	struct Field : CStruct
	{
		String32	name;
		//String	type;
		U32		size;
		U32		offset;
		TInplaceArray< V4f, 4 >	initial_value;	//!< default value
	public:
		mxDECLARE_CLASS(Field,CStruct);
		mxDECLARE_REFLECTION;
	};
	struct ShaderCBuffer : CStruct
	{
		String32	name;
		U32			size;
		U8			slot;	// not fixed in OpenGL

		/// true if this is UB is managed by the engine
		/// (i.e. not by the shader effect)
		bool		is_global;

		TInplaceArray< Field, 64 >	fields;		//!< uniforms
	public:
		mxDECLARE_CLASS(ShaderCBuffer,CStruct);
		mxDECLARE_REFLECTION;
		ShaderCBuffer()
		{
			is_global = false;
		}

		/// global resources are managed (set and updated) by the engine
		bool IsGlobal() const { return is_global; }

		const Field* findUniformByName( const char* uniform_name ) const;
	};
	struct ShaderSampler : CStruct
	{
		String32	name;	//<! sampler name, e.g. "s_trilinear"
		U8			slot;	// not fixed in OpenGL

		bool				is_built_in;
		BuiltIns::ESampler	builtin_sampler_id;
	public:
		mxDECLARE_CLASS(ShaderSampler,CStruct);
		mxDECLARE_REFLECTION;
		ShaderSampler()
		{
			is_built_in = false;
			builtin_sampler_id = BuiltIns::ESampler::DefaultSampler;
		}
	};
	/// represents a texture or buffer binding
	struct ShaderResource : CStruct
	{
		String32	name;	//<! texture name, e.g. "t_diffuse_map"
		U8			slot;	// not fixed in OpenGL
		bool		is_global;

	public:
		mxDECLARE_CLASS(ShaderResource,CStruct);
		mxDECLARE_REFLECTION;
		ShaderResource()
		{
			is_global = false;
		}
	};
	/// Unordered Access View 
	struct ShaderUAV : CStruct
	{
		String32	name;
		U8			slot;
	public:
		mxDECLARE_CLASS(ShaderUAV,CStruct);
		mxDECLARE_REFLECTION;
	};

	///
	struct ShaderMetadata : CStruct
	{
		TInplaceArray< ShaderCBuffer, 16 >	cbuffers;	//!< uniform block bindings
		TInplaceArray< ShaderSampler, 16 >	samplers;	//!< shader sampler bindings
		TInplaceArray< ShaderResource, 32 >	resources;	//!< shader resource bindings
		TInplaceArray< ShaderUAV, 8 >		UAVs;		//!< Unordered Access Views

		U32	vertexAttribMask;	//!< active attributes mask (OpenGL-only)
		U32	instructionCount;	//!< Direct3D-only

	public:
		mxDECLARE_CLASS(ShaderMetadata,CStruct);
		mxDECLARE_REFLECTION;
	public:
		ShaderMetadata();
		void clear();

		void FigureOutGlobalResources();

		const ShaderResource* findShaderResourceBindingByHame( const char* texture_name_in_shader ) const {
			return FindByName( resources, texture_name_in_shader );
		}

		const unsigned getUsedCBufferSlotsMask() const {
			return getUsedSlotsMask( cbuffers.raw(), cbuffers.num() );
		}

		const unsigned getUseSamplerSlotsMask() const {
			return getUsedSlotsMask( samplers.raw(), samplers.num() );
		}

		const unsigned getUsedTextureSlotsMask() const {
			return getUsedSlotsMask( resources.raw(), resources.num() );
		}

		template< class RESOURCE >
		unsigned getUsedSlotsMask( const RESOURCE* resources, unsigned count ) const {
			unsigned result = 0;
			for( unsigned i = 0; i < count; i++ ) {
				result |= resources[i].slot;
			}
			return result;
		}
	};

	//=====================================================
	//		DIRECT 3D
	//=====================================================

#if USE_D3D_SHADER_COMPILER

	enum CompilationFlags
	{
		// Will produce best possible code but may take significantly longer to do so.
		// This will be useful for final builds of an application where performance is the most important factor.
		Compile_Optimize = BIT(0),

		Compile_AvoidFlowControl = BIT(2),

		//Compile_RowMajorMatrices = BIT(3),

		Compile_StripDebugInfo = BIT(4),

		Compile_DefaultFlags = 0//Compile_Optimize//|Compile_StripDebugInfo
	};

	struct Options
	{
		Macro *		defines;
		int			numDefines;

		NwFileIncludeI *	include;

		U32		flags;	// CompilationFlags

	public:
		Options();
	};

	typedef void* ByteCode;

	void* GetBufferData( ByteCode buffer );
	size_t GetBufferSize( ByteCode buffer );
	void ReleaseBuffer( ByteCode &buffer );

	ERet CompileShaderD3D(
		ByteCode &compiledCode,
		const char* sourceCode,
		size_t sourceCodeLength,
		const char* entryPoint,
		NwShaderType8 shaderType,
		const Options* options = nil,
		const char* sourceFile = nil	// name of source file for debugging
	);

	ERet CompileShaderFromFile(
		ByteCode &compiledCode,
		const char* sourceFile,
		const char* entryPoint,
		NwShaderType8 shaderType,
		const Options* options = nil
	);

	enum DisassembleFlags
	{
		DISASM_NoDebugInfo	= BIT(0),
		DISASM_WriteAsHTML	= BIT(1),
	};

	ByteCode DisassembleShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength,
		U32 disassembleFlags,
		const char* comments
	);

	ERet ReflectShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength,
		ShaderMetadata &metadata
	);

	// removes all metadata from compiled byte code
	ByteCode StripShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength
	);

#endif // USE_D3D_SHADER_COMPILER

	//=====================================================
	//		OPENGL
	//	NOTE: a valid OpenGL context must be created
	//	before calling these functions!
	//=====================================================

#if USE_OGL_SHADER_COMPILER

	ERet GetProgramBinaryFormatsGL( TArray< INT32 > &formats );

	struct SourceCode
	{
		const char *text;	//!< can be null
		U32		size;	//!< if 0, then will be auto-calculated
		//U32		line;	//!< start line in the source file
	public:
		SourceCode()
		{
			text = nil;
			size = 0;
			//line = 1;
		}
	};
	ERet CompileProgramGL(
		SourceCode source[MAX],
		ShaderMetadata &metadata,
		TArray< BYTE > &programBinary,	// valid if binaryProgramFormat != 0
		INT32 usedBinaryProgramFormat = 0
	);
	//ERet CompileProgramGL(
	//	const char* source,
	//	const U32 length,
	//	const char* fileName,
	//	const U32 startLine,
	//	ShaderMetadata &metadata,
	//	TArray< BYTE > &programBinary,	// valid if binaryProgramFormat != 0
	//	INT32 usedBinaryProgramFormat = 0
	//);
#endif // USE_OGL_SHADER_COMPILER

	//=====================================================
	//		UTILITIES
	//=====================================================

	ERet MergeMetadataOGL(
		ShaderMetadata &destination,
		const ShaderMetadata &source
	);
	ERet MergeMetadataD3D(
		ShaderMetadata &destination,
		const ShaderMetadata &source
	);
	void ExtractBindings(
		ProgramBindingsOGL *bindings,
		const ShaderMetadata& metadata
	);
}//namespace Shaders
