#include <Base/Base.h>
#pragma hdrstop
#include <GPU/Public/graphics_system.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>
#include <Graphics/Private/shader_effect_impl.h>


namespace NGraphics
{
namespace
{
	struct GraphicsGlobals
	{
		TbTextureLoader		textureLoader;
		TbRenderStateLoader	renderStateLoader;
		TbShaderLoader		shaderLoader;
		TbShaderProgramLoader	shaderProgramLoader;
		TbShaderEffectLoader		techniqueLoader;
		AllocatorI &				_allocator;

		ShaderResourceLUT	resource_table;

	public:
		GraphicsGlobals( ProxyAllocator & allocator )
			: _allocator( allocator )
			, textureLoader( allocator )
			, renderStateLoader( allocator )
			, shaderLoader( allocator )
			, shaderProgramLoader( allocator )
			, techniqueLoader( allocator )
		{
		}
	};

	static GraphicsGlobals*	gs_globals = nil;
}//namespace

namespace
{
	ERet setupResourceLoaders( GraphicsGlobals& globals )
	{
		NwTexture::metaClass().loader = &globals.textureLoader;

		NwRenderState::metaClass().loader = &globals.renderStateLoader;

		//
		NwVertexShader::metaClass().loader = &globals.shaderLoader;
		NwPixelShader::metaClass().loader = &globals.shaderLoader;
		NwGeometryShader::metaClass().loader = &globals.shaderLoader;
		NwComputeShader::metaClass().loader = &globals.shaderLoader;

		NwShaderProgram::metaClass().loader = &globals.shaderProgramLoader;
		NwShaderEffect::metaClass().loader = &globals.techniqueLoader;

		return ALL_OK;
	}
}//namespace

ERet Initialize( ProxyAllocator & allocator )
{
	mxASSERT(nil == gs_globals);
	gs_globals = mxNEW( allocator, GraphicsGlobals, allocator );
	mxDO(setupResourceLoaders( *gs_globals ));
	mxDO(gs_globals->resource_table.Initialize());
	return ALL_OK;
}

void Shutdown()
{
	mxDELETE( gs_globals, gs_globals->_allocator );
	gs_globals = nil;
}

HBuffer CreateGlobalConstantBuffer(
	const U32 cbuffer_size_in_bytes
	, const NwNameHash& name_hash
	)
{
	NwBufferDescription	buffer_description(_InitDefault);
	buffer_description.type = Buffer_Uniform;
	buffer_description.size = cbuffer_size_in_bytes;

	HBuffer cb_handle = NGpu::CreateBuffer(
		buffer_description
		, nil
		IF_DEVELOPER , nwNAMEHASH_STR(name_hash)
		);

	NGraphics::GetGlobalResourcesLUT().RegisterCBuffer(
		name_hash
		, cb_handle
		);

	return cb_handle;
}

void DestroyGlobalConstantBuffer(
	HBuffer & cbuffer_handle
	)
{
	mxASSERT(cbuffer_handle.IsValid());
	//

	//
	NGpu::DeleteBuffer(cbuffer_handle);
	cbuffer_handle.SetNil();
}

ShaderResourceLUT& GetGlobalResourcesLUT()
{
	return gs_globals->resource_table;
}

ERet ShaderResourceLUT::Initialize()
{
	mxDO(cbuffer_table.Initialize(
		cbuffer_namehashes
		, cbuffer_handles
		, MAX_CBUFFER_TABLE_SIZE
		));
	mxDO(resource_table.Initialize(
		resource_namehashes
		, resource_handles
		, MAX_RESOURCE_TABLE_SIZE
		));
	return ALL_OK;
}

ERet ShaderResourceLUT::RegisterCBuffer(
	const NwNameHash& name_hash
	, const HBuffer handle
	)
{
	mxASSERT(!cbuffer_table.ContainsKey(nwNAMEHASH_VAL(name_hash)));
	mxASSERT(handle.IsValid());
	return cbuffer_table.Insert(nwNAMEHASH_VAL(name_hash), handle);
}

ERet ShaderResourceLUT::RegisterResource(
	const NwNameHash& name_hash
	, const HShaderInput handle
	)
{
	mxASSERT(!resource_table.ContainsKey(nwNAMEHASH_VAL(name_hash)));
	mxASSERT(handle.IsValid());
	return resource_table.Insert(nwNAMEHASH_VAL(name_hash), handle);
}

}//namespace NGraphics
