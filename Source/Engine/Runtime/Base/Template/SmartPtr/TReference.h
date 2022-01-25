/*
=============================================================================
	File:	TReference.h
	Desc:	Weak pointer - doesn't take ownership over the object,
			doesn't know about reference counting,
			always holds a valid pointer of a templated type.
=============================================================================
*/

#pragma once
#ifndef __MX_CONTAINTERS_REFERENCE_H__
#define __MX_CONTAINTERS_REFERENCE_H__

///
///	TReference< TYPE > - is a weak pointer
///	that always stores a non-null pointer.
///
template< typename TYPE >
class TReference
{
	TYPE *	m_pointer;

public:
	mxFORCEINLINE TReference( TYPE& reference )
	{
		mxASSERT_PTR( &reference );
		m_pointer = &reference;
	}

	mxFORCEINLINE ~TReference()
	{
		m_pointer = nil;
	}

	mxFORCEINLINE operator TYPE& () const
	{
		//mxASSERT_PTR( m_pointer );
		return *m_pointer;
	}
	mxFORCEINLINE operator const TYPE& () const
	{
		//mxASSERT_PTR( m_pointer );
		return *m_pointer;
	}

	mxFORCEINLINE bool operator == ( TYPE* pObject ) const
	{
		return ( m_pointer == pObject );
	}
	mxFORCEINLINE bool operator != ( TYPE* pObject ) const
	{
		return ( m_pointer != pObject );
	}
	mxFORCEINLINE bool operator == ( const TReference<TYPE>& other ) const
	{
		return ( m_pointer == other.m_pointer );
	}
	mxFORCEINLINE bool operator != ( const TReference<TYPE>& other ) const
	{
		return ( m_pointer != other.m_pointer );
	}

public:

	mxFORCEINLINE void Destruct()
	{
		mxASSERT_PTR( m_pointer );
		{
			m_pointer->~TYPE();
			m_pointer = nil;
		}
	}
	mxFORCEINLINE void Delete()
	{
		mxASSERT_PTR( m_pointer );
		{
			delete m_pointer;
			m_pointer = nil;
		}
	}

private:	PREVENT_COPY( TReference<TYPE> );
};

#endif // ! __MX_CONTAINTERS_REFERENCE_H__
