/*
=============================================================================
	File:	Array.h
	Desc:	Dynamic (variable sized) templated array.
	The array is always stored in a contiguous chunk.
	The array can be resized.
	A size increase will cause more memory to be allocated,
	and may result in relocation of the array memory.
	A size decrease has no effect on the memory allocation.

	ToDo:	Stop reinventing the wheel.
			Study http://mxr.mozilla.org/mozilla-central/source/xpcom/glue/nsTArray.h
			https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md
	Idea:	store size, capacity and memory manager index
			(and other flags) in a single integer (bit mask);
			store size and capacity at the beginning of allocated memory;
			can store additional info in upper 4 bits
			of a (16-byte aligned) pointer to allocated memory.
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/ArraysCommon.h>
#include <Base/Object/Reflection/ArrayDescriptor.h>

class TbMetaType;
class TbMetaClass;

/*
-------------------------------------------------------------------------
	TArray< TYPE >

	- is a resizable array which doubles in size by default.
	Does not allocate memory until the first item is added.
-------------------------------------------------------------------------
*/
template
<
	typename TYPE	// The type of stored elements
>
class TArray
	: public TArrayMixin< TYPE, TArray< TYPE > >
{
public_internal:	// Calling member functions is slow in debug builds.
	/// The pointer to the allocated memory
	TYPE *	_data;
	/// The number of valid, 'live' elements, this should be inside the range [0..capacity)
	U32		_count;
	/// The number of allocated entries + highest bit is set if we cannot deallocate the memory
	U32		_capacity_and_flag;
	//12/16
public:
	typedef	TArray< TYPE >	THIS_TYPE;

	// 1 bit is used for indicating if the memory was externally allocated (and the array cannot delete it)
	enum { CAPACITY_BITS = (sizeof(U32) * BITS_IN_BYTE) - 1 };

	enum We_Store_Capacity_And_Bit_Flags_In_One_Int
	{
		DONT_FREE_MEMORY_MASK = U32(1 << CAPACITY_BITS),	//!< Indicates that the storage is not the array's to delete
		EXTRACT_CAPACITY_MASK = U32(~DONT_FREE_MEMORY_MASK),
	};

public:

	/// Creates a zero length array.
	inline TArray()
	{
		_data = nil;
		_count = 0;
		_capacity_and_flag = 0;
	}

	inline explicit TArray( const THIS_TYPE& other )
	{
		_data = nil;
		_count = 0;
		_capacity_and_flag = 0;
		this->Copy( other );
	}

	/// NOTE: the 'externalStorage' must be uninitialized!
	inline TArray( TYPE* externalStorage, U32 maxCount )
	{
		_data = nil;
		_count = 0;	// data is preallocated, but not constructed yet
		_capacity_and_flag = 0;
		this->initializeWithExternalStorageAndCount( externalStorage, maxCount );
	}

	/// Use it only if you know what you're doing.
	inline explicit TArray(ENoInit)
	{}

	/// Destructs array elements and deallocates array memory.
	inline ~TArray()
	{
		this->clear();
	}

	ERet initializeWithExternalStorageAndCount(
		TYPE* externalMemory, U32 maxCount
		, U32 initial_count = 0
	)
	{
		mxASSERT(_data == nil && _capacity_and_flag == 0);
		mxENSURE(externalMemory && maxCount, ERR_INVALID_PARAMETER, "");
		_data = externalMemory;
		_count = initial_count;
		_capacity_and_flag = maxCount | DONT_FREE_MEMORY_MASK;
		return ALL_OK;
	}

	// Returns the current capacity of this array.
	mxFORCEINLINE U32 capacity() const
	{
		U32 capacity = (_capacity_and_flag & EXTRACT_CAPACITY_MASK);
		return capacity;
	}

	/// Convenience function to get the number of elements in this array.
	/// Returns the size (the number of elements in the array).
	mxFORCEINLINE U32 num() const { return _count; }

	mxFORCEINLINE TYPE * raw() { return _data; }
	mxFORCEINLINE const TYPE* raw() const { return _data; }

	/// Convenience function to empty the array. Doesn't release allocated memory.
	inline void RemoveAll()
	{
		TDestructN_IfNonPOD( _data, _count );
		_count = 0;
	}

	/// Releases allocated memory (calling destructors of elements) and empties the array.
	void clear()
	{
		if(PtrToBool( _data ))
		{
			TDestructN_IfNonPOD( _data, _count );
			this->_releaseMemory();
			_data = nil;
		}
		_count = 0;
		_capacity_and_flag = 0;
	}

	/// Resizes the array to exactly the number of elements it contains or frees up memory if empty.
	void shrink()
	{
		// Condense the array.
		if( _count > 0 ) {
			this->resize( _count );
		} else {
			this->clear();
		}
	}

	/// See: http://www.codercorner.com/blog/?p=494
	void EmptyOrClear()
	{
		const U32 capacity = this->capacity();
		const U32 num = this->num();
		if( num > capacity/2 ) {
			this->empty();
		} else {
			this->clear();
		}
	}

	/// Adds an element to the end.
	inline ERet add( const TYPE& newOne )
	{
		mxDO(this->Reserve( _count + 1 ));
		new(&_data[ _count++ ]) TYPE( newOne );	// copy-construct
		return ALL_OK;
	}

	void AppendItem_NoResize( const TYPE& new_item )
	{
		mxASSERT(_count + 1 <= this->capacity());
		new(&_data[ _count++ ]) TYPE( new_item );
	}


	/// Increments the size and returns a reference to the first default-constructed element.
	inline TYPE & AddNew()
	{
		this->Reserve( _count + 1 );
		//NOTE: do not use TYPE(), because it zeroes out PODs, stealing CPU cycles
		return *new(&_data[ _count++ ]) TYPE;
	}
	inline ERet AddMany( const TYPE* items, U32 numItems )
	{
		const U32 oldNum = _count;
		const U32 newNum = oldNum + numItems;
		mxDO(this->setNum( newNum ));
		TCopyArray( _data + oldNum, items, numItems );
		return ALL_OK;
	}

	inline TYPE & AddUninitialized()
	{
		this->Reserve( _count + 1 );
		return _data[ _count++ ];
	}
	inline TYPE* AddManyUninitialized( U32 num_elements )
	{
		const U32 oldNum = _count;
		const U32 newNum = oldNum + num_elements;
		if(mxFAILED(this->Reserve( newNum ))) {return nil;}
		_count = newNum;
		return _data + oldNum;
	}
	inline ERet AddZeroed( U32 num_elements )
	{
		mxSTATIC_ASSERT(TypeTrait<TYPE>::IsPlainOldDataType);
		const U32 newNum = _count + num_elements;
		mxDO(this->Reserve( newNum ));
		MemZero( (BYTE*)_data + _count*sizeof(TYPE), num_elements*sizeof(TYPE) );
		_count = newNum;
		return ALL_OK;
	}

	// Slow!
	bool AddUnique( const TYPE& item )
	{
		const U32 num = _count;
		for( U32 i = 0; i < num; i++ )
		{
			if( _data[i] == item ) {
				return false;
			}
		}
		this->add( item );
		return true;
	}

	// Use it only if you know what you're doing.
	// This only works if 'capacity' is a power of two.
	inline TYPE& addFastUnsafe()
	{
		mxASSERT(IsPowerOfTwo( this->capacity() ));
		const U32 newIndex = (_count++) & (this->capacity()-1);//avoid checking for overflow
		return _data[ newIndex ];
	}
	inline void addFastUnsafe( const TYPE& newOne )
	{
		mxASSERT(IsPowerOfTwo( this->capacity() ));
		const U32 newIndex = (_count++) & (this->capacity()-1);//avoid checking for overflow
		_data[ newIndex ] = newOne;
	}

	// Slow!
	inline bool Remove( const TYPE& item )
	{
		return Arrays::RemoveElement( *this, item );
	}

	// Slow!
	inline void RemoveAt( U32 index, U32 count = 1 )
	{
		Arrays::RemoveAt( _data, _count, index, count );
	}

	// this method is faster (uses the 'swap trick')
	// Doesn't preserve the relative order of elements.
	inline void RemoveAt_Fast( U32 index )
	{
		Arrays::RemoveAt_Fast( _data, _count, index );
	}

	// Removes all occurrences of value in the array
	// and returns the number of entries removed.
	//
	U32 RemoveAll( const TYPE& theValue )
	{
		U32 numRemoved = 0;
		for( U32 i = 0; i < _count; ++i )
		{
			if( _data[i] == theValue ) {
				this->RemoveAt( i );
				numRemoved++;
			}
		}
		return numRemoved;
	}

	// removes the last element
	TYPE PopLastValue()
	{
		mxASSERT(_count > 0);
		return _data[ --_count ];
	}

	// Slow!
	// inserts a new element at the given index and keeps the relative order of elements.
	TYPE & InsertAt( U32 index )
	{
		mxASSERT( this->isValidIndex( index ) );
		const U32 oldNum = _count;
		const U32 newNum = oldNum + 1;
		TYPE* data = _data;
		this->Reserve( newNum );
		for ( U32 i = oldNum; i > index; --i )
		{
			data[i] = data[i-1];
		}
		_count = newNum;
		return data[ index ];
	}

	inline U32 RemoveContainedItem( const TYPE* o )
	{
		const U32 itemIndex = this->GetContainedItemIndex( o );
		chkRET_X_IF_NOT( this->isValidIndex( itemIndex ), INDEX_NONE );
		this->RemoveAt( itemIndex );
		return itemIndex;
	}

	// Ensures no reallocation occurs until at least size 'num_elements'.
	ERet Reserve( U32 num_elements )
	{
	//	mxASSERT( num_elements > 0 );//<- this helped me to catch errors
		const U32 old_capacity = this->capacity();
		// resize if necessary
		if( num_elements > this->capacity() )
		{
			const U32 new_capacity = Arrays::calculateNewCapacity( num_elements, old_capacity );
			mxDO(this->resize( new_capacity ));
		}
		return ALL_OK;
	}

	// Ensures that there's a space for at least the given number of elements.
	inline ERet ReserveMore( const U32 num_new_elements )
	{
		const U32 old_count = _count;
		const U32 new_count = old_count + num_new_elements;
		return this->Reserve( new_count );
	}

	// Sets the new number of elements. Resizes the array to this number if necessary.
	inline ERet setNum( U32 num_elements )
	{
		// resize to the exact size specified irregardless of granularity.
		if( num_elements > this->capacity() )
		{
			mxDO(this->resize( num_elements ));
		}

		// Call default constructors for new elements.
		if( num_elements > _count )
		{
			const U32 numNewItems = num_elements - _count;
			TConstructN_IfNonPOD( _data + _count, numNewItems );
		}

		_count = num_elements;

		return ALL_OK;
	}

	ERet setCountExactly( const U32 num_elements )
	{
		return setNum( num_elements );
	}

	inline bool ownsMemory() const
	{
		return (_capacity_and_flag & DONT_FREE_MEMORY_MASK) == 0;
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

	inline friend void F_UpdateMemoryStats( MemStatsCollector& stats, const THIS_TYPE& o )
	{
		stats.staticMem += sizeof(o);
		stats.dynamicMem += o.allocatedMemorySize();
	}

public:	// Reflection.

	typedef
	THIS_TYPE
	ARRAY_TYPE;

	class ArrayDescriptor: public MetaArray
	{
	public:
		inline ArrayDescriptor( const char* typeName )
			: MetaArray( typeName, TbTypeSizeInfo::For_Type<ARRAY_TYPE>(), T_DeduceTypeInfo<TYPE>(), sizeof(TYPE) )
		{}
		//=-- MetaArray
		virtual bool IsDynamic() const override
		{
			return true;
		}
		virtual void* Get_Array_Pointer_Address( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return c_cast(void*) &theArray->_data;
		}
		virtual U32 Generic_Get_Count( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return theArray->num();
		}
		virtual ERet Generic_Set_Count( void* pArrayObject, U32 newNum ) const override
		{
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			mxDO(theArray->setNum( newNum ));
			return ALL_OK;
		}
		virtual U32 Generic_Get_Capacity( const void* pArrayObject ) const override
		{
			const ARRAY_TYPE* theArray = static_cast< const ARRAY_TYPE* >( pArrayObject );
			return theArray->capacity();
		}
		virtual ERet Generic_Set_Capacity( void* pArrayObject, U32 newNum ) const override
		{
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			mxDO(theArray->Reserve( newNum ));
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
			ARRAY_TYPE* theArray = static_cast< ARRAY_TYPE* >( pArrayObject );
			theArray->_capacity_and_flag |= DONT_FREE_MEMORY_MASK;
		}
	};

public:	// Binary Serialization.

	friend AWriter& operator << ( AWriter& file, const THIS_TYPE& o )
	{
		const U32 count = o.num();
		file << count;

		file.SerializeArray( o._data, count );

		return file;
	}
	friend AReader& operator >> ( AReader& file, THIS_TYPE& o )
	{
		U32 count;
		file >> count;
		o.setNum( count );

		file.SerializeArray( o._data, count );

		return file;
	}
	friend mxArchive& operator && ( mxArchive & archive, THIS_TYPE & o )
	{
		U32 num = o.num();
		archive && num;

		if( archive.IsReading() )
		{
			o.setNum( num );
		}

		TSerializeArray( archive, o._data, num );

		return archive;
	}

public:
	//TODO: make special shrink(), ReserveAndGrowByHalf(new_capacity) and ReserveAndGrowByNumber(new_capacity,granularity) ?

	//TODO: sorting, binary search, algorithms & iterators

	// Deep copy. Slow!
	/// copy-assignment operator
	void operator = ( const THIS_TYPE& other )
	{
		if( this != &other )
		{
			this->Copy( other );
		}
	}
	mxTODO("move-assignment operator");

	template< class OTHER_ARRAY >
	ERet Copy( const OTHER_ARRAY& other )
	{
		mxASSERT( this != &other );
		const U32 newNum = other.num();
		mxASSERT(newNum < DBG_MAX_ARRAY_CAPACITY);
		//@todo: copy memory allocator?
		mxDO(this->setNum( newNum ));
		if( newNum )
		{
			mxWARNING_NOTE("untested: CopyConstruct should be faster then Assignment");
			//TCopyArray( _data, other.raw(), newNum );
			TCopyConstructArray( _data, other.raw(), newNum );
		}
		return ALL_OK;
	}

	void AddBytes( const void* src, size_t numBytes )
	{
		mxSTATIC_ASSERT( sizeof(TYPE) == sizeof(BYTE) );
		const size_t oldNum = _count;
		const size_t newNum = oldNum + numBytes;
		this->setNum( newNum );
		memcpy( (BYTE*)_data + oldNum, src, numBytes );
	}

	template< typename U >
	void CopyFromArray( const U* src, U32 num )
	{
		this->setNum( num );
		for( U32 i = 0; i < num; i++ )
		{
			_data[ i ] = src[ i ];
		}
	}
	template< typename U, size_t N >
	inline void CopyFromArray( const U (&src)[N] )
	{
		this->CopyFromArray( src, N );
	}

	ERet appendArray( const THIS_TYPE& other )
	{
		const U32 oldNum = _count;
		const U32 newNum = oldNum + other.num();
		mxDO(this->setNum( newNum ));
		TCopyArray( _data + oldNum, other._data, other.num() );
		return ALL_OK;
	}

	//NOTE:extremely slow!
	THIS_TYPE& AppendUniqueElements( const THIS_TYPE& other )
	{
		for( U32 i = 0; i < other.num(); i++ )
		{
			if(!this->contains( other[i] ))
			{
				this->add( other[i] );
			}
		}
		return *this;
	}

	// works only with types that can be copied via assignment
	// returns the number of removed elements
	//
	template< class FUNCTOR >
	U32 Do_RemoveIf( FUNCTOR& functor )
	{
		const U32 oldNum = _count;
		U32 newNum = 0;
		for( U32 i = 0; i < oldNum; i++ )
		{
			// if no need to remove this element
			if( !functor( _data[i] ) )
			{
				// then copy it
				_data[ newNum++ ] = _data[ i ];
			}
			// otherwise, skip it
		}
		_count = newNum;
		return (oldNum - newNum);
	}

public:

	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef TYPE* iterator;
	typedef const TYPE* const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;


	class Iterator
	{
		THIS_TYPE &	m_array;
		U32		m_currentIndex;

	public:
		inline Iterator( THIS_TYPE& rArray )
			: m_array( rArray )
			, m_currentIndex( 0 )
		{}

		// Functions.

		// returns 'true' if this iterator is valid (there are other elements after it)
		inline bool IsValid() const
		{
			return m_currentIndex < m_array.num();
		}
		inline TYPE& Value() const
		{
			return m_array[ m_currentIndex ];
		}
		inline void MoveToNext()
		{
			++m_currentIndex;
		}
		inline void Skip( U32 count )
		{
			m_currentIndex += count;
		}
		inline void Reset()
		{
			m_currentIndex = 0;
		}

		// Overloaded operators.

		// Pre-increment.
		inline const Iterator& operator ++ ()
		{
			++m_currentIndex;
			return *this;
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		inline operator bool () const
		{
			return this->IsValid();
		}
		inline TYPE* operator -> () const
		{
			return &this->Value();
		}
		inline TYPE& operator * () const
		{
			return this->Value();
		}
	};

	class ConstIterator
	{
		const THIS_TYPE &	m_array;
		U32				m_currentIndex;

	public:
		inline ConstIterator( const THIS_TYPE& rArray )
			: m_array( rArray )
			, m_currentIndex( 0 )
		{}

		// Functions.

		// returns 'true' if this iterator is valid (there are other elements after it)
		inline bool IsValid() const
		{
			return m_currentIndex < m_array.num();
		}
		inline const TYPE& Value() const
		{
			return m_array[ m_currentIndex ];
		}
		inline void MoveToNext()
		{
			++m_currentIndex;
		}
		inline void Skip( U32 count )
		{
			m_currentIndex += count;
		}
		inline void Reset()
		{
			m_currentIndex = 0;
		}

		// Overloaded operators.

		// Pre-increment.
		inline const ConstIterator& operator ++ ()
		{
			++m_currentIndex;
			return *this;
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		inline operator bool () const
		{
			return this->IsValid();
		}
		inline const TYPE* operator -> () const
		{
			return &this->Value();
		}
		inline const TYPE& operator * () const
		{
			return this->Value();
		}
	};

public:
	// Testing, Checking & Debugging.

	// checks some invariants
	bool DbgCheckSelf() const
	{
		//chkRET_FALSE_IF_NOT( IsPowerOfTwo( this->capacity() ) );
		chkRET_FALSE_IF_NOT( this->capacity() <= DBG_MAX_ARRAY_CAPACITY );
		chkRET_FALSE_IF_NOT( _count <= this->capacity() );//this can happen after addFastUnsafe()
		chkRET_FALSE_IF_NOT( _data );
		chkRET_FALSE_IF_NOT( sizeof(U32) <= sizeof(U32) );//need to impl size_t for 64-bit systems
		return true;
	}

private:
	void _releaseMemory()
	{
		if( _data && this->ownsMemory() )
		{
			Arrays::defaultAllocator().Deallocate(
				_data/*, this->capacity() * sizeof(TYPE)*/
				);
		}
		_data = nil;
	}

	/// allocates more memory if needed, but doesn't initialize new elements (doesn't change array count)
	ERet resize( U32 new_capacity )
	{
		mxDO(Arrays::resizeMemoryBuffer(
			&_data
			, _count
			, this->capacity()
			, this->ownsMemory() ? Arrays::OWNS_MEMORY : Arrays::MEMORY_IS_MANAGED_EXTERNALLY
			, Arrays::defaultAllocator()
			, new_capacity
			));

		_capacity_and_flag = new_capacity;

		return ALL_OK;
	}

public_internal:

	/// For serialization, we want to initialize the vtables
	/// in classes post data load, and NOT call the default constructor
	/// for the arrays (as the data has already been set).
	inline explicit TArray( _FinishedLoadingFlag )
	{
	}

private:
	NO_COMPARES(THIS_TYPE);
};

//-----------------------------------------------------------------------
// Reflection.
//
template< typename TYPE >
struct TypeDeducer< TArray< TYPE > >
{
	static inline const TbMetaType& GetType()
	{
		static TArray< TYPE >::ArrayDescriptor staticTypeInfo(mxEXTRACT_TYPE_NAME(TArray));
		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};

//-----------------------------------------------------------------------

namespace Arrays
{
	template< class CLASS >	// where CLASS is reference counted via Grab()/Drop()
	inline void DropReferences( TArray<CLASS*> & a )
	{
		struct {
			inline void operator () ( CLASS* o )
			{
				o->Drop();
			}
		} dropThem;

		a.Do_ForAll( dropThem );
	}

}//namespace Arrays

mxREMOVE_THIS
template< class TYPE >	// where TYPE has member 'name' of type 'String'
TYPE* FindByName( TArray< TYPE >& items, const char* name )
{
	for( U32 i = 0; i < items.num(); i++ )
	{
		TYPE& item = items[ i ];
		if( Str::EqualS( item.name, name ) ) {
			return &item;
		}
	}
	return nil;
}
template< class TYPE >	// where TYPE has member 'name' of type 'String'
const TYPE* FindByName( const TArray< TYPE >& items, const char* name )
{
	for( U32 i = 0; i < items.num(); i++ )
	{
		const TYPE& item = items[ i ];
		if( Str::EqualS( item.name, name ) ) {
			return &item;
		}
	}
	return nil;
}

template< class TYPE >	// where TYPE has member 'name' of type 'String'
int FindIndexByName( TArray< TYPE >& items, const char* name )
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
template< class TYPE, class PREDICATE >
TYPE* LinearSearch( TArray< TYPE >& _array, const PREDICATE& _predicate )
{
	return LinearSearch< TYPE, PREDICATE >( _array.begin(), _array.end(), _predicate );
}
template< class TYPE, class PREDICATE >
const TYPE* LinearSearch( const TArray< TYPE >& _array, const PREDICATE& _predicate )
{
	return LinearSearch< TYPE, PREDICATE >( _array.begin(), _array.end(), _predicate );
}


typedef TArray< char >	ByteArrayT;
mxREMOVE_THIS
class ByteArrayWriter: public AWriter
{
	ByteArrayT & m_buffer;
	size_t m_writePointer;

public:
	ByteArrayWriter( ByteArrayT & buffer, U32 startOffset = 0 )
		: m_buffer( buffer )
	{
		this->DbgSetName( "ByteArrayWriter" );
		m_writePointer = startOffset;
	}
	virtual ERet Write( const void* source, size_t numBytes ) override
	{
		const size_t totalBytes =  m_writePointer + numBytes;
		m_buffer.setNum( totalBytes );
		memcpy( m_buffer.raw() + m_writePointer, source, numBytes );
		m_writePointer = totalBytes;
		return ALL_OK;
	}
	inline const char* Raw() const
	{
		return m_buffer.raw();
	}
	inline char* Raw()
	{
		return m_buffer.raw();
	}
	inline size_t Tell() const
	{
		return m_writePointer;
	}
	inline void Rewind()
	{
		m_writePointer = 0;
	}
	inline void Seek( size_t absOffset )
	{
		m_buffer.Reserve( absOffset );
		m_writePointer = absOffset;
	}
private:	PREVENT_COPY( ByteArrayWriter );
};

struct ByteWriter: public AWriter
{
	TArray< BYTE >	m_data;
public:
	ByteWriter()
	{}
	virtual ERet Write( const void* data, size_t size ) override;
	PREVENT_COPY( ByteWriter );
};
