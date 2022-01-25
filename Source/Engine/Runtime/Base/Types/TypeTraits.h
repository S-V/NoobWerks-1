#pragma once

#include <Base/IO/StreamIO.h>


//	Type traits.
//TODO: type traits from Qt?
//
//	TypeTrait<T> - used for carrying compile-time type information.
//
//	Can be specialized for custom types.
//
//	NOTE: these templates could also be done with enums.
//
template< typename TYPE >
struct TypeTrait
{
#if 0 // this is not needed for now; and all memory managers allocate aligned memory
	static bool NeedsDestructor()
	{
		return true;
	}
	static bool NeedsDefaultConstructor()
	{
		return true;
	}
	static bool NeedsCopyConstructor()
	{
		return true;
	}
	// returns true if this class is copied via "operator =" (i.e. it's not bitwise copyable via memcpy())
	static bool NeedsAssignment()
	{
		return true;
	}
	static size_t GetAlignment()
	{
		return EFFICIENT_ALIGNMENT;
	}
#else
	// Plain Old Data types don't need constructors, destructors and assignment operators being called.
	// They can be memset and memcopied.
	enum
	{
		IsPlainOldDataType = false
	};
#endif
};

/// std::is_standard_layout<> 
#define mxDECLARE_POD_TYPE( TYPE )\
	template< > struct TypeTrait< TYPE > {\
		enum { IsPlainOldDataType = true };\
	};\
	template< > struct TypeTrait< const TYPE > {\
		enum { IsPlainOldDataType = true };\
	};\
	inline AWriter& operator << ( AWriter &outStream, const TYPE& o )\
	{\
		outStream.Write( &o, sizeof(TYPE) );\
		return outStream;\
	}\
	inline AReader& operator >> ( AReader& inStream, TYPE &o )\
	{\
		inStream.Read( &o, sizeof(TYPE) );\
		return inStream;\
	}\
	template< class SERIALIZER >\
	inline SERIALIZER& operator & ( SERIALIZER & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\
	inline mxArchive& Serialize( mxArchive & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\
	inline mxArchive& operator && ( mxArchive & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\
	inline void F_UpdateMemoryStats( MemStatsCollector& stats, const TYPE& o )\
	{\
		stats.staticMem += sizeof(o);\
	}

#define mxIMPLEMENT_SERIALIZABLE_AS_POD( TYPE )\
	friend inline AWriter& operator << ( AWriter &outStream, const TYPE& o )\
	{\
		outStream.Write( &o, sizeof(TYPE) );\
		return outStream;\
	}\
	friend inline AReader& operator >> ( AReader& inStream, TYPE &o )\
	{\
		inStream.Read( &o, sizeof(TYPE) );\
		return inStream;\
	}\
	template< class SERIALIZER >\
	friend inline SERIALIZER& operator & ( SERIALIZER & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\
	friend inline mxArchive& Serialize( mxArchive & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\
	friend inline mxArchive& operator && ( mxArchive & serializer, TYPE & o )\
	{\
		serializer.SerializeMemory( &o, sizeof(TYPE) );\
		return serializer;\
	}\


mxDECLARE_POD_TYPE(ANSICHAR);
mxDECLARE_POD_TYPE(UNICODECHAR);
mxDECLARE_POD_TYPE(I8);
mxDECLARE_POD_TYPE(U8);
mxDECLARE_POD_TYPE(I16);
mxDECLARE_POD_TYPE(U16);
mxDECLARE_POD_TYPE(I32);
mxDECLARE_POD_TYPE(U32);
mxDECLARE_POD_TYPE(I64);
mxDECLARE_POD_TYPE(U64);
mxDECLARE_POD_TYPE(LONG);
mxDECLARE_POD_TYPE(ULONG);
mxDECLARE_POD_TYPE(FLOAT);
mxDECLARE_POD_TYPE(DOUBLE);




// Pointers are pod types
template< typename T >
struct TypeTrait<T*>
{
	enum { IsPlainOldDataType = true };
};

// Arrays
template< typename T, unsigned long N >
struct TypeTrait< T[N] >
	: public TypeTrait< T >
{};


//
//	Hash traits.
//

//
//	THashTrait<T> - used for generating hash values.
//
//	Should be specialized for custom types.
//
template< typename KEY >
struct THashTrait
{
	static inline U32 ComputeHash32( const KEY& key )
	{
		return mxGetHashCode( key );
	}
};

//
//	TEqualsTrait - used for comparing keys in hash tables.
//
//	Must be specialized for custom types.
//
template< typename key >
struct TEqualsTrait
{
	static inline bool Equals( const key& a, const key& b )
	{
		return (a == b);
	}
};
template< typename T >
struct THashTrait< T* >
{
	static inline U32 ComputeHash32( const void* pointer )
	{
		return static_cast< UINT >( mxPointerHash( pointer ) );
	}
};
template<>
struct THashTrait< I32 >
{
	static inline U32 ComputeHash32( I32 k )
	{
		return static_cast< UINT >( k );
	}
};
template<>
struct THashTrait< U32 >
{
	static inline U32 ComputeHash32( U32 k )
	{
		return static_cast< UINT >( k );
	}
};
template<>
struct THashTrait< I64 >
{
	static inline U32 ComputeHash32( I64 k )
	{
		return static_cast< UINT >( k );
	}
};
template<>
struct THashTrait< U64 >
{
	static inline U32 ComputeHash32( U64 k )
	{
		return static_cast< UINT >( k );
	}
};
