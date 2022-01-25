/*
=============================================================================
	File:	InsertionSort.h
	Desc:	
=============================================================================
*/
#pragma once

namespace Sorting
{


template< typename T >
struct ByIncreasingOrder
{
	mxFORCEINLINE bool AreInOrder( const T& a, const T& b ) const
	{
		return a < b;
	}
};

template< typename T >
struct ByDecreasingOrder
{
	mxFORCEINLINE bool AreInOrder( const T& a, const T& b ) const
	{
		return a > b;
	}
};


template< typename TYPE >
void InsertionSort_ByIncreasingOrder(
									 TYPE* data,
									 UINT count,
									 UINT start_index = 0
									 )
{
	mxASSERT( count > 1 && start_index < count - 1 );

	TYPE * lo = data + start_index;
	TYPE * hi = data + count - 1;
	TYPE * best;
	TYPE * p;

	while( hi > lo )
	{
		best = lo;

		// find the greatest element in (lo, hi]
		for( p = lo+1; p <= hi; p++ )
		{
			if( *best < *p )
			{
				best = p;
			}
		}

		// move the greatest element to back
		TSwap( *best, *hi );

		hi--;
	}
}


/*
=============================================================================
	Insertion sort

	- Efficient for (quite) small data sets
	- Adaptive, i.e. efficient for data sets that are already substantially sorted: the time complexity is O(n + d), where d is the number of inversions
	- More efficient in practice than most other simple quadratic, i.e. O(n2) algorithms such as selection sort or bubble sort; the best case (nearly sorted input) is O(n)
	- Stable, i.e. does not change the relative order of elements with equal keys
	- In-place, i.e. only requires a constant amount O(1) of additional memory space
	- Online, i.e. can sort a list as it receives it
=============================================================================
*/
template< typename TYPE, class COMPARATOR >
void InsertionSort(
				   TYPE* data, UINT count
				   , const COMPARATOR& comparator = ByIncreasingOrder< TYPE >()
				   , UINT start_index = 0
				   )
{
	mxASSERT( count > 1 && start_index < count - 1 );

	TYPE * lo = data + start_index;
	TYPE * hi = data + count - 1;
	TYPE * best;
	TYPE * p;

	while( hi > lo )
	{
		best = lo;

		// find the greatest element in (lo, hi]
		for( p = lo+1; p <= hi; p++ )
		{
			if( comparator.AreInOrder( *best, *p ) )
			{
				best = p;
			}
		}

		// move the greatest element to back
		TSwap( *best, *hi );

		hi--;
	}
}




/// sorts in increasing order
template
<
	typename Key,
	typename Value,
	class COMPARATOR
>
void InsertionSortKeysAndValues(
								Key * keys, UINT count,
								Value * values,
								const COMPARATOR& comparator = ByIncreasingOrder< Key >()
								)
{
	mxASSERT(count > 0);

	Key * lo = keys;
	Key * hi = keys + count - 1;
	Key * best;

	while( hi > lo )
	{
		best = lo;

		// find the greatest element in (lo, hi]
		for( Key* p = lo+1; p <= hi; p++ )
		{
			if( comparator.AreInOrder( *best, *p ) )
			{
				best = p;
			}
		}

		// move the greatest element to back
		TSwap( *best, *hi );

		// sort the values in the same order as keys
		TSwap( values[best-keys], values[hi-keys] );

		hi--;
	}
}


template
<
	typename Key,
	typename Value0,
	typename Value1,
	class COMPARATOR
>
U32 InsertionSort_Keys_and_Values2(
	Key * keys
	, UINT count
	, Value0 * values0
	, Value1 * values1
	, const COMPARATOR& comparator = ByIncreasingOrder< Key >()
	)
{
	mxASSERT(count > 1);

	U32	num_swaps = 0;

	Key * lo = keys;
	Key * hi = keys + count - 1;
	Key * best;

	while( hi > lo )
	{
		best = lo;

		// find the greatest element in (lo, hi]
		for( Key* p = lo+1; p <= hi; p++ )
		{
			if( comparator.AreInOrder( *best, *p ) )
			{
				best = p;
			}
		}

		// avoid useless copy if the best element is already in place
		if( best != hi )
		{
			// move the greatest element to back
			TSwap( *best, *hi );

			// sort the values in the same order as keys
			const U32 best_idx = best - keys;
			const U32 high_idx = hi   - keys;

			TSwap( values0[ best_idx ], values0[ high_idx ] );
			TSwap( values1[ best_idx ], values1[ high_idx ] );

			++num_swaps;
		}

		hi--;
	}

	return num_swaps;
}


template
<
	typename Key,
	typename Value0,
	typename Value1,
	typename Value2,
	class COMPARATOR
>
U32 InsertionSort_Keys_and_Values3(
	Key * keys
	, UINT count
	, Value0 * values0
	, Value1 * values1
	, Value2 * values2
	, const COMPARATOR& comparator = ByIncreasingOrder< Key >()
	)
{
	mxASSERT(count > 1);

	U32	num_swaps = 0;

	Key * lo = keys;
	Key * hi = keys + count - 1;
	Key * best;

	while( hi > lo )
	{
		best = lo;

		// find the greatest element in (lo, hi]
		for( Key* p = lo+1; p <= hi; p++ )
		{
			if( comparator.AreInOrder( *best, *p ) )
			{
				best = p;
			}
		}

		// avoid useless copy if the best element is already in place
		if( best != hi )
		{
			// move the greatest element to back
			TSwap( *best, *hi );

			// sort the values in the same order as keys
			const U32 best_idx = best - keys;
			const U32 high_idx = hi   - keys;

			TSwap( values0[ best_idx ], values0[ high_idx ] );
			TSwap( values1[ best_idx ], values1[ high_idx ] );
			TSwap( values2[ best_idx ], values2[ high_idx ] );

			++num_swaps;
		}

		hi--;
	}

	return num_swaps;
}



static
void Test_InsertionSort()
{
	const int unsorted_ints[] = {
		2, 5, 6, 0, 9, 7, 3, 1, 0, 1, 2
	};


	//
	int ints_to_sort_increasing_order[mxCOUNT_OF(unsorted_ints)];
	TCopyStaticArray(ints_to_sort_increasing_order, unsorted_ints);

	Sorting::InsertionSort(
		ints_to_sort_increasing_order, mxCOUNT_OF(ints_to_sort_increasing_order)
		, Sorting::ByIncreasingOrder< int >()
		);

	mxASSERT(Arrays::CheckIfIsSortedIncreasing_AllowDuplicates(
		ints_to_sort_increasing_order, mxCOUNT_OF(ints_to_sort_increasing_order)
		));


	//
	int ints_to_sort_decreasing_order[mxCOUNT_OF(unsorted_ints)];
	TCopyStaticArray(ints_to_sort_decreasing_order, unsorted_ints);

	Sorting::InsertionSort(
		ints_to_sort_decreasing_order, mxCOUNT_OF(ints_to_sort_decreasing_order)
		, Sorting::ByDecreasingOrder< int >()
		);

	mxASSERT(Arrays::IsSortedInDecreasingOrder_AllowDuplicates(
		ints_to_sort_decreasing_order, mxCOUNT_OF(ints_to_sort_decreasing_order)
		));
}

}//namespace Sorting
