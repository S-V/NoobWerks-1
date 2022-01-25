// Internal functions:
#pragma once

#include <Base/Util/StaticStringHash.h>
#include <Core/Serialization/Text/TxTCommon.h>

namespace SON
{
	static inline bool ETypeTag_Is_Leaf( ETypeTag _tag ) {
		return (_tag >= FirstLeafType && _tag <= LastLeafType);
	}

	static inline void PrependChild( Node* _parent, Node* _child )
	{
		mxASSERT(!ETypeTag_Is_Leaf(_parent->tag.type));
		mxASSERT(_child->next == nil);
		_child->next = _parent->value.l.kids;
		_parent->value.l.kids = _child;
		_parent->value.l.size++;
	}

}//namespace SON

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
