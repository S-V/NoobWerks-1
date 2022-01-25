#pragma once


///
///	TBool - a boolean type with explicit storage type.
///
template< typename STORAGE = U32 >
class TBool
{
	STORAGE		_value;

public:
	TBool()
	{}

	inline	explicit TBool( bool value )
		: _value( value )
	{}
	
	// Casting to 'bool'.

	inline operator bool () const
	{
		return ( _value != 0 );
	}

	// Assignment.

	inline TBool & operator = ( bool value )
	{
		_value = value;
		return *this;
	}
	inline TBool & operator = ( const TBool other )
	{
		_value = other._value;
		return *this;
	}

	// Comparison.

	inline TBool operator == ( bool value ) const
	{
		return (_value == static_cast< STORAGE >( value ));
	}
	inline TBool operator != ( bool value ) const
	{
		return (_value != static_cast< STORAGE >( value ));
	}

	inline TBool operator == ( const TBool other ) const
	{
		return _value == other._value;
	}
	inline TBool operator != ( const TBool other ) const
	{
		return _value != other._value;
	}

	// TODO: overload 'forbidden' operators like '+', '-', '/', '<<', etc
	// to catch programming errors early?
	//TBool operator + (const TBool& other) const
	//{ StaticAssert( false && "Invalid operation" ); }
};
