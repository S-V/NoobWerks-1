#include <Base/Base.h>
#pragma hdrstop
#include <Core/Core.h>
#include "ShaderCompiler.h"

//#define DBG_VERBOSE

#if USE_D3D_SHADER_COMPILER
#include <GPU/Private/Backend/d3d11/d3d_common.h>
#include <D3DX11.h>
#if MX_AUTOLINK
	#pragma comment( lib, "d3d11.lib" )
	#pragma comment (lib, "dxgi.lib")
	#pragma comment( lib, "dxguid.lib" )
	#pragma comment( lib, "d3dcompiler.lib" )
	#if MX_DEBUG
		#pragma comment( lib, "d3dx11d.lib" )
	#else
		#pragma comment( lib, "d3dx11.lib" )
	#endif
#endif // MX_AUTOLINK
#endif

#if USE_OGL_SHADER_COMPILER
#include <Graphics/private/glcommon.h>
#endif

namespace Shaders
{

namespace
{
	//!< name of the default global constant buffer in Direct3D
	const Chars DEFAULT_GLOBAL_UBO("$Globals");

	//!< engine-managed CBs start with this prefix
	const Chars GLOBAL_UBO_PREFIX("G_");

	bool IsGlobalCBufferName(const String& cbuffer_name)
	{
		return Str::StartsWith( cbuffer_name, GLOBAL_UBO_PREFIX );
	}

	bool IsBuiltInSamplerName(
		const String& sampler_name
		, BuiltIns::ESampler &builtin_sampler_id_
		)
	{
		if(Str::Equal(sampler_name, "g_point_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::PointSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_bilinear_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::BilinearSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_trilinear_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::TrilinearSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_anisotropic_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::AnisotropicSampler;
			return true;
		}


		//
		if(Str::Equal(sampler_name, "g_point_wrap_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::PointWrapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_bilinear_wrap_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::BilinearWrapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_trilinear_wrap_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::TrilinearWrapSampler;
			return true;
		}


		//
		if(Str::Equal(sampler_name, "g_diffuse_map_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::DiffuseMapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_detail_map_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::DetailMapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_normal_map_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::NormalMapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_specular_map_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::SpecularMapSampler;
			return true;
		}
		if(Str::Equal(sampler_name, "g_refraction_map_sampler")) {
			builtin_sampler_id_ = BuiltIns::ESampler::RefractionMapSampler;
			return true;
		}

		return false;
	}
}//namespace


mxDEFINE_CLASS( Field );
mxBEGIN_REFLECTION( Field )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( size ),
	mxMEMBER_FIELD( offset ),
	mxMEMBER_FIELD( initial_value ),
mxEND_REFLECTION

mxDEFINE_CLASS( ShaderCBuffer );
mxBEGIN_REFLECTION( ShaderCBuffer )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( size ),
	mxMEMBER_FIELD( slot ),
	mxMEMBER_FIELD( fields ),
mxEND_REFLECTION

const Field* ShaderCBuffer::findUniformByName( const char* uniform_name ) const
{
	return FindByName( fields, uniform_name );
}

mxDEFINE_CLASS( ShaderSampler );
mxBEGIN_REFLECTION( ShaderSampler )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( slot ),
mxEND_REFLECTION

mxDEFINE_CLASS( ShaderResource );
mxBEGIN_REFLECTION( ShaderResource )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( slot ),
mxEND_REFLECTION

mxDEFINE_CLASS( ShaderUAV );
mxBEGIN_REFLECTION( ShaderUAV )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( slot ),
mxEND_REFLECTION

mxDEFINE_CLASS( ShaderMetadata );
mxBEGIN_REFLECTION( ShaderMetadata )
	mxMEMBER_FIELD( cbuffers ),
	mxMEMBER_FIELD( samplers ),
	mxMEMBER_FIELD( resources ),
	mxMEMBER_FIELD( UAVs ),
	mxMEMBER_FIELD( vertexAttribMask ),
	mxMEMBER_FIELD( instructionCount ),
mxEND_REFLECTION
ShaderMetadata::ShaderMetadata()
{
	this->clear();
}

void ShaderMetadata::clear()
{
	cbuffers.RemoveAll();
	samplers.RemoveAll();
	resources.RemoveAll();
	UAVs.RemoveAll();
	vertexAttribMask = 0;
	instructionCount = 0;
}

void ShaderMetadata::FigureOutGlobalResources()
{
	nwFOR_LOOP(UINT, i, cbuffers.num())
	{
		ShaderCBuffer& ubo = cbuffers._data[i];

		ubo.is_global = IsGlobalCBufferName(ubo.name);
	}

	nwFOR_LOOP(UINT, i, samplers.num())
	{
		ShaderSampler& sampler = samplers._data[i];

		sampler.is_built_in = IsBuiltInSamplerName(
			sampler.name
			, sampler.builtin_sampler_id
			);
	}
}

#if USE_D3D_SHADER_COMPILER

	Options::Options()
	{
		defines		= NULL;
		numDefines	= 0;

		include	= NULL;

		flags = Compile_DefaultFlags;
	}

	void* GetBufferData( ByteCode buffer )
	{
		chkRET_NIL_IF_NIL(buffer);
		return ((ID3DBlob*)buffer)->GetBufferPointer();
	}
	size_t GetBufferSize( ByteCode buffer )
	{
		chkRET_X_IF_NIL(buffer, 0);
		return ((ID3DBlob*)buffer)->GetBufferSize();
	}
	void ReleaseBuffer( ByteCode &buffer )
	{
		if( buffer != NULL ) {
			((ID3DBlob*)buffer)->Release();
		}
	}

	inline const char* D3DBlobToChars( ID3DBlob* msgBlob )
	{
		return (const char*) msgBlob->GetBufferPointer();
	}

	///
	class FileInclude_D3D : public ID3DInclude
	{
		NwFileIncludeI *	m_impl;
	public:
		FileInclude_D3D( NwFileIncludeI* impl )
			: m_impl( impl )
		{}

		STDMETHOD(Open)(
			THIS_ D3D_INCLUDE_TYPE IncludeType,
			LPCSTR pFileName,
			LPCVOID pParentData,
			LPCVOID *ppData,
			UINT *pBytes
		)
		{
			if( m_impl->OpenFile( pFileName, (char**)ppData, pBytes ) )
			{
#ifdef DBG_VERBOSE
				ptPRINT("Opened '%s'", pFileName);
#endif
				return S_OK;
			}

			ptERROR("Failed to open file '%s'. Dumping include stack:\n", pFileName);
			m_impl->DbgPrintIncludeStack();

			return S_FALSE;
		}

		STDMETHOD(Close)(
			THIS_ LPCVOID pData
		)
		{
			m_impl->CloseFile( (char*)pData );
			return S_OK;
		}
	};

	const char* D3D_GetShaderModelString( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_9_1 :	 return "SM_3_0";
		case D3D_FEATURE_LEVEL_9_2 :	 return "SM_3_0";
		case D3D_FEATURE_LEVEL_9_3 :	 return "SM_3_0";
		case D3D_FEATURE_LEVEL_10_0 :	 return "SM_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "SM_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "SM_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetVertexShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_9_1 :	 return "vs_4_0_level_9_1";
		case D3D_FEATURE_LEVEL_9_2 :	 return "vs_4_0_level_9_2";
		case D3D_FEATURE_LEVEL_9_3 :	 return "vs_4_0_level_9_3";
		case D3D_FEATURE_LEVEL_10_0 :	 return "vs_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "vs_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "vs_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetGeometryShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_9_1 :	 return "gs_4_0_level_9_1";
		case D3D_FEATURE_LEVEL_9_2 :	 return "gs_4_0_level_9_2";
		case D3D_FEATURE_LEVEL_9_3 :	 return "gs_4_0_level_9_3";
		case D3D_FEATURE_LEVEL_10_0 :	 return "gs_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "gs_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "gs_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetPixelShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_9_1 :	 return "ps_4_0_level_9_1";
		case D3D_FEATURE_LEVEL_9_2 :	 return "ps_4_0_level_9_2";
		case D3D_FEATURE_LEVEL_9_3 :	 return "ps_4_0_level_9_3";
		case D3D_FEATURE_LEVEL_10_0 :	 return "ps_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "ps_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "ps_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetComputeShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_10_0 :	 return "cs_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "cs_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "cs_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetDomainShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_10_0 :	 return "ds_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "ds_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "ds_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetHullShaderProfile( D3D_FEATURE_LEVEL featureLevel )
	{
		switch( featureLevel )
		{
		case D3D_FEATURE_LEVEL_10_0 :	 return "hs_4_0";
		case D3D_FEATURE_LEVEL_10_1 :	 return "hs_4_1";
		case D3D_FEATURE_LEVEL_11_0 :	 return "hs_5_0";
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	const char* D3D_GetShaderProfile( NwShaderType::Enum shaderType, D3D_FEATURE_LEVEL featureLevel )
	{
		switch( shaderType )
		{
		case NwShaderType::Vertex :	return D3D_GetVertexShaderProfile( featureLevel );
		case NwShaderType::Hull :		return D3D_GetHullShaderProfile( featureLevel );
		case NwShaderType::Domain :	return D3D_GetDomainShaderProfile( featureLevel );
		case NwShaderType::Geometry :	return D3D_GetGeometryShaderProfile( featureLevel );
		case NwShaderType::Pixel :	return D3D_GetPixelShaderProfile( featureLevel );
		case NwShaderType::Compute :	return D3D_GetComputeShaderProfile( featureLevel );
		mxNO_SWITCH_DEFAULT;
		}
		return NULL;
	}

	UINT D3D_Get_HLSL_Compilation_Flags( NwShaderType::Enum shaderType, const Options* options = NULL )
	{
		UINT userDefinedFlags = options ? options->flags : 0;

		UINT hlslCompileFlags = 0;

		hlslCompileFlags |= D3DCOMPILE_ENABLE_STRICTNESS;

		// Insert debug file/line/type/symbol information - causes extreme file size bloat.
		if( !(userDefinedFlags & Compile_StripDebugInfo) )
		{
			hlslCompileFlags |= D3DCOMPILE_DEBUG;
		}

		// Specifying this flag enables strictness which may not allow for legacy syntax.
		//hlslCompileFlags |= D3DCOMPILE_IEEE_STRICTNESS;

		//hlslCompileFlags |= D3DCOMPILE_PARTIAL_PRECISION;

		hlslCompileFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

		if( (userDefinedFlags & Compile_Optimize)
			mxCOMPILER_PROBLEM("Compute Shader won't compile with /O3 because of the following error:"
			"race condition writing to shared memory detected, consider making this write conditional")
			&& (shaderType != NwShaderType::Compute)
			)
		{
			// Lowest optimization level. May produce slower code but will do so more quickly.
			// This may be useful in a highly iterative shader development cycle.
			//hlslCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;

			// Second lowest optimization level.
			//hlslCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;

			// Second highest optimization level.
			//hlslCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;

			// Highest optimization level. Will produce best possible code but may take significantly longer to do so.
			// This will be useful for final builds of an application where performance is the most important factor.
			hlslCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
		}
		else
		{
			hlslCompileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;	// Skip optimization during code generation; generally recommended for debug only.

			//if( shaderType == ShaderType::Compute ) {
			//	mxCOMPILER_PROBLEM("see above");
			//	ptPRINT("WORKAROUND: Compiling Compute Shader with optimizations disabled");
			//	hlslCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
			//}
		}

		if( 0 )
		{
			// Do not validate the generated code against known capabilities and constraints.
			// Only use this with shaders that have been successfully compiled in the past.
			// Shaders are always validated by DirectX before they are set to the device.
			hlslCompileFlags |= D3DCOMPILE_SKIP_VALIDATION;
		}

		// D3DCOMPILE_AVOID_FLOW_CONTROL	Tell compiler to not allow flow-control (when possible).
		if( userDefinedFlags & Compile_AvoidFlowControl )
		{
			hlslCompileFlags |= D3DCOMPILE_AVOID_FLOW_CONTROL;
		}

		// D3DCOMPILE_PARTIAL_PRECISION - Force all computations to be done with partial precision; this may run faster on some hardware.
		// D3DCOMPILE_PREFER_FLOW_CONTROL - Tell compiler to use flow-control (when possible).


		//if( userDefinedFlags & Compile_RowMajorMatrices )
		//{
		//	hlslCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
		//}
		hlslCompileFlags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;


		return hlslCompileFlags;
	}

	ID3DBlob* D3D11_Compile_Shader(
		const char* sourceCode,
		size_t sourceCodeLength,
		const char* entryPoint,
		const D3D_SHADER_MACRO* defines,
		const char* shader_profile,
		UINT compilationFlags,
		ID3DInclude* includeHandler,
		const char* sourceFileName
		)
	{
		chkRET_NIL_IF_NIL(sourceCode);
		chkRET_NIL_IF_NOT(sourceCodeLength > 0);
		//mxASSERT(sourceCodeLength == strlen(sourceCode));
		chkRET_NIL_IF_NIL(shader_profile);
		chkRET_NIL_IF_NIL(entryPoint);

		ID3DBlob *	byteCode = NULL;

		ComPtr< ID3DBlob >	errorMessages;

#if 0
		// D3DX APIs are deprecated
		const HRESULT hr = ::D3DX11CompileFromMemory(
			sourceCode,			// buffer contains shader source code
			sourceCodeLength,	// length of shader source code
			sourceFileName,		// Optional. The name of the shader file. Use either this or pSrcData.
			defines,			// Optional. An array of NULL-terminated macro definitions (see D3D_SHADER_MACRO).
			includeHandler,		// Optional. A pointer to an ID3D10Include for handling include files. Setting this to NULL will cause a compile error if a shader contains a #include.
			entryPoint,			// The name of the shader entry point function.
			profile,			// The shader target or set of shader features to compile against.
			compilationFlags,	// Shader compile options
			0,					// Effect compile options.
			NULL,				// ID3DX11ThreadPump* pPump
			&byteCode,			// The address of a ID3D10Blob that contains the compiled code
			&errorMessages._ptr,	// Optional. A pointer to an ID3D10Blob that contains compiler error messages, or NULL if there were no errors.
			NULL				// HRESULT* pHResult
		);
#else
		const HRESULT hr = ::D3DCompile(
			sourceCode,			// buffer contains shader source code
			sourceCodeLength,	// length of shader source code
			sourceFileName,	// Optional. The name of the shader file. Use either this or pSrcData.
			defines,	// const D3D_SHADER_MACRO* defines
			includeHandler,	// ID3DInclude* includeHandler
			entryPoint,	// const char* entryPoint,
			shader_profile,
			compilationFlags,		// Shader compile options
			0,		// Effect compile options.
			&byteCode,			// The address of a ID3D10Blob that contains the compiled code
			&errorMessages._ptr	// Optional. A pointer to an ID3D10Blob that contains compiler error messages, or nil if there were no errors.
		);
#endif

		if( errorMessages != NULL )
		{
			ptPRINT( D3DBlobToChars(errorMessages) );
		}
		if( FAILED( hr ) )
		{
//clobbers the log
			//dxERROR( hr,
			//	"Failed to compile shader (shader_profile: '%s', entry point: '%s'): %s",
			//	shader_profile, entryPoint, D3DBlobToChars(errorMessages) );
			return NULL;
		}
		mxASSERT_PTR(byteCode);
		return byteCode;
	}

	ERet CompileShaderD3D(
		ByteCode &compiledCode,
		const char* sourceCode,
		size_t sourceCodeLength,
		const char* entryPoint,
		NwShaderType8 shaderType,
		const Options* options /*= nil*/,		
		const char* sourceFile /*= nil*/
	)
	{
		compiledCode = NULL;

		D3D_SHADER_MACRO			storage[64] = { NULL, NULL };
		TArray< D3D_SHADER_MACRO >	macros( storage, mxCOUNT_OF(storage) );

		if( options )
		{
			macros.setNum( options->numDefines + 1 );
			for( int i = 0; i < options->numDefines; i++ )
			{
				macros[i].Name = options->defines[i].name;
				macros[i].Definition = options->defines[i].value;

				//DBGOUT("[%d] %s = %s", i, macros[i].Name, macros[i].Definition);
			}
		}

		const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		const char* shaderProfile = D3D_GetShaderProfile( shaderType, featureLevel );

		const UINT compilationFlags = D3D_Get_HLSL_Compilation_Flags( shaderType, options );

		//
		NwFileIncludeI *	custom_include = options ? options->include : NULL;

		//
		FileInclude_D3D	d3d_include_wrapper( custom_include );

		ID3DBlob* blob = D3D11_Compile_Shader(
			sourceCode,
			sourceCodeLength,
			entryPoint,
			macros.raw(),
			shaderProfile,
			compilationFlags,
			custom_include ? &d3d_include_wrapper : NULL,
			sourceFile
		);

		if( !blob ) {
			return ERR_UNKNOWN_ERROR;
		}

		compiledCode = blob;

		return ALL_OK;
	}

	const char* FindPattern( const char* data, UINT dataLength, const char* pattern, UINT patternLength )
	{
		mxASSERT_PTR(data);
		mxASSERT(dataLength > 0);
		mxASSERT_PTR(pattern);
		mxASSERT(patternLength > 0);
		mxASSERT(dataLength > patternLength);

		for( UINT i = 0; i < dataLength - patternLength; i++ )
		{
			bool match = true;
			for( UINT k = 0; k < patternLength; k++ )
			{
				if( data[i + k] != pattern[k] )
				{
					match = false;
					break;
				}
			}
			if( match )
			{
				return data + i;
			}
		}
		return NULL;
	}

	// Takes a binary shader and returns a buffer containing text assembly.
	ByteCode DisassembleShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength,
		U32 disassembleFlags,
		const char* comments
	)
	{
		mxASSERT_PTR(compiledCode);
		mxASSERT(compiledCodeLength);

		ID3DBlob *	disassembly = NULL;

		UINT disasmFlags = 0;
		if( disassembleFlags & DISASM_WriteAsHTML )
		{
			disasmFlags |= D3D_DISASM_ENABLE_COLOR_CODE;	// Enable the output of color codes.
			disasmFlags |= D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS;	// Enable the output of default values.
			disasmFlags |= D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING;	// Enable instruction numbering.
		}
	//	disasmFlags |= D3D_DISASM_ENABLE_INSTRUCTION_CYCLE;	// No effect. NOTE: the function fails with this flag enabled!
		if( disassembleFlags & DISASM_NoDebugInfo )
		{
			disasmFlags |= D3D_DISASM_DISABLE_DEBUG_INFO;
		}

		const HRESULT hr = ::D3DDisassemble(
			compiledCode,		// A pointer to source data as compiled HLSL code.
			compiledCodeLength,	// length of compiled HLSL code
			disasmFlags,		// Flags affecting the behavior of D3DDisassemble.
			comments,			// The optional comment string at the top of the shader that identifies the shader constants and variables.
			&disassembly		// A pointer to a buffer that receives the ID3D10Blob interface that accesses assembly text.
		);
		if( FAILED( hr ) )
		{
			dxERROR( hr, "Failed to disassemble shader." );
			return NULL;
		}
		mxASSERT_PTR(disassembly);
		return disassembly;
	}

	ERet ReflectShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength,
		ShaderMetadata &metadata
	)
	{
		metadata.clear();

		ComPtr< ID3D11ShaderReflection >	reflection;
		dxTRY(::D3DReflect(
			compiledCode
			, compiledCodeLength
			, IID_ID3D11ShaderReflection
			, (void**)&reflection._ptr
			));

		D3D11_SHADER_DESC	shaderDesc;
		dxTRY(reflection->GetDesc(&shaderDesc));

		// Reflect shader constant buffers.

		mxDO(metadata.cbuffers.Reserve( shaderDesc.ConstantBuffers ));

		for( UINT bufferIndex = 0; bufferIndex < shaderDesc.ConstantBuffers; bufferIndex++ )
		{
			ID3D11ShaderReflectionConstantBuffer* pBufferReflection = reflection->GetConstantBufferByIndex( bufferIndex );

			D3D11_SHADER_BUFFER_DESC	bufferDesc;
			pBufferReflection->GetDesc( &bufferDesc );

			if( bufferDesc.Type != D3D_CT_CBUFFER ) {
				continue;	// gather only constant buffers!
			}

			ShaderCBuffer &new_cbuffer = metadata.cbuffers.AddNew();

			Str::CopyS( new_cbuffer.name, bufferDesc.Name );

			new_cbuffer.is_global = IsGlobalCBufferName(new_cbuffer.name);

			new_cbuffer.size = bufferDesc.Size;

			new_cbuffer.fields.setNum(bufferDesc.Variables);

			// Extract default values of uniform parameters.

			for( UINT fieldIndex = 0; fieldIndex < bufferDesc.Variables; fieldIndex++ )
			{
				ID3D11ShaderReflectionVariable* pFieldReflection = pBufferReflection->GetVariableByIndex( fieldIndex );

				D3D11_SHADER_VARIABLE_DESC	fieldDesc;
				pFieldReflection->GetDesc( &fieldDesc );

				Field& newField = new_cbuffer.fields[ fieldIndex ];
				Str::CopyS( newField.name, fieldDesc.Name );
				newField.size = fieldDesc.Size;
				newField.offset = fieldDesc.StartOffset;

				//
				const int num_vec4s = AlignUp( fieldDesc.Size, 16 ) / 16;
				mxDO(newField.initial_value.setNum( num_vec4s ));

				if( fieldDesc.DefaultValue )
				{
					memcpy(newField.initial_value.raw(), fieldDesc.DefaultValue, fieldDesc.Size);

				}
				else
				{
					// the variable does not have a default value specified in the shader source code
					Arrays::zeroOut( newField.initial_value );
				}
			}
		}

		// Reflect shader resource bindings.

		//metadata.resources.reserve(shaderDesc.BoundResources);

		for( UINT resourceIndex = 0; resourceIndex < shaderDesc.BoundResources; resourceIndex++ )
		{
			D3D11_SHADER_INPUT_BIND_DESC	bindDesc;
			dxTRY(reflection->GetResourceBindingDesc( resourceIndex, &bindDesc ));
			mxASSERT(strlen(bindDesc.Name) > 0);

			switch( bindDesc.Type ) {
				case D3D_SIT_CBUFFER :	// The shader resource is a constant buffer.
					{
						ShaderCBuffer* buffer = FindByName( metadata.cbuffers, bindDesc.Name );
						chkRET_X_IF_NIL(buffer, ERR_OBJECT_NOT_FOUND);
						buffer->slot = bindDesc.BindPoint;
					}
					break;

				case D3D_SIT_TEXTURE :		// The shader resource is a texture.
				case D3D_SIT_STRUCTURED :	// The shader resource is a structured buffer.
					{
						ShaderResource& binding = metadata.resources.AddNew();
						Str::CopyS( binding.name, bindDesc.Name );
						binding.slot = bindDesc.BindPoint;
					}
					break;

				case D3D_SIT_SAMPLER :	// The shader resource is a sampler.
					{
						ShaderSampler& binding = metadata.samplers.AddNew();
						Str::CopyS( binding.name, bindDesc.Name );
						binding.slot = bindDesc.BindPoint;
					}
					break;

				case D3D_SIT_UAV_RWTYPED :		// The shader resource is a read-and-write buffer.
				case D3D_SIT_UAV_RWSTRUCTURED :	// The shader resource is a read-and-write structured buffer.
				case D3D_SIT_UAV_APPEND_STRUCTURED:// The shader resource is an append-structured buffer.
					{
						ShaderUAV& binding = metadata.UAVs.AddNew();
						Str::CopyS( binding.name, bindDesc.Name );
						binding.slot = bindDesc.BindPoint;
					}
					break;

				default:
					ptPRINT("Unknown 'bindDesc.Type': %d", bindDesc.Type);
					Unimplemented;
					return ERR_UNSUPPORTED_FEATURE;
			}
		}

		metadata.instructionCount = shaderDesc.InstructionCount;

		//
		metadata.FigureOutGlobalResources();

		return ALL_OK;
	}

	ByteCode StripShaderD3D(
		const void* compiledCode,
		size_t compiledCodeLength
		)
	{
		UINT stripFlags = 0;
		stripFlags |= D3DCOMPILER_STRIP_REFLECTION_DATA;
		stripFlags |= D3DCOMPILER_STRIP_DEBUG_INFO;
		stripFlags |= D3DCOMPILER_STRIP_TEST_BLOBS;

		ID3DBlob *	strippedCode = NULL;

		const HRESULT hr = ::D3DStripShader(
			compiledCode,
			compiledCodeLength,
			stripFlags,
			&strippedCode
		);
		if( FAILED( hr ) ) {
			dxERROR(hr, "Failed to strip shader");
			return NULL;
		}

		return strippedCode;
	}

#endif // USE_D3D_SHADER_COMPILER


#if USE_OGL_SHADER_COMPILER

	ERet GetProgramBinaryFormatsGL( TArray< INT32 > &formats )
	{
		GLint numBinaryFormats;
		__GL_CALL(::glGetIntegerv( GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats ));
		formats.setNum(numBinaryFormats);
		__GL_CALL(::glGetIntegerv( GL_PROGRAM_BINARY_FORMATS, formats.raw() ));
		return ALL_OK;
	}

	GLuint CompileShaderGL( EShaderType type, const GLchar* sourceCode, GLint sourceLength )
	{
		const GLenum shaderTypeGL = NGpu::ConvertShaderTypeGL( type );
		const GLuint shaderHandle = glCreateShader( shaderTypeGL );__GL_CHECK_ERRORS;

		__GL_CALL(glShaderSource( shaderHandle, 1, &sourceCode, &sourceLength ));
		__GL_CALL(glCompileShader( shaderHandle ));

		int status;
		__GL_CALL(glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &status ));

		if( status != GL_TRUE )
		{
			int loglen;
			__GL_CALL(glGetShaderiv( shaderHandle, GL_INFO_LOG_LENGTH, &loglen ));
			if( loglen > 1 )
			{
				StackAllocator	stackAlloc(gCore.frameAlloc);
				char* log = (char*) stackAlloc.Alloc( loglen );
				__GL_CALL(glGetShaderInfoLog( shaderHandle, loglen, &loglen, log ));
				ptERROR("Failed to compile shader: %s\n", log);
			}
			else {
				ptERROR("Failed to compile shader, no log available\n");
			}
			__GL_CALL(glDeleteShader( shaderHandle ));
			return 0;
		}

		DBGOUT("ShaderCompiler: compiled '%s' (GLid=%u, %d bytes)\n", EShaderTypeToChars(type), shaderHandle, sourceLength);

		return shaderHandle;
	}

	ERet CompileProgramGL(
		SourceCode source[MAX],
		ShaderMetadata &metadata,
		TArray< BYTE > &programBinary,
		INT32 usedBinaryProgramFormat /*= 0*/
		)
	{
		metadata.clear();

		const GLuint program = glCreateProgram();__GL_CHECK_ERRORS;

		for( UINT shaderType = 0; shaderType < MAX; shaderType++ )
		{
			SourceCode& code = source[ shaderType ];
			if( code.text )
			{
				if( !code.size ) {
					code.size = strlen(code.text);
				}
				const GLuint shader = CompileShaderGL( (EShaderType)shaderType, code.text, code.size );
				if( !shader ) {
					return ERR_COMPILATION_FAILED;
				}
				__GL_CALL(glAttachShader( program, shader ));
				__GL_CALL(glDeleteShader( shader ));
			}
		}

		if( usedBinaryProgramFormat )
		{
			// Indicate to the implementation the intention of the application to retrieve the program's binary representation with glGetProgramBinary.
			// The implementation may use this information to store information that may be useful for a future query of the program's binary.
			__GL_CALL(glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE ));
		}

		__GL_CALL(glLinkProgram( program ));

		int logLength = 0;
		__GL_CALL(glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength ));
		if( logLength > 1 )
		{
			StackAllocator	stackAlloc(gCore.frameAlloc);
			char* log = (char*) stackAlloc.Alloc( logLength );
			__GL_CALL(glGetProgramInfoLog( program, logLength, &logLength, log ));
			ptPRINT("%s\n", log);
		}

		int linkStatus = GL_FALSE;
		__GL_CALL(glGetProgramiv( program, GL_LINK_STATUS, &linkStatus ));

		if( linkStatus != GL_TRUE )
		{
			__GL_CALL(glDeleteProgram( program ));
			return ERR_LINKING_FAILED;
		}

		__GL_CALL(glValidateProgram( program ));

		int validStatus;
		__GL_CALL(glGetProgramiv( program, GL_VALIDATE_STATUS, &validStatus ));

		if( validStatus != GL_TRUE )
		{
			__GL_CALL(glDeleteProgram( program ));
			return ERR_VALIDATION_FAILED;
		}

		// Get the binary representation of the program object's compiled and linked executable source.

		if( usedBinaryProgramFormat )
		{
			GLint	programBinarySize;
			__GL_CALL(glGetProgramiv( program, GL_PROGRAM_BINARY_LENGTH, &programBinarySize ));
			if( programBinarySize > 0 )
			{
				programBinary.setNum( programBinarySize );
				__GL_CALL(glGetProgramBinary( program, programBinarySize, NULL/*length*/, (GLenum*)&usedBinaryProgramFormat, programBinary.raw() ));
			}
		}

		// Retrieve information about all active attribute variables for the program object.

		GLint numActiveAttribs = 0;
		__GL_CALL(glGetProgramiv( program, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs ));

		if( numActiveAttribs )
		{
			GLint maxAttribNameLength;
			__GL_CALL(glGetProgramiv( program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttribNameLength ));

			StackAllocator	stackAlloc(gCore.frameAlloc);
			GLchar* attribName = (GLchar*) stackAlloc.Alloc( maxAttribNameLength * sizeof(GLchar) );

			for( int attribIndex = 0; attribIndex < numActiveAttribs; attribIndex++ )
			{
				GLenum attribType = 0;
				GLint attribArraySize = 0;
				GLsizei	attribNameLength = 0;
				__GL_CALL(glGetActiveAttrib( program, attribIndex, maxAttribNameLength, &attribNameLength, &attribArraySize, &attribType, attribName ));
				//DBGOUT("Attribute: '%s' of type '%s'\n", attribName, NGpu::UniformTypeToChars(attribType));

				if( strstr(attribName, "gl_") ) {
					continue;	// built-in vertex attribute
				}

				const UINT attribSemanticId = NGpu::GetAttribIdByName(attribName);
				mxASSERT(attribSemanticId != -1);
				metadata.vertexAttribMask |= (1UL << attribSemanticId);
			}
		}

		// Get the list of all active uniforms.

		GLint numActiveUniforms = 0;
		__GL_CALL(glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &numActiveUniforms ));


		if( numActiveUniforms > 0 )
		{
			GLint maxNameLength;
			__GL_CALL(glGetProgramiv( program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength ));

			StackAllocator	stackAlloc(gCore.frameAlloc);
			GLchar* uniformName = (GLchar*) stackAlloc.Alloc( maxNameLength * sizeof(GLchar) );

			for( int uniformIndex = 0; uniformIndex < numActiveUniforms; uniformIndex++ )
			{
				GLsizei	nameLength;
				GLint	uniformSize;
				GLenum	uniformType;
				__GL_CALL(glGetActiveUniform( program, uniformIndex, maxNameLength, &nameLength, &uniformSize, &uniformType, uniformName));

				if( uniformType == GL_SAMPLER_1D || uniformType == GL_SAMPLER_2D || uniformType == GL_SAMPLER_3D
					|| uniformType == GL_TEXTURE_CUBE_MAP || uniformType == GL_TEXTURE_RECTANGLE
					|| uniformType == GL_TEXTURE_1D_ARRAY || uniformType == GL_TEXTURE_2D_ARRAY || uniformType == GL_TEXTURE_CUBE_MAP_ARRAY
					|| uniformType == GL_TEXTURE_2D_MULTISAMPLE || uniformType == GL_TEXTURE_2D_MULTISAMPLE_ARRAY
					|| uniformType == GL_TEXTURE_BUFFER )
				{
					const UINT textureUnit = metadata.samplers.num();
					ShaderSampler& newSampler = metadata.samplers.AddNew();
					Str::CopyS( newSampler.name, uniformName );
					newSampler.slot = textureUnit;
				}
			}
		}

		// Query the number of uniform buffer blocks and get their info.

		GLint	numUniformBlocks = 0;
		__GL_CALL(glGetProgramiv( program, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks ));

		metadata.cbuffers.setNum(numUniformBlocks);

		if( numUniformBlocks > 0 )
		{
			// The length of the longest active uniform block name for program, including the null termination character
			// (i.e., the size of the character buffer required to store the longest uniform block name).
			GLint maxNameLength;
			__GL_CALL(glGetProgramiv( program, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxNameLength ));

			StackAllocator	stackAlloc(gCore.frameAlloc);
			GLchar* uniformBlockName = (GLchar*) stackAlloc.Alloc( maxNameLength * sizeof(GLchar) );

			for( int uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; ++uniformBlockIndex )
			{
				// Assign a binding point to the uniform block.
				// This sets state in the program (which is why you shouldn't be calling it every frame).
				__GL_CALL(glUniformBlockBinding( program, uniformBlockIndex, uniformBlockIndex ));

				//GLint	nameLength = 0;
				//glGetActiveUniformBlockiv( program, uniformBlockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength );

				GLsizei	nameLength = 0;	// The length of this block's name.
				__GL_CALL(glGetActiveUniformBlockName( program, uniformBlockIndex, maxNameLength, &nameLength, uniformBlockName ));

				GLint	uniformBlockDataSize = 0;	// The buffer object storage size needed for this block.
				__GL_CALL(glGetActiveUniformBlockiv( program, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockDataSize ));

				// If 'param' is GL_UNIFORM_BLOCK_BINDING, then the index of the uniform buffer binding point
				// last selected by the uniform block specified by uniformBlockIndex for program is returned.
				// If no uniform block has been previously specified, zero is returned.
				GLint	uniformBlockBinding = 0;	// The current block binding, as set either within the shader or from glUniformBlockBinding?.
				__GL_CALL(glGetActiveUniformBlockiv( program, uniformBlockIndex, GL_UNIFORM_BLOCK_BINDING, &uniformBlockBinding ));

				// If 'param' is GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER, GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER, or GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER,
				// then a boolean value indicating whether the uniform block identified by uniformBlockIndex is referenced by the vertex, geometry, or fragment programming stages of program, respectively, is returned.

				// Get the number of active uniforms within this block.
				GLint	numUniforms = 0;
				__GL_CALL(glGetActiveUniformBlockiv( program, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms ));

				//DBGOUT("CB[%d] = '%s' (%d bytes at slot %d)\n",
				//	uniformBlockIndex, uniformBlockName, uniformBlockDataSize, uniformBlockBinding);

				ShaderCBuffer& newUBO = metadata.cbuffers[ uniformBlockIndex ];
				Str::CopyS( newUBO.name, uniformBlockName );
				newUBO.size = uniformBlockDataSize;
				newUBO.slot = uniformBlockBinding;
				newUBO.fields.setNum(numUniforms);

				mxSTOLEN("VSShaderLib: https://github.com/lighthouse3d/VSL/blob/master/VSL/source/vsShaderLib.cpp");
				// Get the list of the active uniform indices for the uniform block.
				if( numUniforms )
				{
					StackAllocator	uniformDataAlloc(gCore.frameAlloc);
					GLuint* uniformIndices = uniformDataAlloc.AllocMany< GLuint >( numUniforms );

					// Get the indices of active uniforms within this block.
					__GL_CALL(glGetActiveUniformBlockiv( program, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)uniformIndices ));

					GLint* uniformTypes = uniformDataAlloc.AllocMany< GLint >( numUniforms );
					GLint* uniformSizes = uniformDataAlloc.AllocMany< GLint >( numUniforms );
					GLint* uniformOffsets = uniformDataAlloc.AllocMany< GLint >( numUniforms );
					GLint* uniformArrayStrides = uniformDataAlloc.AllocMany< GLint >( numUniforms );
					GLint* uniformMatrixStrides = uniformDataAlloc.AllocMany< GLint >( numUniforms );

					__GL_CALL(glGetActiveUniformsiv( program, numUniforms, uniformIndices, GL_UNIFORM_TYPE, uniformTypes ));
					// For active uniforms that are arrays, the size is the number of active elements in the array; for all other uniforms, the size is one.
					__GL_CALL(glGetActiveUniformsiv( program, numUniforms, uniformIndices, GL_UNIFORM_SIZE, uniformSizes ));
					// For uniforms in a named uniform block, the returned value will be its offset, in basic machine units, relative to the beginning of the uniform block in the buffer object data store. For all other uniforms, -1 will be returned.
					__GL_CALL(glGetActiveUniformsiv( program, numUniforms, uniformIndices, GL_UNIFORM_OFFSET, uniformOffsets ));
					// The byte stride for elements of the array, for uniforms in a uniform block. For non-array uniforms in a block, this value is 0. For uniforms not in a block, the value will be -1.
					__GL_CALL(glGetActiveUniformsiv( program, numUniforms, uniformIndices, GL_UNIFORM_ARRAY_STRIDE, uniformArrayStrides ));
					// The byte stride for columns of a column-major matrix or rows for a row-major matrix, for uniforms in a uniform block. For non-matrix uniforms in a block, this value is 0. For uniforms not in a block, the value will be -1.
					__GL_CALL(glGetActiveUniformsiv( program, numUniforms, uniformIndices, GL_UNIFORM_MATRIX_STRIDE, uniformMatrixStrides ));

					//DBGOUT("Uniform block[%d]: '%s' (%u bytes):\n", uniformBlockIndex, uniformBlockName, uniformBlockDataSize);
					for( int i = 0; i < numUniforms; i++ )
					{
						// Calculate the size of the uniform in basic machine units (i.e. in bytes).
						UINT uniformByteWidth = 0;

						const GLint uniformType = uniformTypes[ i ];
						const GLint uniformSize = uniformSizes[ i ];
						const GLint arrayStride = uniformArrayStrides[ i ];
						const GLint matrixStride = uniformMatrixStrides[ i ];

						if( arrayStride > 0 )
						{
							uniformByteWidth = arrayStride * uniformSize;
						}
						else if( matrixStride > 0 )
						{
							switch( uniformType )
							{
							case GL_FLOAT_MAT2:
							case GL_FLOAT_MAT2x3:
							case GL_FLOAT_MAT2x4:
							case GL_DOUBLE_MAT2:
							case GL_DOUBLE_MAT2x3:
							case GL_DOUBLE_MAT2x4:
								uniformByteWidth = 2 * matrixStride;
								break;
							case GL_FLOAT_MAT3:
							case GL_FLOAT_MAT3x2:
							case GL_FLOAT_MAT3x4:
							case GL_DOUBLE_MAT3:
							case GL_DOUBLE_MAT3x2:
							case GL_DOUBLE_MAT3x4:
								uniformByteWidth = 3 * matrixStride;
								break;
							case GL_FLOAT_MAT4:
							case GL_FLOAT_MAT4x2:
							case GL_FLOAT_MAT4x3:
							case GL_DOUBLE_MAT4:
							case GL_DOUBLE_MAT4x2:
							case GL_DOUBLE_MAT4x3:
								uniformByteWidth = 4 * matrixStride;
								break;
								mxNO_SWITCH_DEFAULT;
							}
						}
						else
						{
							uniformByteWidth = NGpu::UniformTypeSize( uniformType );
						}

						Field &	field = newUBO.fields[ i ];

						GLchar	uniformName[256];
						GLsizei	uniformNameLength;
						__GL_CALL(glGetActiveUniformName( program, uniformIndices[ i ], mxCOUNT_OF(uniformName), &uniformNameLength, uniformName ));
						mxASSERT(uniformNameLength < mxCOUNT_OF(uniformName));

						Str::CopyS( field.name, uniformName );
						field.size = uniformByteWidth;
						field.offset = uniformOffsets[ i ];
						//DBGOUT("\tUniform[%d]: '%s' (offset = %u, size = %u)\n", i, uniformName, field.offset, field.size);
					}//for each uniform
				}//if any uniforms
			}//for each uniform block
		}//if any uniform blocks

		DBGOUT("Created program (id=%u)\n", program);

		__GL_CALL(glDeleteProgram( program ));

		return ALL_OK;
	}

#endif // USE_OGL_SHADER_COMPILER

	static inline bool BindingsMatch( const ShaderCBuffer& a, const ShaderCBuffer& b )
	{
		return a.size == b.size
			&& a.slot == b.slot
			;
	}
	static inline bool BindingsMatch( const ShaderSampler& a, const ShaderSampler& b )
	{
		return a.slot == b.slot;
	}
	static inline bool BindingsMatch( const ShaderResource& a, const ShaderResource& b )
	{
		return a.slot == b.slot;
	}
	static inline bool BindingsMatch( const ShaderUAV& a, const ShaderUAV& b )
	{
		return a.slot == b.slot;
	}

	template< class BINDING >
	ERet MergeBindingsArray(
		TArray< BINDING > &destination,
		const TArray< BINDING >& source
		)
	{
		mxDO(destination.Reserve( source.num() ));
		for( UINT bindingIndex = 0; bindingIndex < source.num(); bindingIndex++ )
		{
			const BINDING& binding = source[ bindingIndex ];
			mxASSERT(binding.name.length() > 0);
			const BINDING* existing = FindByName( destination, binding.name.c_str() );
			if( existing )
			{
				// ensure all bindings have the same slot
				if( !BindingsMatch( binding, *existing ) ) {
					ptWARN("'%s' has different slots: [%u] vs [%d]", binding.name.c_str(), binding.slot, existing->slot);
					return ERR_LINKING_FAILED;
				}
			}
			else
			{
				destination.add( binding );
			}
		}
		return ALL_OK;
	}

#if 0
	template< bool VALIDATE_BINDPOINTS_ARE_SAME >
	ERet MergeMetadataTemplate(
		ShaderMetadata &destination,
		const ShaderMetadata &source
		)
	{
		for( UINT bufferIndex = 0; bufferIndex < source.cbuffers.num(); bufferIndex++ )
		{
			const ShaderCBuffer& binding = source.cbuffers[ bufferIndex ];
			const ShaderCBuffer* existing = FindByName( destination.cbuffers, binding.name.raw() );
			if( existing )
			{
				chkRET_X_IF_NOT( existing->size == binding.size, ERR_LINKING_FAILED );
				if( VALIDATE_BINDPOINTS_ARE_SAME ) {
					chkRET_X_IF_NOT( existing->slot == binding.slot, ERR_LINKING_FAILED );
				}
			}
			else
			{
				destination.cbuffers.add( binding );
			}
		}
		for( UINT samplerIndex = 0; samplerIndex < source.samplers.num(); samplerIndex++ )
		{
			const ShaderSampler& binding = source.samplers[ samplerIndex ];
			const ShaderSampler* existing = FindByName( destination.samplers, binding.name.raw() );
			if( existing )
			{
				if( VALIDATE_BINDPOINTS_ARE_SAME ) {
					chkRET_X_IF_NOT( existing->slot == binding.slot, ERR_LINKING_FAILED );
				}
			}
			else
			{
				destination.samplers.add( binding );
			}
		}
		destination.vertexAttribMask |= source.vertexAttribMask;
		return ALL_OK;
	}

	ERet MergeMetadataOGL(
		ShaderMetadata &destination,
		const ShaderMetadata &source
		)
	{
		mxDO(MergeMetadataTemplate< false >(destination, source));
		return ALL_OK;
	}
#endif

	ERet MergeMetadataD3D(
		ShaderMetadata &destination,
		const ShaderMetadata &source
		)
	{
		mxDO(MergeBindingsArray( destination.cbuffers, source.cbuffers ));
		mxDO(MergeBindingsArray( destination.samplers, source.samplers ));
		mxDO(MergeBindingsArray( destination.resources, source.resources ));
		mxDO(MergeBindingsArray( destination.UAVs, source.UAVs ));
		destination.vertexAttribMask |= source.vertexAttribMask;
		destination.instructionCount = largest(destination.instructionCount, source.instructionCount);
		return ALL_OK;
	}

#if 0
	void ExtractBindings(
		ProgramBindingsOGL *bindings,
		const ShaderMetadata& metadata
	)
	{
		const UINT numCBuffers = metadata.cbuffers.num();
		bindings->cbuffers.setNum(numCBuffers);
		for( UINT bufferIndex = 0; bufferIndex < numCBuffers; bufferIndex++ )
		{
			const ShaderCBuffer& buffer = metadata.cbuffers[ bufferIndex ];
			CBufferBindingOGL &binding = bindings->cbuffers[ bufferIndex ];
			binding.name = buffer.name;
			binding.slot = bufferIndex;
			binding.size = buffer.size;
		}
		const UINT numSamplers = metadata.samplers.num();
		bindings->samplers.setNum(numSamplers);
		for( UINT samplerIndex = 0; samplerIndex < numSamplers; samplerIndex++ )
		{
			const ShaderSampler& sampler = metadata.samplers[ samplerIndex ];
			SamplerBindingOGL &binding = bindings->samplers[ samplerIndex ];
			binding.name = sampler.name;
			binding.slot = samplerIndex;
		}
		bindings->activeVertexAttributes = metadata.vertexAttribMask;
	}
#endif

}//namespace Shaders
