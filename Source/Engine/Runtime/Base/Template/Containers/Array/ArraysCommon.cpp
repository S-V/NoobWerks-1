/*
=============================================================================
	File:	Array.cpp
	Desc:
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

namespace Arrays
{
	/// figure out the size for allocating a new buffer
	U32 calculateNewCapacity( const UINT new_element_count, const UINT old_capacity )
	{
		mxASSERT( old_capacity < new_element_count );

		// if number is small then default to 8 elements
		enum { DEFAULT_CAPACITY = 8 };

		U32 new_capacity = (new_element_count >= DEFAULT_CAPACITY)
 			? NextPowerOfTwo( new_element_count )
			: DEFAULT_CAPACITY
			;

		mxASSERT( new_capacity > old_capacity );

		return new_capacity;
	}

}//namespace Arrays
