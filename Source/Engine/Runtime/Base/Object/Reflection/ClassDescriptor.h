/*
=============================================================================
	File:	ClassDescriptor.h
	Desc:	Class reflection metadata descriptor.
	ToDo:	strip class names from the executable in release version
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/Reflection.h>

/*
=============================================================================
	ASSET LOADING
=============================================================================
*/

// each metaclass has a pointer to TbAssetLoaderI ('resource type/class')
class TbAssetLoaderI;

template< typename TYPE >
struct TbAssetTypeTraits
{
	static inline TbAssetLoaderI* getAssetLoader()
	{
		return nil;
	}
};

/*
=============================================================================
	Object factory
=============================================================================
*/

// function prototypes (used for creating object instances by type information)

// allocates memory and calls constructor
typedef void* F_CreateObject();
// constructs the object in-place (used to init vtables)
typedef void F_ConstructObject( void* objMem );
//F_DestroyObject
typedef void F_DestructObject( void* objMem );

//
// Object factory function generators,
// wrapped in the common 'friend' class.
// this is wrapped in a struct rather than a namespace
// to allow access to private/protected class members
// (via 'friend' keyword).
//
struct TypeHelper
{
	template< typename TYPE >
	static inline void* CreateObjectTemplate()
	{
		//@todo: get rid of warning C4316 : object allocated on the heap may not be aligned 16
		return new TYPE();
	}
	template< typename TYPE >
	static inline void ConstructObjectTemplate( void* objMem )
	{
		::new (objMem) TYPE();
	}
	template< typename TYPE >
	static inline void DestructObjectTemplate( void* objMem )
	{
		((TYPE*)objMem)->TYPE::~TYPE();
	}
};

//
//	contains some common parameters for initializing TbMetaClass structure
//
struct SClassDescription: public TbTypeSizeInfo
{
	F_CreateObject *	creator;	// 'create' function, null for abstract classes
	F_ConstructObject *	constructor;// 'create in place' function, null for abstract classes
	F_DestructObject *	destructor;

public:
	inline SClassDescription()
	{
		creator = nil;
		constructor = nil;
		destructor = nil;
	}
	inline SClassDescription(ENoInit)
		: TbTypeSizeInfo(_NoInit)
	{}
	template< typename CLASS >
	static inline SClassDescription For_Class_With_Default_Ctor()
	{
		SClassDescription	class_info(_NoInit);
		class_info.Collect_Common_Properties_For_Class< CLASS >();

		class_info.alignment = mxALIGNOF( CLASS );

		class_info.creator = TypeHelper::CreateObjectTemplate< CLASS >;
		class_info.constructor = TypeHelper::ConstructObjectTemplate< CLASS >;
		class_info.destructor = TypeHelper::DestructObjectTemplate< CLASS >;

		return class_info;
	}
	template< typename CLASS >
	static inline SClassDescription For_Class_With_No_Default_Ctor()
	{
		SClassDescription	class_info(_NoInit);
		class_info.Collect_Common_Properties_For_Class< CLASS >();

		class_info.alignment = mxALIGNOF( CLASS );

		class_info.creator = nil;
		class_info.constructor = nil;
		class_info.destructor = TypeHelper::DestructObjectTemplate< CLASS >;

		return class_info;
	}
	template< typename CLASS >
	static inline SClassDescription For_Abstract_Class()
	{
		SClassDescription	class_info(_NoInit);
		class_info.Collect_Common_Properties_For_Class< CLASS >();

		class_info.alignment = 0;

		class_info.creator = nil;
		class_info.constructor = nil;
		class_info.destructor = TypeHelper::DestructObjectTemplate< CLASS >;

		return class_info;
	}
private:
	template< typename CLASS >
	inline void Collect_Common_Properties_For_Class()
	{
		this->size = sizeof(CLASS);
	}
};

/*
-------------------------------------------------------------------------
	TbMetaClass

	this is a base class for providing information about C++ classes

	NOTE: only single inheritance class hierarchies are supported!
-------------------------------------------------------------------------
*/
class TbMetaClass: public TbMetaType
{
public:
	const char *	GetTypeName() const;	// GetTypeName, because GetClassName is defined in Windows headers.
	const TypeGUID	GetTypeGUID() const;

	// Returns the size of a single instance of the class, in bytes.
	size_t		GetInstanceSize() const;

	const TbClassLayout& GetLayout() const;

	const TbMetaClass *	GetParent() const;

	bool	IsDerivedFrom( const TbMetaClass& other ) const;
	bool	IsDerivedFrom( const TypeGUID& typeCode ) const;
	bool	IsDerivedFrom( const char* class_name ) const;

	bool	operator == ( const TbMetaClass& other ) const;
	bool	operator != ( const TbMetaClass& other ) const;

	// Returns 'true' if this type inherits from the given type.
	template< class CLASS >	// where CLASS : AObject
	inline bool IsA() const
	{
		return this->IsDerivedFrom( CLASS::metaClass() );
	}

	// Returns 'true' if this type is the same as the given type.
	template< class CLASS >	// where CLASS : AObject
	inline bool Is() const
	{
		return this == &CLASS::metaClass();
	}

	bool IsAbstract() const;
	bool IsConcrete() const;

	F_CreateObject *	GetCreator() const;
	F_ConstructObject *	GetConstructor() const;
	F_DestructObject *	GetDestructor() const;

	AObject* CreateInstance() const;
	bool ConstructInPlace( void* o ) const;
	bool DestroyInstance( void* o ) const;

	bool IsEmpty() const;

public_internal:

	// these constructors are wrapped in macros
	TbMetaClass(
		const char* class_name,
		const TypeGUID& class_guid,
		const TbMetaClass* const base_class,
		const SClassDescription& class_info,
		const TbClassLayout& reflected_members,
		TbAssetLoaderI* asset_loader
		);

private:
	const TypeGUID			m_uid;	// unique type identifier

	const TbMetaClass * const	m_base;	// base class of this class

	const TbClassLayout &	m_members;	// reflected members of this class (not including inherited members)

//#if MX_USE_FAST_RTTI
//	TypeGUID	m_dynamicId;	// dynamic id of this class
//	TypeGUID	m_lastChild;	// dynamic id of the last descendant of this class
//#endif // MX_USE_FAST_RTTI

	F_CreateObject *	m_creator;
	F_ConstructObject *	m_constructor;
	F_DestructObject *	m_destructor;

	// these are used for building the linked list of registered classes
	// during static initialization
	TbMetaClass *			m_next;
	// this is the head of the singly linked list of all class descriptors
	static TbMetaClass *	m_head;


	friend class TypeRegistry;
	// this is used for inserting into a hash table
	TbMetaClass *		m_link;

public:
	/// callbacks for resource loading;
	/// If nil, the default resource loader will be used.
	TbAssetLoaderI *	loader;	// callbacks for asset loading

	//void *	fallbackInstance;	// [optional] e.g. a pointer to the fallback asset instance
	//const class EdClassInfo *	editorInfo;	// additional metadata (loaded from external files)
	//UINT32		allocationGranularity;	// new objects in clumps should be allocated in batches

private:
	NO_COPY_CONSTRUCTOR( TbMetaClass );
	NO_ASSIGNMENT( TbMetaClass );
};

#include <Base/Object/Reflection/ClassDescriptor.inl>

/*
=============================================================================
	
	Macros for declaring and implementing type information

=============================================================================
*/


//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// NOTE: 'virtual' is omitted intentionally in order to avoid vtbl placement in 'non-virtual' classes.
//
#define mxDECLARE_CLASS_COMMON( CLASS, BASE_KLASS )\
	mxUTIL_CHECK_BASE_CLASS( CLASS, BASE_KLASS );\
	private:\
		static TbMetaClass ms_staticTypeInfo;\
		friend class TypeHelper;\
	public:\
		mxFORCEINLINE static TbMetaClass& metaClass() { return ms_staticTypeInfo; };\
		mxFORCEINLINE /*virtual*/ TbMetaClass& rttiClass() const { return ms_staticTypeInfo; }\
		typedef CLASS ThisType;\
		typedef BASE_KLASS Super;\



//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	This macro must be included in the definition of any subclass of AObject (usually in header files).
//	This must be used on single inheritance concrete classes only!
//-----------------------------------------------------------------------
//
#define mxDECLARE_CLASS( CLASS, BASE_KLASS )\
	mxDECLARE_CLASS_COMMON( CLASS, BASE_KLASS );\



//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	This macro must be included in the definition of any abstract subclass of AObject (usually in header files).
//	This must be used on single inheritance abstract classes only!
//-----------------------------------------------------------------------
//
#define mxDECLARE_ABSTRACT_CLASS( CLASS, BASE_KLASS )\
	mxDECLARE_CLASS_COMMON( CLASS, BASE_KLASS );\
	NO_ASSIGNMENT(CLASS);\
	mxUTIL_CHECK_ABSTRACT_CLASS( CLASS );



//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxDEFINE_CLASS must be included in the implementation of any subclass of AObject (usually in source files).
//-----------------------------------------------------------------------
//
#define mxDEFINE_CLASS( CLASS )\
	TbMetaClass	CLASS::ms_staticTypeInfo(\
					mxEXTRACT_TYPE_NAME( CLASS ), mxEXTRACT_TYPE_GUID( CLASS ),\
					&T_DeduceClass< Super >(),\
					SClassDescription::For_Class_With_Default_Ctor< CLASS >(),\
					CLASS::staticLayout(),\
					CLASS::getAssetLoader()\
					);


//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxDEFINE_CLASS_NO_DEFAULT_CTOR must be included in the implementation of any subclass of AObject (usually in source files).
//-----------------------------------------------------------------------
//
#define mxDEFINE_CLASS_NO_DEFAULT_CTOR( CLASS )\
	TbMetaClass	CLASS::ms_staticTypeInfo(\
					mxEXTRACT_TYPE_NAME( CLASS ), mxEXTRACT_TYPE_GUID( CLASS ),\
					&Super::metaClass(),\
					SClassDescription::For_Class_With_No_Default_Ctor< CLASS >(),\
					CLASS::staticLayout(),\
					CLASS::getAssetLoader()\
					);

//== MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	mxDEFINE_ABSTRACT_CLASS must be included in the implementation of any abstract subclass of AObject (usually in source files).
//-----------------------------------------------------------------------
//
#define mxDEFINE_ABSTRACT_CLASS( CLASS )\
	TbMetaClass	CLASS::ms_staticTypeInfo(\
					mxEXTRACT_TYPE_NAME( CLASS ), mxEXTRACT_TYPE_GUID( CLASS ),\
					&Super::metaClass(),\
					SClassDescription::For_Abstract_Class< CLASS >(),\
					CLASS::staticLayout(),\
					CLASS::getAssetLoader()\
					);


//-----------------------------------------------------------------------
// Helper macros used for type checking
//-----------------------------------------------------------------------

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// compile-time inheritance test for catching common typing errors
//
#define mxUTIL_CHECK_BASE_CLASS( CLASS, BASE_KLASS )\
	private:\
		static void PP_JOIN( CLASS, __SimpleInheritanceCheck )()\
		{\
			BASE_KLASS* base;\
			CLASS* derived = static_cast<CLASS*>( base );\
			(void)derived;\
		}\
	public:


//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// all abstract classes must be derived from AObject
//
#define mxUTIL_CHECK_ABSTRACT_CLASS( CLASS )\
	private:\
		static void PP_JOIN( Abstract_Class_,\
						PP_JOIN( CLASS, _Must_Derive_From_AObject )\
					)()\
		{\
			AObject* base;\
			CLASS* derived = static_cast<CLASS*>( base );\
			(void)derived;\
		}\
	public:


//-----------------------------------------------------------------
//	STypedObject
//-----------------------------------------------------------------
//
struct STypedObject
{
	CStruct *		o;
	const TbMetaClass *	type;

public:
	STypedObject();

	bool IsValid() const
	{
		return o != nil;
	}
};

//-----------------------------------------------------------------
//	SClassIdType
//-----------------------------------------------------------------
//
struct SClassId
{
	const TbMetaClass* type;
};
struct SClassIdType: public TbMetaType
{
	inline SClassIdType()
		: TbMetaType( Type_ClassId, mxEXTRACT_TYPE_NAME(SClassId), TbTypeSizeInfo::For_Type< SClassId >() )
	{}

	static SClassIdType ms_staticInstance;
};

template<>
struct TypeDeducer< const TbMetaClass* >
{
	static inline ETypeKind GetTypeKind()	{ return ETypeKind::Type_ClassId; }
	static inline const TbMetaType& GetType()	{ return SClassIdType::ms_staticInstance; }
};
template<>
struct TypeDeducer< TbMetaClass* >
{
	static inline ETypeKind GetTypeKind()	{ return ETypeKind::Type_ClassId; }
	static inline const TbMetaType& GetType()	{ return SClassIdType::ms_staticInstance; }
};
template<>
struct TypeDeducer< TPtr< const TbMetaClass > >
{
	static inline ETypeKind GetTypeKind()	{ return ETypeKind::Type_ClassId; }
	static inline const TbMetaType& GetType()	{ return SClassIdType::ms_staticInstance; }
};
template<>
struct TypeDeducer< TPtr< TbMetaClass > >
{
	static inline ETypeKind GetTypeKind()	{ return ETypeKind::Type_ClassId; }
	static inline const TbMetaType& GetType()	{ return SClassIdType::ms_staticInstance; }
};

// K usually has a member function:
// void SetObject( void* o, const TbMetaClass& type )
// 
template< class T, class K >
void TSetObject( K & k, T & t )
{
	k.SetObject( &t, T_DeduceClass< T >() );
}
