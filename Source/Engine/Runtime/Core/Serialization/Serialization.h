/*
=============================================================================
	File:	Serialization.h
	Desc:	Serialization functions.
=============================================================================
*/
#pragma once

#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/AssetManagement.h>

namespace Serialization
{
	//
	// Memory image dump based on reflection metadata:
	// serializes into native memory layout for in-place loading (LIP).
	// NOTE: only POD types are supported (constructors are not called).
	// NOTE: pointers to external memory blocks are not supported!
	// Don't forget to release allocated memory to prevent memory leaks!
	//

	/// NB: appends the serialized data to the stream
	ERet SaveMemoryImage(
		const void* instance
		, const TbMetaClass& type
		, AWriter &stream
		);

	ERet LoadMemoryImage(
		void **new_instance_
		, const TbMetaClass& type
		, AReader& stream
		, AllocatorI & allocator
		);

	ERet loadMemoryImageInPlace(
		void * buffer
		, U32 buffer_size
		, const TbMetaClass& type
		, void **instance
		);

	/// NB: appends to the stream
	template< typename CLASS >
	ERet SaveMemoryImage( const CLASS& o, AWriter& writer ) {
		return SaveMemoryImage( &o, CLASS::metaClass(), writer );
	}

	template< typename CLASS >
	ERet LoadMemoryImage( CLASS *&o_, AReader& reader, AllocatorI& allocator ) {
		return LoadMemoryImage( (void**)&o_, CLASS::metaClass(), reader, allocator );
	}


	//
	// Automatic binary serialization (via reflection):
	// serializes to a compact binary format.
	// NOTE: only internal pointers to objects (structs or classes) are supported.
	//

	ERet SaveBinary( const void* o, const TbMetaClass& type, AWriter &writer );
	ERet LoadBinary( void *o, const TbMetaClass& type, AReader& reader );

	template< typename CLASS >
	ERet SaveBinary( const CLASS& o, AWriter& writer ) {
		return SaveBinary( &o, CLASS::metaClass(), writer );
	}
	template< typename CLASS >
	ERet LoadBinary( CLASS &o_, AReader& reader ) {
		return LoadBinary( &o_, CLASS::metaClass(), reader );
	}

	template< typename CLASS >
	ERet loadBinaryFromStream( CLASS &o_, AReader& reader ) {
		return LoadBinary( &o_, CLASS::metaClass(), reader );
	}

	ERet SaveBinaryToFile( const void* o, const TbMetaClass& type, const char* file );
	ERet LoadBinaryFromFile( void *o, const TbMetaClass& type, const char* file );


	// Clumps

	ERet SaveClumpImage( const NwClump& clump, AWriter &writer );
	ERet LoadClumpImage( AReader& reader, NwClump *&clump, AllocatorI& allocator );
	ERet LoadClumpInPlace( AReader& reader, U32 _payload, void *buffer );

	ERet SaveClumpBinary( const NwClump& clump, AWriter &reader );
	ERet LoadClumpBinary( AReader& reader, NwClump& clump );

}//namespace Serialization
