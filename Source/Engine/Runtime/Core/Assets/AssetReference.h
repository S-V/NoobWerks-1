
/*
=============================================================================
	File:	AssetReference.h
	Desc:	An intrusive reference counting smart pointer
			for shared asset instances.
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/MetaAssetReference.h>
#include <Core/Assets/AssetManagement.h>


/// NOTE: must be synced with TResPtr<>!
struct NwAssetRef
{
	void *		ptr;
	AssetID		id;
};

///
template< class ASSET >	// where ASSET: NwSharedResource
struct TResPtr
{
	mutable ASSET *	_ptr;	// a raw pointer to the asset instance
	AssetID			_id;	// the id of the asset

public:
	mxFORCEINLINE TResPtr()
		: _ptr( nil )
	{
		mxSTATIC_ASSERT( sizeof(*this) == sizeof(NwAssetRef) );
	}

	mxFORCEINLINE TResPtr( const TResPtr& other )
		: _id( other._id ), _ptr( other._ptr )
	{}

	/// doesn't load the asset!
	mxFORCEINLINE TResPtr( const AssetID& id )
		: _id( id ), _ptr( nil )
	{}

	~TResPtr()
	{
		mxBUG("leak!");
		mxTODO("when we figure out how to deal with Clump destruction");
		//release();
	}

	mxFORCEINLINE bool IsValid() const	{ return _ptr != nil; }
	mxFORCEINLINE bool IsNil() const	{ return _ptr == nil; }
	
	mxFORCEINLINE ASSET* ptr() const
	{
		mxASSERT_PTR(_ptr);
		return _ptr;
	}

	///
	void release()
	{
		if( _ptr )
		{
			_ptr->Drop(
				_id
				, ASSET::metaClass()
				);

			_ptr = nil;
		}
	}

public:	// Overloaded operators.

	/// overload casting/conversion to asset pointer type
	mxFORCEINLINE operator ASSET* () const
	{
		mxASSERT_PTR(_ptr);
		return _ptr;
	}

	mxFORCEINLINE ASSET* operator -> () const
	{
		mxASSERT_PTR(_ptr);
		return _ptr;
	}

#if 0
	mxFORCEINLINE ASSET **		operator & ()			{ return &_ptr; }

	mxFORCEINLINE const ASSET &	operator * () const		{ return *_ptr; }
	mxFORCEINLINE ASSET &		operator * ()			{ return *_ptr; }

#endif

	TResPtr& operator = ( const TResPtr& other )
	{
		if( this == &other ) {
			return *this;
		}

		release();

		_id = other._id;

		_ptr = other._ptr;
		if( other._ptr ) {
			other._ptr->Grab();
		}

		return *this;
	}

	mxFORCEINLINE const AssetKey getAssetKey() const
	{
		return AssetKey(
			_id
			, ASSET::metaClass().GetTypeGUID()
			);
	}

public:
	ERet Load(
		ObjectAllocatorI * storage = nil
		, const LoadFlagsT load_flags = 0
		) const
	{
		mxASSERT(AssetId_IsValid(_id));
		if( !_ptr && NwGlobals::canLoadAssets )
		{
			mxTRY(Resources::Load(
				_ptr
				, _id
				, storage
				, load_flags
				));
			_ptr->Grab();
		}
		return ALL_OK;
	}

public:
	/// for writing in tools
	void _setId( const AssetID& id )
	{
		mxASSERT( nil == _ptr );
		_id = id;
	}
};

/// Reflection.
template< typename TYPE >
struct TypeDeducer< TResPtr< TYPE > >
{
	static inline const TbMetaType& GetType()
	{
		static MetaAssetReference static_type_info(
			mxEXTRACT_TYPE_NAME(TResPtr),
			TbTypeSizeInfo::For_Type< TYPE* >(),
			TYPE::metaClass()
		);
		return static_type_info;
	}

	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_AssetReference;
	}
};

///
template< class SHARED_RESOURCE_CLASS >
struct DeclareTypedefsForSharedResource: public NwSharedResource
{
	typedef TResPtr< SHARED_RESOURCE_CLASS >	PtrT;
};



namespace NwGlobals
{
	/// Asset Compiler should not load resources
	extern bool	canLoadAssets;

}//namespace NwGlobals
