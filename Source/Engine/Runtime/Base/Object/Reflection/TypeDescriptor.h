/*
=============================================================================
	File:	TypeDescriptor.h
	Desc:	classes for representing C++ types
	ToDo:	don't store type names in release mode;
			make reflection structures self-reflectable?
			could possibly get away with shorts/bytes
			for storing size, alignment, etc.
=============================================================================
*/
#pragma once

// for compile-time string hashing
#include <Base/Util/StaticStringHash.h>

/// Defines.
/// Use idTech4-style RTTI (IsKindOf() check consists of two comparisons).
/// NOTE: increases program start up time.
#define MX_USE_FAST_RTTI	(1)

// Forward declarations.
class CStruct;
class AObject;

class TbMetaType;
class TbMetaClass;
class SClassId;
class mxBlobType;
class MetaArray;
class MetaDictionary;
class MetaPointer;
class mxUserPointerType;
class MetaAssetReference;

class TbTypeSizeInfo;

/*
-------------------------------------------------------------------------
	TypeGUID
-------------------------------------------------------------------------
*/

/// Unique type identifier, it's usually a hash of a class name.
/// (aka 'static, persistent class GUID', it stays the same when saving/loading).
/// The base class for POD structs has zero index.
/// The base class for virtual classes has index 1.
///
typedef UINT32 TypeGUID;

/// invalid type GUID
enum { mxNULL_TYPE_ID	= ((TypeGUID)0) };

mxIMPLEMENT_FUNCTION_READ_SINGLE(TypeGUID,ReadTypeID);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(TypeGUID,WriteTypeID);


//// Dynamic (volatile) type identifier,
//// assigned at program startup.
//// It's mainly used to perform fast RTTI checks
//// (without visiting entire hierarchy).
////
//typedef UINT16 TypeGUID;
//typedef const UINT16 const TypeGUID&;
//
//// invalid type ID
//enum { mxNULL_TYPE_ID	= ((TypeGUID)-1) };



///!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/// macros for extracting name string
///
#define mxEXTRACT_NAME( X )		TO_STR( X )

///!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/// macros for extracting type name string and type id at compile-time
///
#define mxEXTRACT_TYPE_NAME( TYPENAME )		mxEXTRACT_NAME( TYPENAME )

///!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/// uses a compile-time string hashing function
///
#define mxEXTRACT_TYPE_GUID( TYPENAME )		GetStaticStringHash( TO_STR( TYPENAME ) )

///!----------------------------------------------------------------------

/// should really be ptrdiff_t/size_t, but an unsigned int will suffice.
/// a 64bit offset would bloat the size for almost all uses
/// Unsigned means they can only point in one direction, which typically is forward (towards a higher memory location).
/// Any backwards offsets will be explicitly marked as such.
typedef UINT32 MetaOffset;

/// should really be size_t, but an unsigned int should suffice
typedef UINT32 MetaSize;


/**
*	ETypeKind
*
*	An enumeration of all possible kinds of types.
*
*	NOTE: the order is important (optimized for range checks) !
*	NOTE: if you change this, make sure to update
*		ETypeKind_Is_Bitwise_Serializable()
*
*	Side note: this enum describes all types of 'serialization atoms',
*	their representation/storage format is wildly different
*	between binary (raw bytes) and buffer form (human-readable version)
*/
enum ETypeKind
{
	Type_Unknown = 0,	/// bad type, means that errors occurred

	/// special type - zero size, zero alignment - was introduced for script binding
	Type_Void,

	/// Plain old data types (bitwise serializable)

	/// Primitive (fundamental) types

	Type_Integer,	/// integer
	Type_Float,		/// floating point

	Type_Bool,		/// native 'bool' type (usually, 1 byte in size)

	/// Enumerations
	Type_Enum,

	/// Bit masks
	Type_Flags,


	// Complex types

	/// Built-in complex types
	Type_String,	/// dynamic String (slow, bloated & unpredictable)


	/// Aggregate types
	Type_Class,		/// C++ struct or class (only single inheritance is supported)


	/// References
	Type_Pointer,			/// generic pointer, but only pointers to POD-type descendants of CStruct are supported (raw pointers -> unclear ownership)
	Type_AssetReference,	/// asset/resource reference (TResPtr< ASSET_TYPE >)
	Type_RelativePointer,	/// 32-bit relative offset (self-based/relocatable/frozen pointer for in-place loading of precompiled blobs)

	/// Special types:
	Type_ClassId,	/// pointer to TbMetaClass at run-time, ClassGUID when serialized
	Type_AssetId,	/// (unique) asset/resource/object identifier

	Type_UserData,	/// pointer to internal engine object (e.g. render technique, pass or vertex layout info)
	Type_Blob,		/// memory buffer with a predefined layout

	/// Array types:
	Type_Array,		/// generic array of values

	/// Key-Value Dictionary/Map types:
	Type_Dictionary,

	Type_MAX	/// Must be last! Don't use!
};

#if MX_DEVELOPER
	const char* ETypeKind_To_Chars( ETypeKind typeKind );
#endif // MX_DEVELOPER

mxDECLARE_ENUM_TYPE( ETypeKind, UINT8, TypeKind );

/// returns true if the type is bitwise-copyable (can be serialized via reading/writing bytes)
inline bool Type_Is_Bitwise_Serializable( const ETypeKind inTypeKind )
{
	return inTypeKind >= ETypeKind::Type_Integer
		&& inTypeKind <= ETypeKind::Type_Flags
		;
}

///
/// contains parameters for initializing the TbMetaType structure
///
struct TbTypeSizeInfo
{
	UINT32	size;	/// size of object of this class, in bytes
	UINT32	alignment;	/// the alignment of this data type (zero for abstract classes)
public:
	inline TbTypeSizeInfo(ENoInit)
	{}
	inline TbTypeSizeInfo()
	{
		size = 0;
		alignment = 0;
	}
	template< typename TYPE >
	static inline TbTypeSizeInfo For_Type()
	{
		TbTypeSizeInfo	typeInfo(_NoInit);

		typeInfo.size = sizeof(TYPE);
		typeInfo.alignment = mxALIGNOF( TYPE );

		return typeInfo;
	}
};

/*
*-------------------------------------------------------------------------=
*	TbMetaType
*
*	this is the base class for providing information about C++ types
*
*	NOTE: everything should be statically allocated
*-------------------------------------------------------------------------=
*/
struct TbMetaType
{
	const char *	m_name;	//!< pointer to static string - name of this type
	const MetaSize	m_size;	//!< size of an object/instance of this type, in bytes
	const TypeKind	m_kind;	//!< class of this type
	const UINT8		m_align;//!< the alignment of this data type

public:
	inline TbMetaType( const ETypeKind typeKind, const char* typeName, const TbTypeSizeInfo& size_info )
		: m_name( typeName )
		, m_kind( typeKind )
		, m_size( size_info.size )
		, m_align( size_info.alignment )
	{}

	template< class DERIVED >
	inline const DERIVED& UpCast() const
	{
		const DERIVED* pDERIVED = checked_cast< const DERIVED* >( this );
		return *pDERIVED;
	}

	inline const char* GetTypeName() const { return m_name; }

	bool IsClass() const { return m_kind == Type_Class; }
	bool IsArray() const { return m_kind == Type_Array; }
	bool IsPointer() const { return m_kind == Type_Pointer; }

protected:
	virtual ~TbMetaType()
	{}
};

/// helper class for extracting type information
/// (via partial template specialization)
///
template< typename TYPE >
struct TypeDeducer
{
	static inline ETypeKind GetTypeKind()
	{
		// Compile-time assert: Type not implemented
		Error__Failed_To_Deduce_Type_Info;
		return ETypeKind::Type_Unknown;
	}
	static inline const TbMetaType& GetType()
	{
		//// Compile-time assert: Type not implemented
		//Error__Failed_To_Deduce_Type_Info;
		//return *(TbMetaType*) nil;

		// by default, assume it's a struct
		return TYPE::metaClass();
	}
	static inline const TbMetaClass& GetClass()
	{
		// by default, assume it's a struct
		return TYPE::metaClass();
	}
};

/// a templated function to get the type descriptor for the given type
///
template< typename TYPE >
inline const TbMetaType& T_DeduceTypeInfo()
{
	return TypeDeducer< TYPE >::GetType();
}

template< typename TYPE >
inline const TbMetaType& T_DeduceTypeInfo( const TYPE& )
{
	return T_DeduceTypeInfo< TYPE >();
}

template< typename TYPE >
inline ETypeKind T_DeduceTypeKind()
{
	return TypeDeducer< TYPE >::GetTypeKind();
}

/// a templated function to get the class descriptor for the given type
///
template< typename CLASS >
inline const TbMetaClass& T_DeduceClass()
{
	return TypeDeducer< CLASS >::GetClass();
}

#define mxTYPE_OF( VALUE )	T_DeduceTypeInfo( (VALUE) )
#define mxCLASS_OF( VALUE )	((VALUE).metaClass())



/*
-------------------------------------------------------------------------
	VoidType
-------------------------------------------------------------------------
*/
struct VoidType: public TbMetaType
{
public:
	inline VoidType()
		: TbMetaType( ETypeKind::Type_Void, "void", TbTypeSizeInfo() )
	{}
};

template<>
struct TypeDeducer< void >
{
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Void;
	}
	static inline const TbMetaType& GetType()
	{
		static VoidType s_type;
		return s_type;
	}
};

/*
-------------------------------------------------------------------------
	mxBuiltInType

	represents a built-in type
-------------------------------------------------------------------------
*/
template< typename TYPE >
struct mxBuiltInType: public TbMetaType
{
public:
	inline mxBuiltInType( const ETypeKind typeKind, const char* typeName )
		: TbMetaType( typeKind, typeName, TbTypeSizeInfo::For_Type< TYPE >() )
	{}
};

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
#define mxDECLARE_BUILTIN_TYPE( TYPE, KIND )\
template<>\
struct TypeDeducer< TYPE >\
{\
	static inline ETypeKind GetTypeKind()\
	{\
		return KIND;\
	}\
	static inline const TbMetaType& GetType()\
	{\
		static mxBuiltInType< TYPE >	type( KIND, mxEXTRACT_TYPE_NAME( TYPE ) );\
		return type;\
	}\
};
//!----------------------------------------------------------------------

mxDECLARE_BUILTIN_TYPE( char,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( INT8,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( UINT8, ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( INT16,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( UINT16, ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( INT32,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( UINT32, ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( INT64,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( UINT64, ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( long,	ETypeKind::Type_Integer );
mxDECLARE_BUILTIN_TYPE( float,	ETypeKind::Type_Float );
mxDECLARE_BUILTIN_TYPE( double, ETypeKind::Type_Float );
mxDECLARE_BUILTIN_TYPE( bool,	ETypeKind::Type_Bool );

// windows crap
mxDECLARE_BUILTIN_TYPE( DWORD,	ETypeKind::Type_Integer );

//#undef mxDECLARE_BUILTIN_TYPE




//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// this is used to serialize persistent handles
//
#define mxREFLECT_AS_BUILT_IN_INTEGER( INTEGER_TYPE )\
template<>\
struct TypeDeducer< INTEGER_TYPE >\
{\
	static inline ETypeKind GetTypeKind()\
	{\
		return ETypeKind::Type_Integer;\
	}\
	static inline const TbMetaType& GetType()\
	{\
		static mxBuiltInType< INTEGER_TYPE >	type(\
			ETypeKind::Type_Integer,\
			mxEXTRACT_TYPE_NAME( INTEGER_TYPE )\
		);\
		return type;\
	}\
};
//!----------------------------------------------------------------------

#define mxREFLECT_TYPE_AS_STRUCT( ALIAS, BASE )\
	template<>\
	struct TypeDeducer< ALIAS >\
	{\
		static inline ETypeKind GetTypeKind()\
		{\
			return ETypeKind::Type_Class;\
		}\
		static inline const TbMetaType& GetType()\
		{\
			return TypeDeducer< BASE >::GetType();\
		}\
		static inline const TbMetaClass& GetClass()\
		{\
			return TypeDeducer< BASE >::GetClass();\
		}\
	};

//!----------------------------------------------------------------------
/// used to register structs generated via MAKE_ALIAS
#define mxREFLECT_HANDLE( HANDLE )\
	template<>\
	struct TypeDeducer< HANDLE >\
	{\
		static inline ETypeKind GetTypeKind()\
		{\
			return ETypeKind::Type_Integer;\
		}\
		static inline const TbMetaType& GetType()\
		{\
			static mxBuiltInType< HANDLE::TYPE >	s_type( ETypeKind::Type_Integer, mxEXTRACT_TYPE_NAME( HANDLE ) );\
			return s_type;\
		}\
	};
