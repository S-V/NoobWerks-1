/*
=============================================================================
	File:	TBuffer.h
	Desc:	Dynamic (variable sized) templated array
			that occupies relatively little space.
	Note:	number of allocated elements is always equal to the array size
			(which leads to excessive memory fragmentation).
			This array is best used with an in-place loading system.
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/ArraysCommon.h>
#include <Base/Object/Reflection/ArrayDescriptor.h>

//
//	TBuffer< TYPE >
//
template< typename TYPE >
struct TBuffer: public TArrayMixin< TYPE, TBuffer< TYPE > >
{
	/// The pointer to the allocated memory.
	TYPE *	_data;

	/// The number of valid, 'live' elements (== allocated entries),
	/// the highest bit tells if the array owns memory.
	U32		_count_and_flag;

	// 8/12

public:
	typedef TYPE ITEM_TYPE;
	typedef TBuffer< TYPE > THIS_TYPE;

	enum { ITEMS_COUNT_SHIFT = (sizeof(U32) * BITS_IN_BYTE) - 1 };

	enum We_Store_Capacity_And_Bit_Flags_In_One_Int
	{
		DONT_FREE_MEMORY_MASK = U32(1 << ITEMS_COUNT_SHIFT),	//!< Indicates that the storage is not the array's to delete
		GET_NUM_OF_ITEMS_MASK = U32(~DONT_FREE_MEMORY_MASK),
	};

public:
	inline TBuffer()
	{
		_data = nil;
		_count_and_flag = 0;
	}
	inline explicit TBuffer( const THIS_TYPE& other )
	{
		_data = nil;
		_count_and_flag = 0;
		this->Copy( other );
	}
	inline TBuffer( TYPE* externalStorage, UINT maxCount )
	{
		_data = nil;
		_count_and_flag = 0;
		this->initializeWithExternalStorageAndCount( externalStorage, maxCount );
	}

	// Deallocates array memory.
	inline ~TBuffer()
	{
		this->clear();
	}

	/// The memory is assumed to be constructed.
	void initializeWithExternalStorageAndCount( TYPE* externalMemory, UINT count )
	{
		mxASSERT(_data == nil);
		_data = externalMemory;
		_count_and_flag = count;
		_count_and_flag |= DONT_FREE_MEMORY_MASK;
	}

	inline bool ownsMemory() const
	{
		return (_count_and_flag & DONT_FREE_MEMORY_MASK) == 0;
	}

	// Convenience function to get the number of elements in this array.
	// Returns the size (the number of elements in the array).
	inline UINT num() const {
		return (_count_and_flag & GET_NUM_OF_ITEMS_MASK);
	}
	inline TYPE * raw() {
		return _data;
	}
	inline const TYPE* raw() const {
		return _data;
	}

	// Returns the current capacity of this array.
	inline UINT capacity() const {
		return this->num();
	}

	// Convenience function to empty the array. Doesn't release allocated memory.
	// Doesn't call objects' destructors.
	inline void empty()
	{
		_count_and_flag = (_count_and_flag & DONT_FREE_MEMORY_MASK);	// set to zero all bits below the highest one
	}

	// Convenience function to empty the array. Doesn't release allocated memory.
	// Invokes objects' destructors.
	inline void destroyAndEmpty()
	{
		TDestructN_IfNonPOD( _data, this->num() );
		this->empty();
	}

	// One-time init.
	// Sets the number of elements.
	inline ERet setNum( UINT numElements )
	{
		if( numElements ) {
			return this->resize( numElements );
		} else {
			this->clear();
		}
		return ALL_OK;
	}

	// Releases allocated memory (calling destructors of elements) and empties the array.
	inline void clear()
	{
		if( _data != nil )
		{
			TDestructN_IfNonPOD( _data, this->num() );
			this->_releaseMemory();
			_data = nil;
		}
		_count_and_flag = 0;
	}


	// Adds an element to the end. NOTE: can cause excessive memory fragmentation!
	TYPE & add( const TYPE& newOne )
	{
		const UINT oldNum = this->num();
		this->setNum( oldNum + 1 );
		_data[ oldNum ] = newOne;
		return _data[ oldNum ];
	}
	// Increments the size by 1 and returns a reference to the first element created.
	// NOTE: can cause excessive memory fragmentation!
	TYPE & add()
	{
		const UINT oldNum = this->num();
		this->setNum( oldNum + 1 );
		return _data[ oldNum ];
	}

	// Deep copy. Slow!
	template< class OTHER_ARRAY >
	THIS_TYPE& Copy( const OTHER_ARRAY& other )
	{
		const UINT newNum = other.num();
		this->setNum( newNum );
		if( newNum ) {
			TCopyConstructArray( _data, other.raw(), newNum );
		}
		return *this;
	}

	THIS_TYPE& operator = ( const THIS_TYPE& other )
	{
		return this->Copy( other );
	}

	// Returns the amount of reserved memory in bytes (memory allocated for storing the elements).
	inline size_t allocatedMemorySize() const
	{
		return this->capacity() * sizeof(TYPE);
	}

	// Returns the total amount of occupied memory in bytes.
	inline size_t usedMemorySize() const
	{
		return this->allocatedMemorySize() + sizeof(*this);
	}

	inline void SetDontFreeMemory()
	{
		_count_and_flag |= DONT_FREE_MEMORY_MASK;
	}

public:	// Binary Serialization.

	friend AWriter& operator << ( AWriter& file, const THIS_TYPE& o )
	{
		const UINT32 number = o._count;
		file << number;

		file.SerializeArray( o._data, number );

		return file;
	}
	friend AReader& operator >> ( AReader& file, THIS_TYPE& o )
	{
		UINT32 number;
		file >> number;
		o.setNum( number );

		file.SerializeArray( o._data, number );

		return file;
	}

public:	// Reflection.

	class ArrayDescriptor: public MetaArray
	{
	public:
		inline ArrayDescriptor( const char* typeName )
			: MetaArray( typeName, TbTypeSizeInfo::For_Type<THIS_TYPE>(), T_DeduceTypeInfo<ITEM_TYPE>(), sizeof(ITEM_TYPE) )
		{}
		//=-- MetaArray
		virtual bool IsDynamic() const override
		{
			return true;
		}
		virtual void* Get_Array_Pointer_Address( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return c_cast(void*) &theArray->_data;
		}
		virtual UINT Generic_Get_Count( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->num();
		}
		virtual ERet Generic_Set_Count( void* pArrayObject, UINT newNum ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			mxDO(theArray->setNum( newNum ));
			return ALL_OK;
		}
		virtual UINT Generic_Get_Capacity( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->capacity();
		}
		virtual ERet Generic_Set_Capacity( void* pArrayObject, UINT newNum ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			mxDO(theArray->setNum( newNum ));
			return ALL_OK;
		}
		virtual void* Generic_Get_Data( void* pArrayObject ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual const void* Generic_Get_Data( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual void SetDontFreeMemory( void* pArrayObject ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			if(theArray->_data) {
				theArray->SetDontFreeMemory();
			}
		}
	};

public:	// Iterators.
	class Iterator
	{
		THIS_TYPE &	m_array;
		UINT		m_currentIndex;
	public:
		inline Iterator( THIS_TYPE& rArray )
			: m_array( rArray )
			, m_currentIndex( 0 )
		{}
		inline bool IsValid() const
		{
			return m_currentIndex < m_array.num();
		}
		inline ITEM_TYPE& Value() const
		{
			return m_array[ m_currentIndex ];
		}
		inline void MoveToNext()
		{
			++m_currentIndex;
		}
		inline void Skip( UINT count )
		{
			m_currentIndex += count;
		}
		inline void Reset()
		{
			m_currentIndex = 0;
		}
		inline const Iterator& operator ++ ()
		{
			++m_currentIndex;
			return *this;
		}
		inline operator bool () const
		{
			return this->IsValid();
		}
		inline ITEM_TYPE* operator -> () const
		{
			return &this->Value();
		}
		inline ITEM_TYPE& operator * () const
		{
			return this->Value();
		}
	};
	class ConstIterator
	{
		const THIS_TYPE &	m_array;
		UINT				m_currentIndex;
	public:
		inline ConstIterator( const THIS_TYPE& rArray )
			: m_array( rArray )
			, m_currentIndex( 0 )
		{}
		inline bool IsValid() const
		{
			return m_currentIndex < m_array.num();
		}
		inline const ITEM_TYPE& Value() const
		{
			return m_array[ m_currentIndex ];
		}
		inline void MoveToNext()
		{
			++m_currentIndex;
		}
		inline void Skip( UINT count )
		{
			m_currentIndex += count;
		}
		inline void Reset()
		{
			m_currentIndex = 0;
		}
		inline const ConstIterator& operator ++ ()
		{
			++m_currentIndex;
			return *this;
		}
		inline operator bool () const
		{
			return this->IsValid();
		}
		inline const ITEM_TYPE* operator -> () const
		{
			return &this->Value();
		}
		inline const ITEM_TYPE& operator * () const
		{
			return this->Value();
		}
	};

private:
	void _releaseMemory()
	{
		if( this->ownsMemory() )
		{
			mxFREE( _data, this->num() * sizeof(TYPE) );
			_data = nil;
		}		
	}

	ERet resize( UINT32 newCount )
	{
		mxASSERT( newCount > 0 );

		const UINT32 oldCount = this->num();
		TYPE * const oldArray = this->raw();		

		if( newCount <= oldCount ) {
			return ALL_OK;
		}

		// Allocate a new memory buffer
		TYPE * newArray = static_cast< TYPE* >( mxALLOC(
			newCount * sizeof(TYPE), mxALIGNOF(TYPE)
		));
		if( !newArray ) {
			DBGOUT("Failed to alloc %u items", newCount);
			return ERR_OUT_OF_MEMORY;
		}

		if( PtrToBool( oldArray ) )
		{
			// copy-construct the new elements
			const UINT32 overlap = smallest( oldCount, newCount );
			TCopyConstructArray( newArray, oldArray, overlap );
			// destroy the old contents
			TDestructN_IfNonPOD( oldArray, oldCount );
			// deallocate old memory buffer
			this->_releaseMemory();
		}

		// call default constructors for the rest
		const UINT32 numNewItems = (newCount > oldCount) ? (newCount - oldCount) : 0;
		TConstructN_IfNonPOD( newArray + oldCount, numNewItems );

		_data = newArray;
		_count_and_flag = newCount;

		return ALL_OK;
	}

public:	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef TYPE* iterator;
	typedef const TYPE* const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;

private:
	NO_COMPARES(THIS_TYPE);
};

//-----------------------------------------------------------------------
// Reflection.
//
template< typename TYPE >
struct TypeDeducer< TBuffer< TYPE > >
{
	static inline const TbMetaType& GetType()
	{
		static TBuffer< TYPE >::ArrayDescriptor staticTypeInfo(mxEXTRACT_TYPE_NAME(TBuffer));
		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};

mxREMOVE_THIS
template< class TYPE >	// where TYPE has member 'name' of type 'String'
TYPE* FindByName( TBuffer< TYPE >& items, const char* name )
{
	for( UINT i = 0; i < items.num(); i++ )
	{
		TYPE& item = items[ i ];
		if( Str::EqualS( item.name, name ) ) {
			return &item;
		}
	}
	return nil;
}
template< class TYPE >	// where TYPE has member 'name' of type 'String'
const TYPE* FindByName( const TBuffer< TYPE >& items, const char* name )
{
	for( UINT i = 0; i < items.num(); i++ )
	{
		const TYPE& item = items[ i ];
		if( Str::EqualS( item.name, name ) ) {
			return &item;
		}
	}
	return nil;
}

template< class TYPE >	// where TYPE has member 'name' of type 'String'
int FindIndexByName( TBuffer< TYPE >& items, const char* name )
{
	for( int i = 0; i < items.num(); i++ )
	{
		const TYPE& item = items[ i ];
		if( Str::EqualS( item.name, name ) ) {
			return i;
		}
	}
	return -1;
}

mxTEMP
template< class TYPE, class PREDICATE >	// where TYPE has member 'name' of type 'String'
TYPE* LinearSearch( TBuffer< TYPE >& _array, const PREDICATE& _predicate )
{
	//return LinearSearch< TYPE, PREDICATE >( _array.begin(), _array.end(), _predicate );
	//return LinearSearch< TYPE, PREDICATE >( _array, _predicate );
	UNDONE;
	return nil;
}

typedef TBuffer< BYTE > ByteBuffer32;
