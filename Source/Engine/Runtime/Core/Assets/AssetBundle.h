// Asset Bundles are used for merging asset files into a large files for faster loading.
// They serve to "package" all the files needed by the game (maps, interface graphics, models, textures, music and all other game data).
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>	// TInplaceArray<T,N>

#include <Core/Assets/AssetID.h>
#include <Core/Assets/AssetManagement.h>


#define nwUSE_LOOSE_ASSETS	(1)


/*
-----------------------------------------------------------------------------
	TbAssetBundle

	optimized for fast loading and reading
-----------------------------------------------------------------------------
*/
class TbAssetBundle
	: public AssetPackage
	, public ReferenceCounted
{
public:
	typedef TInplaceArray< AssetID, 32 >	IDList;

	enum {
		FILE_BLOCK_ALIGNMENT = 16,
	};


#pragma pack (push,1)

	struct ChunkInfo: CStruct
	{
		U32			offset;	// always aligned by FILE_BLOCK_ALIGNMENT
		U32			size;
	public:
		mxDECLARE_CLASS( ChunkInfo, CStruct );
		mxDECLARE_REFLECTION;
	};
	ASSERT_SIZEOF(ChunkInfo, 8);

	struct AssetDataRef: CStruct
	{
		TypeGUID	type_id;
		U32			skip;	// the total size of data following this header
		ChunkInfo	files[AssetPackage::MAX_STREAMS];
	public:
		mxDECLARE_CLASS( AssetDataRef, CStruct );
		mxDECLARE_REFLECTION;
	};

#pragma pack (pop)


	//
	struct MemoryResidentData: CStruct, NonCopyable
	{
		TBuffer< AssetID >			sorted_asset_ids;	// sorted lexicographically in ascending order
		TBuffer< AssetDataRef >		asset_data_refs;
	public:
		mxDECLARE_CLASS( MemoryResidentData, CStruct );
		mxDECLARE_REFLECTION;
		MemoryResidentData() {}
		PREVENT_COPY( MemoryResidentData );
	};

private:

	TPtr< MemoryResidentData >	_mrd;

	FileReader	_file_stream;

	int		_num_readers;

public:
	mxDECLARE_CLASS( TbAssetBundle, CStruct );
	mxDECLARE_REFLECTION;

	TbAssetBundle();
	~TbAssetBundle();

	ERet mount( const char* filepath );
	void unmount();

	//@ AssetPackage

	virtual ERet Open(
		const AssetKey& key,
		AssetReader * stream,
		const U32 subresource = 0
	) override;

	virtual void Close(
		AssetReader * stream
	) override;

	virtual ERet read( AssetReader * reader, void *buffer, size_t size ) override;
	virtual size_t length( const AssetReader * reader ) const override;
	virtual size_t tell( const AssetReader * reader ) const override;

private:
	ERet _openStream(
		AssetReader * stream
		, const AssetID& asset_id
		, const U32 asset_data_index
		, const U32 subresource
		);

	void _closeStream(
		AssetReader * stream
		);

	PREVENT_COPY( TbAssetBundle );
};

/*
-----------------------------------------------------------------------------
	TbDevAssetBundle
	allows appending new assets at the end

	The structure is based on Volition's (Red Faction 1) VPP files
	http://www.redfactionwiki.com/wiki/VPP_File_Specification
-----------------------------------------------------------------------------
*/
class TbDevAssetBundle
	: public AssetPackage
	, public ReferenceCounted
{
public:
	enum {
		SIGNATURE = MCHAR4('B','N','D','L'),
		FILE_BLOCK_ALIGNMENT = 16,
	};
	
#pragma pack (push,1)
	/// declarations for loading/parsing renderer assets
	struct Header_d
	{
		U32	signature;	// four-character code
		U32 version;
		U32	num_entries;// number of AssetDataRef
		U32	padding;	// unused space that may be overwritten with zeros after fseek() and fwrite()
		//U32	filesize;	// the size of the whole file (including the header)
	};
	mxSTATIC_ASSERT(sizeof(Header_d) == 16);
#pragma pack (pop)

	//
	typedef THashMap< AssetID, TbAssetBundle::AssetDataRef >	AssetTableT;

public_internal:	// accessible by the Asset Pipeline

	Header_d	_header;

	AssetTableT	_assets;

	// reader in production, writer during development
	IOStreamFILE	_file_stream;

	//
	AssetID		_name;

public:
	mxDECLARE_CLASS( TbDevAssetBundle, AssetPackage );

	TbDevAssetBundle(
		AllocatorI & allocator
		, const AssetID& name
		);

	~TbDevAssetBundle();

	ERet mount( const char* filepath );
	void unmount();

	//@ AssetPackage

	virtual ERet Open(
		const AssetKey& key,
		AssetReader * stream,
		const U32 subresource = 0
	) override;

	virtual void Close(
		AssetReader * stream
	) override;

	virtual ERet read( AssetReader * reader, void *buffer, size_t size ) override;
	virtual size_t length( const AssetReader * reader ) const override;
	virtual size_t tell( const AssetReader * reader ) const override;

public:
	ERet initialize( U32 expected_asset_count );

	bool containsAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		) const;

	const char* dbgName() const { return _name.d.c_str(); }

	PREVENT_COPY( TbDevAssetBundle );
};

/*
-----------------------------------------------------------------------------
	LocalFolder
	can be used for reading asset files in development mode
-----------------------------------------------------------------------------
*/
class TbAssetBundle_LocalFolder
	: public AssetPackage
	, public ReferenceCounted
{
public:
	/// normalized directory name (ends with '/')
	FilePathStringT	_path;

	THashMap< AssetID, FilePathStringT >	_file_name_by_asset_id;

	/// used by asset pipeline only
	AssetID	_asset_bundle_id;

public:
	mxDECLARE_CLASS( TbAssetBundle_LocalFolder, AssetPackage );

	TbAssetBundle_LocalFolder(
		AllocatorI & allocator = MemoryHeaps::global()
		);
	~TbAssetBundle_LocalFolder();

	ERet Mount( const char* folder );
	void Unmount();

	virtual ERet Open(
		const AssetKey& key,
		AssetReader * stream,
		const U32 subresource = 0
	) override;

	virtual void Close(
		AssetReader * stream
	) override;

	virtual ERet read( AssetReader * reader, void *buffer, size_t size ) override;
	virtual size_t length( const AssetReader * reader ) const override;
	virtual size_t tell( const AssetReader * reader ) const override;

public:	// Asset Pipeline

	// a special ctor used only by the asset pipeline!
	TbAssetBundle_LocalFolder(
		AllocatorI & allocator
		, const AssetID& asset_bundle_id
		);

	ERet CreateFolderInFolder( const char* containing_folder );

	const char* GetFolder() const { return _path.c_str(); }

	const char* dbgName() const { return _path.c_str(); }
};


#if nwUSE_LOOSE_ASSETS

typedef TbAssetBundle_LocalFolder	NwAssetBundleT;

#else

typedef TbDevAssetBundle	NwAssetBundleT;

#endif
