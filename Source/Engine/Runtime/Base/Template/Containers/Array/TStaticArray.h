/*
=============================================================================
	File:	TStaticArray.h
	Desc:	C-style array
	ToDo:	Stop reinventing the wheel.
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/ArrayDescriptor.h>

///
///	TStaticArray - static, fixed-size array (the size is known at compile time).
///
///	It's recommended to use this class instead of bare C-style arrays.
///
template< typename TYPE, const UINT SIZE >
class TStaticArray
	: public TArrayMixin< TYPE, TStaticArray<TYPE,SIZE> >
{
	TYPE	_data[ SIZE ];

public:
	typedef TStaticArray
	<
		TYPE,
		SIZE
	>
	THIS_TYPE;

	typedef
	TYPE
	ITEM_TYPE;

public:
	inline TStaticArray()
	{}

	inline explicit TStaticArray(EInitZero)
	{
		mxZERO_OUT_ARRAY( _data );
	}

	inline ~TStaticArray()
	{}

	inline UINT num() const
	{
		return SIZE;
	}
	inline UINT capacity() const
	{
		return SIZE;
	}

	inline void setNum( UINT newNum )
	{
		mxASSERT(newNum <= SIZE);
	}
	inline void empty()
	{
	}
	inline void clear()
	{
	}

	THIS_TYPE& operator = ( const THIS_TYPE& other )
	{
		return this->Copy( other );
	}

	THIS_TYPE& Copy( const TStaticArray& other )
	{
		for( UINT i = 0; i < SIZE; i++ ) {
			_data[i] = other[i];
		}
		return *this;
	}

	inline TYPE* raw()
	{ return _data; }

	inline const TYPE* raw() const
	{ return _data; }

public:	// Reflection.

	typedef	THIS_TYPE	ARRAY_TYPE;

	class ArrayDescriptor: public MetaArray
	{
	public:
		inline ArrayDescriptor( const char* typeName )
			: MetaArray( typeName, TbTypeSizeInfo::For_Type<ARRAY_TYPE>(), T_DeduceTypeInfo<ITEM_TYPE>(), sizeof(ITEM_TYPE) )
		{}

		//=-- MetaArray
		virtual bool IsDynamic() const override
		{
			return false;
		}
		virtual void* Get_Array_Pointer_Address( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return c_cast(void*) &theArray->_data;
		}
		virtual UINT Generic_Get_Count( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return theArray->num();
		}
		virtual ERet Generic_Set_Count( void* pArrayObject, UINT newNum ) const override
		{
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			theArray->setNum( newNum );
			return ALL_OK;
		}
		virtual UINT Generic_Get_Capacity( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return theArray->capacity();
		}
		virtual ERet Generic_Set_Capacity( void* pArrayObject, UINT newNum ) const override
		{
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			mxASSERT(newNum <= theArray->capacity());
			return ALL_OK;
		}
		virtual void* Generic_Get_Data( void* pArrayObject ) const override
		{
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual const void* Generic_Get_Data( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual void SetDontFreeMemory( void* pArrayObject ) const override
		{
			mxUNUSED(pArrayObject);
		}
	};

public:	// Serialization.

	friend AWriter& operator << ( AWriter& file, const THIS_TYPE &o )
	{
		file.SerializeArray( o._data, SIZE );
		return file;
	}
	friend AReader& operator >> ( AReader& file, THIS_TYPE &o )
	{
		file.SerializeArray( o._data, SIZE );
		return file;
	}
	friend mxArchive& operator && ( mxArchive & archive, THIS_TYPE & o )
	{
		TSerializeArray( archive, o._data, SIZE );
		return archive;
	}

public_internal:

	/// For serialization, we want to initialize the vtables
	/// in classes post data load, and NOT call the default constructor
	/// for the arrays (as the data has already been set).
	inline explicit TStaticArray( _FinishedLoadingFlag )
	{
	}

	NO_COMPARES(THIS_TYPE);
};

//
//	TStaticArray_InitZeroed
//
template< typename TYPE, const size_t SIZE >
class TStaticArray_InitZeroed: public TStaticArray< TYPE, SIZE >
{
public:
	TStaticArray_InitZeroed()
		: TStaticArray(_InitZero)
	{}
};

template< typename TYPE, const UINT SIZE >
struct TypeDeducer< TStaticArray< TYPE, SIZE > >
{
	static inline const TbMetaType& GetType()
	{
		static TStaticArray< TYPE, SIZE >::ArrayDescriptor staticTypeInfo(mxEXTRACT_TYPE_NAME(TStaticArray));
		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};

// specialization for embedded one-dimensional static arrays of arbitrary values
template< typename TYPE, UINT32 SIZE >
inline const TbMetaType& T_DeduceTypeInfo( const TYPE (&) [SIZE] )
{
	static TStaticArray< TYPE, SIZE >::ArrayDescriptor staticTypeInfo(mxEXTRACT_TYPE_NAME(TStaticArray));
	return staticTypeInfo;
}

// ... could be extended to handle 2D arrays [][] and so on...

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
