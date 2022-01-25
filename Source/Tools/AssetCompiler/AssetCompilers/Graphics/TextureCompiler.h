//
#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_TEXTURE_COMPILER
#include <Graphics/Public/graphics_formats.h>

struct TextureImage_m;

namespace AssetBaking
{

//enum ETextureUnitSemantics
//{
//	TUS_Unknown,
//	TUS_Diffuse,	// Albedo
//    TUS_Normals,
//    TUS_Specular,
//    TUS_Emissive,
//    TUS_Environment,
//};

///
struct TextureAssetMetadata: CStruct
{
	bool	convert_equirectangular_to_cubemap;

public:
	mxDECLARE_CLASS(TextureAssetMetadata,CStruct);
	mxDECLARE_REFLECTION;
	TextureAssetMetadata();
};

///
struct TextureCompilerSettings: CStruct
{
public:
	mxDECLARE_CLASS(TextureCompilerSettings,CStruct);
	mxDECLARE_REFLECTION;
	TextureCompilerSettings();
};

///
struct TextureCompiler : public AssetCompilerI
{
	mxDECLARE_CLASS( TextureCompiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual const TbMetaClass* getOutputAssetClass() const override
	{
		return &NwTexture::metaClass();
	}

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

public:
	static ERet compileTexture_RawR8G8B8A8(
		const TextureImage_m& image
		, AWriter &writer
		);

	static ERet ConvertTexture(
					const TextureImage_m& _image
					, AWriter &_writer
					);

	static ERet TryConvertDDS(
		const NwBlob& source_data
		, AssetCompilerOutputs &outputs
		);

private:
	TextureCompilerSettings	m_settings;
};

}//namespace AssetBaking
#endif
