//
#include "stdafx.h"
#pragma hdrstop
#include <AssetCompiler/AssetCompilers/Graphics/TextureCompiler.h>

#if WITH_TEXTURE_COMPILER

#include <stb/stb_image.h>
#include <stb/stb_dxt.h>
#define STB_DXT_IMPLEMENTATION

#include <Base/Util/Cubemap_Util.h>
#include <Base/Template/Containers/Blob.h>

#include <Graphics/Public/graphics_utilities.h>
#include <GPU/Private/Backend/d3d11/dds_reader.h>

//
#include <stb/stb_image.h>
#include <Utility/Image/stb_image_Util.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <AssetCompiler/AssetMetadata.h>


namespace AssetBaking
{

mxDEFINE_CLASS( TextureAssetMetadata );
mxBEGIN_REFLECTION( TextureAssetMetadata )
	mxMEMBER_FIELD( convert_equirectangular_to_cubemap ),
mxEND_REFLECTION
TextureAssetMetadata::TextureAssetMetadata()
{
	convert_equirectangular_to_cubemap = false;
}


struct ImageData
{
	//
	U32	width;
	U32	height;
};

class FatImage: NonCopyable
{
public:
	typedef RGBAf PixelType;

	U32		width;
	U32		height;
	U32		depth;
	DynamicArray< PixelType >	pixels;

public:
	FatImage( AllocatorI & allocator )
		: pixels( allocator )
	{
	}

	const PixelType& pixel( U32 idx ) const
	{
		mxASSERT(idx < width * height * depth);
		return pixels[idx];
	}

	PixelType& pixel( U32 idx )
	{
		mxASSERT(idx < width * height * depth);
		return pixels[idx];
	}


	const PixelType& pixelAt( U32 x, U32 y, U32 z = 0 ) const
	{
		mxASSERT(x < width && y < height && z < depth);
		return pixel((z * height + y) * width + x);
	}

	PixelType& pixelAt( U32 x, U32 y, U32 z = 0 )
	{
		mxASSERT(x < width && y < height && z < depth);
		return pixel((z * height + y) * width + x);
	}
};



#if 0
V2f convert_direction_to_UV( const V3f& direction )
{
	//
}

RGBAf sampleEquirectangularMap( const V2f& uv )
{
	//
}
#endif
ERet writeCubemapFace(
					  NwBlobWriter & cubemap_writer
					  , int cubemap_face_index
					  , int cubemap_face_resolution
					  , AllocatorI & allocator
					  )
{
	//
	FatImage	cubemap_face_image( allocator );

UNDONE;
	////
	//const float cfs = float(cubemap_face_resolution);
	//const float invCfs = 1.0f/cfs;

	//float yyf = 1.0f;
	//for( int iY = 0; iY < cubemap_face_resolution; iY++, yyf+=2.0f )
	//{
	//	float xxf = 1.0f;
	//	for( int iX = 0; iX < cubemap_face_resolution; iX++, xxf+=2.0f )
	//	{
	//		//scale up to [-1, 1] range (inclusive), offset by 0.5 to point to texel center.

	//		// From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
	//		// Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
	//		//      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
	//		const float uu = xxf*invCfs - 1.0f;
	//		const float vv = yyf*invCfs - 1.0f;

	//		const V3f direction = Cubemap_Util::texelCoordToVec( uu, vv, cubemap_face_index );
	//		const V2f uv = convert_direction_to_UV( direction );

	//		//
	//		const RGBAf color = sampleEquirectangularMap( uv );

	//		//
	//		cubemap_face_image.pixelAt( iX, iY ) = color;
	//	}//x
	//}//y

	////

	return ALL_OK;
}

ERet writeCubemapFaces(
					   NwBlobWriter & cubemap_writer
					   , int cubemap_face_resolution
					   , AllocatorI & allocator
					   )
{
	for( int iCubeFace = 0; iCubeFace < 6; iCubeFace++ )
	{
		mxDO(writeCubemapFace(
			cubemap_writer
			, iCubeFace
			, cubemap_face_resolution
			, allocator
			));
	}

	return ALL_OK;
}

ERet convertEquirectangularToCubemap(
									 NwBlob &destination_data_
									 , const AssetCompilerInputs& inputs
									 , const NwBlob& source_data
									 , const TextureAssetMetadata& texture_asset_metadata
									 , AllocatorI & allocator
									 )
{
	const int required_components = STBI_rgb_alpha;

	int source_image_width, source_image_height;
	int source_image_bytes_per_pixel;

	//
	unsigned char* source_image_data = stbi_load_from_memory(
		(stbi_uc*) source_data.raw(), source_data.rawSize(),
		&source_image_width, &source_image_height, &source_image_bytes_per_pixel, required_components
		);
	//
	stb_image_Util::ImageDeleter	stb_image_data_deleter( source_image_data );

	//
	if( !source_image_data ) {
		ptWARN("Failed to parse '%s', reason: '%s'", inputs.path.c_str(), stbi_failure_reason());
		return ERR_FAILED_TO_PARSE_DATA;
	}

	//
	if( source_image_width != source_image_height * 2 ) {
		ptWARN("'%s': for converting equirectangular map to cube map, the width must be twice the height (got width=%d, height=%d)",
			inputs.path.c_str(), source_image_width, source_image_height
			);
		return ERR_INVALID_PARAMETER;
	}

	//
	if( source_image_width % 4 != 0 || source_image_height % 4 != 0 ) {
		ptWARN("'%s': width and height must be divisible by 4 in order to compress into BC1 (got width=%d, height=%d)",
			inputs.path.c_str(), source_image_width, source_image_height
			);
		return ERR_INVALID_PARAMETER;
	}
UNDONE;
	//
	const U32 cubemap_resolution = source_image_height;

	const U32 cubemap_dds_texture_total_size = sizeof(U32)	// DDS_MAGIC_NUM
		+ sizeof(DDS_HEADER)
		//+ ?
		mxTEMP
		;

	mxDO(destination_data_.reserve( cubemap_dds_texture_total_size ));

	//
	NwBlobWriter	cubemap_writer( destination_data_ );
	mxDO(cubemap_writer.Put(U32(DDS_MAGIC_NUM)));

	//
	// BC1

	//
	DDS_HEADER	dds_header;
	mxZERO_OUT(dds_header);
	{
		dds_header.size = sizeof(dds_header);
		dds_header.flags = 0;

		dds_header.height	= cubemap_resolution;
		dds_header.width	= cubemap_resolution;
		UNDONE;
		dds_header.pitchOrLinearSize	= 0;

		UNDONE;
		dds_header.mipMapCount = 0;
	}
	mxDO(cubemap_writer.Put( dds_header ));

	//
	mxDO(writeCubemapFaces( cubemap_writer, cubemap_resolution, allocator ));

	return ALL_OK;
}

//========================================================

mxDEFINE_CLASS( TextureCompilerSettings );
mxBEGIN_REFLECTION( TextureCompilerSettings )
	//mxMEMBER_FIELD( search_paths ),
mxEND_REFLECTION
TextureCompilerSettings::TextureCompilerSettings()
{
}

mxDEFINE_CLASS(TextureCompiler);

ERet TextureCompiler::Initialize()
{
	return ALL_OK;
}
void TextureCompiler::Shutdown()
{
}

ERet TextureCompiler::compileTexture_RawR8G8B8A8(
	const TextureImage_m& image
	, AWriter &writer
	)
{
	TextureHeader_d	header;
	mxZERO_OUT(header);

	header.magic	= TEXTURE_FOURCC;
	header.size		= image.size;
	header.width	= image.width;
	header.height	= image.height;
	header.type		= TEXTURE_2D;
	header.format	= NwDataFormat::RGBA8;
	header.num_mips	= 1;
	header.array_size= 1;

	mxTRY(writer.Put( header ));
	mxTRY(writer.Write( image.data, image.size ));

	return ALL_OK;
}

/// Converts the given DDS texture into the internal engine format.
ERet TextureCompiler::ConvertTexture(
					const TextureImage_m& image
					, AWriter &_writer
					)
{
	const int numSlices = (image.type == TEXTURE_CUBEMAP) ? image.numMips*6 : image.numMips;

	MipLevel_m	slices[128];
	mxASSERT(numSlices <= mxCOUNT_OF(slices));
	mxTRY(ParseMipLevels( image, slices, mxCOUNT_OF(slices) ));

	UINT textureDataSize = 0;
	for( UINT sliceIndex = 0; sliceIndex < numSlices; sliceIndex++ )
	{
		const MipLevel_m& mip = slices[ sliceIndex ];
		textureDataSize += mip.size;
	}

	TextureHeader_d	header;
	mxZERO_OUT(header);
	header.magic	= TEXTURE_FOURCC;
	header.size		= textureDataSize;
	header.width	= image.width;
	header.height	= image.height;
	header.format	= image.format;
	header.num_mips	= image.numMips;
	header.array_size	= image.arraySize;
	header.type		= image.type;

	mxDO(_writer.Put(header));

	//
	_writer.VPreallocate(sizeof(header) + textureDataSize);
	for( int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++ )
	{
		const MipLevel_m& mip = slices[ sliceIndex ];
		mxDO(_writer.Write(mip.data, mip.size));
	}

	return ALL_OK;
}

ERet TextureCompiler::TryConvertDDS(
									const NwBlob& source_data
									, AssetCompilerOutputs &outputs
									)
{
	TextureImage_m	sourceImage;
	mxTRY(DDS_Parse( source_data.raw(), source_data.rawSize(), sourceImage ));

	mxTRY(ConvertTexture( sourceImage, NwBlobWriter( outputs.object_data ) ));
	return ALL_OK;
}

ERet TextureCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI & allocator = MemoryHeaps::temporary();

	//
	const TextureCompilerConfig& texture_compiler_config = inputs.cfg.texture_compiler;

	FilePathStringT	path_to_asset_metadata;
	mxDO(composePathToAssetMetadata( path_to_asset_metadata, inputs.path.c_str() ));

	//
	TextureAssetMetadata	texture_asset_metadata;
	SON::LoadFromFile( path_to_asset_metadata.c_str(), texture_asset_metadata, allocator );

	//
	NwBlob	source_data( allocator );
	mxDO(NwBlob_::loadBlobFromStream( inputs.reader, source_data ));


	////
	//if(mxUNLIKELY( texture_asset_metadata.convert_equirectangular_to_cubemap ))
	//{
	//	mxTRY(convertEquirectangularToCubemap( outputs.object_data, inputs, source_data, texture_asset_metadata, allocator ));
	//}
	//else
	{

		const bool is_DDS = Str::HasExtensionS( inputs.path.c_str(), "dds" );

		const bool can_be_loaded_by_engine_without_conversion = is_DDS;

		const bool needs_conversion_to_engine_texture_format
			= !can_be_loaded_by_engine_without_conversion
			|| texture_compiler_config.use_proprietary_texture_formats_instead_of_DDS
			;

		//
		if( needs_conversion_to_engine_texture_format )
		{
			bool converted = false;
			if( is_DDS )
			{
				if(mxSUCCEDED(TryConvertDDS( source_data, outputs ))) {
					converted = true;
				}
			}

			if( !converted )
				//if(
				//Str::HasExtensionS(inputs.file.c_str(), "png")
				//|| Str::HasExtensionS(inputs.file.c_str(), "tga")
				//|| Str::HasExtensionS(inputs.file.c_str(), "bmp")
				//)
			{
				const int required_components = STBI_rgb_alpha;

				int image_width, image_height;
				int bytes_per_pixel;
				unsigned char* image_data = stbi_load_from_memory(
					(stbi_uc*) source_data.raw(), source_data.rawSize(),
					&image_width, &image_height, &bytes_per_pixel, required_components
					);
				if( !image_data ) {
					ptWARN("Failed to parse '%s', reason: '%s'", inputs.path.c_str(), stbi_failure_reason());
					return ERR_FAILED_TO_PARSE_DATA;
				}

				{
					const int size_of_image_data = image_width * image_height * required_components * sizeof(char);

					TextureHeader_d	header;
					mxZERO_OUT(header);

					header.magic	= TEXTURE_FOURCC;
					header.size		= size_of_image_data;
					header.width	= image_width;
					header.height	= image_height;

					header.type		= TEXTURE_2D;
					header.format	= NwDataFormat::RGBA8;
					header.num_mips	= 1;
					header.array_size= 1;

					//
					NwBlobWriter	writer( outputs.object_data );
					mxTRY(writer.Put( header ));
					mxTRY(writer.Write( image_data, size_of_image_data ));
				}

				stbi_image_free( image_data );

				converted = true;
			}

			if( !converted )
			{
				return ERR_UNSUPPORTED_FEATURE;
			}
		}
		else
		{
			outputs.object_data = source_data;
		}
	}

	return ALL_OK;
}

}//namespace AssetBaking
#endif
