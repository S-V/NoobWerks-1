/*
=============================================================================
	File:	Reflection.h
	Desc:	contains code for defining structure layouts
			and inspecting them at run-time.

			Reflection - having access at runtime
			to information about the C++ classes in your program.
			Reflection is used mainly for serialization.

	Note:	This has been written in haste and is by no means complete.

	Note:	base classes are assumed to start at offset 0 (see mxFIELD_SUPER)

	ToDo:	don't leave string names in release,
			they should only be used in editor mode
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>

// these are defined in the core engine module
struct AssetID;
struct NwAssetRef;

enum EFieldFlags
{
	/// The field won't be initialized with default values (e.g. fallback resources).
	Field_NoDefaultInit = BIT(0),

	/// (mainly for text-based formats such as JSON/SON/XML)
	Field_NoSerialize	= BIT(1),

	// Potential field flags, see:
	// http://www.altdevblogaday.com/2012/01/03/reflection-in-c-part-2-the-simple-implementation-of-splinter-cell/
	/*
	// Is this a transient field, ignored during serialisation?
	F_Transient = 0x02,

	// Is this a network transient field, ignored during network serialisation?
	// A good example for this use-case is a texture type which contains a description
	// and its data. For disk serialisation you want to save everything, for network
	// serialisation you don't really want to send over all the texture data.
	F_NetworkTransient = 0x04,

	// Can this field be edited by tools?
	F_ReadOnly = 0x08,

	// Is this a simple type that can be serialised in terms of a memcpy?
	// Examples include int, float, any vector types or larger types that you're not
	// worried about versioning.
	F_SimpleType = 0x10,

	// Set if the field owns the memory it points to.
	// Any loading code must allocate it before populating it with data.
	F_OwningPointer = 0x20
	*/

	//REMOVE THIS:
	// Create a save file dialog for editing this field
	// (Only for string fields)
	//Field_EditAsSaveFile	= BIT(1),

	Field_DefaultFlags = 0,
};

typedef UINT32 FieldFlags;

/*
-------------------------------------------------------------------------
	MetaField

	structure field definition
	,used for reflecting contained, nested objects

	@todo: don't leave name string in release exe
	@todo: each one of offset/flags could fit in 16 bits
-------------------------------------------------------------------------
*/
struct MetaField
{
	const TbMetaType &	type;	//!< type of the field
	const char *		name;	//!< name of the variable in the code
	const MetaOffset	offset;	//!< byte offset in the structure (relative to the immediately enclosing type)
	FieldFlags			flags;	//!< combination of EFieldFlags::Field_* bits
};

/*
-------------------------------------------------------------------------
	Metadata

	reflection metadata for describing the contents of a structure
-------------------------------------------------------------------------
*/
struct TbClassLayout
{
	const MetaField *	fields;		//!< array of fields in the structure
	const int			numFields;	//!< number of fields in the structure

	// time stamp for version tracking
	// metadata is implemented in source files ->
	// time stamp changes when file is recompiled
	//STimeStamp	timeStamp;
public:
	static TbClassLayout	dummy;	//!< empty, 'null' instance
};

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxNO_REFLECTED_MEMBERS - must be included in the declaration of a class.
//-----------------------------------------------------------------------
//
#define mxNO_REFLECTED_MEMBERS\
	public:\
		static TbClassLayout& staticLayout() { return TbClassLayout::dummy; }\


//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxDECLARE_REFLECTION - must be included in the declaration of a class.
//-----------------------------------------------------------------------
//
#define mxDECLARE_REFLECTION\
	public:\
		static TbClassLayout& staticLayout();\


//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxBEGIN_REFLECTION( CLASS ) - should be placed in source file.
//-----------------------------------------------------------------------
//
#define mxBEGIN_REFLECTION( CLASS )\
	TbClassLayout& CLASS::staticLayout() {\
		typedef CLASS OuterType;\
		static MetaField fields[] = {\

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxMEMBER_FIELD( VAR )\
	{\
		T_DeduceTypeInfo( ((OuterType*)0)->VAR ),\
		mxEXTRACT_NAME( VAR ),\
		OFFSET_OF( OuterType, VAR ),\
		Field_DefaultFlags\
	}

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// allows the programmer to specify her own type
// 'TYPE' must be (const TbMetaType&)
//
#define mxMEMBER_FIELD_OF_TYPE( VAR, TYPE )\
	{\
		TYPE,\
		mxEXTRACT_NAME( VAR ),\
		OFFSET_OF( OuterType, VAR ),\
		Field_DefaultFlags\
	}

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxMEMBER_FIELD_WITH_CUSTOM_NAME( VAR, CUSTOM_NAME )\
	{\
		T_DeduceTypeInfo( ((OuterType*)0)->VAR ),\
		#CUSTOM_NAME,\
		OFFSET_OF( OuterType, VAR ),\
		Field_DefaultFlags\
	}

//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// FIXME: always add unused member to allow empty member list decls
#if 1 || MX_DEVELOPER

	// allow empty reflection metadata descriptions in debug mode
	#define mxEND_REFLECTION\
				{ T_DeduceTypeInfo< INT >(), nil, 0, 0 }\
			};\
			static TbClassLayout reflectedMembers = { fields, mxCOUNT_OF(fields)-1 };\
			return reflectedMembers;\
		}

#else

	// saves a little bit of memory (size of pointer) in release mode
	#define mxEND_REFLECTION\
			};\
			static TbClassLayout reflectionMetadata = { fields, mxCOUNT_OF(fields) };\
			return reflectionMetadata;\
		}

#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// A visitor interface for serializable fields defined in a class

namespace Reflection
{

mxREFACTOR("bogus names, inconvenient function signatures");

/*
-------------------------------------------------------------------------
	AVisitor
	low-level interface for iterating over object fields via reflection

	NOTE: its functions accept some user-defined data and return user data.
-------------------------------------------------------------------------
*/
mxDEPRECATED
class AVisitor
{
	friend class Walker;

protected:
	// this function is called to visit a class member field
	// (e.g. this is used by buffer/JSON/XML serializers).
	// it returns a pointer to some user-defined data.
	// NOTE: don't forget to call the parent's function if needed
	virtual void* Visit_Field( void * _o, const MetaField& _field, void* user_data );

	// structures
	// NOTE: don't forget to call the parent's function if needed
	virtual void* Visit_Aggregate( void * _o, const TbMetaClass& _type, void* user_data );

	// arrays
	// NOTE: don't forget to call the parent's function if needed
	virtual void* Visit_Array( void * _array, const MetaArray& _type, void* user_data );
	virtual void* Visit_Blob( void * _blob, const mxBlobType& _type, void* user_data );


	// built-in types (int, float, bool), enums, bitmasks
	virtual void* Visit_POD( void * _o, const TbMetaType& _type, void* user_data )
	{
		return user_data;
	}
	virtual void* Visit_TypeId( SClassId * _o, void* user_data )
	{
		return user_data;
	}
	virtual void* Visit_String( String & _string, void* user_data )
	{
		return user_data;
	}
	virtual void* Visit_AssetId( AssetID & _assetId, void* user_data )
	{
		return user_data;
	}
	//virtual void* Visit_AssetReference( NwAssetRef & asset_ref, void* user_data )
	//{
	//	return user_data;
	//}

	// references
	virtual void* Visit_Pointer( VoidPointer& _pointer, const MetaPointer& _type, void* user_data )
	{
		return user_data;
	}
	virtual void* Visit_UserPointer( void * _o, const mxUserPointerType& _type, void* user_data )
	{
		return user_data;
	}

protected:
	virtual ~AVisitor() {}
};

// Object/Field iterator
mxDEPRECATED
struct Walker {
	static void* Visit( void * _object, const TbMetaType& _type, AVisitor* _visitor, void *user_data = nil );
	static void* VisitArray( void * _array, const MetaArray& _type, AVisitor* _visitor, void* user_data = nil );
	static void* VisitAggregate( void * _struct, const TbMetaClass& _type, AVisitor* _visitor, void *user_data = nil );
	static void* VisitStructFields( void * _struct, const TbMetaClass& _type, AVisitor* _visitor, void* user_data = nil );
};

class PointerChecker: public Reflection::AVisitor
{
public:
	virtual void* Visit_Array( void * arrayObject, const MetaArray& arrayType, void* user_data ) override;
	virtual void* Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* user_data ) override;
};

void ValidatePointers( const void* o, const TbMetaClass& type );

void CheckAllPointersAreInRange( const void* o, const TbMetaClass& type, const void* start, UINT32 size );


class CountReferences: public Reflection::AVisitor
{
	void *	m_pointer;
	UINT	m_numRefs;
public:
	inline CountReferences( void* o )
	{
		m_pointer = o;
		m_numRefs = 0;
	}
	inline UINT GetNumReferences() const
	{
		return m_numRefs;
	}
protected:
	virtual void* Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* user_data ) override
	{
		if( p.o == m_pointer ) {
			++m_numRefs;
		}
		return user_data;
	}
};

mxDEPRECATED
class AVisitor2
{
	friend class Walker2;

public:
	// for passing parameters between callbacks
	struct Context
	{
		const Context *	parent;
		const MetaField *	member;	// for class members
		//UINT32			index;	// for arrays only
		const char *	userName;
		mutable void *	userData;
		const int		depth;	// for debug print
	public:
		Context( int _depth = 0 )
			: depth( _depth )
		{
			parent = NULL;
			member = NULL;
			//index = ~0;
			userName = "?";
			userData = NULL;
		}
		const char* GetMemberName() const {
			return member ? member->name : userName;
		}
		const char* GetMemberType() const {
			return member ? member->type.m_name : userName;
		}
	};
protected:
	// return false to skip further processing
	//virtual bool Visit( void * _memory, const TbMetaType& _type, const Context& _context )
	//{ return true; }

	// this function is called to visit a class member field
	// (e.g. this is used by buffer/JSON/XML serializers).
	// return false to skip further processing
	virtual bool Visit_Field( void * _memory, const MetaField& _field, const Context& _context )
	{ return true; }

	//this adds too much overhead:
	//virtual void Visit_Array_Item( void * _pointer, const TbMetaType& _type, UINT32 _index, const Context& _context ) {}

	// structures
	virtual bool Visit_Class( void * _object, const TbMetaClass& _type, const Context& _context )
	{ return true; }

	// arrays
	// return false to skip processing the array elements
	virtual bool Visit_Array( void * _array, const MetaArray& _type, const Context& _context ) {return true;}

	virtual void visit_Dictionary(
		void * dictionary_instance
		, const MetaDictionary& dictionary_type
		, const Context& context
		);

	// 'Leaf' types:

	// built-in types (int, float, bool), enums, bitmasks
	virtual void Visit_POD( void * _memory, const TbMetaType& _type, const Context& _context ) {}
	virtual void Visit_String( String & _string, const Context& _context ) {}
	virtual void Visit_TypeId( SClassId * _class, const Context& _context ) {}
	virtual void Visit_AssetId( AssetID & _assetId, const Context& _context ) {}

	// references
	virtual void Visit_Pointer( VoidPointer & _pointer, const MetaPointer& _type, const Context& _context ) {}
	virtual void Visit_UserPointer( void * _pointer, const mxUserPointerType& _type, const Context& _context ) {}

	virtual void visit_AssetReference(
		NwAssetRef * asset_ref
		, const MetaAssetReference& asset_reference_type
		, const Context& context
		)
	{}

protected:
	virtual ~AVisitor2() {}
};
mxDEPRECATED
// Object/Field iterator
struct Walker2 {
	static void Visit( void * _memory, const TbMetaType& _type, AVisitor2* _visitor, void *user_data = nil );

	static void Visit( void * _memory, const TbMetaType& _type, AVisitor2* _visitor, const AVisitor2::Context& _context );
	static void VisitArray( void * _array, const MetaArray& _type, AVisitor2* _visitor, const AVisitor2::Context& _context );
	static void VisitAggregate( void * _struct, const TbMetaClass& _type, AVisitor2* _visitor, const AVisitor2::Context& _context );
	static void VisitStructFields( void * _struct, const TbMetaClass& _type, AVisitor2* _visitor, const AVisitor2::Context& _context );

	static void visit_Items_stored_in_Dictionary(
		void * instance
		, const MetaDictionary& dictionary_type
		, AVisitor2* visitor
		, const AVisitor2::Context& context
		);
};



struct Context3 
{
	const Context3*	parent;
	const MetaField *	member;	// for class members
	//UINT32			index;	// for arrays only
	const char *	userName;
	mutable void *	userData;
	const int		depth;	// for debug print
public:
	Context3( int _depth = 0 )
		: depth( _depth )
	{
		parent = NULL;
		member = NULL;
		//index = ~0;
		userName = "?";
		userData = NULL;
	}
};

template< typename USER_DATA, class VISITOR >	// where VISITOR : Visitor3
ERet Visit_Array_Elements(
	void* _items,			//!< the array base pointer
	UINT32 _count,			//!< the number of array elements
	const TbMetaType& _type,	//!< the type of stored array elements
	const Context3& _context,
	VISITOR & _visitor,
	USER_DATA user_data )
{
	_visitor.DebugPrint(_context,"#ARRAY of '%s' (%u items, %u bytes each)", _type.m_name, _count, _type.m_size);

	const UINT32 byteStride = _type.m_size;
	const UINT arrayDataSize = byteStride * _count;

	Context3 itemContext( _context.depth + 1 );
	itemContext.parent = &_context;
	itemContext.userData = _context.userData;

	for( UINT32 itemIndex = 0; itemIndex < _count; itemIndex++ )
	{
		const MetaOffset itemOffset = itemIndex * byteStride;
		void* itemPointer = mxAddByteOffset( _items, itemOffset );

		mxDO(_visitor.Visit( itemPointer, _type, itemContext, user_data ));
	}

	return ALL_OK;
}
template< typename USER_DATA, class VISITOR >	// where VISITOR : Visitor3
ERet VisitStructFields(
	void* _struct,
	const TbMetaClass& _type,
	const Context3& _context,
	VISITOR & _visitor,
	USER_DATA user_data )
{
	const TbClassLayout& layout = _type.GetLayout();
	for( UINT fieldIndex = 0 ; fieldIndex < layout.numFields; fieldIndex++ )
	{
		const MetaField& field = layout.fields[ fieldIndex ];

		Context3	fieldContext( _context.depth + 1 );
		fieldContext.parent = &_context;
		fieldContext.member = &field;
		fieldContext.userData = _context.userData;

		_visitor.DebugPrint(_context,"#FIELD: '%s' ('%s') in '%s' at '%u' (%u bytes)",
			field.name, field.type.m_name, _type.m_name, field.offset, field.type.m_size);

		void* memberVariable = mxAddByteOffset( _struct, field.offset );

		mxDO(_visitor.Visit_Field( memberVariable, field, fieldContext, user_data ));
	}
	return ALL_OK;
}
template< typename USER_DATA, class VISITOR >	// where VISITOR : Visitor3
ERet VisitAggregate(
	void* _struct,
	const TbMetaClass& _type,
	const Context3& _context,
	VISITOR & _visitor,
	USER_DATA user_data )
{
	mxASSERT_PTR(_struct);

	_visitor.DebugPrint(_context,"#OBJECT: class '%s' (%u bytes)", _type.m_name, _type.m_size);

	{
		// First recursively visit the parent classes.
		const TbMetaClass* parentType = _type.GetParent();
		while( parentType != nil )
		{
			mxDO(VisitStructFields< USER_DATA >( _struct, *parentType, _context, _visitor, user_data ));
			parentType = parentType->GetParent();
		}

		// Now visit the members of this class.
		mxDO(VisitStructFields< USER_DATA >( _struct, _type, _context, _visitor, user_data ));
	}
	return ALL_OK;
}

/*
http://cbloomrants.blogspot.ru/2013/02/02-16-13-reflection-visitor-pattern.html
*/
template< typename USER_DATA = void* >
class Visitor3 {
public:
	typedef USER_DATA USER_DATA_T;

	//// this function is called to visit a class member field
	//// (e.g. this is used by buffer/JSON/XML serializers).
	//// return false to skip further processing
	//virtual bool Enter_Field( void * _memory, const MetaField& _field, const Context3& _context, USER_DATA user_data )
	//{return true;}

	// structures
	virtual ERet Visit_Class( void * _object, const TbMetaClass& _type, const Context3& _context, USER_DATA user_data )
	{
		mxDO(Reflection::VisitAggregate< USER_DATA >( _object, _type, _context, *this, user_data ));
		return ALL_OK;
	}

	virtual ERet Visit_Field( void * _memory, const MetaField& _field, const Context3& _context, USER_DATA user_data )
	{
		mxDO(this->Visit( _memory, _field.type, _context, user_data ));
		return ALL_OK;
	}

	//virtual bool Enter_Class( void * _object, const TbMetaClass& _type, const Context3& _context, USER_DATA user_data )
	//{return true;}

	// arrays
	//// return false to skip processing the array elements
	//virtual bool Enter_Array( void * _array, const MetaArray& _type, const Context3& _context, USER_DATA user_data )
	//{return true;}

	virtual ERet Visit_Array( void * _array, const MetaArray& _type, const Context3& _context, USER_DATA user_data )
	{
		const UINT32 count = _type.Generic_Get_Count( _array );
		void* basePointer = _type.Generic_Get_Data( _array );
		const TbMetaType& elementType = _type.m_itemType;
		mxDO(Reflection::Visit_Array_Elements< USER_DATA >( basePointer, count, elementType, _context, *this, user_data ));
		return ALL_OK;
	}

	// 'Leaf' types:

	// built-in types (int, float, bool), enums, bitmasks

	virtual ERet Visit_POD( void * _memory, const TbMetaType& _type, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	virtual ERet Visit_String( String * _string, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	virtual ERet Visit_TypeId( SClassId * _class, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	virtual ERet Visit_AssetId( AssetID * _assetId, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	// references

	// visit a pointer to the pointer field
	virtual ERet Visit_Pointer( void * _pointer, const MetaPointer& _type, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	virtual ERet Visit_UserPointer( void * _pointer, const mxUserPointerType& _type, const Context3& _context, USER_DATA user_data )
	{return ALL_OK;}

	virtual void DebugPrint( const char* _format, ... )
	{
		//va_list	args;
		//va_start( args, _format );
		//mxGetLog().PrintV( LL_Debug, _format, args );
		//va_end( args );
	}
	virtual void DebugPrint( const Context3& _context, const char* _format, ... )
	{
		//va_list	args;
		//va_start( args, _format );
		//LogStream(LL_Debug).Repeat('\t', _context.depth).PrintV(_format, args);
		//va_end( args );
	}

public:
	ERet Visit( void* _memory, const TbMetaType& _type, USER_DATA user_data )
	{
		mxASSERT_PTR(_memory);

		Context3	context;

		mxDO(Visit( _memory,_type, context, user_data ));

		return ALL_OK;
	}
	ERet Visit( void* _memory, const TbMetaType& _type, const Context3& _context, USER_DATA user_data )
	{
		switch( _type.m_kind )
		{
		case ETypeKind::Type_Class :
			{
				const TbMetaClass& classType = _type.UpCast< TbMetaClass >();
				mxDO(VisitAggregate<USER_DATA>( _memory, classType, _context, *this, user_data ));
			}
			break;

		case ETypeKind::Type_Array :
			{
				const MetaArray& arrayType = _type.UpCast< MetaArray >();
				mxDO(this->Visit_Array( _memory, arrayType, _context, user_data ));
			}
			break;

		case ETypeKind::Type_Integer :
		case ETypeKind::Type_Float :
		case ETypeKind::Type_Bool :
		case ETypeKind::Type_Enum :
		case ETypeKind::Type_Flags :
			{
				mxDO(Visit_POD( _memory, _type, _context, user_data ));
			}
			break;

		case ETypeKind::Type_String :
			{
				String* stringPointer = static_cast< String* >( _memory );
				mxDO(Visit_String( stringPointer, _context, user_data ));
			}
			break;

		case ETypeKind::Type_Pointer :
			{
				const MetaPointer& pointerType = _type.UpCast< MetaPointer >();
				mxDO(Visit_Pointer( _memory, pointerType, _context, user_data ));
			}
			break;

		case ETypeKind::Type_AssetId :
			{
				AssetID* assetId = static_cast< AssetID* >( _memory );
				mxDO(Visit_AssetId( assetId, _context, user_data ));
			}
			break;

		case ETypeKind::Type_ClassId :
			{
				SClassId* classId = static_cast< SClassId* >( _memory );
				mxDO(Visit_TypeId( classId, _context, user_data ));
			}
			break;

		case ETypeKind::Type_UserData :
			{
				const mxUserPointerType& userPointerType = _type.UpCast< mxUserPointerType >();
				mxDO(Visit_UserPointer( _memory, userPointerType, _context, user_data ));
			}
			break;

		case ETypeKind::Type_Blob :
			{
				Unimplemented;
			}
			break;

			mxNO_SWITCH_DEFAULT;
		}//switch

		return ALL_OK;
	}
};







struct TellNotToFreeMemory: public Reflection::AVisitor2
{
	typedef Reflection::AVisitor2 Super;
	//-- Reflection::AVisitor
	virtual bool Visit_Array( void * _array, const MetaArray& _type, const Context& _context ) override;
	virtual void Visit_String( String & _string, const Context& _context ) override;

	virtual void visit_Dictionary(
		void * dictionary_instance
		, const MetaDictionary& dictionary_type
		, const Context& context
		) override;
};

void MarkMemoryAsExternallyAllocated( void* _memory, const TbMetaClass& _type );

bool HasCrossPlatformLayout( const TbMetaClass& _type );

bool ObjectsAreEqual( const TbMetaClass& _type1, const void* _o1, const TbMetaClass& _type2, const void* _o2 );

mxFORCEINLINE
bool objectOfThisTypeMayContainPointers( const TbMetaType& type )
{
	return !Type_Is_Bitwise_Serializable( type.m_kind );
}

}//namespace Reflection


//!----------------------------------------------------------------------

template< typename TYPE >
struct TPODCast
{
	mxFORCEINLINE static const TYPE Get( const void* ptr )
	{
		return *static_cast< TYPE* >( ptr );
	}
	mxFORCEINLINE static void Set( void* dst, const TYPE newValue )
	{
		*static_cast< TYPE* >( dst ) = newValue;
	}

	mxFORCEINLINE static void Set( void* const base, const MetaOffset offset, const TYPE& newValue )
	{
		TYPE & o = *(TYPE*) ( (BYTE*)base + offset );
		o = newValue;
	}
	mxFORCEINLINE static const TYPE& Get( const void*const base, const MetaOffset offset )
	{
		return *(TYPE*) ( (BYTE*)base + offset );
	}
	mxFORCEINLINE static TYPE& GetNonConst( void *const base, const MetaOffset offset )
	{
		return *(TYPE*) ( (BYTE*)base + offset );
	}
	mxFORCEINLINE static const TYPE& GetConst( const void*const objAddr )
	{
		return *(const TYPE*) objAddr;
	}
	mxDEPRECATED
	mxFORCEINLINE static TYPE& GetNonConst( void*const objAddr )
	{
		return *(TYPE*) objAddr;
	}
	static inline void GetNonConst2( void*const _address, TYPE *& _o )
	{
		_o = *static_cast< TYPE* >( _address );
	}
};

mxDEPRECATED
inline INT64 GetInteger( const void* _pointer, const int _byteWidth )
{
	if( _byteWidth == 1 ) {
		return *(INT8*) _pointer;
	}
	else if( _byteWidth == 2 ) {
		return *(INT16*) _pointer;
	}
	else if( _byteWidth == 4 ) {
		return *(INT32*) _pointer;
	}
	else if( _byteWidth == 8 ) {
		return *(INT64*) _pointer;
	}
	else {
		mxUNREACHABLE;
		return 0;
	}
}
mxDEPRECATED
inline void PutInteger( void *_destination, const int _size, const INT64 _value )
{
	if( _size == 1 ) {
		*(INT8*)_destination = _value;
	}
	else if( _size == 2 ) {
		*(INT16*)_destination = _value;
	}
	else if( _size == 4 ) {
		*(INT32*)_destination = _value;
	}
	else if( _size == 8 ) {
		*(INT64*)_destination = _value;
	}
	else {
		mxUNREACHABLE;
	}
}
mxDEPRECATED
inline double GetDouble( const void* _pointer, const int _byteWidth )
{
	if( _byteWidth == 4 ) {
		return *(float*) _pointer;
	}
	else if( _byteWidth == 8 ) {
		return *(double*) _pointer;
	}
	else {
		mxUNREACHABLE;
		return 0;
	}
}
mxDEPRECATED
inline void PutDouble( void *_destination, const int _size, const double _value )
{
	if( _size == 4 ) {
		*(float*)_destination = _value;
	}
	else if( _size == 8 ) {
		*(double*)_destination = _value;
	}
	else {
		mxUNREACHABLE;
	}
}

namespace Reflection
{

inline ERet SetBooleanValue( void * _o, const TbMetaType& _type, const bool _value )
{
	chkRET_X_IF_NOT(_type.m_kind == Type_Bool, ERR_OBJECT_OF_WRONG_TYPE);
	*(bool*)_o = _value;
	return ALL_OK;
}

inline ERet SetIntegerValue( void * _o, const TbMetaType& _type, const int _value )
{
	chkRET_X_IF_NOT(_type.m_kind == Type_Integer, ERR_OBJECT_OF_WRONG_TYPE);
	const MetaSize size = _type.m_size;
	if( size == 1 ) {
		*(I8*)_o = _value;
	}
	else if( size == 2 ) {
		*(I16*)_o = _value;
	}
	else if( size == 4 ) {
		*(I32*)_o = _value;
	}
	else if( size == 8 ) {
		*(I64*)_o = _value;
	}
	else {
		mxUNREACHABLE;
		return ERR_NOT_IMPLEMENTED;
	}
	return ALL_OK;
}

}//namespace Reflection
