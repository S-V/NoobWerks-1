/*
=============================================================================
	File:	TReferenceCounted.h
	Desc:	A very simple basic class for reference counted objects (analog of Boost's 'intrusive_ptr').
	Note:	Reference counted objects MUST be accessed through smart pointers
			at all times to make sure reference counting scheme operates correctly.
			Failure to do so will result in strange and unpredictable behaviour and crashes.
	ToDo:	Reduce code bloat due to the usage of templates.
=============================================================================
*/
#pragma once

/*
=======================================================

	TReferenceCounted

	Base class for intrusively reference-counted objects.
	Calls T::onReferenceCountDroppedToZero() when the reference count reaches zero.

=======================================================
*/
template< class T >
struct TReferenceCounted: NonCopyable
{
private:
	mutable AtomicInt	__reference_counter;	//!< The number of references to this object.

public:

	/// increases the reference count by 1
	mxFORCEINLINE void Grab() const
	{
		AtomicIncrement( &__reference_counter );
	}

	/// decreases the reference count by 1
	/// ( Returns true if the object has been deleted. )
	mxFORCEINLINE bool Drop() const
	{
		mxASSERT( this->getReferenceCount() > 0 );
		if ( 0 == AtomicDecrement( &__reference_counter ) )
		{
			T* nonConstPtr = static_cast< T* >( const_cast<TReferenceCounted*>( this ) );
			T::onReferenceCountDroppedToZero( nonConstPtr );
			return true;
		}
		return false;
	}

	TReferenceCounted& operator = ( const TReferenceCounted& other )
	{
		return *this;
	}

	/// gives the number of things pointing to it
	mxFORCEINLINE int getReferenceCount() const
	{
		return AtomicLoad( __reference_counter );
	}

protected:
	mxFORCEINLINE TReferenceCounted()
		: __reference_counter( 0 )
	{}

	mxFORCEINLINE ~TReferenceCounted()
	{
		mxASSERT( this->getReferenceCount() == 0 );
	}
};
