/*
=============================================================================
meta type database
(description of properties of C++ types for building convenient editor GUIs)
=============================================================================
*/
#include <Base/Base.h>
#include <TypeDatabase/TypeDatabase.h>
#include <Core/Serialization/Text/TxTSerializers.h>

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
mxBEGIN_FLAGS( PropertyFlagsT )
mxREFLECT_BIT( PF_HideInPropertyGrid, EPropertyFlags::PF_HideInPropertyGrid ),
mxREFLECT_BIT( PF_ReadOnly, EPropertyFlags::PF_ReadOnly ),
mxEND_FLAGS;

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
mxBEGIN_FLAGS( ClassFlagsT )
mxREFLECT_BIT( CF_HiddenInEditor, EClassFlags::CF_HiddenInEditor ),
mxREFLECT_BIT( CF_ManagedInternally, EClassFlags::CF_ManagedInternally ),
mxREFLECT_BIT( CF_UsedOnlyInEditor, EClassFlags::CF_UsedOnlyInEditor ),
mxEND_FLAGS;

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdStructField );
mxBEGIN_REFLECTION( EdStructField )
mxMEMBER_FIELD( baseTypeName ),
mxMEMBER_FIELD( cppFieldName ),
mxMEMBER_FIELD( nameInEditor ),
mxMEMBER_FIELD( description ),
mxMEMBER_FIELD( annotation ),
mxMEMBER_FIELD( statusTip ),
mxMEMBER_FIELD( toolTip ),
mxMEMBER_FIELD( widget ),
mxMEMBER_FIELD( flags ),
mxEND_REFLECTION;

EdStructField::EdStructField()
{
	flags = PF_DefaultFlags;
}

/*
-----------------------------------------------------------------------------
EdIntegerField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdIntegerField );
mxBEGIN_REFLECTION( EdIntegerField )
mxMEMBER_FIELD( minimumValue ),
mxMEMBER_FIELD( maximumValue ),
mxMEMBER_FIELD( defaultValue ),
mxMEMBER_FIELD( singleStep ),
mxMEMBER_FIELD( wraps_around ),
mxEND_REFLECTION;

EdIntegerField::EdIntegerField()
{
	widget.setReference("SpinBox");

	minimumValue = 0;
	maximumValue = 10000;
	defaultValue = 0;
	singleStep = 1;
	wraps_around = false;
}

/*
-----------------------------------------------------------------------------
EdBooleanField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdRealField );
mxBEGIN_REFLECTION( EdRealField )
mxMEMBER_FIELD( minimumValue ),
mxMEMBER_FIELD( maximumValue ),
mxMEMBER_FIELD( defaultValue ),
mxMEMBER_FIELD( singleStep ),
mxMEMBER_FIELD( numDigits ),
mxMEMBER_FIELD( wraps_around ),
mxEND_REFLECTION;

EdRealField::EdRealField()
{
	widget.setReference("DoubleSpinBox");

	minimumValue = -1e6;
	maximumValue = +1e6;
	defaultValue = 0;
	singleStep = 0.001;
	numDigits = 7;
	wraps_around = false;
}

/*
-----------------------------------------------------------------------------
EdBooleanField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdBooleanField );
mxBEGIN_REFLECTION( EdBooleanField )
mxMEMBER_FIELD( defaultValue ),
mxEND_REFLECTION;

EdBooleanField::EdBooleanField()
{
	widget.setReference("CheckBox");

	defaultValue = false;
}

/*
-----------------------------------------------------------------------------
EdStringField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdStringField );
mxBEGIN_REFLECTION( EdStringField )
mxMEMBER_FIELD( defaultValue ),
mxEND_REFLECTION;

EdStringField::EdStringField()
{
	widget.setReference("LineEdit");
}

/*
-----------------------------------------------------------------------------
EdPointerField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdPointerField );
mxBEGIN_REFLECTION( EdPointerField )
mxMEMBER_FIELD( pointeeType ),
mxEND_REFLECTION;

EdPointerField::EdPointerField()
{
}

/*
-----------------------------------------------------------------------------
EdArrayField
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdArrayField );
mxBEGIN_REFLECTION( EdArrayField )
mxMEMBER_FIELD( itemType ),
mxEND_REFLECTION;

EdArrayField::EdArrayField()
{
}

/*
-----------------------------------------------------------------------------
EdClassInfo
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( EdClassInfo );
mxBEGIN_REFLECTION( EdClassInfo )
mxMEMBER_FIELD( cppClassName ),
mxMEMBER_FIELD( nameInEditor ),
//mxMEMBER_FIELD( hideInEditor ),
mxMEMBER_FIELD( flags ),
mxMEMBER_FIELD( integers ),
mxMEMBER_FIELD( booleans ),
mxMEMBER_FIELD( floats ),
mxMEMBER_FIELD( strings ),
mxMEMBER_FIELD( structs ),
mxEND_REFLECTION;

EdClassInfo::EdClassInfo()
{
	//hideInEditor = false;
	flags = CF_DefaultFlags;
}

#if 0
const char* EdClassInfo::GetEditorName( const TbMetaClass& classInfo )
{
	const EdClassInfo* metaClass = classInfo.editorInfo;
	if( metaClass != nil ) {
		return metaClass->nameInEditor.raw();
	}
	return classInfo.GetTypeName();
}

ClassFlagsT EdClassInfo::GetClassFlags( const TbMetaClass& classInfo )
{
	const EdClassInfo* metaClass = classInfo.editorInfo;
	if( metaClass != nil ) {
		return metaClass->flags;
	}
	return CF_DefaultFlags;
}
#endif
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

mxDEFINE_CLASS( EdTypeDatabase );
mxBEGIN_REFLECTION( EdTypeDatabase )
mxMEMBER_FIELD( classes ),
mxEND_REFLECTION;

EdTypeDatabase::EdTypeDatabase()
{
}

EdTypeDatabase::~EdTypeDatabase()
{
}

static void InitFieldInfo( EdStructField& fieldInfo, const MetaField& field )
{
	UNDONE;
	//fieldInfo.baseTypeName.setReference(field.type.m_name);
	//fieldInfo.cppFieldName.setReference(Chars(field.name,_InitSlow));
	//fieldInfo.nameInEditor.setReference(Chars(field.name,_InitSlow));
}

static void AddCppField( EdClassInfo& classInfo, const MetaField& field )
{
	const TbMetaType& fieldType = field.type;
	const ETypeKind typeKind = fieldType.m_kind;

	switch( typeKind )
	{
	case ETypeKind::Type_Integer :
		{
			EdIntegerField& fieldInfo = classInfo.integers.AddNew();
			InitFieldInfo( fieldInfo, field );
		} break;

	case ETypeKind::Type_Float :
		{
			EdRealField& fieldInfo = classInfo.floats.AddNew();
			InitFieldInfo( fieldInfo, field );
		} break;

	case ETypeKind::Type_Bool :
		{
			EdBooleanField& fieldInfo = classInfo.booleans.AddNew();
			InitFieldInfo( fieldInfo, field );
		} break;

		//case ETypeKind::Type_Enum :
		//	{
		//		const mxEnumType& enumInfo = typeInfo.UpCast<mxEnumType>();
		//		result = Serialize_Enum( enumInfo, objAddr );
		//	}
		//	break;

		//case ETypeKind::Type_Flags :
		//	{
		//		const mxFlagsType& flagsType = typeInfo.UpCast<mxFlagsType>();
		//		result = Serialize_Flags( flagsType, objAddr );
		//	}
		//	break;

	case ETypeKind::Type_String :
		{
			EdStringField& fieldInfo = classInfo.strings.AddNew();
			InitFieldInfo( fieldInfo, field );
		} break;

	case ETypeKind::Type_Class :
		{
			//const TbMetaClass& classDescriptor = fieldType.UpCast< TbMetaClass >();
			EdStructField& fieldInfo = classInfo.structs.AddNew();
			InitFieldInfo( fieldInfo, field );
		} break;

	//case ETypeKind::Type_Pointer :
	//	{
	//		const mxPointerType& pointerType = fieldType.UpCast< mxPointerType >();
	//		EdPointerField	fieldDesc;
	//		fieldDesc.InitFromField( fieldInfo );
	//		fieldDesc.pointeeType.Copy(Chars(pointerType.m_pointeeType.GetTypeName()));
	//		fieldNode = JSON_EncodeObjectIncludingTypeInfo( fieldDesc );
	//	} break;

	//	//case ETypeKind::Type_AssetRef :
	//	//	{
	//	//		const mxAssetReferenceType& handleType = typeInfo.UpCast<mxAssetReferenceType>();
	//	//		result = Serialize_AssetReference( handleType, objAddr );
	//	//	} break;

	//case ETypeKind::Type_Array :
	//	{
	//		const mxArray& arrayType = fieldType.UpCast< mxArray >();
	//		EdArrayField	fieldDesc;
	//		fieldDesc.InitFromField( fieldInfo );
	//		fieldDesc.itemType.Copy(Chars(arrayType.m_elemType.GetTypeName()));
	//		fieldNode = JSON_EncodeObjectIncludingTypeInfo( fieldDesc );
	//	} break;

	default:

#if MX_DEVELOPER
		ptWARN("TypeDatabase::Visit_Field() : Unknown type (%s) of field %s\n",
			ETypeKind_To_Chars(typeKind),
			field.name
			);
#endif // MX_DEVELOPER
		break;

	}//switch
}

static void AddCppClass( TbMetaClass& type, void* userData )
{
	if( type.IsDerivedFrom(EdStructField::metaClass())
		|| type.IsDerivedFrom(EdTypeDatabase::metaClass())
		)
	{
		return;
	}

	EdTypeDatabase& typeDb = *static_cast< EdTypeDatabase* >( userData );

	EdClassInfo& newClassInfo = typeDb.classes.AddNew();
	UNDONE;
	//newClassInfo.cppClassName.setReference(type.m_name);
	//newClassInfo.nameInEditor.setReference(type.m_name);
	newClassInfo.flags = CF_DefaultFlags;

	const TbClassLayout& layout = type.GetLayout();
	for( int iField = 0 ; iField < layout.numFields; iField++ )
	{
		const MetaField& field = layout.fields[ iField ];
		AddCppField( newClassInfo, field );
	}
}

void EdTypeDatabase::PopulateFromTypeRegistry()
{
	UNDONE;
#if 0
	TypeRegistry& typeRegistry = TypeRegistry::Get();
	mxASSERT(typeRegistry.IsInitialized());

	typeRegistry.ForAllClasses( &AddCppClass, this );
#endif
}

//void EdTypeDatabase::AddClassFromFile( const char* path )
//{
//	EdClassInfo& newClassInfo = classes.AddNew();
//	if(mxFAILED(JSON::LoadObjectFromFile(path, newClassInfo)))
//	{
//		ptWARN("EdTypeDatabase: Failed to load class at path '%s'\n", path);
//		classes.PopLast();
//	}
//	else
//	{
//		ptPRINT("EdTypeDatabase: Loaded metadata for class '%s'\n", newClassInfo.cppClassName.raw());
//	}
//}

#if 0
void EdTypeDatabase::LinkToEngineClasses() const
{
	for( UINT i = 0 ; i < classes.num(); i++ )
	{
		const EdClassInfo& classInfo = classes[ i ];
		TbMetaClass* engineClass = (TbMetaClass*) TypeRegistry::FindClassByName( classInfo.cppClassName.raw() );
		if( engineClass ) {
			engineClass->editorInfo = &classInfo;
		} else {
			ptWARN("EdTypeDatabase: Failed to find engine class '%s'\n", classInfo.cppClassName.raw());
		}
	}
}

void EdTypeDatabase::UnlinkFromEngineClasses() const
{
	for( UINT i = 0 ; i < classes.num(); i++ )
	{
		const EdClassInfo& classInfo = classes[ i ];
		TbMetaClass* engineClass = (TbMetaClass*) TypeRegistry::FindClassByName( classInfo.cppClassName.raw() );
		if( engineClass ) {
			engineClass->editorInfo = nil;
		}
	}
}
#endif

const EdClassInfo* EdTypeDatabase::FindClassInfo( const TbMetaClass& type ) const
{
	for( UINT i = 0 ; i < classes.num(); i++ )
	{
		const EdClassInfo& classInfo = classes[ i ];
		if( strcmp(classInfo.cppClassName.raw(), type.GetTypeName()) == 0 )
		{
			return &classInfo;
		}
	}
	return nil;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
