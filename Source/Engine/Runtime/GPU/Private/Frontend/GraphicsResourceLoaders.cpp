// TODO: shader and program instances are so small (16-bit handles) that they can be stored inside the asset table
// 
#include <Base/Base.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Serialization/Serialization.h>
#include <Graphics/Public/graphics_formats.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>

mxDEFINE_CLASS(NwRenderStateDesc);
mxBEGIN_REFLECTION(NwRenderStateDesc)
	mxMEMBER_FIELD(rasterizer),
	mxMEMBER_FIELD(depth_stencil),
	mxMEMBER_FIELD(blend),
mxEND_REFLECTION;
//tbPRINT_SIZE_OF(NwRenderStateDesc);

TbRenderStateLoader::TbRenderStateLoader( ProxyAllocator & allocator )
	: TbAssetLoader_Null( NwRenderState::metaClass(), allocator )
{
}

ERet TbRenderStateLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	NwRenderStateDesc	description;
	mxDO(stream.Get(description));

	//
	NwRenderState *render_state = static_cast< NwRenderState* >( instance );
	NwRenderState32_::Create(
		*render_state
		, description
		, context.key.id.c_str()
		);

	return ALL_OK;
}

void TbRenderStateLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	NwRenderState *renderState = static_cast< NwRenderState* >( instance );
	DBGOUT("TODO: ref counting");
	//NGpu::DeleteDepthStencilState( renderState->depth_stencil_state );
	//NGpu::DeleteRasterizerState( renderState->rasterizer_state );
	//NGpu::DeleteBlendState( renderState->blend_state );	
}


namespace NwRenderState32_
{
	void Create(
		NwRenderState32 &render_state_
		, const NwRenderStateDesc& description
		, const char* debug_name
		)
	{
		render_state_.blend_state = NGpu::CreateBlendState( description.blend, debug_name );
		render_state_.rasterizer_state = NGpu::CreateRasterizerState( description.rasterizer, debug_name );
		render_state_.depth_stencil_state = NGpu::CreateDepthStencilState( description.depth_stencil, debug_name );
		render_state_.stencil_reference_value = 0;
	}
}//namespace

/*
-----------------------------------------------------------------------------
	TbShaderLoader
-----------------------------------------------------------------------------
*/
TbShaderLoader::TbShaderLoader( ProxyAllocator & allocator )
{
}

ERet TbShaderLoader::create( NwResource **newInstance_, const TbAssetLoadContext& context )
{
	NwResource* newShader = static_cast< NwResource* >( context.object_allocator.allocateObject( context.metaclass ) );
	mxENSURE(newShader != nil, ERR_OUT_OF_MEMORY, "");

	//
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	const size_t shaderBytecodeSize = stream.Length();

	const NGpu::Memory* mem = NGpu::Allocate( shaderBytecodeSize );
	mxENSURE(mem, ERR_OUT_OF_MEMORY, "");
	mxDO(stream.Read( mem->data, shaderBytecodeSize ));

	const HShader shaderHandle = NGpu::CreateShader( mem );

	*((HShader*)newShader) = shaderHandle;

	*newInstance_ = newShader;

	return ALL_OK;
}

void TbShaderLoader::destroy( NwResource * instance, const TbAssetLoadContext& context )
{
	//context.storage.deallocate( instance );
}

/*
-----------------------------------------------------------------------------
	TbShaderProgramLoader
-----------------------------------------------------------------------------
*/
TbShaderProgramLoader::TbShaderProgramLoader( ProxyAllocator & allocator )
	: TbAssetLoader_Null( NwShaderProgram::metaClass(), allocator )
{
}

ERet TbShaderProgramLoader::create( NwResource **newInstance_, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	TbProgramPermutation *	programPermutation;
	mxTRY(Serialization::LoadMemoryImage( programPermutation, stream, context.scratch_allocator ));

	//
	NwShaderProgram* newShaderProgram;
	mxDO(context.object_allocator.allocateObject( &newShaderProgram ));

	//
	NwProgramDescription	programDescription;

	for( int iShaderType = 0; iShaderType < NwShaderType::MAX; iShaderType++ )
	{
		const NwShaderType::Enum shaderType = (NwShaderType::Enum) iShaderType;

		const AssetID& shaderAssetId = programPermutation->shaderAssetIds[shaderType];

		if( AssetId_IsValid( shaderAssetId ) )
		{
			const TbMetaClass& shaderClass = Testbed::Graphics::shaderAssetClassForShaderTypeEnum( shaderType );

			NwResource *	shaderResource;
			mxDO(Resources::Load( shaderResource, shaderAssetId, shaderClass, &context.object_allocator ));

			programDescription.shaders[shaderType] = *(HShader*) shaderResource;
		}
	}

	//
	mxDELETE( programPermutation, context.scratch_allocator );

	//
	newShaderProgram->handle = NGpu::CreateProgram( programDescription );

	*newInstance_ = newShaderProgram;

	return ALL_OK;
}

void TbShaderProgramLoader::destroy( NwResource * instance, const TbAssetLoadContext& context )
{
	NwShaderProgram *shaderProgram = static_cast< NwShaderProgram* >( instance );
	UNDONE;
}


mxDEFINE_CLASS(TbProgramPermutation);
mxBEGIN_REFLECTION(TbProgramPermutation)
	mxMEMBER_FIELD( shaderAssetIds ),
mxEND_REFLECTION;


/*
-----------------------------------------------------------------------------
	TbTextureLoader
-----------------------------------------------------------------------------
*/
TbTextureLoader::TbTextureLoader( ProxyAllocator & allocator )
	: TbAssetLoader_Null( NwTexture::metaClass(), allocator )
{
}

ERet TbTextureLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	const U32 textureSize = stream.Length();

	const NGpu::Memory* mem_block = NGpu::Allocate( textureSize );
	mxDO(stream.Read( mem_block->data, textureSize ));

	NwTexture *texture = static_cast< NwTexture* >( instance );

	mxDO(texture->loadFromMemory( mem_block ));

	return ALL_OK;
}

void TbTextureLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	NwTexture *texture = static_cast< NwTexture* >( instance );
	if( texture->m_texture.IsValid() ) {
		NGpu::DeleteTexture(texture->m_texture);
		texture->m_texture.SetNil();
	}
	texture->m_resource.SetNil();
}
