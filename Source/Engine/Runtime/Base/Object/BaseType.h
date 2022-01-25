/*
=============================================================================
	File:	BaseType.h
	Desc:	Base class for many objects, root of our class hierarchy.
			It's quite lightweight (only vtbl overhead).
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeRegistry.h>
#include <Base/Object/Reflection/Reflection.h>
//#include <Base/Memory/MemoryBase.h>	// AllocatorI

class mxArchive;
struct SDbgDumpContext;

/**
*	CStruct represents a C-style structure.
*	This is the base class for C-compatible structs (ideally, bitwise copyable, POD types),
*	i.e. derived classes MUST *NOT* have virtual functions and pointers to self.
*/
struct CStruct
{
	// NOTE: we don't define any constructors or destructors
	// so that we could put derived structs into unions, etc.

public:	// Reflection

	/// returns static type information
	static TbMetaClass & metaClass() { return ms_staticTypeInfo; };

	/// returns run-time type information
	inline TbMetaClass& rttiClass() const { return ms_staticTypeInfo; }

	static TbClassLayout& staticLayout()
	{
		static TbClassLayout struct_layout = { NULL, 0 };
		return struct_layout;
	}

public:	// Asset / Resource Loading

	/// can be statically override by derived classes
	static TbAssetLoaderI* getAssetLoader() { return nil; }

private:
	static TbMetaClass	ms_staticTypeInfo;
};

/**
*	The abstract base class for most polymorphic classes in our class hierarchy,
*	provides its own implementation of fast Run-Time Type Information (RTTI).
*
*	NOTE: if this class is used as a base class when using multiple inheritance,
*	this class should go first (to ensure the vtable is placed at the beginning).
*
*	NOTE: this class must be abstract!
*	NOTE: this class must not define any member fields!
*/
class AObject: public CStruct {
public:
	/// Static Reflection
	static TbMetaClass & metaClass();

	//
	//	Run-Time Type Information (RTTI)
	//
	virtual TbMetaClass& rttiClass() const;

	// These functions are provided for convenience:

	const char *	rttiTypeName() const;
	const TypeGUID	rttiTypeID() const;

	/// Returns 'true' if this type inherits from the given type.
	bool	IsA( const TbMetaClass& type ) const;
	bool	IsA( const TypeGUID& typeCode ) const;

	/// Returns 'true' if this type is the same as the given type.
	bool	IsInstanceOf( const TbMetaClass& type ) const;
	bool	IsInstanceOf( const TypeGUID& typeCode ) const;

	/// Returns 'true' if this type inherits from the given type.
	template< class CLASS >
	inline bool IsA() const
	{
		return this->IsA( CLASS::metaClass() );
	}

	/// Returns 'true' if this type is the same as the given type.
	template< class CLASS >
	inline bool Is() const
	{
		return IsInstanceOf( CLASS::metaClass() );
	}

public:	// Serialization / Streaming

	/// serialize persistent data (including pointers to other objects)
	/// in binary form (MFC-style).
	/// NOTE: extremely brittle, use with caution!
	///
	virtual void Serialize( mxArchive & s ) {}

	/// complete loading (e.g. fixup asset references)
	/// this function is called immediately after deserialization to complete the object's initialization
	virtual void PostLoad() {}

public:	// Cloning (virtual constructor, deep copying)
	//virtual AObject* Clone() { return nil; }

public:	// Asset / Resource Loading

	static TbAssetLoaderI* getAssetLoader() { return nil; }

public:	// Testing & Debugging

	virtual void DbgAssertValid() {}
	virtual void DbgDumpContents( const SDbgDumpContext& dc ) {}

public:
	virtual	~AObject() = 0;

protected:
	AObject() {}

private:	NO_COMPARES(AObject);
	static TbMetaClass	ms_staticTypeInfo;
};

#include <Base/Object/BaseType.inl>

///
///	DynamicCast< T > - safe (checked) cast, returns a nil pointer on failure
///
template< class TypeTo >
mxINLINE
TypeTo* DynamicCast( AObject* pObject )
{
	mxASSERT_PTR( pObject );
	return pObject->IsA( TypeTo::metaClass() ) ?
		static_cast< TypeTo* >( pObject ) : nil;
}

///
///	ConstCast< T >
///
template< class TypeTo, class TypeFrom > /// where TypeTo : AObject and TypeFrom : AObject
mxINLINE
const TypeTo* ConstCast( const TypeFrom* pObject )
{
	mxASSERT_PTR( pObject );
	return pObject->IsA( TypeTo::metaClass() ) ?
		static_cast< const TypeTo* >( pObject ) : nil;
}

///
///	UpCast< T > - unsafe cast, raises an error on failure,
///	assumes that you know what you're doing.
///
template< class TypeTo >
mxINLINE
TypeTo* UpCast( AObject* pObject )
{
	mxASSERT_PTR( pObject );
	mxASSERT( pObject->IsA( TypeTo::metaClass() ) );
	return checked_cast< TypeTo* >( pObject );
}

template< class TypeTo, class TypeFrom > /// where TypeTo : AObject and TypeFrom : AObject
mxINLINE
TypeTo* SafeCast( TypeFrom* pObject )
{
	if( pObject != nil )
	{
		return pObject->IsA( TypeTo::metaClass() ) ?
			static_cast< TypeTo* >( pObject ) : nil;
	}
	return nil;
}


namespace ObjectUtil
{
	inline bool Class_Has_Fields( const TbMetaClass& classInfo )
	{
		return (classInfo.GetLayout().numFields > 0)
			// the root classes don't have any serializable state
			&& (classInfo != CStruct::metaClass())
			&& (classInfo != AObject::metaClass())
			;
	}
	inline bool Serializable_Class( const TbMetaClass& classInfo )
	{
		return Class_Has_Fields(classInfo);
	}

	AObject* Create_Object_Instance( const TbMetaClass& classInfo );
	AObject* Create_Object_Instance( const TypeGUID& classGuid );

	template< class CLASS >
	AObject* Create_Object_Instance()
	{
		AObject* pNewInstance = Create_Object_Instance( CLASS::metaClass() );
		return UpCast< CLASS >( pNewInstance );
	}

}//namespace ObjectUtil




///!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
/// NOTE: can only be used on classes deriving from AObject
///
#define mxDECLARE_SERIALIZABLE( CLASS )\
	friend mxArchive& operator && ( mxArchive & archive, CLASS *& o )\
	{\
		archive.SerializePointer( *(AObject**)&o );\
		return archive;\
	}\
	friend mxArchive& operator && ( mxArchive & archive, TPtr<CLASS> & o )\
	{\
		archive.SerializePointer( *(AObject**)&o._ptr );\
		return archive;\
	}

/*
	friend mxArchive& operator && ( mxArchive & archive, TRefPtr<CLASS> & o )\
	{\
		if( archive.IsWriting() )\
		{\
			AObject* basePtr = o;\
			archive.SerializePointer( basePtr );\
		}\
		else\
		{\
			AObject* basePtr;\
			archive.SerializePointer( basePtr );\
			o = UpCast<CLASS>( basePtr );\
		}\
		return archive;\
	}
*/
