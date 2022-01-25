/*
=============================================================================
	File:	ReferenceCounted.h
	Desc:	A very simple basic class for intrusively reference-counted objects
			(analog of Boost's 'intrusive_ptr').
	Note:	Reference counted objects MUST be accessed through smart pointers
			at all times to make sure reference counting scheme operates correctly.
			Failure to do so will result in strange and unpredictable behaviour and crashes.
=============================================================================
*/
#pragma once

//
//	ReferenceCounted - calls 'delete' on the object when the reference count reaches zero.
//
struct ReferenceCounted
{
	void	Grab() const;
	bool	Drop() const;	// ( Returns true if the object has been deleted. )

	INT	getReferenceCount() const;

	ReferenceCounted& operator = ( const ReferenceCounted& other );

protected:
			ReferenceCounted();
	virtual	~ReferenceCounted();

private:
	//@TODO: AtomicInt
	mutable int	_reference_counter;	//!< Number of references to this object.
};

/*================================
		ReferenceCounted
================================*/

mxFORCEINLINE ReferenceCounted::ReferenceCounted()
	: _reference_counter( 0 )
{}

mxFORCEINLINE ReferenceCounted::~ReferenceCounted()
{
	mxASSERT( 0 == _reference_counter );
}

mxFORCEINLINE void ReferenceCounted::Grab() const {
	++_reference_counter;
}

mxFORCEINLINE bool ReferenceCounted::Drop() const
{
	mxASSERT( _reference_counter > 0 );
	--_reference_counter;
	if ( _reference_counter == 0 )
	{
		// 'delete this' is evil (?)
		delete( this );
		return true;
	}
	return false;
}

mxFORCEINLINE INT ReferenceCounted::getReferenceCount() const
{
	return _reference_counter;
}

mxFORCEINLINE ReferenceCounted& ReferenceCounted::operator = ( const ReferenceCounted& other )
{
	return *this;
}

///
///	ReferenceCountedX - the same as ReferenceCounted,
///	but calls deleteSelf() when the reference count reaches zero.
///
class ReferenceCountedX
{
	mutable AtomicInt	_reference_counter;	//!< Number of references to this object.

public:

	/// deletes this object using whatever heap it was allocated in
	virtual void deleteSelf() { delete( this ); }	// 'delete this' is evil (?)

	/// increases the reference count by 1
	void	Grab() const;
	/// decreases the reference count by 1
	bool	Drop() const;	// ( Returns true if the object has been deleted. )

	ReferenceCountedX& operator = ( const ReferenceCountedX& other );

public:	// Internal.
	INT	getReferenceCount() const;

	// so that property system works
	mxFORCEINLINE U32 GetNumRefs() const
	{
		return static_cast<U32>(this->getReferenceCount());
	}

protected:
			ReferenceCountedX();
	virtual	~ReferenceCountedX();
};

/*================================
		ReferenceCountedX
================================*/

mxFORCEINLINE ReferenceCountedX::ReferenceCountedX()
	: _reference_counter( 0 )
{}

mxFORCEINLINE ReferenceCountedX::~ReferenceCountedX()
{
	mxASSERT( AtomicLoad( _reference_counter ) == 0 );
}

mxFORCEINLINE void ReferenceCountedX::Grab() const
{
	AtomicIncrement( &_reference_counter );
}

mxFORCEINLINE bool ReferenceCountedX::Drop() const
{
	mxASSERT( AtomicLoad( _reference_counter ) > 0 );
	if ( 0 == AtomicDecrement( &_reference_counter ) )
	{
		ReferenceCountedX* nonConstPtr = const_cast<ReferenceCountedX*>( this );
		nonConstPtr->deleteSelf();
		return true;
	}
	return false;
}

mxFORCEINLINE INT ReferenceCountedX::getReferenceCount() const
{
	return AtomicLoad( _reference_counter );
}

mxFORCEINLINE ReferenceCountedX& ReferenceCountedX::operator = ( const ReferenceCountedX& other )
{
	return *this;
}

//------------------------------------------------------------------------------------------------------------------

//
//	Grab( ReferenceCounted* ) - increments the reference count of the given object.
//
mxFORCEINLINE void GRAB( ReferenceCounted* p )
{
	if (PtrToBool( p ))
	{
		p->Grab();
	}
}

//
//	Drop( ReferenceCounted * ) - decrements the reference count of the given object and sets the pointer to null.
//
mxFORCEINLINE void DROP( ReferenceCounted *& p )
{
	if (PtrToBool( p ))
	{
		p->Drop();
		p = nil;
	}
}

//------------------------------------------------------------------------------------------------------------------

//
//	TRefPtr< T > - a pointer to a reference-counted object (aka 'instrusive pointer').
//
template< typename T >	// where T : ReferenceCounted
struct TRefPtr
{
	T *		_ptr;	// The shared reference counted object.

public:
			TRefPtr();
			TRefPtr( T* refCounted );
			TRefPtr( T* refCounted, EDontAddRef );
			TRefPtr( const TRefPtr& other );
			~TRefPtr();

	// Implicit conversions.

			operator T*	() const;
	T &		operator *	() const;
	T *		operator ->	() const;

	bool	operator !	() const;

	// Assignment.

	TRefPtr &	operator = ( T* pObject );
	TRefPtr &	operator = ( const TRefPtr& other );

	// Comparisons.

	bool	operator == ( T* pObject ) const;
	bool	operator != ( T* pObject ) const;
	bool	operator == ( const TRefPtr& other ) const;
	bool	operator != ( const TRefPtr& other ) const;

	mxFORCEINLINE		bool	IsNull() const { return (nil == _ptr); }
	mxFORCEINLINE		bool	IsValid() const { return (nil != _ptr); }

	// Unsafe...
	mxFORCEINLINE		T *		get_ptr()	{ return _ptr; }
	mxFORCEINLINE		T *&	get_ref()	{ return _ptr; }

private:
	void _dbgChecks() { mxCOMPILE_TIME_ASSERT(sizeof(*this) == sizeof(T*)); }
};

template< typename T >
mxFORCEINLINE TRefPtr< T >::TRefPtr()
	: _ptr( nil )
{
	_dbgChecks();
}

template< typename T >
mxFORCEINLINE TRefPtr< T >::TRefPtr( T* refCounted )
	: _ptr( refCounted )
{
	if (PtrToBool( _ptr )) {
		_ptr->Grab();
	}
	_dbgChecks();
}

template< typename T >
mxFORCEINLINE TRefPtr< T >::TRefPtr( T* refCounted, EDontAddRef )
	: _ptr( refCounted )
{
	_dbgChecks();
}

template< typename T >
mxFORCEINLINE TRefPtr< T >::TRefPtr( const TRefPtr& other )
	: _ptr( other._ptr )
{
	if (PtrToBool( _ptr )) {
		_ptr->Grab();
	}
	_dbgChecks();
}

template< typename T >
mxFORCEINLINE TRefPtr< T >::~TRefPtr()
{
	if (PtrToBool( _ptr )) {
		_ptr->Drop();
	}
	_ptr = nil;
}

template< typename T >
mxFORCEINLINE TRefPtr< T >::operator T* () const
{
	return _ptr;
}

template< typename T >
mxFORCEINLINE T & TRefPtr< T >::operator * () const
{	mxASSERT( _ptr );
	return *_ptr;
}

template< typename T >
mxFORCEINLINE T * TRefPtr< T >::operator -> () const
{
	return _ptr;
}

template< typename T >
mxFORCEINLINE bool TRefPtr< T >::operator ! () const
{
	return _ptr == nil;
}

template< typename T >
mxFORCEINLINE TRefPtr< T > & TRefPtr< T >::operator = ( T* pObject )
{
	if ( pObject == nil )
	{
		if (PtrToBool( _ptr )) {
			_ptr->Drop();
			_ptr = nil;
		}
		return *this;
	}
	if ( _ptr != pObject )
	{
		if (PtrToBool( _ptr )) {
			_ptr->Drop();
		}
		_ptr = pObject;
		_ptr->Grab();
		return *this;
	}
	return *this;
}

template< typename T >
mxFORCEINLINE TRefPtr< T > & TRefPtr< T >::operator = ( const TRefPtr& other )
{
	return ( *this = other._ptr );
}

template< typename T >
mxFORCEINLINE bool TRefPtr< T >::operator == ( T* pObject ) const
{
	return _ptr == pObject;
}

template< typename T >
mxFORCEINLINE bool TRefPtr< T >::operator != ( T* pObject ) const
{
	return _ptr != pObject;
}

template< typename T >
mxFORCEINLINE bool TRefPtr< T >::operator == ( const TRefPtr& other ) const
{
	return _ptr == other._ptr;
}

template< typename T >
mxFORCEINLINE bool TRefPtr< T >::operator != ( const TRefPtr& other ) const
{
	return _ptr != other._ptr;
}

/*
----------------------------------------------------------
	TRefCounted< T >
----------------------------------------------------------
*/
template< class TYPE >
class TRefCounted: public ReferenceCounted
{
public:
	typedef TRefPtr< TYPE >	Ref;
};

//------------------------------------------------------------------------------------------------------------------

template< class TYPE >
class RefPtrDropUtil {
public:
	inline RefPtrDropUtil()
	{}

	inline void operator () ( TRefPtr<TYPE>& ptr ) {
		ptr = nil;
	}
	inline void operator () ( ReferenceCounted* ptr ) {
		ptr->Drop();
	}
	inline void operator () ( ReferenceCountedX* ptr ) {
		ptr->Drop();
	}
};

template< class TYPE >
class RefPtrDropUtilX
{
	UINT numDecrements;

public:
	inline RefPtrDropUtilX( UINT numDecrements = 1 )
		: numDecrements(numDecrements)
	{}

	inline void operator () ( TRefPtr<TYPE>& ptr ) {
		for( UINT i = 0; i < numDecrements; i++ ) {
			ptr = nil;
		}
	}
	inline void operator () ( ReferenceCounted* ptr ) {
		for( UINT i = 0; i < numDecrements; i++ ) {
			ptr->Drop();
		}
	}
	inline void operator () ( ReferenceCountedX* ptr ) {
		for( UINT i = 0; i < numDecrements; i++ ) {
			ptr->Drop();
		}
	}
};

template< typename T >
inline const T& GetConstObjectReference( const TRefPtr< T >& o )
{
	return *o;
}
