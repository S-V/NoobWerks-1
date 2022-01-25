/*
=============================================================================
	File:	TPointerMap.h
	Desc:	A templated hash table which maps pointer-sized integers
			to arbitrary values.
	ToDo:	Stop reinventing the wheel.
=============================================================================
*/

#ifndef __MX_TEMPLATE_POINTER_MAP_H__
#define __MX_TEMPLATE_POINTER_MAP_H__

#include <Base/Template/Containers/HashMap/THashMap.h>

// hash table which maps pointer-sized integers to arbitrary values
template<
	typename VALUE,
	class HASH_FUNC = mxPointerHasher
>
class TPointerMap
	: public THashMap
	<
		const void*,
		VALUE,
		HASH_FUNC
	>
{
public:
	explicit TPointerMap( AllocatorI & allocator )
		: THashMap( allocator )
	{}
};

#endif // ! __MX_TEMPLATE_POINTER_MAP_H__
