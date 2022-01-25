#pragma once

#include <Core/Serialization/Text/TxTCommon.h>

class NwClump;

namespace SON
{
	struct SaveOptions {
		bool	wrapRootInBraces;
	public:
		SaveOptions() {
			wrapRootInBraces = false;
		}
	};

	// NOTE: only objects without pointers can be (de)serialized.

	ERet LoadFromBuffer(
		char* _text, int _size
		, void *o, const TbMetaType& type
		, AllocatorI & temporary_allocator
		, const char* filename = "", int linenum = 1
	);

	ERet LoadFromStream(
		void *o, const TbMetaType& type
		, AReader& stream_reader
		, AllocatorI & temporary_allocator
		, const char* filename = "", int linenum = 1
	);

	template< typename TYPE >
	ERet Load( AReader& _stream, TYPE &_o, AllocatorI & temporary_allocator, const char* _file = "", int _line = 1 )
	{
		return LoadFromStream(
			&_o, mxTYPE_OF(_o)
			, _stream
			, temporary_allocator
			, _file, _line
			);
	}

	template< typename TYPE >
	ERet LoadFromFile( const char* filename, TYPE &o_, AllocatorI & temporary_allocator )
	{
		DBGOUT("Loading object 0x%p from file '%s'...", &o_, filename);

		FileReader	stream;
		mxTRY(stream.Open( filename, FileRead_NoErrors ));
		mxTRY(LoadFromStream(
			&o_, mxTYPE_OF(o_)
			, stream
			, temporary_allocator
			, filename
			));
		return ALL_OK;
	}

	Node* ParseTextBuffer(
		char* text_buf, int buf_size
		, NodeAllocator & node_allocator
		, const char* filename = "", int linenum = 1
	);




	ERet Decode( const Node* _root, const TbMetaType& _type, void *_o );

	template< typename TYPE >
	ERet Decode( const Node* _root, TYPE &_o )
	{
		return Decode( _root, mxTYPE_OF(_o), &_o );
	}

	Node* Encode( const void* _o, const TbMetaType& _type, NodeAllocator & temporary_allocator );

	template< typename TYPE >
	Node* Encode( const TYPE& _o, NodeAllocator & temporary_allocator )
	{
		return Encode( &_o, mxTYPE_OF(_o), temporary_allocator );
	}

	ERet SaveToStream(
		const void* _o, const TbMetaType& _type,
		AWriter &_stream
	);

	template< typename TYPE >
	ERet Save( const TYPE& _o, AWriter &_stream )
	{
		return SaveToStream( &_o, mxTYPE_OF(_o), _stream );
	}

	template< typename TYPE >
	ERet SaveToFile( const TYPE& o, const char* filename )
	{
		DBGOUT("Saving object 0x%p to file '%s'...", &o, filename);

		// make sure the destination directory exists
		String256 destination_folder;
		Str::CopyS( destination_folder, filename );
		Str::StripFileName( destination_folder );
		if( destination_folder.nonEmpty() && !OS::IO::FileOrPathExists( destination_folder.c_str() ) )
		{
			if( !OS::IO::MakeDirectory( destination_folder.c_str() ) ) {
				return ERR_FAILED_TO_CREATE_FILE;
			}
		}

		FileWriter	stream;
		mxTRY(stream.Open( filename, FileWrite_NoErrors ));
		mxTRY(SaveToStream( &o, mxTYPE_OF(o), stream ));
		return ALL_OK;
	}

	ERet SaveClump(
		const NwClump& _clump,
		AWriter &_stream,
		const SaveOptions& _options = SaveOptions()
	);

	ERet LoadClump(
		AReader &_stream, NwClump &_clump,
		const char* _file = "", int _line = 1
	);

	ERet LoadClump( const SON::Node* root, NwClump &_clump );

	ERet SaveClumpToFile( const NwClump& _clump, const char* _file );
	ERet LoadClumpFromFile( const char* _file, NwClump &_clump );

}//namespace SON
