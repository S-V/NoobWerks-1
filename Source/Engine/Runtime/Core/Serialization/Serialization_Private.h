//
#pragma once

#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/AssetManagement.h>


#define DEBUG_SERIALIZATION (0)

#if DEBUG_SERIALIZATION
	#define DBG_MSG(...)\
		DBGOUT(__VA_ARGS__)
	#define DBG_MSG2(ctx,...)\
		LogStream(LL_Info).Repeat(' ',ctx.depth).PrintF(__VA_ARGS__);
#else
	#define DBG_MSG(...)
	#define DBG_MSG2(ctx,...)
#endif



namespace Serialization
{


using namespace Reflection;



	///
	enum ESaveFileType
	{
		SAVE_FILE_IMAGE		= MCHAR4('S','I','M','G'),	// Memory Image
		SAVE_FILE_BINARY	= MCHAR4('S','B','I','N'),	// Binary File
	};

#pragma pack (push,1)
	/// info for in-place loading (a 'frozen' memory image with pointer patch tables)
	struct NwImageSaveFileHeader
	{
		U32			filetype;	// ESaveFileType
		U32			unused[3];

		TbSession	session;	//!< 8 platform/engine info
		TypeGUID	classId;	//!< 4 metaclass of stored object
		U32			payload;	//!< 4 size of stored data following this header

		// make sure to align the read/write cursor when the serialized data follows a string
		enum { ALIGNMENT = 4 };
	};
	ASSERT_SIZEOF(NwImageSaveFileHeader, 32);

	/// binary format (aka 'tag file') (a stream of binary object data)
	struct NwBinarySaveFileHeader
	{
		U32			filetype;	// ESaveFileType
		U32			unused[3];

		TbSession	session;	//!< 8 platform/engine info
		TypeGUID	classId;	//!< 4 metaclass of stored object
		U32			_unused;	//!< 4 padding to align data to 16 bytes
	};
	ASSERT_SIZEOF(NwBinarySaveFileHeader, 32);
#pragma pack (pop)





#if MX_DEBUG
	static const U32 PADDING_VALUE = MCHAR4('P','A','D','N');
#else
	static const U32 PADDING_VALUE = 0;
#endif

	static const U32 NULL_POINTER_OFFSET = ~0UL;

	enum {
		FIXUP_TABLE_ALIGNMENT = sizeof(U32)
	};

	mxSTATIC_ASSERT(sizeof(TypeGUID) == sizeof(U32));

	//
	// Memory image serialization and in-place loading
	// NOTE: all pointer offsets are relative to start of object data
	// (object data starts right after the header).

	/// each chunk represents a contiguous memory block
	struct SChunk
	{
		const void *data;	// pointer to the start of the memory block		
		const char *name;	// for debugging		
		U32		size;		// unaligned size of this chunk
		U32		offset;		// (aligned) file offset this chunk begins at (0 for the first chunk)
		U32		alignment;	// alignment requirement for the memory block of this chunk
	};

	/// represents a pointer that needs to be fixed-up during in-place loading
	struct SPointer
	{
		const void *address;	// memory address of this pointer itself
		const void *target;		// memory address this pointer points at
		const char *name;		// for debugging
	};

	/// represents a pointer to an external asset instance that needs to be fixed-up during in-place loading
	struct SAssetReference
	{
		NwAssetRef * ref;
		const MetaAssetReference* type;
		const char *name;		// for debugging
	};

	///
	struct STypeInfo
	{
		SClassId *	o;
		const char *name;		// for debugging
	};

	static inline bool ContainsAddress( const SChunk& chunk, const void* pointer )
	{
		mxASSERT_PTR(pointer);
		return mxPointerInRange( pointer, chunk.data, chunk.size );
	}
	static inline U32 GetFileOffset( const void* pointer, const SChunk& chunk )
	{
		mxASSERT(ContainsAddress(chunk, pointer));
		const U32 relativeOffset = mxGetByteOffset32( chunk.data, pointer );
		const U32 absoluteOffset = chunk.offset + relativeOffset;
		return absoluteOffset;
	}
	static U32 GetFileOffset( const void* pointer, const DynamicArray< SChunk >& chunks )
	{
		mxASSERT_PTR(pointer);
		for( U32 iChunk = 0; iChunk < chunks.num(); iChunk++ )
		{
			const SChunk &	chunk = chunks[ iChunk ];
			const ptrdiff_t relativeOffset = (char*)pointer - (char*)chunk.data;
			if( relativeOffset >= 0 && relativeOffset < chunk.size )
			{
				return chunk.offset + relativeOffset;
			}
		}
		mxASSERT2(false, "Bad pointer: 0x%p\n", pointer);
		return NULL_POINTER_OFFSET;
	}

	struct FixUpTable
	{
		U32		numPointerFixUps;
		U32		numClassIdFixUps;
		U32		numAssetIdFixUps;		// pointers to AssetID
		U32		num_asset_references;	// SAssetReference
	};

	// Gathers information necessary for memory image serialization: collects all memory blocks and pointers.
	struct LIPInfoGatherer: public Reflection::AVisitor2
	{
		DynamicArray< SChunk > 				chunks;		// memory blocks to be serialized
		DynamicArray< SPointer > 			pointers;	// pointers to be patched after loading; they can only point inside the above memory blocks
		DynamicArray< STypeInfo >			typeFixups;	// references to type IDs (serialized as TypeGUIDs)
		DynamicArray< AssetID* > 			assetIdFixups;
		DynamicArray< SAssetReference > 	assetRefsFixups;

	public:
		LIPInfoGatherer( AllocatorI & _heap )
			: chunks( _heap )
			, pointers( _heap )
			, typeFixups( _heap )
			, assetIdFixups( _heap )
			, assetRefsFixups( _heap )
		{}
		const SChunk* FindChunk( const void* _memory ) const
		{
			for( U32 i = 0; i < chunks.num(); i++ )
			{
				const SChunk& chunk = chunks[ i ];
				if( chunk.data == _memory ) {
					return &chunk;
				}
			}
			return nil;
		}

		/// add a memory chunk that will be written to the output archive
		SChunk& addMemoryChunk( const void* start_ptr, U32 length, U32 alignment, const Context& ctx )
		{
			const char* _name = ctx.GetMemberName();
			DBG_MSG2(ctx,"addMemoryChunk(): start=0x%p, length=%u, align=%u (\'%s\')", start_ptr, length, alignment, _name);
			mxASSERT_PTR(start_ptr);
			mxASSERT2(length > 0, "no objects of type '%s', so it's meaningless to call this function", _name);
			mxASSERT2(length < mxMiB(128), "mem block size is too large: %d", length);

			alignment = largest(alignment, MINIMUM_ALIGNMENT);

			SChunk & new_chunk = chunks.AddNew();
			{
				new_chunk.name = _name;
				new_chunk.data = start_ptr;
				new_chunk.size = length;
				new_chunk.offset = ~0UL;	// file offset will be resolved after collecting all chunks
				new_chunk.alignment = alignment;
			}

			return new_chunk;
		}

		/// 
		SPointer& addPointerToFixUpWhenLoading(
			const void* address_of_pointer
			, const void* address_of_pointed_object
			, const Context& _ctx
			)
		{
			const char* _name = _ctx.GetMemberName();
			DBG_MSG2(_ctx,"addPointerToFixUpWhenLoading() at 0x%p to 0x%p (\'%s\')"
				, address_of_pointer, address_of_pointed_object, _name
				);

			SPointer &	newPointer = pointers.AddNew();
			newPointer.address = address_of_pointer;
			newPointer.target = address_of_pointed_object;
			newPointer.name = _name;

			return newPointer;
		}

		/// returns the total size of all memory blocks
		U32 ResolveChunkOffsets()
		{
			// Sort memory blocks as needed (e.g. by increasing addresses to slightly improve data locality, etc.).
			// ...

			// Calculate absolute file offsets of all memory blocks
			// and the total size of the serialized memory image.
			U32 current_offset = 0;

			for( U32 iChunk = 0; iChunk < chunks.num(); iChunk++ )
			{
				SChunk & chunk = chunks[ iChunk ];
				chunk.offset = AlignUp( current_offset, chunk.alignment );
				current_offset = chunk.offset + chunk.size;
				DBG_MSG("WRITE: Chunk '%s': %u bytes at %u", chunk.name, chunk.size, chunk.offset);
			}

			return current_offset;
		}

		//@TODO: single pass (need to use the TellOffset() function)
		ERet WriteChunksAndFixUpTables(
			AWriter &stream
			, U32 relocation_table_offset	// for debug checking
			)
		{
			// Write all memory blocks to file.
			U32 bytesWritten = 0;	//!< not including size of blob header
			for( U32 iChunk = 0; iChunk < chunks.num(); iChunk++ )
			{
				const SChunk & chunk = chunks[ iChunk ];
				const U32 current_offset = bytesWritten;
				const U32 aligned_offset = chunk.offset;
				const U32 sizeOfPadding = aligned_offset - current_offset;
				if( sizeOfPadding > 0 ) {
					mxDO(Stream_Fill( stream, sizeOfPadding, PADDING_VALUE ));
				}
				mxDO(stream.Write( chunk.data, chunk.size ));
				bytesWritten += ( sizeOfPadding + chunk.size );
			}

			//
			U32	aligned_bytes_written = bytesWritten;
			{
				const U32 current_offset = bytesWritten;
				const U32 aligned_offset = AlignUp( current_offset, FIXUP_TABLE_ALIGNMENT );
				const U32 sizeOfPadding = aligned_offset - current_offset;
				Stream_Fill( stream, sizeOfPadding, PADDING_VALUE );
				aligned_bytes_written = aligned_offset;
			}

			// Relocation data begin starts right after serialized object data.
			const U32 relocationTableOffset = sizeof(NwImageSaveFileHeader) + aligned_bytes_written;
			mxASSERT(relocationTableOffset == relocation_table_offset);

			// Append pointer patch tables.
			FixUpTable	fixUpTable;
			fixUpTable.numPointerFixUps = pointers.num();
			fixUpTable.numClassIdFixUps = typeFixups.num();
			fixUpTable.numAssetIdFixUps = assetIdFixups.num();
			fixUpTable.num_asset_references = assetRefsFixups.num();
			mxDO(stream.Put( fixUpTable ));

			for( U32 i = 0; i < fixUpTable.numPointerFixUps; i++ )
			{
				const SPointer& pointer = pointers[ i ];
				const U32 pointerOffset = GetFileOffset( pointer.address, chunks );
				const U32 targetOffset = GetFileOffset( pointer.target, chunks );
				stream << pointerOffset;
				stream << targetOffset;
				DBG_MSG("WRITE: Pointer '%s': %u -> %u", pointer.name, pointerOffset, targetOffset);
			}
			for( U32 i = 0; i < fixUpTable.numClassIdFixUps; i++ )
			{
				const STypeInfo& pointer = typeFixups[ i ];
				const U32 pointerOffset = GetFileOffset( pointer.o, chunks );
				const TypeGUID classID = pointer.o->type->GetTypeGUID();
				stream << pointerOffset;
				stream << classID;
				DBG_MSG("WRITE: ClassID '%s': %u -> '%s'", pointer.name, pointerOffset, pointer.o->type->GetTypeName());
			}
			for( U32 iAssetRef = 0; iAssetRef < fixUpTable.numAssetIdFixUps; iAssetRef++ )
			{
				const AssetID* assetID = assetIdFixups[ iAssetRef ];
				const U32 pointerOffset = GetFileOffset( assetID, chunks );
				stream << pointerOffset;
				writeAssetID( *assetID, stream );
			}

			//
			for( U32 iAssetRef = 0; iAssetRef < fixUpTable.num_asset_references; iAssetRef++ )
			{
				const SAssetReference& asset_ref = assetRefsFixups[ iAssetRef ];

				//
				const U32 pointerOffset = GetFileOffset( &asset_ref.ref->ptr, chunks );
				stream << pointerOffset;

				//
				const TypeGUID classID = asset_ref.type->asset_type.GetTypeGUID();
				stream << classID;

				//
				writeAssetID( asset_ref.ref->id, stream );
			}

			DBG_MSG("WRITE: %u chunks, %u pointers, %u typeIDs, %u assetIDs at %u (table at %u)",
				chunks.num(),pointers.num(),typeFixups.num(),assetIdFixups.num(),relocationTableOffset,relocationTableOffset
				);
			return ALL_OK;
		}
	public:
		typedef Reflection::AVisitor2 Super;

		//-- Reflection::AVisitor
		virtual void Visit_Pointer( VoidPointer& p, const MetaPointer& type, const Context& _context ) override
		{
			if( p.o != nil )
			{
				this->addPointerToFixUpWhenLoading( &p.o, p.o, _context );
			}
			else
			{
				// null pointers will be written as it is - zeros
			}
		}

		virtual void visit_AssetReference(
			NwAssetRef * asset_ref
			, const MetaAssetReference& asset_reference_type
			, const Context& context
			) override
		{
			SAssetReference	new_asset_ref;
			new_asset_ref.ref = asset_ref;
			new_asset_ref.type = &asset_reference_type;
			new_asset_ref.name = context.GetMemberName();

			assetRefsFixups.add( new_asset_ref );
		}

		virtual void Visit_TypeId( SClassId * o, const Context& _context ) override
		{
			DBG_MSG2(_context,"Visit_TypeId(): '%s' at 0x%p (\'%s\')", o->type->GetTypeName(), o->type, _context.GetMemberName());
			STypeInfo & newItem = typeFixups.AddNew();
			newItem.o = o;
			newItem.name = _context.GetMemberName();
		}
		virtual bool Visit_Array( void * _array, const MetaArray& type, const Context& _context ) override
		{
			const U32 capacity = type.Generic_Get_Capacity( _array );
			if( capacity > 0 )
			{
				const void* arrayBase = type.Generic_Get_Data( _array );
				mxASSERT_PTR(arrayBase);
				const TbMetaType& itemType = type.m_itemType;
				if( type.IsDynamic() )
				{
					this->addMemoryChunk( arrayBase, capacity * itemType.m_size, itemType.m_align, _context );
					this->addPointerToFixUpWhenLoading( type.Get_Array_Pointer_Address( _array ), arrayBase, _context );
				}
			}
			const bool bIterateOverElements = Reflection::objectOfThisTypeMayContainPointers( type.m_itemType );
			return bIterateOverElements;
		}

		virtual void visit_Dictionary(
			void * dictionary_instance
			, const MetaDictionary& dictionary_type
			, const Context& context
			) override
		{
			AVisitor2::Context	dictionary_context( context.depth + 1 );
			dictionary_context.parent = &context;
			dictionary_context.userData = context.userData;

			struct CustomParameters {
				LIPInfoGatherer *		me;
				AVisitor2::Context *	context;
			} parameters = {
				this,
				&dictionary_context,
			};

			//
			struct Callbacks
			{
				static void iterateMemoryBlock(
					void ** p_memory_pointer
					, const U32 memory_size
					, const bool is_dynamically_allocated
					, const TbMetaType& item_type
					, const U32 live_item_count
					, const U32 item_stride
					, void* user_data
					)
				{
					CustomParameters* params = (CustomParameters*) user_data;

					void *const pointer_to_stored_items = *p_memory_pointer; 

					if( is_dynamically_allocated )
					{
						params->me->addMemoryChunk( pointer_to_stored_items, memory_size, item_type.m_align, *params->context );
						params->me->addPointerToFixUpWhenLoading( p_memory_pointer, pointer_to_stored_items, *params->context );
					}

					if( Reflection::objectOfThisTypeMayContainPointers( item_type ) )
					{
						void * instance = pointer_to_stored_items;
						for( U32 i = 0; i < live_item_count; i++ )
						{
							Walker2::Visit( instance, item_type, params->me, *params->context );
							instance = mxAddByteOffset( instance, item_stride );
						}
					}
				}
			};

			dictionary_type.iterateMemoryBlocks(
				dictionary_instance
				, &Callbacks::iterateMemoryBlock
				, &parameters
				);
		}

		virtual void Visit_String( String & _string, const Context& _context ) override
		{
			if( _string.nonEmpty() )
			{
				this->addMemoryChunk( _string.raw(), _string.length()+1, STRING_ALIGNMENT, _context );
				this->addPointerToFixUpWhenLoading( _string.getBufferAddress(), _string.raw(), _context );
			}
		}
		virtual void Visit_AssetId( AssetID & _assetId, const Context& _context )
		{
			assetIdFixups.add( &_assetId );
		}
	};

	ERet ReadAndApplyFixups(
		AReader& reader
		, void* object_buffer, U32 buffer_size
		);











	template< class HEADER >
	static ERet ValidatePlatformAndType(
		const HEADER& header
		, ESaveFileType filetype
		, const TbMetaClass& metaclass
		)
	{
		mxENSURE(
			header.filetype == filetype,
			ERR_INCOMPATIBLE_VERSION,
			"wrong file format parser"
			);
		mxDO(TbSession::ValidateSession(header.session));
		chkRET_X_IF_NOT(metaclass.GetTypeGUID() == header.classId, ERR_OBJECT_OF_WRONG_TYPE);
		return ALL_OK;
	}

	template< class HEADER >
	static ERet ValidateSizeAndAlignment(
		const HEADER& header
		, ESaveFileType filetype
		, const TbMetaClass& metaclass
		, const void* buffer, U32 length
		)
	{
		mxENSURE(
			header.filetype == filetype,
			ERR_INCOMPATIBLE_VERSION,
			"wrong file format parser"
			);

		chkRET_X_IF_NOT(length >= header.payload, ERR_BUFFER_TOO_SMALL);
		chkRET_X_IF_NOT(metaclass.m_size <= header.payload, ERR_BUFFER_TOO_SMALL);
		chkRET_X_IF_NOT(IsAlignedBy(buffer, metaclass.m_align), ERR_INVALID_ALIGNMENT);
		chkRET_X_IF_NOT(metaclass.GetTypeGUID() == header.classId, ERR_OBJECT_OF_WRONG_TYPE);
		return ALL_OK;
	}

}//namespace Serialization
