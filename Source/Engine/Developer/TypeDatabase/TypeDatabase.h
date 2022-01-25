/*
=============================================================================
meta type database
(description of properties of C++ types for building convenient editor GUIs)
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Set/Set.h>
#include <Base/Template/Containers/HashMap/THashMap.h>
#include <Core/Assets/AssetManagement.h>

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
enum EPropertyFlags
{
	PF_ReadOnly				= BIT(0),

	PF_HideInPropertyGrid	= BIT(1),

	// default combination of flags
	PF_DefaultFlags = 0
};
mxDECLARE_FLAGS( EPropertyFlags, U32, PropertyFlagsT );

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
enum EClassFlags
{
	// don't show instances of this class in the editor browser windows
	CF_HiddenInEditor		= BIT(0),

	// e.g. instances of this class cannot be deleted inside the editor
	CF_ManagedInternally	= BIT(1),

	// don't load objects of this type in release mode
	CF_UsedOnlyInEditor		= BIT(2),

	// default combination of flags
	CF_DefaultFlags = 0
};
mxDECLARE_FLAGS( EClassFlags, U32, ClassFlagsT );

/*
-----------------------------------------------------------------------------
contains editor properties of a generic field in some C++ structure
-----------------------------------------------------------------------------
*/
struct EdStructField: CStruct
{
	String32		baseTypeName;	// name of the corresponding C++ type
	String32		cppFieldName;	// name of class field in C++ code
	String32		nameInEditor;	// name in the property grid
	String32		description;	// what's this?
	String32		annotation;
	String32		statusTip;
	String32		toolTip;	// tooltip popup
	String32		widget;		// GUI widget for displaying/editing this item
	PropertyFlagsT	flags;
public:
	mxDECLARE_CLASS( EdStructField, CStruct );
	mxDECLARE_REFLECTION;
	EdStructField();
};

/*
-----------------------------------------------------------------------------
EdIntegerField
-----------------------------------------------------------------------------
*/
struct EdIntegerField : EdStructField
{
	long	minimumValue;
	long	maximumValue;
	long	defaultValue;
	long	singleStep;
	bool	wraps_around;
public:
	mxDECLARE_CLASS( EdIntegerField, EdStructField );
	mxDECLARE_REFLECTION;
	EdIntegerField();
};

/*
-----------------------------------------------------------------------------
EdRealField
-----------------------------------------------------------------------------
*/
struct EdRealField : EdStructField
{
	double	minimumValue;
	double	maximumValue;
	double	defaultValue;
	double	singleStep;
	int		numDigits;
	bool	wraps_around;
public:
	mxDECLARE_CLASS( EdRealField, EdStructField );
	mxDECLARE_REFLECTION;
	EdRealField();
};

/*
-----------------------------------------------------------------------------
EdBooleanField
-----------------------------------------------------------------------------
*/
struct EdBooleanField : EdStructField
{
	bool	defaultValue;
public:
	mxDECLARE_CLASS( EdBooleanField, EdStructField );
	mxDECLARE_REFLECTION;
	EdBooleanField();
};

/*
-----------------------------------------------------------------------------
EdStringField
-----------------------------------------------------------------------------
*/
struct EdStringField : EdStructField
{
	String	defaultValue;
public:
	mxDECLARE_CLASS( EdStringField, EdStructField );
	mxDECLARE_REFLECTION;
	EdStringField();
};

/*
-----------------------------------------------------------------------------
EdPointerField
-----------------------------------------------------------------------------
*/
struct EdPointerField : EdStructField
{
	String	pointeeType;
public:
	mxDECLARE_CLASS( EdPointerField, EdStructField );
	mxDECLARE_REFLECTION;
	EdPointerField();
};

/*
-----------------------------------------------------------------------------
EdAssetIdField
-----------------------------------------------------------------------------
*/
struct EdAssetIdField : EdStructField
{
	AssetID	assetId;
public:
	mxDECLARE_CLASS( EdAssetIdField, EdStructField );
	mxDECLARE_REFLECTION;
	EdAssetIdField();
};

/*
-----------------------------------------------------------------------------
EdArrayField
-----------------------------------------------------------------------------
*/
struct EdArrayField : EdStructField
{
	String	itemType;
public:
	mxDECLARE_CLASS( EdArrayField, EdStructField );
	mxDECLARE_REFLECTION;
	EdArrayField();
};

/*
-----------------------------------------------------------------------------
EdClassInfo
-----------------------------------------------------------------------------
*/
struct EdClassInfo: CStruct
{
	String			cppClassName;	// name of the corresponding C++ type
	String			nameInEditor;	// name in the property grid
	//bool			hideInEditor;
	ClassFlagsT		flags;
	TArray< EdIntegerField >	integers;
	TArray< EdBooleanField >	booleans;
	TArray< EdRealField >		floats;
	TArray< EdStringField >		strings;
	TArray< EdStructField >		structs;
public:
	mxDECLARE_CLASS( EdClassInfo, CStruct );
	mxDECLARE_REFLECTION;
	EdClassInfo();

	static const char* GetEditorName( const TbMetaClass& type );
	static ClassFlagsT GetClassFlags( const TbMetaClass& type );
};

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
struct EdTypeDatabase: CStruct
{
	TArray< EdClassInfo >	classes;
public:
	mxDECLARE_CLASS( EdClassInfo, CStruct );
	mxDECLARE_REFLECTION;
	EdTypeDatabase();
	~EdTypeDatabase();

	void PopulateFromTypeRegistry();
	void AddClassFromFile( const char* path );

	void LinkToEngineClasses() const;
	void UnlinkFromEngineClasses() const;

	const EdClassInfo* FindClassInfo( const TbMetaClass& type ) const;
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
