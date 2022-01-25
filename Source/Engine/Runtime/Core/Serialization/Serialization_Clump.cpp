//
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
	using namespace Reflection;


	ERet SaveClumpImage( const NwClump& clump, AWriter &stream )
	{
		AllocatorI & scratch = MemoryHeaps::temporary();

		LIPInfoGatherer	lip( scratch );

		Reflection::AVisitor2::Context	clumpCtx;
		clumpCtx.userName = "NwClump";

		// add the root object body.
		{
			lip.addMemoryChunk( &clump, sizeof(NwClump), EFFICIENT_ALIGNMENT, clumpCtx );
			// add NwClump::_object_lists, etc.
			Reflection::Walker2::Visit( c_cast(void*)(&clump), mxCLASS_OF(clump), &lip, clumpCtx );
		}

		// Recursively visit all referenced objects.
		NwObjectList::Head currentList = clump.GetObjectLists();
		while( currentList != nil )
		{
			const TbMetaClass& type = currentList->getType();
			//DBGOUT("Object list of type '%s':", type.GetTypeName());

			Reflection::AVisitor2::Context	objectListCtx( clumpCtx.depth + 1 );
			objectListCtx.userName = type.GetTypeName();

			// add NwObjectList body.
			{
				lip.addMemoryChunk( currentList, sizeof(NwObjectList), EFFICIENT_ALIGNMENT, objectListCtx );
				Reflection::Walker2::Visit( c_cast(void*)currentList, mxCLASS_OF(*currentList), &lip, objectListCtx );
			}

			// Visit contained objects.
			{
				lip.addMemoryChunk( currentList->raw(), currentList->usableSize(), type.m_align, objectListCtx );

				Reflection::AVisitor2::Context	objectCtx( objectListCtx.depth + 1 );
				objectCtx.userName = type.GetTypeName();

				NwObjectList::IteratorBase it( *currentList );
				while( it.IsValid() )
				{
					void* o = it.ToVoidPtr();
					Reflection::Walker2::Visit( o, type, &lip, objectCtx );
					it.MoveToNext();
				}
			}

			currentList = currentList->_next;
		}

		// Determine file offsets of all memory blocks.
		const U32 chunk_data_size = lip.ResolveChunkOffsets();

		// Write the header.
		NwImageSaveFileHeader	header;
		mxZERO_OUT(header);
		{
			header.filetype = SAVE_FILE_IMAGE;
			//header.classId = mxCLASS_OF(clump).GetTypeGUID();
			header.session = TbSession::CURRENT;
			header.classId = NwClump::FOURCC;
			header.payload = chunk_data_size;
		}
		mxDO(stream.Put( header ));

		const U32 relocation_table_offset = AlignUp(
			sizeof(header) + chunk_data_size
			, FIXUP_TABLE_ALIGNMENT
			);

		DBG_MSG("WRITE: Object: '%s' (%#010x), data size: '%u', relocation table starts at '%u'",
			"NwClump", header.classId, chunk_data_size, relocation_table_offset
			);

		// Write all memory blocks and relocation tables.
		mxDO(lip.WriteChunksAndFixUpTables(
			stream
			, relocation_table_offset
			));

		return ALL_OK;
	}

	ERet LoadClumpInPlace( AReader& stream, U32 _payload, void *_buffer )
	{
		mxASSERT( _payload > 0 );
		mxASSERT_PTR( _buffer );

		// Read clump data.
		mxDO(stream.Read( _buffer, _payload ));

		// initialize vtable
		NwClump * clump = new(_buffer) NwClump(_FinishedLoadingFlag());

		// Patch the clump after loading.
		mxDO(ReadAndApplyFixups( stream, _buffer, _payload ));

		new(&clump->_object_lists_pool)FreeListAllocator();
		clump->_object_lists_pool.Initialize( sizeof(NwObjectList), 16 );

		TellNotToFreeMemory		markMemoryAsExternallyAllocated;
		clump->IterateObjects( &markMemoryAsExternallyAllocated, nil );

		return ALL_OK;
	}

	ERet LoadClumpImage( AReader& stream, NwClump *&clump, AllocatorI& allocator )
	{
		Serialization::NwImageSaveFileHeader	header;
		mxDO(stream.Get( header ));

		mxENSURE(header.filetype == SAVE_FILE_IMAGE, ERR_OBJECT_OF_WRONG_TYPE, "");
		mxENSURE(header.classId == NwClump::FOURCC, ERR_OBJECT_OF_WRONG_TYPE, "");

		mxDO(TbSession::ValidateSession( header.session ));

		void* buffer = allocator.Allocate( header.payload, 16 );
		mxENSURE(buffer, ERR_OUT_OF_MEMORY, "");

		mxDO(Serialization::LoadClumpInPlace( stream, header.payload, buffer ));

		// Return a pointer to the created clump.
		clump = static_cast< NwClump* >( buffer );
		return ALL_OK;
	}

}//namespace Serialization
