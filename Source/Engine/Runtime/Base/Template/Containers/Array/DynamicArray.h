/*
=============================================================================
	File:	DynamicArray.h
	Desc:	Dynamic (variable sized) templated array.
	The array is always stored in a contiguous chunk.
	The array can be resized.
	A size increase will cause more memory to be allocated,
	and may result in relocation of the array memory.
	A size decrease has no effect on the memory allocation.
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/ArraysCommon.h>
#include <Base/Object/Reflection/ArrayDescriptor.h>

template
<
	typename TYPE,	//!< The type of stored elements
	UINT ALIGNMENT = GET_ALIGNMENT_FOR_CONTAINERS(TYPE)	//!< Alignment of allocated memory
>
class DynamicArray
	: public TArrayMixin< TYPE, DynamicArray< TYPE > >
{
public_internal:	// Calling member functions is slow in debug builds.

	/// The pointer to the allocated memory.
	TYPE *	_data;

	/// The number of valid, 'live' elements, this should be inside the range [0..capacity).
	U32		_count;

	/// The number of allocated entries + highest bit is set if we cannot deallocate the memory.
	U32		_capacity_and_flag;

	/// The allocator used by this array.
	AllocatorI &	_allocator;

	//16/24

public:
	// 1 bit is used for indicating if the memory was externally allocated (and the array cannot delete it)
	enum { CAPACITY_BITS = (sizeof(U32) * BITS_IN_BYTE) - 1 };

	enum We_Store_Capacity_And_Bit_Flags_In_One_Int
	{
		DONT_FREE_MEMORY_MASK = U32(1 << CAPACITY_BITS),	//!< Indicates that the storage is not the array's to delete.
		EXTRACT_CAPACITY_MASK = U32(~DONT_FREE_MEMORY_MASK),
	};

	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef TYPE* iterator;
	typedef const TYPE* const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;

public:
	DynamicArray( AllocatorI & _heap )
		: _allocator( _heap )
	{
		_data = nil;
		_count = 0;
		_capacity_and_flag = 0;
	}

	explicit DynamicArray( const DynamicArray& other )
		: _allocator( other._allocator )
	{
		_data = nil;
		_count = 0;
		_capacity_and_flag = 0;
		Arrays::Copy( *this, other );
	}

	/// Use it only if you know what you're doing.
	mxFORCEINLINE explicit DynamicArray(ENoInit)
	{}

	/// move ctor for C++03
	DynamicArray( DynamicArray & other, TagStealOwnership )
		: _data( other._data )
		, _count( other._count )
		, _capacity_and_flag( other._capacity_and_flag )
		, _allocator( other._allocator )
	{
		other._data = nil;
		other._count = 0;
		other._capacity_and_flag = 0;
	}

	/// Destructs array elements and deallocates array memory.
	~DynamicArray()
	{
		this->clear();
	}

	/// use it only if you know whatcha doin'
	void initializeWithExternalStorageAndCount( TYPE* data, U32 capacity, U32 initial_count = 0 )
	{
		mxASSERT(_data == nil && _capacity_and_flag == 0);
		_data = data;
		_count = initial_count;
		_capacity_and_flag = capacity | DONT_FREE_MEMORY_MASK;
	}

	/// use it only if you know whatcha doin'
	void takeOwnershipOfAllocatedData( void * data, U32 capacity, U32 initial_count = 0 )
	{
		mxASSERT(_data == nil && _capacity_and_flag == 0);
		_data = static_cast< TYPE* >( data );
		_count = initial_count;
		_capacity_and_flag = capacity;
	}


	// Returns the current capacity of this array.
	mxFORCEINLINE U32 capacity() const { return _capacity_and_flag & EXTRACT_CAPACITY_MASK; }

	/// Convenience function to get the number of elements in this array.
	/// Returns the size (the number of elements in the array).
	mxFORCEINLINE U32 num() const { return _count; }

	mxFORCEINLINE TYPE * raw() { return _data; }
	mxFORCEINLINE const TYPE* raw() const { return _data; }

	mxFORCEINLINE AllocatorI& allocator() { return _allocator; }

	/// Convenience function to empty the array. Doesn't release allocated memory.
	void RemoveAll()
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
			this->_changeCapacity( _count );
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
			this->RemoveAll();
		} else {
			this->clear();
		}
	}

	/// Adds an element to the end.
	ERet add( const TYPE& newOne )
	{
		mxDO(this->reserve( _count + 1 ));
		new(&_data[ _count++ ]) TYPE( newOne );	// copy-construct
		return ALL_OK;
	}

	void AppendItem_NoResize( const TYPE& new_item )
	{
		mxASSERT(_count + 1 <= this->capacity());
		new(&_data[ _count++ ]) TYPE( new_item );
	}

	/// Increments the size and returns a reference to the first uninitialized element.
	/// The user is responsible for constructing the returned elements.
	TYPE* AllocateUninitialized(UINT num_items_to_allocate = 1)
	{
		const U32 old_count = _count;
		mxENSURE(
			this->reserve( old_count + num_items_to_allocate ) == ALL_OK
			, nil
			, ""
			);
		_count += num_items_to_allocate;
		return &_data[ old_count ];
	}

	/// Increments the size and returns a reference to the first default-constructed element.
	mxDEPRECATED
	TYPE & AddNew()
	{
		this->reserve( _count + 1 );
		//NOTE: do not use TYPE(), because it zeroes out PODs, stealing CPU cycles
		return *new(&_data[ _count++ ]) TYPE;
	}


	/// assumes that the user has allocated enough space via reserve()
	mxFORCEINLINE void AddFastUnsafe( const TYPE& new_item )
	{
		mxASSERT( _count < this->capacity() );
		_data[ _count++ ] = new_item;
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

	// Slow!
	bool Remove( const TYPE& item )
	{
		return Arrays::RemoveElement( *this, item );
	}

	// Slow!
	void RemoveAt( U32 index, U32 count = 1 )
	{
		Arrays::RemoveAndShift( *this, index, count );
	}

	// this method is faster (uses the 'swap trick')
	// Doesn't preserve the relative order of elements.
	void RemoveAt_Fast( U32 index )
	{//I KNOW THIS IS CRAP!
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
		this->reserve( newNum );
		for ( U32 i = oldNum; i > index; --i )
		{
			data[i] = data[i-1];
		}
		_count = newNum;
		return data[ index ];
	}

	/// Ensures no reallocation occurs until at least size 'element_count'.
	ERet reserve( U32 element_count )
	{
		const U32 old_capacity = this->capacity();
	//	mxASSERT( element_count > 0 );//<- this helped me to catch errors
		mxASSERT( element_count <= DBG_MAX_ARRAY_CAPACITY );
		// change the capacity if necessary
		if( element_count > old_capacity )
		{
			const U32 new_capacity = Arrays::calculateNewCapacity( element_count, old_capacity );
			mxDO(this->_changeCapacity( new_capacity ));
		}
		return ALL_OK;
	}

	/// Ensures no reallocation occurs until the given element count is reached.
	ERet ReserveExactly( const U32 element_count )
	{
		const U32 old_capacity = this->capacity();

		// change the capacity if necessary
		if( element_count > old_capacity )
		{
			mxDO(this->_changeCapacity( element_count ));
		}

		return ALL_OK;
	}

	/// Sets the new number of elements. Resizes the array to this number if necessary.
	ERet setNum( const U32 num_elements )
	{
		const U32 old_count = _count;
		const U32 old_capacity = this->capacity();

		// change the capacity if necessary
		if( num_elements > old_capacity )
		{
			const U32 new_capacity = Arrays::calculateNewCapacity( num_elements, old_capacity );
			mxDO(this->_changeCapacity( new_capacity ));
		}

		// Call default constructors for new elements.
		if( num_elements > old_count )
		{
			TConstructN_IfNonPOD(
				_data + old_count,
				num_elements - old_count
				);
		}

		_count = num_elements;

		return ALL_OK;
	}

	/// Resizes the container so that it contains exactly the given number elements.
	/// NOTE: invokes the new objects' constructors!
	ERet setCountExactly( const U32 new_num_elements )
	{
		const U32 old_count = _count;
		const U32 old_capacity = this->capacity();

		// change the capacity if necessary
		if( new_num_elements > old_capacity )
		{
			const U32 new_capacity = new_num_elements;
			mxDO(this->_changeCapacity( new_capacity ));
		}

		// Call default constructors for new elements.
		if( new_num_elements > old_count )
		{
			TConstructN_IfNonPOD(
				_data + old_count,
				new_num_elements - old_count
				);
		}

		_count = new_num_elements;

		return ALL_OK;
	}

	bool ownsMemory() const
	{
		return (_capacity_and_flag & DONT_FREE_MEMORY_MASK) == 0;
	}

	// Returns the amount of reserved memory in bytes (memory allocated for storing the elements).
	size_t allocatedMemorySize() const
	{
		return this->capacity() * sizeof(TYPE);
	}

	// Returns the total amount of occupied memory in bytes.
	size_t usedMemorySize() const
	{
		return this->allocatedMemorySize() + sizeof(*this);
	}

	friend void F_UpdateMemoryStats( MemStatsCollector& stats, const DynamicArray& o )
	{
		stats.staticMem += sizeof(o);
		stats.dynamicMem += o.allocatedMemorySize();
	}

public:	// Reflection.

	typedef
	DynamicArray
	ARRAY_TYPE;

	class ArrayDescriptor: public MetaArray
	{
	public:
		ArrayDescriptor( const char* typeName )
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
			mxDO(theArray->setCountExactly( newNum ));
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
			mxDO(theArray->reserve( newNum ));
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
			if(theArray->_data) {
				theArray->_capacity_and_flag |= DONT_FREE_MEMORY_MASK;
			}
		}
	};

public:	// Binary Serialization.

	ERet SaveAsPOD(AWriter& writer) const
	{
		mxSTATIC_ASSERT(TypeTrait<TYPE>::IsPlainOldDataType);
		const U32 count = _count;
		mxDO(writer.Put(count));
		if( count ) {
			return writer.Write(this->raw(), this->rawSize());
		} else {
			return ALL_OK;
		}
	}

	ERet LoadAsPOD(AReader& reader)
	{
		mxSTATIC_ASSERT(TypeTrait<TYPE>::IsPlainOldDataType);
		U32 count;
		mxDO(reader.Get(count));
		mxDO(this->setNum(count));
		if( count ) {
			return reader.Read(this->raw(), this->rawSize());
		} else {
			return ALL_OK;
		}
	}

	friend AWriter& operator << ( AWriter& file, const DynamicArray& o )
	{
		const U32 number = o._count;
		file << number;

		file.SerializeArray( o._data, number );

		return file;
	}

	friend AReader& operator >> ( AReader& file, DynamicArray& o )
	{
		U32 number;
		file >> number;
		o.setNum( number );

		file.SerializeArray( o._data, number );

		return file;
	}

	friend mxArchive& operator && ( mxArchive & archive, DynamicArray & o )
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

public:	// Operator overloads.

	bool operator == ( const DynamicArray& other ) const
	{
		return this->num() == other.num()
			&& Arrays::equal( this->raw(), other.raw(), this->num() );
			;
	}

	bool operator != ( const DynamicArray& other ) const
	{
		return !( this->num() == other.num() );
	}

	mxFORCEINLINE operator const TSpan< TYPE >()				{ return TSpan< TYPE >( _data, _count ); }
	mxFORCEINLINE operator const TSpan< const TYPE >() const	{ return TSpan< const TYPE >( _data, _count ); }

public:
	//TODO: make special shrink(), ReserveAndGrowByHalf(new_capacity) and ReserveAndGrowByNumber(new_capacity,granularity) ?

	//TODO: sorting, binary search, algorithms & iterators

	/// Deep copy. Slow!
	/// copy-assignment operator
	void operator = ( const DynamicArray& other )
	{
		Arrays::Copy( *this, other );
	}
	mxTODO("move-assignment operator");

	void AddBytes( const void* src, size_t numBytes )
	{
		mxSTATIC_ASSERT( sizeof(TYPE) == sizeof(BYTE) );
		const size_t oldNum = _count;
		const size_t newNum = oldNum + numBytes;
		this->setNum( newNum );
		memcpy( (BYTE*)_data + oldNum, src, numBytes );
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
	// Testing, Checking & Debugging.

	// you better check urself, before u wreck urself
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
			Arrays::freeMemoryBuffer(
				&_data
				, this->capacity()
				, _allocator
				);
		}
		_data = nil;
	}

	/// allocates more memory if needed, but doesn't initialize new elements (doesn't change array count)
	ERet _changeCapacity( U32 new_capacity )
	{
		mxDO(Arrays::resizeMemoryBuffer(
			&_data
			, _count
			, this->capacity()
			, this->ownsMemory() ? Arrays::OWNS_MEMORY : Arrays::MEMORY_IS_MANAGED_EXTERNALLY
			, _allocator
			, new_capacity
			, ALIGNMENT
			));

		_capacity_and_flag = new_capacity;

		return ALL_OK;
	}

public_internal:

	/// For serialization, we want to initialize the vtables
	/// in classes post data load, and NOT call the default constructor
	/// for the arrays (as the data has already been set).
	explicit DynamicArray( _FinishedLoadingFlag )
	{
	}
};



/// contains an embedded buffer to avoid initial memory allocations
template< typename TYPE, UINT32 RESERVED >
class DynamicArrayWithLocalBuffer
	: public DynamicArray< TYPE >
	, NonCopyable
{
	typedef typename std::aligned_storage
	<
		sizeof(TYPE),
		std::alignment_of<TYPE>::value
	>::type
	StorageType;

	StorageType	m__embeddedStorage[RESERVED];	// uninitialized storage for N objects of TYPE

public:
	DynamicArrayWithLocalBuffer( AllocatorI & allocator )
		: DynamicArray< TYPE >( allocator )
	{
		this->initializeWithExternalStorageAndCount(
			reinterpret_cast< TYPE* >( m__embeddedStorage ),
			mxCOUNT_OF(m__embeddedStorage)
		);
	}
};

//-----------------------------------------------------------------------
// Reflection.
//
template< typename TYPE >
struct TypeDeducer< DynamicArray< TYPE > >
{
	static inline const TbMetaType& GetType()
	{
		static DynamicArray< TYPE >::ArrayDescriptor staticTypeInfo(mxEXTRACT_TYPE_NAME(DynamicArray));
		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};


template< typename TYPE >
U64 computeHash( const DynamicArray< TYPE >& items )
{
	return MurmurHash64( items.raw(), items.rawSize() );
}
