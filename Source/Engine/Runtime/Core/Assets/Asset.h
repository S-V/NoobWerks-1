// Base class for resource objects.
#pragma once

#include <Core/Assets/AssetID.h>


/*
-----------------------------------------------------------------------------
	BASE RESOURCE CLASS
-----------------------------------------------------------------------------
*/

/// Represents a generic resource.
/// NOTE: Must be EMPTY! Must NOT define any virtual member functions!
/// Provides common functionality for asset types via static polymorphism.
struct NwResource: CStruct
{
	mxDECLARE_CLASS( NwResource, CStruct );
	mxDECLARE_REFLECTION;

	enum SerializationMethod
	{
		// will use the loader from the class
		Serialization_Unspecified,

		/// The fastest - (de-)serialize with a single memory write or read.
		Serialize_as_POD,

		/// Saves a memory dump/shapshot - similar to the POD method above,
		/// but can be used for serializing objects with internal pointers.
		/// Very fast reads - needs only one memory allocation to allocate the object.
		Serialize_as_MemoryImage,

		/// serialize field by field in binary format,
		/// needs memory allocations to resize arrays.
		Serialize_as_BinaryFile,

		/// Slowest, many allocations, but stored in a human-readable JSON-like text format.
		Serialize_as_Text,
	};
	/// can be overridden on per-class basis!
	static const SerializationMethod SERIALIZATION_METHOD = Serialization_Unspecified;
};

/// Reference counting mixin functionality
class NwSharedResource: public NwResource, NonCopyable
{
	//AtomicInt	_reference_count;

public:
	mxDECLARE_CLASS( NwSharedResource, NwResource );
	mxDECLARE_REFLECTION;

	NwSharedResource();
	~NwSharedResource();

	void Grab();

	void Drop(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		);

	/// gives the number of things pointing to it
	U32 getUseCount() const;
};

mxFORCEINLINE U32 NwSharedResource::getUseCount() const
{
	return 0;///AtomicLoad( _reference_count );
}






///
class ObjectAllocatorProxyWrapper: public ObjectAllocatorI
{
	ProxyAllocator &	_underlyingAllocator;

public:
	ObjectAllocatorProxyWrapper( ProxyAllocator & underlyingAllocator )
		: _underlyingAllocator( underlyingAllocator )
	{}

	virtual void* allocateObject( const TbMetaClass& object_type, U32 granularity = 1 ) override
	{
		return _underlyingAllocator.Allocate( object_type.GetInstanceSize(), object_type.m_align );
	}
	virtual void deallocateObject( const TbMetaClass& object_type, void* object_instance ) override
	{
		_underlyingAllocator.Deallocate( object_instance );
	}
};
