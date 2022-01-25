/*
=============================================================================
	File:	Serialization.cpp
	Desc:	Binary serialization of arbitrary types via reflection.
	ToDo:
		add compatibility checks;
		don't use a pointer patch table, use a linked list of pointer offsets?
		write MAKEFOURCC('P','N','T','R'); in place of pointers for debugging?
	Useful references:
	The serialization library FlatBuffers:
	http://google.github.io/flatbuffers/
	Fast mmap()able data structures
	https://deplinenoise.wordpress.com/2013/03/31/fast-mmapable-data-structures/
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <bx/uint32_t.h>
#include <Base/Memory/MemoryBase.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Util/FourCC.h>
#include <Core/Assets/AssetReference.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Util/ScopedTimer.h>
#include <Core/Serialization/Serialization_Private.h>



namespace Serialization
{

	ERet SaveMemoryImage(
		const void* instance
		, const TbMetaClass& type
		, AWriter &stream
		)
	{
		AllocatorI & scratch = MemoryHeaps::temporary();

		LIPInfoGatherer	lip( scratch );

		Reflection::AVisitor2::Context	ctx;

		// add the root object body.
		lip.addMemoryChunk( instance, type.m_size, type.m_align, ctx );

		// Recursively visit all referenced objects.
		Reflection::Walker2::Visit( const_cast<void*>(instance), type, &lip );

		// Determine file offsets of all memory blocks.
		const U32 chunk_data_size = lip.ResolveChunkOffsets();

		// Write the header.
		NwImageSaveFileHeader	header;
		mxZERO_OUT(header);
		{
			header.filetype = SAVE_FILE_IMAGE;
			header.session = TbSession::CURRENT;
			header.classId = type.GetTypeGUID();
			header.payload = chunk_data_size;
		}
		mxDO(stream.Put( header ));

		const U32 relocation_table_offset = AlignUp(
			sizeof(header) + chunk_data_size
			, FIXUP_TABLE_ALIGNMENT
			);

		DBG_MSG("WRITE: Object: '%s' (%#010x), data size: '%u', relocation table start: '%u'",
			type.GetTypeName(), header.classId, chunk_data_size, relocation_table_offset
			);

		// Write all memory blocks and relocation tables.
		lip.WriteChunksAndFixUpTables(
			stream
			, relocation_table_offset
			);

		return ALL_OK;
	}

	static ERet _readClassGUID(
		AReader & reader
		, const TbMetaClass **metaclass_
		)
	{
		TypeGUID classGUID;
		mxDO(reader.Get(classGUID));

		const TbMetaClass* metaclass = TypeRegistry::FindClassByGuid( classGUID );
		if( !metaclass ) {
			ptERROR("No class with id=0x%X", classGUID);
			return ERR_OBJECT_NOT_FOUND;
		}

		*metaclass_ = metaclass;

		return ALL_OK;
	}

	ERet ReadAndApplyFixups(
		AReader& reader
		, void* object_buffer, U32 buffer_size
		)
	{
		const U32 current_offset = reader.Tell();

		mxDO(Reader_Align_Forward( reader, FIXUP_TABLE_ALIGNMENT ));

#if DEBUG_SERIALIZATION
		const U32 fixup_table_offset = reader.Tell();
#endif // DEBUG_SERIALIZATION

		// Load and apply fixup tables.
		FixUpTable	fixUpTable;
		mxDO(reader.Get(fixUpTable));

#if DEBUG_SERIALIZATION
		DBGOUT("Read Fixup Table at '%d': numPointerFixUps=%d, numClassIdFixUps=%d, numAssetIdFixUps=%d, num_asset_references=%d",
			fixup_table_offset, fixUpTable.numPointerFixUps, fixUpTable.numClassIdFixUps, fixUpTable.numAssetIdFixUps, fixUpTable.num_asset_references
			);
#endif // DEBUG_SERIALIZATION

		// Relocate pointers.
		for( U32 i = 0; i < fixUpTable.numPointerFixUps; i++ )
		{
			U32 pointerOffset;
			U32 targetOffset;
			mxDO(reader.Get(pointerOffset));
			mxDO(reader.Get(targetOffset));

			void* pointerAddress = mxAddByteOffset( object_buffer, pointerOffset );
			void* targetAddress = mxAddByteOffset( object_buffer, targetOffset );
			*((void**)pointerAddress) = targetAddress;
		}

		// Fixup type ids.
		for( U32 i = 0; i < fixUpTable.numClassIdFixUps; i++ )
		{
			U32 pointerOffset;
			mxDO(reader.Get(pointerOffset));

			const TbMetaClass* metaclass;
			mxDO(_readClassGUID(reader, &metaclass));

			void* pointerAddress = mxAddByteOffset( object_buffer, pointerOffset );
			SClassId* ptr = static_cast< SClassId* >( pointerAddress );
			ptr->type = metaclass;
			DBG_MSG("READ: ClassID '%s': -> [%u]", metaclass->GetTypeName(), pointerOffset);
		}

		// AssetID's
		for( U32 i = 0; i < fixUpTable.numAssetIdFixUps; i++ )
		{
			U32 pointerOffset;
			mxDO(reader.Get(pointerOffset));
			mxASSERT(pointerOffset < buffer_size);

			AssetID* assetId = (AssetID*) mxAddByteOffset( object_buffer, pointerOffset );
			new(assetId) AssetID();
			mxDO(readAssetID( *assetId, reader ));
		}

		// Asset references
		for( U32 iAssetRef = 0; iAssetRef < fixUpTable.num_asset_references; iAssetRef++ )
		{
			U32 pointerOffset;
			mxDO(reader.Get(pointerOffset));

			//
			const TbMetaClass* metaclass;
			mxDO(_readClassGUID(reader, &metaclass));

			//
			AssetID* asset_id = (AssetID*) mxAddByteOffset(
				object_buffer
				, pointerOffset + FIELD_OFFSET(NwAssetRef, id)
				);
			new(asset_id) AssetID();
			mxDO(readAssetID( *asset_id, reader ));

			//
			void* address_of_pointer_to_asset_instance = mxAddByteOffset( object_buffer, pointerOffset );
			void* pointer_to_asset_instance = nil;	// will be loaded later, explicitly
			*((void**)address_of_pointer_to_asset_instance) = pointer_to_asset_instance;
		}

		return ALL_OK;
	}

	ERet LoadMemoryImage(
		void **new_instance_
		, const TbMetaClass& type
		, AReader& stream
		, AllocatorI & allocator
		)
	{
		const U32 start_offset = stream.Tell();
		mxASSERT(IS_ALIGNED_BY(start_offset, Serialization::NwImageSaveFileHeader::ALIGNMENT));

		//
		NwImageSaveFileHeader	header;
		mxDO(stream.Get(header));

		mxDO(ValidatePlatformAndType(header, SAVE_FILE_IMAGE, type));

		DBG_MSG("READ: Object: '%s' (%#010x), data size: '%u', table start: '%u'",
			type.GetTypeName(), header.classId, header.payload, sizeof(header) + header.payload
			);

		// cast to char* so that we can view the contents in the debugger
		char * buffer = (char*) allocator.Allocate( header.payload, type.m_align );
		mxENSURE(buffer, ERR_OUT_OF_MEMORY, "out of mem");

		mxDO(ValidateSizeAndAlignment(
			header
			, SAVE_FILE_IMAGE
			, type
			, buffer
			, header.payload
			));

		//
		mxDO(stream.Read(buffer, header.payload));

		mxDO(ReadAndApplyFixups( stream, buffer, header.payload ));

		//
		Reflection::MarkMemoryAsExternallyAllocated( buffer, type );

		*new_instance_ = buffer;

		return ALL_OK;
	}

}//namespace Serialization
