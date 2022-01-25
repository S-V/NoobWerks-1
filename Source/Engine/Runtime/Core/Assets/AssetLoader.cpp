/*
=============================================================================

=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Assets/AssetLoader.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>

/*
-----------------------------------------------------------------------------
	TbMemoryImageAssetLoader
-----------------------------------------------------------------------------
*/
ERet TbMemoryImageAssetLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxTRY(Resources::OpenFile( context.key, &stream, AssetPackage::OBJECT_DATA ));

	mxTODO("free memory on failure?");
	mxDO(Serialization::LoadMemoryImage(
		(void**)new_instance_, context.metaclass
		, stream
		, context.raw_allocator
		));
	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	TbBinaryAssetLoader
-----------------------------------------------------------------------------
*/
ERet TbBinaryAssetLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	AssetReader	stream;
	mxTRY(Resources::OpenFile( context.key, &stream, AssetPackage::OBJECT_DATA ));

	mxDO(Serialization::LoadBinary( instance, context.metaclass, stream ));

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	NwTextAssetLoader
-----------------------------------------------------------------------------
*/
ERet NwTextAssetLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	AssetReader	stream_reader;
	mxTRY(Resources::OpenFile( context.key, &stream_reader, AssetPackage::OBJECT_DATA ));

	mxDO(SON::LoadFromStream(
		instance, context.metaclass
		, stream_reader
		, context.scratch_allocator
		));

	return ALL_OK;
}
