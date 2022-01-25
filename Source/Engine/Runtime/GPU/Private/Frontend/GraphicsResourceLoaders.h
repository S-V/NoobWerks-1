//
#pragma once

#include <Base/Template/LoadInPlace/LoadInPlaceTypes.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Assets/AssetReference.h>
#include <GPU/Public/graphics_types.h>
#include <Graphics/Public/graphics_shader_system.h>


/// In the new *.fx file format, render state can be declared in the same file (like in traditional effects).
/// The old effect file format is deprecated.
#define USE_NEW_EFFECT_FORMAT_WHERE_RENDER_STATES_ARE_INLINED	(1)




/// must be POD!
struct NwRenderStateDesc: CStruct
{
	NwRasterizerDescription		rasterizer;
	NwDepthStencilDescription	depth_stencil;
	NwBlendDescription			blend;

	//32: 24

public:
	mxDECLARE_CLASS(NwRenderStateDesc, CStruct);
	mxDECLARE_REFLECTION;
};
ASSERT_SIZEOF(NwRenderStateDesc, 24);	// for aligned reading

///
class TbRenderStateLoader: public TbAssetLoader_Null
{
public:
	TbRenderStateLoader( ProxyAllocator & allocator );
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
};


namespace NwRenderState32_
{
	void Create(
		NwRenderState32 &render_state_
		, const NwRenderStateDesc& description
		, const char* debug_name = nil
		);
}//namespace






class TbShaderLoader: public TbAssetLoaderI
{
public:
	TbShaderLoader( ProxyAllocator & allocator );
  	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual void destroy( NwResource * instance, const TbAssetLoadContext& context ) override;
};




class TbShaderProgramLoader: public TbAssetLoader_Null
{
public:
	TbShaderProgramLoader( ProxyAllocator & allocator );
  	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual void destroy( NwResource * instance, const TbAssetLoadContext& context ) override;
};

struct TbProgramPermutation: CStruct
{
	AssetID		shaderAssetIds[NwShaderType::MAX];

public:
	mxDECLARE_CLASS(TbProgramPermutation, CStruct);
	mxDECLARE_REFLECTION;
};







///
class TbTextureLoader: public TbAssetLoader_Null
{
public:
	TbTextureLoader( ProxyAllocator & allocator );
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
};
