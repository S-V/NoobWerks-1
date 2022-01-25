#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/Reflection/EnumType.h>

UINT MetaEnum::ItemList::GetItemIndexByValue( const ValueT _itemValue ) const
{
	for( UINT itemIndex = 0; itemIndex < count; itemIndex++ )
	{
		const Item& item = items[ itemIndex ];
		if( item.value == _itemValue ) {
			return itemIndex;
		}
	}
	return INDEX_NONE;
}

UINT MetaEnum::ItemList::GetItemIndexByString( const char* _itemName ) const
{
	mxASSERT_PTR(_itemName);
	for( UINT itemIndex = 0; itemIndex < count; itemIndex++ )
	{
		const Item& item = items[ itemIndex ];
		if( stricmp( item.name, _itemName ) == 0 ) {
			return itemIndex;
		}
	}
	//ptWARN("no enum member named '%s'\n",name);
	return INDEX_NONE;
}

const char* MetaEnum::FindString( const ValueT itemValue ) const
{
	const UINT enumItemIndex = m_items.GetItemIndexByValue( itemValue );
	chkRET_X_IF_NOT(enumItemIndex != INDEX_NONE, mxSTRING_QUESTION_MARK);
	return m_items.items[ enumItemIndex ].name;
}

const char* MetaEnum::GetString( const ValueT itemValue ) const
{
	const UINT enumItemIndex = m_items.GetItemIndexByValue( itemValue );
	mxENSURE(enumItemIndex != INDEX_NONE,
		"",
		"Enum '%s': couldn't find string for value %u",
		m_name,
		itemValue
		);
	return m_items.items[ enumItemIndex ].name;
}

MetaEnum::ValueT MetaEnum::FindValue( const char* _name ) const
{
	MetaEnum::ValueT	value;
	if( mxSUCCEDED(this->FindValueSafe( _name, &value )) ) {
		return value;
	}
	ptERROR("No enum value with name='%s'",_name);
	return 0;
}

ERet MetaEnum::FindValueSafe( const char* _name, MetaEnum::ValueT *_result ) const
{
	const UINT enumItemIndex = m_items.GetItemIndexByString( _name );
	if( enumItemIndex == INDEX_NONE ) {
		return ERR_OBJECT_NOT_FOUND;
	}
	*_result = m_items.items[ enumItemIndex ].value;
	return ALL_OK;
}
