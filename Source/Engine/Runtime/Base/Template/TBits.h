#pragma once




/// ToDo: http://molecularmusings.wordpress.com/2011/08/23/flags-on-steroids/#more-125
///
///	TBits - is a simple class representing a bitmask.
///
template< typename TYPE, typename STORAGE = U32 >
struct TBits
{
	typedef STORAGE ValueType;

	STORAGE	raw_value;

	inline TBits()
	{}
	inline TBits( TYPE e )
	{
		raw_value = static_cast< STORAGE >( e );
	}
	inline explicit TBits( STORAGE value )
		: raw_value( value )
	{}

	inline operator TYPE () const
	{
		return static_cast< TYPE >( raw_value );
	}
	inline void operator = ( TYPE e )
	{
		raw_value = static_cast< STORAGE >( e );
	}

	inline bool operator == ( TYPE e ) const
	{
		return (raw_value == static_cast< STORAGE >( e ));
	}
	inline bool operator != ( TYPE e ) const
	{
		return (raw_value != static_cast< STORAGE >( e ));
	}

	inline TBits & operator &= ( TBits other )
	{
		raw_value &= other.raw_value;
		return *this;
	}
	inline TBits & operator ^= ( TBits other )
	{
		raw_value ^= other.raw_value;
		return *this;
	}
	inline TBits & operator |= ( TBits other )
	{
		raw_value |= other.raw_value;
		return *this;
	}
	inline TBits operator ~ () const
	{
		return TBits( ~raw_value );
	}
	inline bool operator ! () const
	{
		return !raw_value;
	}

	inline void clear()
	{
		raw_value = 0;
	}
	inline void setAll( STORAGE s )
	{
		raw_value = s;
	}
	inline void OrWith( STORAGE s )
	{
		raw_value |= s;
	}
	inline void XorWith( STORAGE s )
	{
		raw_value ^= s;
	}
	inline void AndWith( STORAGE s )
	{
		raw_value &= s;
	}
	inline void SetWithMask( STORAGE s, STORAGE mask )
	{
		raw_value = (raw_value & ~mask) | (s & mask);
	}

	inline STORAGE Get() const
	{
		return raw_value;
	}
	inline STORAGE Get( STORAGE mask ) const
	{
		return raw_value & mask;
	}
	inline bool AnyIsSet( STORAGE mask ) const
	{
		return (raw_value & mask) != 0;
	}
	inline bool AllAreSet( STORAGE mask ) const
	{
		return (raw_value & mask) == mask;
	}
};
