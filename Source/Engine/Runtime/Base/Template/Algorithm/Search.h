#pragma once

/*
===============================================================================
	LINEAR SEARCH
===============================================================================
*/

template< class CONTAINER, class PREDICATE >
const int LinearSearchIndex( const CONTAINER& _items, const PREDICATE& _predicate )
{
	int itemIndex = 0;
	CONTAINER::ConstIterator it( _items );
	while( it.IsValid() )
	{
		if( _predicate( it.Value() ) ) {
			return itemIndex;
		}
		it.MoveToNext();
		++itemIndex;
	}
	return -1;
}

template< class TYPE >	// where TYPE has member 'name' of type 'String'
class NamesEqual2 {
	const char* m_name;
public:
	NamesEqual2( const char* name ) : m_name( name )
	{}
	bool operator() ( const TYPE& other ) const
	{ return 0 == strcmp( m_name, other.name.c_str() ); }
};

template< class CONTAINER >
const int FindIndexByName( const CONTAINER& items, const char* name )
{
	return LinearSearchIndex(
		items,
		NamesEqual2< CONTAINER::ITEM_TYPE >( name )
	);
}

template< class TYPE, class PREDICATE >
TYPE* LinearSearch( TYPE* _start, TYPE* _end, const PREDICATE& _predicate )
{
	while( _start != _end )
	{
		if( _predicate( *_start ) ) {
			return _start;
		}
		++_start;
	}
	return nil;
}

/*
===============================================================================
	BINARY SEARCH
===============================================================================
*/

/// Assumes that the keys are sorted in decreasing order (duplicates are allowed).
/// Finds the first array element which is greater than or equal to the given value.
template< typename TYPE >
inline
const UINT LowerBoundDescending(
	const TYPE value,	//!< searched item
	const TYPE* keys,	//!< sorted in decreasing order
	const UINT count	//!< right bound (exclusive)
	)
{
	UINT offset = 0;
	UINT length = count;
	UINT middle = length;
	while( middle ) {
		middle = length / 2;
		// array is sorted in decreasing order, i.e.
		// items[0] >= items[mid] >= items[right-1]
		if( keys[ middle + offset ] >= value ) {
			offset += middle;	// item in [mid..right)
		}// else - the searched item is in [0..mid)
		length -= middle;
	}
	return offset;
}

/// Assumes that keys are sorted in increasing order (duplicates are allowed).
/// Finds the first array element which is less than or equal to the given value.
template< typename TYPE >
inline
const UINT LowerBoundAscending(
	const TYPE value,	//!< searched item
	const TYPE* keys,	//!< sorted in increasing order
	const UINT right	//!< right bound (exclusive)
	)
{
	UINT offset = 0;
	UINT length = right;
	UINT middle = length;
	while( middle ) {
		middle = length / 2;
		// array is sorted in increasing order, i.e.
		// items[0] <= items[mid] <= items[right-1]
		if( keys[ middle + offset ] <= value ) {
			offset += middle;	// item in [mid..right)
		}// else - the searched item is in [0..mid)
		length -= middle;
	}
	return offset;
}



template< typename T >
struct LessEqual
{
	mxFORCEINLINE bool operator () ( const T& a, const T& b ) const
	{
		return a <= b;
	}
};

template
<
	typename TYPE,
	class LESS_OR_EQUAL
>
const UINT LowerBoundAscending(
	const TYPE value,	//!< searched item
	const TYPE* keys,	//!< sorted in increasing order
	const UINT count,	//!< right bound (exclusive)
	const LESS_OR_EQUAL& less_or_equal = LessEqual< TYPE >()
	)
{
	UINT offset = 0;
	UINT length = count;
	UINT middle = length;
	while( middle ) {
		middle = length / 2;
		// array is sorted in increasing order, i.e.
		// items[0] <= items[mid] <= items[right-1]
		if( less_or_equal( keys[ middle + offset ], value ) ) {
			offset += middle;	// item in [mid..right)
		}// else - the searched item is in [0..mid)
		length -= middle;
	}
	return offset;
}




namespace UnitTests
{

template< typename T, int N >
static
bool Test_BinSearch_Function(
	const T searched_value
	, const T (&elems)[N]
	)
{
	mxASSERT(
		Arrays::IsSortedInDecreasingOrder_AllowDuplicates(elems, N)
		);

	const int index_bin_search = LowerBoundDescending(
		searched_value
		, elems
		, N
	);
	bool found_via_bin_search = false;
	if( elems[index_bin_search] == searched_value ) {
		found_via_bin_search = true;
	}
	
	for(int i = 0; i < N; i++ ) {
		if(elems[i] == searched_value) {
			mxASSERT(found_via_bin_search);
			mxASSERT(i == index_bin_search);
			return true;
		}
	}

	// not found
	mxASSERT(!found_via_bin_search);
 	return false;
}

template< typename T, int N >
static
void Test_BinarySearch_SingleArray(
	T (&elems)[N]
)
{
	//
	Test_BinSearch_Function(
		9,
		elems
	);
	
	Test_BinSearch_Function(
		0,
		elems
	);
	
	Test_BinSearch_Function(
		1,
		elems
	);
	
	Test_BinSearch_Function(
		5,
		elems
	);
	
	Test_BinSearch_Function(
		4,
		elems
	);
}

static
void Test_BinarySearch()
{
	const int sorted_ints_10_in_descending_order[] = {
		9, 8, 7, 6, 5,
		4, 3, 2, 1, 0
	};
	
	const int sorted_ints_9_in_descending_order[] = {
		9, 8, 7, 6,
		5,
		4, 3, 2, 1
	};
	
	const int sorted_ints_2_in_descending_order[] = {
		9, 8,
	};
	
	Test_BinarySearch_SingleArray(
		sorted_ints_10_in_descending_order
	);
	
	Test_BinarySearch_SingleArray(
		sorted_ints_9_in_descending_order
	);

	Test_BinarySearch_SingleArray(
		sorted_ints_2_in_descending_order
	);
	

	//
	const U32 test_array_0x63[] = {
		0x00001edb,
		0x00001eda,
		0x00001ed9,
		0x00001ed8,
		0x00001ed3,
		0x00001ed2,
		0x00001ed1,
		0x00001ed0,
		0x00001ecb,
		0x00001eca,
		0x00001ec9,
		0x00001ec8,
		0x00001ec3,
		0x00001ec2,
		0x00001ec1,
		0x00001ec0,
		0x00001e9b,
		0x00001e9a,
		0x00001e99,
		0x00001e98,
		0x00001e93,
		0x00001e92,
		0x00001e91,
		0x00001e90,
		0x00001e8b,
		0x00001e8a,
		0x00001e89,
		0x00001e88,
		0x00001e83,
		0x00001e82,
		0x00001e81,
		0x00001e80,
		0x00001e5b,
		0x00001e5a,
		0x00001e59,
		0x00001e58,
		0x00001e53,
		0x00001e52,
		0x00001e51,
		0x00001e50,
		0x00001e4b,
		0x00001e4a,
		0x00001e49,
		0x00001e48,
		0x00001e43,
		0x00001e42,
		0x00001e41,
		0x00001e40,
		0x00001e1b,
		0x00001e1a,
		0x00001e19,
		0x00001e18,
		0x00001e13,
		0x00001e12,
		0x00001e11,
		0x00001e10,
		0x00001e0b,
		0x00001e0a,
		0x00001e09,
		0x00001e08,
		0x00001e03,
		0x00001e02,
		0x00001e01,
		0x00001e00,
		0x00001adb,
		0x00001ada,
		0x00001ad9,
		0x00001ad8,
		0x00001ad3,
		0x00001ad2,
		0x00001ad1,
		0x00001ad0,
		0x00001a9b,
		0x00001a9a,
		0x00001a99,
		0x00001a98,
		0x00001a93,
		0x00001a92,
		0x00001a91,
		0x00001a90,
// sorted order breaks here:
		0x00001cdb,
		0x00001cda,
		0x00001cd9,
		0x00001cd8,
		0x00001ccb,
		0x00001cca,
		0x00001cc9,//<=
		0x00001cc8,
		0x00001c5b,
		0x00001c5a,
		0x00001c59,
		0x00001c58,
		0x00001c4b,
		0x00001c4a,
		0x00001c49,
		0x00001c48,
		0x000018db,
		0x000018da,
		0x000018d9,
		0x000018d8,
	};

	//
	Test_BinSearch_Function<U32>(
		0x00001cc9,
		test_array_0x63
	);
	//first
	Test_BinSearch_Function<U32>(
		0x00001edb,
		test_array_0x63
	);
	//last
	Test_BinSearch_Function<U32>(
		0x000018d8,
		test_array_0x63
	);

	DBGOUT("All checks completed!");
}

}//namespace UnitTests
