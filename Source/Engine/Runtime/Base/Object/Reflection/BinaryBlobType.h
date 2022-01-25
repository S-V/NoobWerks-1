/*
=============================================================================
	File:	BinaryBlobType.h
	Desc:	memory block which can be serialized
			using a custom buffer layout
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>


typedef TBuffer< BYTE >		BinaryBlob;


/*
-------------------------------------------------------------------------
	mxBlobType

	memory buffer with a predefined layout
-------------------------------------------------------------------------
*/
struct mxBlobType: public TbMetaType
{
public:
	inline mxBlobType( const char* typeName )
		: TbMetaType( ETypeKind::Type_Blob, typeName, TbTypeSizeInfo::For_Type< BinaryBlob >() )
	{}

	virtual const TbMetaClass& GetBufferLayout( void* blobObject ) const = 0;
	virtual void* GetBufferPointer( void* blobObject ) const = 0;
};

#if 0
/*
-------------------------------------------------------------------------
	RawBlob
-------------------------------------------------------------------------
*/
class RawBlob
{
	// the pointer to the allocated memory;
	// the highest bit is zero if the memory can safely be freed
	TypedPtr2< void >	m_data;

public:
	RawBlob()
	{
		m_data.Set( nil );
	}
	~RawBlob()
	{
		//
	}

	inline void* ToVoidPtr()
	{
		return m_data.GetPtr();
	}
	inline bool ownsMemory() const
	{
		return m_data.GetTag() != 0;
	}
	inline void clear()
	{
		void* memory = this->ToVoidPtr();
		if( memory != nil ) {
			mxFREE( memory );
		}
		m_data.Set( nil );
	}
};
#endif

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
