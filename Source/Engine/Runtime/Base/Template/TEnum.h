#pragma once


///
///	TEnum< enumName, storage > - a wrapper to store an enum with explicit size.
///
template< typename ENUM, typename STORAGE = U32 >
struct TEnum
{
	STORAGE		raw_value;

public:
	typedef ENUM					ENUM_TYPE;
	typedef TEnum< ENUM, STORAGE >	THIS_TYPE;

	inline TEnum()
	{}

	inline TEnum( ENUM e )
	{
		raw_value = static_cast< STORAGE >( e );
	}

	inline operator ENUM () const
	{
		return static_cast< ENUM >( raw_value );
	}

	inline void operator = ( ENUM e )
	{
		raw_value = static_cast< STORAGE >( e );
	}

	inline bool operator == ( ENUM e ) const
	{
		return (raw_value == static_cast< STORAGE >( e ));
	}
	inline bool operator != ( ENUM e ) const
	{
		return (raw_value != static_cast< STORAGE >( e ));
	}

	inline STORAGE AsInt() const
	{
		return raw_value;
	}

	// ??? overload 'forbidden' operators like '+', '-', '/', '<<', etc
	// to catch programming errors early?
	inline friend AWriter& operator << ( AWriter& file, const THIS_TYPE& o )
	{
		file << o.raw_value;
		return file;
	}
	inline friend AReader& operator >> ( AReader& file, THIS_TYPE& o )
	{
		file >> o.raw_value;
		return file;
	}
	inline friend mxArchive& operator && ( mxArchive & serializer, THIS_TYPE & o )
	{
		serializer && o.raw_value;
		return serializer;
	}
};

#if MX_DEBUG

	// view names in debugger
	#define mxDECLARE_ENUM_TYPE( enumName, storage, newType )	\
		typedef enumName	newType;

#else // !MX_DEBUG

	#define mxDECLARE_ENUM_TYPE( enumName, storage, newType )	\
		typedef TEnum< enumName, storage >	newType;

#endif
