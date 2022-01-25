//
#pragma once

#include <typeinfo>

#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Assets/Asset.h>


typedef U64 LoadFlagsT;

///
struct TbAssetLoadContext: NonCopyable
{
	const AssetKey			key;
	const TbMetaClass &		metaclass;

	/// custom flags for overriding loading behaviour
	const LoadFlagsT		load_flags;

	AllocatorI &			raw_allocator;
	ObjectAllocatorI &		object_allocator;	//!< allocator for memory-resident data (MRD)
	AllocatorI &			scratch_allocator;	//!< allocator for temporary storage

public:
	TbAssetLoadContext(
		const AssetKey& asset_key
		, const TbMetaClass& asset_class
		, const LoadFlagsT load_flags
		, ObjectAllocatorI & object_allocator
		, AllocatorI & raw_allocator = MemoryHeaps::resources()
		, AllocatorI & scratch_allocator = MemoryHeaps::temporary()
		)
		: key( asset_key ) 
		, metaclass( asset_class )
		, load_flags( load_flags )
		, raw_allocator( raw_allocator )
		, object_allocator( object_allocator )
		, scratch_allocator( scratch_allocator )
	{}
};

/// Resource managers register themselves automatically upon initialization.
/// There should only be a single instance of the loader for each asset type.
class TbAssetLoaderI: NonCopyable
{
public:
	virtual ~TbAssetLoaderI() {}

	/// [optional]
	virtual ObjectAllocatorI* preferredMemoryHeap()
	{
		return nil;
	}

	/// Allocates memory for a new asset instance and constructs it.
	/// Loads memory-resident data (happens in a background thread).
	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) = 0;

	/// Initializes the asset instance from the supplied data streams.
	/// Resolves references and 'finishes' the instance (e.g. uploads buffers to GPU).
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context )
	{
		return ALL_OK;
	}

	/// Unloads the asset instance; it can be re-loaded with new data
	/// (reverses the effects of Load()).
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context )
	{
	}

	/// Destroys the instance and frees the allocated memory.
	virtual void destroy( NwResource * instance, const TbAssetLoadContext& context ) = 0;

	///
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context )
	{
		unload( instance, context );
		mxDO(load( instance, context ));
		return ALL_OK;
	}
};

///
class TbAssetLoader_Null: public TbAssetLoaderI
{
protected:
	const TbMetaClass &		_asset_type;
	ProxyAllocator			_proxyAllocator;
	ObjectAllocatorProxyWrapper	_proxyWrapper;

public:
	TbAssetLoader_Null( const TbMetaClass& asset_type, ProxyAllocator & parent_allocator )
		: _asset_type( asset_type )
		, _proxyAllocator( asset_type.GetTypeName(), parent_allocator )
		, _proxyWrapper( _proxyAllocator )
	{
	}

	virtual ObjectAllocatorI* preferredMemoryHeap() override
	{
		return &_proxyWrapper;
	}

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override
	{
		NwResource* o = static_cast< NwResource* >( context.object_allocator.allocateObject( context.metaclass ) );
		if( o ) {
			context.metaclass.ConstructInPlace( o );
			*new_instance_ = o;
			return ALL_OK;
		}
		return ERR_OUT_OF_MEMORY;
	}

	virtual void destroy( NwResource * instance, const TbAssetLoadContext& context ) override
	{
		context.metaclass.DestroyInstance( instance );
		context.object_allocator.deallocateObject( context.metaclass, instance );
	}
};

/*
-----------------------------------------------------------------------------
	TbMemoryImageAssetLoader
	- cannot reload resources on-the-fly
-----------------------------------------------------------------------------
*/
class TbMemoryImageAssetLoader: public TbAssetLoader_Null
{
public:
	typedef TbAssetLoader_Null Super;

	TbMemoryImageAssetLoader( const TbMetaClass& asset_type, ProxyAllocator & parent_allocator )
		: TbAssetLoader_Null( asset_type, parent_allocator )
	{
	}

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;

	///
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context )
	{
		// we cannot reload memory images without changing their memory address
		return ERR_NOT_IMPLEMENTED;
	}
};

/*
-----------------------------------------------------------------------------
	TbBinaryAssetLoader

	binary (de-)serializer (reading/writing field by field)
	- can reload resources on-the-fly
-----------------------------------------------------------------------------
*/
class TbBinaryAssetLoader: public TbAssetLoader_Null
{
public:
	typedef TbAssetLoader_Null Super;

	TbBinaryAssetLoader( const TbMetaClass& asset_type, ProxyAllocator & parent_allocator )
		: TbAssetLoader_Null( asset_type, parent_allocator )
	{
	}

	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context );
};


/*
-----------------------------------------------------------------------------
	NwTextAssetLoader
-----------------------------------------------------------------------------
*/
class NwTextAssetLoader: public TbAssetLoader_Null
{
public:
	typedef TbAssetLoader_Null Super;

	NwTextAssetLoader( const TbMetaClass& asset_type, ProxyAllocator & parent_allocator )
		: TbAssetLoader_Null( asset_type, parent_allocator )
	{
	}

	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context );
};
