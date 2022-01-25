/*
=============================================================================
	File:	Templates.h
	Desc:	
=============================================================================
*/
#pragma once

// std::is_base_of, std::is_polymorphic
#include <type_traits>


template< typename DERIVED, typename BASE >
static inline void TCopyBase( DERIVED &_derivedDst, const BASE& _baseSrc )
{
	mxSTATIC_ASSERT((std::is_base_of< BASE, DERIVED >::value));
	mxSTATIC_ASSERT((!std::is_polymorphic< DERIVED >::value));
	BASE & baseDst = _derivedDst;	// (slicing)
	baseDst = _baseSrc;
}


template< class TYPE >
static inline void TCopyPOD( TYPE &_destination, const TYPE& _source )
{
	memcpy( &_destination, &_source, sizeof(TYPE) );
}

///
mxREFACTOR("TReservedStorage, DynamicArrayWithLocalBuffer");
template< typename T, U32 COUNT >
class TStorage: NonCopyable
{
	char	_storage[ sizeof(T) * COUNT ];

public:
	mxFORCEINLINE T*  raw()
	{
		return reinterpret_cast< T* >( _storage );
	}

	mxFORCEINLINE const T*  raw() const
	{
		return reinterpret_cast< const T* >( _storage );
	}

	mxFORCEINLINE U32 num() const
	{
		return COUNT;
	}

public:
	typedef T (&StaticArrayType)[COUNT];

	StaticArrayType asStaticArray()
	{
		return StaticArrayType( _storage );
	}
};
