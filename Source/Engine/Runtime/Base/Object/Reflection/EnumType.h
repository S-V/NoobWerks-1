// @TODO: enums inside namespaces are not supported!
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/Reflection.h>

/*
-------------------------------------------------------------------------
	MetaEnum
-------------------------------------------------------------------------
*/
struct MetaEnum: public TbMetaType
{
	/// The largest supported storage type.
	typedef U32	ValueT;	/// Enum/Flags value

	template< typename TYPE >
	struct Converter
	{
		mxFORCEINLINE static ValueT Get( const void* ptr )
		{
			return *static_cast< const TYPE* >( ptr );
		}
		mxFORCEINLINE static void Set( void* dst, ValueT newValue )
		{
			*static_cast< TYPE* >( dst ) = static_cast< TYPE >( newValue );
		}
	};

	struct Item
	{
		const char* name;	//!< name of the value in the code
		ValueT		value;
	};
	struct ItemList
	{
		const Item* items;
		const UINT	count;
	public:
		UINT GetItemIndexByValue( const ValueT _itemValue ) const;
		UINT GetItemIndexByString( const char* _itemName ) const;	// NOTE: very slow!
	};

	typedef ValueT GetterF( const void* _ptr );
	typedef void SetterF( void* _dst, ValueT _newValue );

	const ItemList	m_items;	//!<
	GetterF * const	m_getter;
	SetterF * const	m_setter;
	//bool m_caseSensitive;

public:
	inline MetaEnum(
		const  ETypeKind _kind	//!< can be ETypeKind::Type_Enum or ETypeKind::Type_Flags
		, const char* typeName
		, const ItemList& _items
		, GetterF* const _getter
		, SetterF* const _setter
		, const TbTypeSizeInfo& info
		)
		: TbMetaType( _kind, typeName, info )
		, m_items( _items )
		, m_getter( _getter )
		, m_setter( _setter )
	{}

	const char* FindString( const ValueT itemValue ) const;
	const char* GetString( const ValueT itemValue ) const;

	ValueT FindValue( const char* _name ) const;
	ERet FindValueSafe( const char* _name, ValueT *_result ) const;

	mxFORCEINLINE ValueT GetValue( const void* _ptr ) const
	{
		return (*m_getter)( _ptr );
	}
	mxFORCEINLINE void SetValue( void *_dst, ValueT _newValue ) const
	{
		(*m_setter)( _dst, _newValue );
	}
};

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- should be placed into header files
//!=- ENUM - name of enumeration
//!=- STORAGE - type of storage
//!=- TYPEDEF - type of TEnum<ENUM,STORAGE>
//
#define mxDECLARE_ENUM( ENUM, STORAGE, TYPEDEF )\
	typedef TEnum< enum ENUM, STORAGE > TYPEDEF;\
	inline const MetaEnum& PP_JOIN( TYPEDEF, _Type )()\
	{\
		extern const MetaEnum::ItemList PP_JOIN(TYPEDEF,_members)();\
		static MetaEnum s_enumType(\
							ETypeKind::Type_Enum,\
							mxEXTRACT_TYPE_NAME(ENUM),\
							PP_JOIN(TYPEDEF,_members)(),\
							&(MetaEnum::Converter< STORAGE >::Get),\
							&(MetaEnum::Converter< STORAGE >::Set),\
							TbTypeSizeInfo::For_Type< STORAGE >()\
						);\
		return s_enumType;\
	}\
	template<>\
	struct TypeDeducer< TYPEDEF >\
	{\
		mxSTATIC_ASSERT( sizeof(TYPEDEF) <= sizeof(U32) );\
		static inline const TbMetaType& GetType()\
		{\
			return PP_JOIN( TYPEDEF, _Type )();\
		}\
		static inline ETypeKind GetTypeKind()\
		{\
			return ETypeKind::Type_Enum;\
		}\
	};


//!----------------------------------------------------------------------
#define mxGET_ENUM_TYPE( ENUM_NAME )	PP_JOIN( ENUM_NAME, _Type )()


//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- should be placed into source files
//
#define mxBEGIN_REFLECT_ENUM( TYPEDEF )\
	const MetaEnum::ItemList PP_JOIN(TYPEDEF,_members)() {\
		static MetaEnum::Item items[] = {\

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxREFLECT_ENUM_ITEM( NAME, VALUE )\
		{\
			mxEXTRACT_NAME(NAME),\
			VALUE\
		}

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxREFLECT_ENUM_ITEM1( X )\
		{\
			mxEXTRACT_NAME( X ),\
			X\
		}

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxEND_REFLECT_ENUM\
		};\
		const MetaEnum::ItemList result = { items, mxCOUNT_OF(items) };\
		return result;\
	}

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- should be placed into header files
//!=- ENUM - name of enumeration
//!=- STORAGE - type of storage
//!=- TYPEDEF - type of TEnum<ENUM,STORAGE>
//
#define mxBEGIN_ENUM( ENUM, STORAGE, TYPEDEF )\
	mxDECLARE_ENUM( ENUM, STORAGE, TYPEDEF )

//!----------------------------------------------------------------------




template< typename ENUM_TYPE, typename STORAGE >
ERet GetEnumFromString(
					   TEnum< ENUM_TYPE, STORAGE > *enum_
					   , const char* enum_value_string
					   )
{
	const TbMetaType& metaType = TypeDeducer< TEnum< ENUM_TYPE, STORAGE > >::GetType();
	const MetaEnum& metaEnum = metaType.UpCast<MetaEnum>();

	MetaEnum::ValueT	enumValue;
	mxDO(metaEnum.FindValueSafe( enum_value_string, &enumValue ));

	*enum_ = static_cast< ENUM_TYPE >( enumValue );

	return ALL_OK;
}
