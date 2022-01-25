/*
=============================================================================
	File:	TAutoPtr.h
	Desc:	Smart pointer for automatic memory deallocation.
			Analog of std::auto_ptr or Boost's 'scoped_ptr'.
=============================================================================
*/
#pragma once

//
//	TAutoPtr< T >
//
//	NOTE: This class is not safe for array new's.
//	It will not properly call the destructor for each element
//	and you will silently leak memory.
//	It does work for classes requiring no destructor however.
//
template< typename T >
class TAutoPtr {
public:
	inline explicit TAutoPtr( T* pointer = nil )
		: Ptr( pointer )
	{}

	inline ~TAutoPtr()
	{
		if( Ptr ) {
			delete( Ptr );
		}
	}

	void DisOwn()
	{
		Ptr = nil;
	}

	inline T & operator * () const
	{
		mxASSERT_PTR( Ptr );
		return *Ptr;
	}

	inline T * operator -> () const
	{
		mxASSERT_PTR( Ptr );
		return Ptr;
	}

	inline operator T* ()
	{
		return Ptr;
	}
	inline operator const T* () const
	{
		return Ptr;
	}

	inline void operator = ( T* pointer )
	{
		if( Ptr != pointer )
		{
			delete( Ptr );
		}
		Ptr = pointer;
	}

	inline bool operator == ( T* pObject ) const
	{
		return ( Ptr == pObject );
	}
	inline bool operator != ( T* pObject ) const
	{
		return ( Ptr != pObject );
	}
	inline bool operator == ( const TAutoPtr<T>& other ) const
	{
		return ( Ptr == other.Ptr );
	}
	inline bool operator != ( const TAutoPtr<T>& other ) const
	{
		return ( Ptr != other.Ptr );
	}

private:
	T *		Ptr;

private:
	// Disallow copies.
	TAutoPtr( const TAutoPtr<T>& other );
	TAutoPtr<T> & operator=( const TAutoPtr<T>& other );
};

template< typename T >
inline const T& GetConstObjectReference( const TAutoPtr< T >& o )
{
	return *o;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
