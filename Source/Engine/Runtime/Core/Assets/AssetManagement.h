/*
=============================================================================
	File:	Asset.h
	Desc:	Asset/Resource management.
			Our resource system is tightly coupled with our reflection system.
	ToDo:	
=============================================================================
*/
#pragma once

#include <Core/Assets/AssetID.h>
#include <Core/Assets/Asset.h>
#include <Core/Assets/AssetLoader.h>
#include <Core/Assets/AssetReference.h>


// Load request priority.
enum ELoadPriority
{
	Load_Priority_Low = 0,
	Load_Priority_Normal = 64,
	Load_Priority_High = 128,
	Load_Priority_Critical = 255,
	Load_Priority_MAX,
};
enum {
	Load_Priority_Collision_Hull = Load_Priority_Critical,
	Load_Priority_Animation = Load_Priority_Critical,
	Load_Priority_Sound = Load_Priority_High,
	Load_Priority_Music = Load_Priority_Normal,
	Load_Priority_Texture = Load_Priority_Low,
};
enum ELoadStatus
{
	LoadStatus_HadErrors = 0,
	LoadStatus_Loaded,
	LoadStatus_Unloaded,
	LoadStatus_Pending,
	LoadStatus_Cancelled,
};
enum { MAX_IO_READ_SIZE = 2*mxMEGABYTE };

// Numeric identifier for an asset load request,
// it could be 64-bit to protect against wraparound errors
mxDECLARE_32BIT_HANDLE(LoadRequestId);





///	Resource/asset data/stream reader interface (resource/content provider).
/// Designed for low-overhead at the cost of being less generic.
struct AssetReader
	: public AReader
{
	union {
		char	file[sizeof(FileReader)];

		struct {
			U32		file_entry_index;
			U32		stream_index;
			U32		read_cursor;
		} bundle_reader;

		struct {
			void *	file_entry_ptr;
			U32		stream_index;
			U32		read_cursor;
		} dev_bundle_reader;
	};

	TPtr< AssetPackage >	parent;

public:
	virtual ERet Read( void *buffer, size_t size ) override;
	virtual size_t Length() const override;
	virtual size_t Tell() const override;
};

///
struct AssetPackage abstract
	: public AObject
	, TLinkedList< AssetPackage >
	, NonCopyable
{
public:
	enum {
		OBJECT_DATA = 0,	//!< memory resident data
		STREAM_DATA0 = 1,	//!< streaming data (e.g. a mesh buffer, a mip level)
		MAX_STREAMS
	};

	virtual ERet Open(
		const AssetKey& _key,
		AssetReader *stream_,
		const U32 _subresource = OBJECT_DATA
	) = 0;

	virtual void Close(
		AssetReader * stream
	) = 0;

	virtual ERet read( AssetReader * reader, void *buffer, size_t size ) = 0;

	virtual size_t length( const AssetReader * reader ) const = 0;

	virtual size_t tell( const AssetReader * reader ) const = 0;

protected:
	~AssetPackage();
};




/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
namespace Resources
{
	//
	// Initialization/Shutdown
	//
	ERet Initialize();
	void Shutdown();

	TbAssetLoaderI* binaryAssetLoader();
	TbAssetLoaderI* memoryImageAssetLoader();
	TbAssetLoaderI* GetTextAssetLoader();

	template< class RESOURCE >	// where RESOURCE: NwResource
	mxFORCEINLINE TbAssetLoaderI* GetLoaderFor()
	{
		switch(RESOURCE::SERIALIZATION_METHOD)
		{
		case RESOURCE::Serialization_Unspecified:
			return nil;

		case RESOURCE::Serialize_as_MemoryImage:
			return memoryImageAssetLoader();

		case RESOURCE::Serialize_as_BinaryFile:
			return binaryAssetLoader();

		case RESOURCE::Serialize_as_Text:
			return GetTextAssetLoader();

			mxNO_SWITCH_DEFAULT;
		}
		mxUNREACHABLE;
		return nil;
	}


	//
	// Lightweight virtual file system
	//
	ERet MountPackage( AssetPackage* package );
	void UnmountPackage( AssetPackage* package );
	bool IsPackageMounted( AssetPackage* package );
	//[must be threadsafe] returns null on failure
	ERet OpenFile(
		const AssetKey& _id,
		AssetReader *stream_,
		const U32 _subresource = AssetPackage::OBJECT_DATA
	);

	//
	// Resource management
	//
	NwResource* find( const AssetKey& key );

	ERet insert(
		const AssetKey& key
		, NwResource* o
		, ObjectAllocatorI* storage = nil
		);

	ERet replaceExisting(
		const AssetKey& key
		, NwResource* o
		, ObjectAllocatorI* storage = nil
		);

	// NOTE: costly!
	ERet remove( const AssetKey& key );

	/// Replaces existing instance
//	void Set( const AssetKey& key, Resource* o, AllocatorI* storage );

	/// Slow!
	void unload(
		NwResource* o
		, const AssetKey& key
		);

	/// unload and destroy all asset instances;
	/// this is used to prevent memory leaks
	void unloadAndDestroyAll();

	// Development

	// attempts to reload the given asset instance, does nothing if the asset is not loaded;
	// this should be called by the file/directory watcher/monitor
	ERet Reload( const AssetKey& key );	// thread-safe
//	ERet ReloadAssetsOfType( const AssetTypeT& type );	// thread-safe
	ERet ReloadAll();	// thread-safe


#if MX_DEVELOPER

	// Generate a manifest file which contains a set of IDs of loaded assets
	// so that assets can be preloaded beforehand, thus improving the startup performance
	// of the game and also preventing frame drops/spikes during gameplay.
	ERet DumpUsedAssetsList( const char* dest_folder );

	ERet MergeWithPreviousUsedAssetsList( const char* dest_folder );

#endif // MX_DEVELOPER

}//namespace Resources


namespace Resources
{
	/// Creates and loads the resource.
	ERet Load(
		NwResource *&resource_
		, const AssetID& asset_id
		, const TbMetaClass& _type
		, ObjectAllocatorI* storage = nil
		, const LoadFlagsT flags = 0
		, NwResource* fallback_instance = nil
		, TbAssetLoaderI* override_loader = nil
	);

	ERet Unload(
		const AssetID& _id,
		const TbMetaClass& _type
	);

	/// Creates and loads the resource.
	template< typename RESOURCE >
	ERet Load(
		RESOURCE *&o_
		, const AssetID& asset_id
		, ObjectAllocatorI* storage = nil
		, const LoadFlagsT flags = 0
		, RESOURCE* fallback_instance = nil
		)
	{
		mxSTATIC_ASSERT( (std::is_base_of< NwResource, RESOURCE >::value) );

		o_ = fallback_instance;

		NwResource* base_resource = nil;

		mxTRY(Load(
			base_resource
			, asset_id
			, RESOURCE::metaClass()
			, storage
			, flags
			, fallback_instance
			, GetLoaderFor<RESOURCE>()
			));

		o_ = static_cast< RESOURCE* >( base_resource );

		return ALL_OK;
	}

	/// Unloads and destroys the resource.
	template< typename RESOURCE >
	void Unload(
		RESOURCE *&_o,
		const AssetID& _id
		)
	{
		mxSTATIC_ASSERT( (std::is_base_of< NwResource, RESOURCE >::value) );
		Unload( _id, RESOURCE::metaClass() );
		_o = nil;
	}

}//namespace Resources








/*
Asset Imports are references to objects in other Clumps.
Asset Exports are asset instances that exist in a Clump.
*/


/*
--------------------------------------------------------------
	AssetImport

	External Assets -> Pointers in Clump

	asset references are stored in clumps;
	they keep references to external assets
--------------------------------------------------------------
*/
struct AssetImport: public AssetKey
{
	//Resource **	o;		//!< linked list of pointers to asset instances within the clump
public:
	mxDECLARE_CLASS(AssetImport,AssetKey);
	mxDECLARE_REFLECTION;
	AssetImport();
	bool CheckIsValid() const;
	//// this could be done asynchronously
	//bool Resolve() const;
};

/*
--------------------------------------------------------------
	AssetExport

	'exports' assets in a clump -> asset cache

	points to the memory-resident part of asset:
	the asset instance which is always stored in a clump.

	Asset instances are initialized with data
	retrieved by their id and then get registered in the asset cache.

	Asset instances are unregistered from the asset cache
	when the corresponding clump is unloaded.
--------------------------------------------------------------
*/
struct AssetExport: public AssetKey
{
	NwResource *	o;	//!< pointer to the asset instance within the clump
public:
	mxDECLARE_CLASS(AssetExport,AssetKey);
	mxDECLARE_REFLECTION;
	AssetExport();
	bool CheckIsValid() const;
};


/*
-----------------------------------------------------------------------------
	UsedAssetsList
-----------------------------------------------------------------------------
*/
struct UsedAssetsList: CStruct, NonCopyable
{
	TArray< AssetKey >	used_assets;

public:
	mxDECLARE_CLASS(UsedAssetsList,CStruct);
	mxDECLARE_REFLECTION;
};


#if MX_DEVELOPER

namespace Resources
{
	struct CopyUsedAssetsParams
	{
		const char* used_assets_manifest_file;

		const char* src_assets_folder;
		const char* dst_assets_folder;

	public:
		CopyUsedAssetsParams()
		{
			mxZERO_OUT(*this);
		}
		
		void SetDefaults()
		{
			used_assets_manifest_file = "R:/used_assets.son";

			src_assets_folder = "R:/NoobWerks/.Build/Assets/";
			dst_assets_folder = "R:/temp/";
		}
	};
	ERet CopyOnlyUsedAssets(
		const CopyUsedAssetsParams& params
		, AllocatorI& scratchpad
		);

}//namespace Resources

#endif // MX_DEVELOPER
