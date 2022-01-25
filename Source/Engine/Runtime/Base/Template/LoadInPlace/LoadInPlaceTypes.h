/*
=============================================================================
	File:	LoadInPlace.h
	Desc:	Relocatable / Relative data structure.
=============================================================================
*/
#pragma once

mxSTOLEN("Based on Eight engine by Brooke Hodgeman")

/// Represents a relative offset of some structure in memory (aka "self-based pointer").
/// NOTE: offsets must not be created on stack!
template
<
	typename ObjectType,		// type of the pointed object
	typename OffsetType = I32	//!< NOTE: offset must be signed so that we can go in both directions!
>
struct TOffset
{
	OffsetType		_offset;	//!< (signed) byte offset of the structure relative to 'this'
	// null pointer is represented by a zero offset ('offset to self')
public:
	// no ctor so that this struct can be put into unions
	//mxFORCEINLINE TOffset()
	//{
	//	_offset = 0;
	//}

	mxFORCEINLINE bool		SetNil() const		{ _offset = 0; }

	mxFORCEINLINE bool		IsValid() const		{ return _offset != 0; }
	mxFORCEINLINE bool		isNull() const		{ return _offset == 0; }

	mxFORCEINLINE const ObjectType*	ptr() const	{ return (ObjectType*) (((BYTE*)this) + _offset); }
	mxFORCEINLINE ObjectType*		ptr()		{ return (ObjectType*) (((BYTE*)this) + _offset); }

	mxFORCEINLINE const ObjectType* nullablePtr() const { return _offset ? this->ptr() : 0; }
	mxFORCEINLINE 	    ObjectType* nullablePtr()       { return _offset ? this->ptr() : 0; }


	mxFORCEINLINE const ObjectType*	operator->() const
	{
		mxASSERT(_offset != 0);	// must not be NULL
		return this->ptr();
	}
	mxFORCEINLINE ObjectType* operator->()
	{
		mxASSERT(_offset != 0);	// must not be NULL
		return this->ptr();
	}

	mxFORCEINLINE const ObjectType&	operator *() const	{ return *this->ptr(); }
	mxFORCEINLINE ObjectType&		operator *()		{ return *this->ptr(); }

	//mxFORCEINLINE bool		operator ! () const	{ return !_offset; }

	//mxFORCEINLINE bool operator == ( const TOffset& other ) const
	//{
	//	return this->_offset == other->_offset;
	//}
	//mxFORCEINLINE bool operator != ( const TOffset& other ) const
	//{
	//	return this->_offset != other->_offset;
	//}

	mxFORCEINLINE TOffset& operator = ( ObjectType* o )
	{
		_offset = PtrToBool(o) ? ((BYTE*)o - (BYTE*)this) : 0;
		return *this;
	}

	void SetNil()
	{
		_offset = 0;
	}

	ERet SetupFromRawPtr( const void* pointedObject )
	{
		mxASSERT(IsAlignedBy( pointedObject, mxALIGNOF( ObjectType ) ));

		const ptrdiff_t relativeOffset = ((BYTE*)pointedObject - (BYTE*)this);

		const U64 unsignedRange = U64(1) << BYTES_TO_BITS(sizeof(_offset));
		const U64 signedRange = unsignedRange / 2;

		const bool fitsWithinRange = relativeOffset < signedRange;

		if( fitsWithinRange ) {
			_offset = relativeOffset;
			return ALL_OK;
		} else {
			return ERR_BUFFER_TOO_SMALL;
		}
	}
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
mxTEMP
//template<class T> struct TArrayOffset
//{
//	const T* Ptr() const { return (T*)(((BYTE*)&_offset) + _offset); }
//		  T* Ptr()       { return (T*)(((BYTE*)&_offset) + _offset); }
//	const T& operator *() const { return *Ptr(); }
//		  T& operator *()       { return *Ptr(); }
//	const T& operator[](unsigned i) const { return Ptr()[i]; }
//		  T& operator[](unsigned i)       { return Ptr()[i]; }
//	unsigned DataSize(unsigned count) const { return sizeof(T) * count; }
//	TArrayOffset& operator=(T* p) { _offset = (I32)( ((BYTE*)p)-((BYTE*)this) ); return *this; }
////private:
//	I32 _offset;
//};


//template< class T >
//struct TLipArray
//{
//	const T* begin() const { return (T*)(&count + 1); }
//		  T* begin()       { return (T*)(&count + 1); }
//	const T* end  () const { return Begin()+count; }
//	      T* end  ()       { return Begin()+count; }
//	const T& operator[](unsigned i) const { eiASSERT(i < count); return Begin()[i]; }
//		  T& operator[](unsigned i)       { eiASSERT(i < count); return Begin()[i]; }
//	unsigned DataSize() const { return sizeof(T) * count; }
//	unsigned Bytes()    const { return sizeof(List) + DataSize(); }
////private:
//	u32 count;
//};


template
<
	class T,
	typename OffsetType = U32
>
struct TRelArray: TArrayMixin< T, TRelArray<T,OffsetType> >
{
	/// relative offset of the data starting from the beginning of this struct
	OffsetType	offset;

	///
	OffsetType	count;

public:
	mxFORCEINLINE const T* begin() const { return (T*) ( ((char*)this) + offset ); }
	mxFORCEINLINE 	    T* begin()       { return (T*) ( ((char*)this) + offset ); }
	mxFORCEINLINE const T* end  () const { return begin()+count; }
	mxFORCEINLINE       T* end  ()       { return begin()+count; }

	mxFORCEINLINE const T& operator[](unsigned i) const
	{
		mxASSERT(i < count);
		return begin()[i];
	}

	mxFORCEINLINE T& operator[](unsigned i)
	{
		mxASSERT(i < count);
		return begin()[i];
	}

	mxFORCEINLINE unsigned AllocatedSize() const { return sizeof(T) * count; }
	mxFORCEINLINE unsigned TotalSize() const { return sizeof(*this) + AllocatedSize(); }

public:	// TArrayMixin

	mxFORCEINLINE U32 num() const { return count; }

	mxFORCEINLINE T * raw()				{ return (T*) ( ((char*)this) + offset ); }
	mxFORCEINLINE const T* raw() const	{ return (T*) ( ((char*)this) + offset ); }
};
