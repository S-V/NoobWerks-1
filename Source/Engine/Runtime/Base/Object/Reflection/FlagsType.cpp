#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/Reflection/FlagsType.h>

void Dbg_FlagsToString( const void* _o, const MetaFlags& _type, String &_string )
{
	_string.empty();
	const MetaFlags::ValueT mask = _type.GetValue( _o );
	for( UINT i = 0; i < _type.m_items.count; i++ )
	{
		const MetaFlags::Item& member = _type.m_items.items[i];
		if( member.value & mask )
		{
			if( !_string.IsEmpty() )
			{
				Str::Append( _string, '|' );
			}
			Str::AppendS( _string, member.name );
		}		
	}
}
