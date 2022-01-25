/*
=============================================================================
	SON (Simple Object Notation)
=============================================================================
*/
#pragma once

#include <Base/Memory/FreeList/FreeList.h>

// preserve the original values' order as they appear in the source text?
// (slightly slower during parsing, but leads to faster query operations)
#define txtPARSER_PRESERVE_ORDER	(1)

#define txtPARSER_ALLOW_C_STYLE_COMMENTS	(0)

namespace SON
{
	struct Node;

	typedef U64 ValueT;

	// EValueType
	enum ETypeTag
	{
		TypeTag_Nil,		//!< None, empty value: 'nil'

		TypeTag_List,		//!< linked list of values: ()
		TypeTag_Array,		//!< contiguous array/vector of values: []
		TypeTag_Object,		//!< unordered set/hash/map/table/dictionary/associative array: {}

		TypeTag_String,		//!< null-terminated string: ""
		TypeTag_Number,		//!< double-precision floating-point number
		//TypeTag_Boolean,	//!< 'true' or 'false'
		//TypeTag_HashKey,	//!< hashed identifier: #name
		//TypeTag_DateTime,	//!< Coordinated Universal Time (UTC)

		TypeTag_MAX = 7		//!< we use 3 bits for storing a type tag
	};

	static const ETypeTag FirstLeafType = TypeTag_String;
	static const ETypeTag LastLeafType = TypeTag_Number;

	const char* ETypeTag_To_Chars( ETypeTag _tag );

	struct ListValue {
		Node *	kids;	// linked list of children, 0 if empty
		U32	size;	// number of children / array elements
	};
	struct ArrayValue {
		Node *	kids;	// linked list of children, 0 if empty
		U32	size;	// number of children / array elements
	};
	struct ObjectValue {
		Node *	kids;	// linked list of children, 0 if empty
		U32	size;	// number of children / array elements
	};
	struct StringValue {
		const char*	start;
		U32		length;
	};
	union NumberValue {
		double	f;
		U64	i;
	};
	struct HashKeyValue {
		U32	hash;
		U32	offset;
	};
	struct DateTimeValue {
		BITFIELD	year   : 32;//!< year starting from 0
		BITFIELD	month  : 4;	//!< [0-11]
		BITFIELD	day    : 5;	//!< [0-30] (day of month)
		BITFIELD	hour   : 5;	//!< [0-23] (hours since midnight)
		BITFIELD	minute : 6;	//!< minutes after the hour - [0,59]
		BITFIELD	second : 6;	//!< seconds after the minute - [0,59]
	};

	union ValueU
	{
		ListValue		l;
		ArrayValue		a;
		ObjectValue		o;

		StringValue		s;
		NumberValue		n;
		HashKeyValue	h;
		DateTimeValue	d;

		U64			u;
	};//8/12
	union NodeFlagsU
	{
		struct {
			BITFIELD	hash : 28;	//!< hash key for searching faster than by name string
			ETypeTag	type : 4;	//!< the type of this node
		};
		U32			bits;
	};//4

	struct Node
	{
		ValueU		value;	//!< the value stored in this node
		Node *		next;	//!< index of the next sibling in the nodes array/list
		const char*	name;	//!< the string key of this value (iff. the parent is object)
		NodeFlagsU	tag;	//!< type and hash of this node

	public:
		mxFORCEINLINE bool IsObject() const { return tag.type == TypeTag_Object; }
		mxFORCEINLINE bool IsArray() const { return tag.type == TypeTag_Array; }
		mxFORCEINLINE bool IsNumber() const { return tag.type == TypeTag_Number; }
		mxFORCEINLINE bool IsString() const { return tag.type == TypeTag_String; }

		const Node* FindChildNamed( const char* key ) const;

		const char* GetStringValue() const;
	};//20/32

	typedef U32 NodeIndexT;

	/// tree node allocator
	class NodeAllocator
	{
		FreeListAllocator	nodes;	//!< node allocator

	public:
		NodeAllocator( AllocatorI& allocator, U32 freeListGranularity = 64 );
		~NodeAllocator();

		Node* AllocateNode();
		Node* AllocateArray( U32 _count );
		void ReleaseNode( Node* _node );
	};

	const char* AsString( const Node* _value );
	const double AsDouble( const Node* _value );
	const bool AsBoolean( const Node* _value );

	const Node* FindValue( const Node* _object, const char* _key );

	const U32 GetArraySize( const Node* _array );
	const Node* GetArrayItem( const Node* _array, NodeIndexT _index );

	const Node* FindValueByHash( const U32 _hash );
	const Node* FindValueByHash( const U32 _hash );

	void MakeNumber( Node *_node, double _value );
	void MakeString( Node *_node, const char* _value );
	void MakeBoolean( Node *_node, bool _value );
	void MakeObject( Node *_node );
	void MakeArray( Node *_node, U32 _size );
	void MakeList( Node *_node );

	Node* NewNumber( double _value, NodeAllocator & _allocator );
	Node* NewString( const char* _value, U32 _length, NodeAllocator & _allocator );
	Node* NewBoolean( bool _value, NodeAllocator & _allocator );
	Node* NewObject( NodeAllocator & _allocator );
	Node* NewArray( U32 item_count, NodeAllocator & _allocator );
	Node* NewList( NodeAllocator & _allocator );

	// O(1)
	//NOTE: children are added in reverse order (i.e. prepended to parent)
	void AddChild( Node* _object, const char* name, Node* _child );
	void AddChild( Node* _list, Node* _child );

	// O(n)
	void AppendChild( Node* _list, Node* _child );

	Node* NewString( const char* _value, NodeAllocator & _allocator );

	enum EResultCode
	{
		Ret_OK = 0,
		Ret_BAD_NUMBER,
		Ret_BAD_STRING,
		Ret_BAD_IDENTIFIER,
		Ret_STACK_OVERFLOW,
		Ret_STACK_UNDERFLOW,
		Ret_MISMATCH_BRACKET,
		Ret_UNEXPECTED_CHARACTER,
		Ret_UNQUOTED_KEY,
		Ret_BREAKING_BAD
	};

	namespace Debug
	{
		void PrintTree( const Node& _root );

	}//namespace Debug

}//namespace SON
