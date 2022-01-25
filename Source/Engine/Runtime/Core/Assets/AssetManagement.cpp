/*
=============================================================================
	File:	Asset.cpp
	Desc:	Asset (Resource) management.
	References:

	Solving Problems With Asynchrony: Asset Loading:
	http://www.altdevblogaday.com/2011/04/19/solving-problems-with-asynchrony-asset-loading/

	Asset loading in emscripten and PNaCl:
	http://flohofwoe.blogspot.ru/2013/12/asset-loading-in-emscripten-and-pnacl.html

	Asynchronous Asset Loading (Unreal Engine 4.5)
	https://docs.unrealengine.com/latest/INT/Programming/Assets/AsyncLoading/index.html

	Asset Manager Guide
	https://github.com/GarageGames/Torque2D/wiki/Asset-Manager-Guide

	Asset Management (Explorer, Pipeline, and Database) (Visual 3D)
	http://game-engine.visual3d.net/wiki/visual3d-asset-database-asset-explorer-pipeline-and-persistence

	A Resource Manager for Game Assets
	http://www.gamedev.net/page/resources/_/technical/game-programming/a-resource-manager-for-game-assets-r3807



	Basics for asynchronous loading:

	https://github.com/angrave/SystemProgramming/wiki/Synchronization,-Part-8:-Ring-Buffer-Example
	https://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/
	// Single producer, single consumer
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms686903(v=vs.85).aspx


	http://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer
	https://github.com/krizhanovsky/NatSys-Lab/blob/master/lockfree_rb_q.cc

// Resource manager state machine (https://gist.github.com/rygorous/3982477):
//       +------+         +--------+
//       | Live |<------->| Locked |
//       +------+         +--------+
//       /     \                  ^
//      /       \                  \
//     v         v                  \
// +------+    +------+    +------+  |
// | Dead |--->| Free |<---| User |  |
// +------+    +------+    +------+  |
//     ^         ^    ^       ^      |
//      \       /       \     |      |
//       \     /          v   |      |
//     +--------+         +-------+ /
//     | Pinned |<--------| Alloc |/
//     +--------+         +-------+

	https://jfdube.wordpress.com/2011/09/21/guid-generation/
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Assets/AssetLoader.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/ObjectModel/Clump.h>

#if MX_DEVELOPER
#include <Core/Serialization/Text/TxTSerializers.h>
#endif

/*
--------------------------------------------------------------
	AssetReader
--------------------------------------------------------------
*/
ERet AssetReader::Read( void *buffer, size_t size )
{
	return parent->read( this, buffer, size );
}

size_t AssetReader::Length() const
{
	return parent->length( this );
}

size_t AssetReader::Tell() const
{
	return parent->tell( this );
}

/*
--------------------------------------------------------------
	AssetPackage
--------------------------------------------------------------
*/
AssetPackage::~AssetPackage()
{
	//mxASSERT(!Resources::IsPackageMounted(this));
}

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
namespace Resources
{
	namespace
	{
		struct DefaultObjectHeap : ObjectAllocatorI
		{
			TrackingHeap	m_heap;
		public:
			DefaultObjectHeap()
				: m_heap( "Resources", MemoryHeaps::global() )
			{
			}
			virtual void* allocateObject( const TbMetaClass& _type, U32 _granularity = 1 ) override
			{
				UNDONE;
				return m_heap.Allocate( _type.GetInstanceSize(), _type.m_align );
			}
			virtual void deallocateObject( const TbMetaClass& _type, void* _pointer ) override
			{
				UNDONE;
				m_heap.Deallocate( _pointer );
			}
		};

		static void __Do_Unload_And_Destroy( NwResource * _resource, const AssetID& _id, const TbMetaClass& resourceType, ObjectAllocatorI & storage )
		{
			DEVOUT("Destroying '%s' of type '%s' (0x%p in heap 0x%p)..."
				, AssetId_ToChars(_id), resourceType.GetTypeName(), _resource, &storage);

			TbAssetLoaderI* resourceLoader = resourceType.loader;

			AssetKey	assetKey;
			assetKey.id = _id;
			assetKey.type = resourceType.GetTypeGUID();

			//
			TbAssetLoadContext context(
				assetKey,
				resourceType,
				LoadFlagsT(0),
				storage
				);

			resourceLoader->unload( _resource, context );
		
			resourceLoader->destroy( _resource, context );
		}
	}//namespace

	struct ResourceTableEntry
	{
		NwResource *	resource;	//!< A pointer to the asset instance or a handle to the resource if it fits within a pointer.
		ObjectAllocatorI *	storage;	//!< A pointer to the memory heap where the above object 'lives'.
	};

	/// maps pairs (id,type) to asset instance pointers
	typedef THashMap< AssetKey, ResourceTableEntry >	ResourceMapType;

	struct ResourceManagerData
	{
		ProxyAllocator		proxyAllocator;
		ResourceMapType		loaded_resources;		//!< loaded_resources resources for sharing asset instances

		AssetPackage::Head		packages;	//!< linked list of mounted file packages
		DefaultObjectHeap		defaultHeap;//!< default/global asset/resource heap

		// fallback asset loaders
		TbMemoryImageAssetLoader	memory_image_asset_loader;
		TbBinaryAssetLoader			binary_asset_loader;
		NwTextAssetLoader			text_based_asset_loader;

	public:
		ResourceManagerData()
			: proxyAllocator( "resource mgr", MemoryHeaps::resources() )
			, loaded_resources( proxyAllocator )
			, binary_asset_loader( NwResource::metaClass(), MemoryHeaps::resources() )
			, memory_image_asset_loader( NwResource::metaClass(), MemoryHeaps::resources() )
			, text_based_asset_loader( NwResource::metaClass(), MemoryHeaps::resources() )
		{
		}
	};
	static TPtr< ResourceManagerData >	me;

	ERet Initialize()
	{
		DEVOUT("Initializing Resource system...");

		me.ConstructInPlace();

		mxDO(me->loaded_resources.resize(1024*4));
		me->packages = NULL;

		return ALL_OK;
	}

	void Shutdown()
	{
		DEVOUT("Shutting down Resource system...");

		const ResourceMapType::PairsArray& loaded_resources = me->loaded_resources.GetPairs();
		if( !loaded_resources.IsEmpty() )
		{
			DEVOUT("%u resources are still loaded:", loaded_resources.num());
			for( UINT iResource = 0; iResource < loaded_resources.num(); iResource++ ) {
				const ResourceMapType::Pair& pair = loaded_resources[ iResource ];
				const TbMetaClass* type = TypeRegistry::FindClassByGuid( pair.key.type );
				DEVOUT("\t'%s' of type '%s'", AssetId_ToChars(pair.key.id), type->GetTypeName());
			}
		}

		mxASSERT2( me->packages == NULL, "Did you forget to unmount a resource package?" ); 

		me.Destruct();
	}

	TbAssetLoaderI* binaryAssetLoader()
	{
		return &me->binary_asset_loader;
	}

	TbAssetLoaderI* memoryImageAssetLoader()
	{
		return &me->memory_image_asset_loader;
	}

	TbAssetLoaderI* GetTextAssetLoader()
	{
		return &me->text_based_asset_loader;
	}

	static TbAssetLoaderI* defaultAssetLoader()
	{
		// as the most efficient
		return memoryImageAssetLoader();
	}

	void unloadAndDestroyAll()
	{
mxTEMP
		//// HACK: destroy clumps last - they act as memory heaps
		////U32 	temp[32];
		////DynamicArray< U32 >	clumps( MemoryHeaps::temporary() );
		////clumps.initializeWithExternalStorageAndCount( temp, mxCOUNT_OF(temp) );

		//struct ClumpInfo {
		//	AssetID			uid;
		//	NwClump *			clump;
		//	ObjectAllocatorI *	storage;
		//};

		//ClumpInfo 					temp[32];
		//DynamicArray< ClumpInfo >	clumps( MemoryHeaps::temporary() );
		//clumps.initializeWithExternalStorageAndCount( temp, mxCOUNT_OF(temp) );

		//ResourceMapType::PairsArray & loaded_resources = me->loaded_resources.GetPairs();

		//for( UINT iResource = 0; iResource < loaded_resources.num(); iResource++ )
		//{
		//	ResourceMapType::Pair & pair = loaded_resources[ iResource ];
		//	ResourceTableEntry & entry = pair.value;
		//	//mxASSERT(entry.refCount >= 0);
		//	const TbMetaClass* type = TypeRegistry::FindClassByGuid( pair.key.type );
		//	mxASSERT(type);
		//	if( type )
		//	{
		//		if( type != &NwClump::metaClass() )
		//		{
		//			__Do_Unload_And_Destroy( entry.resource, pair.key.id, *type, *entry.storage );
		//		}
		//		else
		//		{
		//			//const ClumpInfo clumpInfo = {
		//			//	pair.key.id,
		//			//	entry.resource,
		//			//	entry.storage
		//			//};
		//			//clumps.add( clumpInfo );
		//			clumps.add( iResource );
		//		}
		//	}
		//}

		//for( UINT iClump = 0; iClump < clumps.num(); iClump++ )
		//{
		//	ResourceMapType::Pair & pair = loaded_resources[ clumps[ iClump ] ];
		//	ResourceTableEntry & entry = pair.value;
		//	const TbMetaClass& type = NwClump::metaClass();
		//	__Do_Unload_And_Destroy( entry.resource, pair.key.id, type, *entry.storage );
		//}

		//me->loaded_resources.RemoveAll();
	}

	ERet MountPackage( AssetPackage* package )
	{
		chkRET_X_IF_NIL(package, ERR_NULL_POINTER_PASSED);
		chkRET_X_IF_NOT(!package->FindSelfInList( me->packages ), ERR_DUPLICATE_OBJECT);
		//DBGOUT("Asset Manager: Mounting package.\n");
		package->PrependSelfToList( &me->packages );
		return ALL_OK;
	}

	void UnmountPackage( AssetPackage* package )
	{
		chkRET_IF_NIL(package);
		package->RemoveSelfFromList( &me->packages );
	}

	bool IsPackageMounted( AssetPackage* package )
	{
		return package->FindSelfInList( me->packages );
	}

	ERet OpenFile(
		const AssetKey& _id,
		AssetReader *stream_,
		const U32 _subresource
	)
	{
		AssetPackage* current = me->packages;
		while(PtrToBool( current ))
		{
			if(mxSUCCEDED(current->Open( _id, stream_, _subresource )))
			{
				return ALL_OK;
			}
			current = current->_next;
		}
		DBGOUT("Failed to locate asset '%s' of type '%s'.\n",
			AssetId_ToChars(_id.id), AssetType_To_Chars(_id.type));
		return ERR_FILE_OR_PATH_NOT_FOUND;
	}

	static ResourceTableEntry* FindEntry( const AssetKey& key )
	{
		return me->loaded_resources.FindValue( key );
	}

	NwResource* find( const AssetKey& key )
	{
		ResourceTableEntry* existing = FindEntry( key );
		return existing ? existing->resource : NULL;
	}

	ERet insert(
		const AssetKey& key
		, NwResource* o
		, ObjectAllocatorI* storage /*= nil*/
		)
	{
		ResourceTableEntry* existing = FindEntry( key );
		mxENSURE(nil == existing, ERR_DUPLICATE_OBJECT, "");

		ResourceTableEntry	new_entry;
		new_entry.resource = o;
		new_entry.storage = storage;
		me->loaded_resources.Insert( key, new_entry );

		return ALL_OK;
	}

	ERet replaceExisting(
		const AssetKey& key
		, NwResource* o
		, ObjectAllocatorI* storage /*= nil*/
		)
	{
		ResourceTableEntry* existing = FindEntry( key );
		mxENSURE(nil != existing, ERR_OBJECT_NOT_FOUND, "");
		existing->resource = o;
		existing->storage = storage;
		return ALL_OK;
	}

	ERet remove( const AssetKey& key )
	{
		me->loaded_resources.Remove( key );
		return ALL_OK;
	}

	void unload(
		NwResource* o
		, const AssetKey& key
		)
	{
		UNDONE;
	}

	//ERet Unload( Resource* _o )
	//{
	//	ResourceMapType::PairsArray & loaded_resources = me->loaded_resources.GetPairs();
	//	for( UINT iResource = 0; iResource < loaded_resources.num(); iResource++ )
	//	{
	//		ResourceMapType::Pair & pair = loaded_resources[ iResource ];
	//		ResourceTableEntry & entry = pair.value;
	//		//mxASSERT(entry.refCount >= 0);
	//		const TbMetaClass* type = TypeRegistry::FindClassByGuid( pair.key.type );
	//		mxASSERT(type);
	//		if( entry.resource == _o )
	//		{
	//			__Do_Unload_And_Destroy( entry.resource, pair.key.id, *type, *entry.storage );
	//			me->loaded_resources.Remove( pair.key );
	//			return ALL_OK;
	//		}
	//	}
	//	return ERR_OBJECT_NOT_FOUND;
	//}

	//DBGOUT("Register asset '%s'", AssetId_ToChars(key.id));

}//namespace Resources

/*
-----------------------------------------------------------------------------
	AssetImport
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( AssetImport );
mxBEGIN_REFLECTION( AssetImport )
	//mxMEMBER_FIELD( resource ),
mxEND_REFLECTION

AssetImport::AssetImport()
{
	//resource = NULL;
}
bool AssetImport::CheckIsValid() const
{
	chkRET_FALSE_IF_NOT(AssetKey::IsOk());
	//chkRET_FALSE_IF_NIL(resource);
	return true;
}

/*
-----------------------------------------------------------------------------
	AssetExport
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(AssetExport);
mxBEGIN_REFLECTION(AssetExport)
	mxMEMBER_FIELD(o),
mxEND_REFLECTION

AssetExport::AssetExport()
{
	o = NULL;
}

bool AssetExport::CheckIsValid() const
{
	chkRET_FALSE_IF_NOT(AssetKey::IsOk());
	chkRET_FALSE_IF_NIL(o);
	return true;
}



namespace Resources
{
	TbAssetLoaderI* getResourceLoaderForClass(
		const TbMetaClass& type
		, TbAssetLoaderI* fallback_loader = nil
		)
	{
		if( !type.loader )
		{
			ptWARN( "No resource loader for class '%s'!", type.GetTypeName() );
			if( fallback_loader ) {
				ptWARN( "Using default loader for class '%s'...", type.GetTypeName() );
			}
		}

		TbAssetLoaderI* loader = type.loader
			? type.loader
			: fallback_loader;

		return loader;
	}


	ERet Load(
		NwResource *&resource_
		, const AssetID& _id
		, const TbMetaClass& _type
		, ObjectAllocatorI* _storage /*= nil*/
		, const LoadFlagsT flags
		, NwResource* fallback_instance /*= nil*/
		, TbAssetLoaderI* override_loader /*= nil*/
		)
	{
		mxASSERT(AssetId_IsValid( _id ));

#if MX_DEBUG
		//mxASSERT2(_type.loader,
		//	"No resource loader for class '%s'", _type.GetTypeName());

		if( nil == _type.loader )
		{
			ptWARN( "No resource asset_loader for class '%s'! Using default asset_loader...", _type.GetTypeName() );
		}
#endif

		TbAssetLoaderI* asset_loader = override_loader;
		if( !asset_loader )
		{
			asset_loader = getResourceLoaderForClass(
				_type
				, defaultAssetLoader()
				);
		}

		//
		resource_ = fallback_instance;

		//
		AssetKey key;
		key.id = _id;
		key.type = _type.GetTypeGUID();

		Resources::ResourceTableEntry* existing = Resources::FindEntry( key );
		if( existing ) {
			resource_ = existing->resource;
			return ALL_OK;
		}

		NwResource *	new_resource = nil;

		// 1. Create an asset instance.
		if( !_storage ) {
			_storage = asset_loader->preferredMemoryHeap();
		}
		if( !_storage ) {
			_storage = &me->defaultHeap;
		}

		//
		const TbAssetLoadContext	resource_context(
			key,
			_type,
			flags,
			*_storage
			);

		DEVOUT("Loading '%s' of type '%s' in heap 0x%p..."
			, AssetId_ToChars(_id), _type.GetTypeName(), _storage
			);

		mxDO(asset_loader->create( &new_resource, resource_context ));
		mxASSERT_PTR(new_resource);

		// 2. Load the asset instance: allocate load-and-stay resident (LSR) data.
		mxTRY(asset_loader->load( new_resource, resource_context ));

		//
		ResourceTableEntry new_entry;
		new_entry.resource = new_resource;
		new_entry.storage = _storage;

		//
		me->loaded_resources.Insert( key, new_entry );

		//
		resource_ = new_resource;

		return ALL_OK;
	}

	ERet Unload(
		const AssetID& _id,
		const TbMetaClass& _type
	)
	{
		AssetKey key;
		key.id = _id;
		key.type = _type.GetTypeGUID();

		Resources::ResourceTableEntry* existing = Resources::FindEntry( key );
		if( existing )
		{
			//if( AtomicDecrement( &existing->refCount ) <= 1 )
			{
				__Do_Unload_And_Destroy( existing->resource, _id, _type, *existing->storage );
				me->loaded_resources.Remove( key );
			}
			return ALL_OK;
		}
		return ERR_OBJECT_NOT_FOUND;
	}

	// Attempts to reload the given asset instance.
	static ERet ReloadAssetInstance( const AssetKey& key, ResourceTableEntry& entry )
	{
#if MX_DEVELOPER
		// The resource has already been created, we only need to re-initialize it.
		NwResource * resource = entry.resource;
		mxASSERT_PTR(resource);

		// find the C++ class of the resource.
		const TbMetaClass* object_class = TypeRegistry::FindClassByGuid( key.type );
		mxENSURE(object_class, ERR_OBJECT_NOT_FOUND, "No class with id=0x%X", key.type);
//		mxENSURE(object_class->asset_loader, ERR_INVALID_PARAMETER, "Class '%s' is not an asset", object_class->GetTypeName());

		LogStream(LL_Info) << GetCurrentTimeString<String32>(':')
			<< ": Reloading '" << AssetId_ToChars(key.id) << "' of type " << object_class->GetTypeName();

		// Get the resource 'manager' (actually, a bunch of callbacks).
		TbAssetLoaderI* resourceLoader = getResourceLoaderForClass(
			*object_class
			, defaultAssetLoader()
			);

		// Unload the resource but don't destroy it (and release old shaders/textures/etc)
		mxDO(resourceLoader->reload( resource,
									TbAssetLoadContext(
										key,
										*object_class,
										LoadFlagsT(0),
										*entry.storage
									)
		));

#endif	// #if MX_DEVELOPER

		return ALL_OK;
	}

	ERet Reload( const AssetKey& key )
	{
		ResourceTableEntry* existing = me->loaded_resources.FindValue( key );
		if( existing ) {
			mxDO(ReloadAssetInstance( key, *existing ));
		}
		return ALL_OK;
	}



#if MX_DEVELOPER

	ERet DumpUsedAssetsList( const char* dest_folder )
	{
		const ResourceMapType::PairsArray& loaded_resources_pairs = me->loaded_resources.GetPairs();

		DBGOUT("Dumping used assets list (%d) into folder: '%s'...",
			loaded_resources_pairs.num(),
			dest_folder
			);

		//
		UsedAssetsList	used_assets_list;

		mxDO(used_assets_list.used_assets.setNum(
			loaded_resources_pairs.num()
			));

		//
		nwFOR_EACH_INDEXED( const ResourceMapType::Pair& pair, loaded_resources_pairs, i )
		{
			AssetKey &dst_asset_key = used_assets_list.used_assets._data[i];
			dst_asset_key = pair.key;
		}

		//
		FilePathStringT	used_assets_manifest_filepath;
		Str::ComposeFilePath(used_assets_manifest_filepath, dest_folder, "used_assets.son");

		mxDO(SON::SaveToFile(
			used_assets_list,
			used_assets_manifest_filepath.c_str()
			));

		return ALL_OK;
	}

	ERet MergeWithPreviousUsedAssetsList( const char* dest_folder )
	{
		UNDONE;
		return ALL_OK;
	}

#endif // MX_DEVELOPER


}//namespace Resources




/*
-----------------------------------------------------------------------------
	UsedAssetsList
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(UsedAssetsList);
mxBEGIN_REFLECTION(UsedAssetsList)
	mxMEMBER_FIELD(used_assets),
mxEND_REFLECTION;


namespace NwGlobals
{
	bool	canLoadAssets = true;

}//namespace NwGlobals

#if MX_DEVELOPER

namespace Resources
{
	ERet CopyOnlyUsedAssets(
		const CopyUsedAssetsParams& params
		, AllocatorI& scratchpad
		)
	{
		UsedAssetsList	used_assets_list;
		mxDO(SON::LoadFromFile(
			params.used_assets_manifest_file,
			used_assets_list,
			scratchpad
			));

		//
		nwFOR_EACH( const AssetKey& used_asset_key, used_assets_list.used_assets )
		{
			//
			const TbMetaClass* metaclass = TypeRegistry::FindClassByGuid( used_asset_key.type );
			mxENSURE(
				metaclass,
				ERR_OBJECT_NOT_FOUND,
				"%s", used_asset_key.id.c_str()
				);

			//
			FilePathStringT	src_file_path;
			Str::CopyS(src_file_path, params.src_assets_folder);
			Str::AppendS(src_file_path, metaclass->GetTypeName());
			Str::AppendSlash(src_file_path);
			Str::AppendS(src_file_path, used_asset_key.id.c_str());

			//
			FilePathStringT	dst_folder;
			Str::CopyS(dst_folder, params.dst_assets_folder);
			Str::AppendS(dst_folder, metaclass->GetTypeName());
			Str::AppendSlash(dst_folder);
			//
			bool dst_dir_is_ok = OS::IO::MakeDirectoryIfDoesntExist(dst_folder.c_str());
			mxASSERT(dst_dir_is_ok);

			//
			FilePathStringT	dst_file_path;
			Str::Copy(dst_file_path, dst_folder);
			Str::AppendS(dst_file_path, used_asset_key.id.c_str());

			//
			bool res = ::CopyFileA(
				src_file_path.c_str()//LPCSTR lpExistingFileName
				,dst_file_path.c_str()//LPCSTR lpNewFileName
				,false //false == overwrite// BOOL bFailIfExists
				);
			mxASSERT(res);

			//
			{
				FilePathStringT	src_file_path0;
				Str::Copy(src_file_path0, src_file_path);
				Str::AppendS(src_file_path0, ".0");

				FilePathStringT	dst_file_path0;
				Str::Copy(dst_file_path0, dst_file_path);
				Str::AppendS(dst_file_path0, ".0");

				//
				bool res = ::CopyFileA(
					src_file_path0.c_str()//LPCSTR lpExistingFileName
					,dst_file_path0.c_str()//LPCSTR lpNewFileName
					,false //false == overwrite// BOOL bFailIfExists
					);
				if (res)
				{
					DBGOUT("!");
				}
			}//stream .0
		}//for each used asset

		return ALL_OK;
	}

}//namespace Resources

#endif // MX_DEVELOPER
