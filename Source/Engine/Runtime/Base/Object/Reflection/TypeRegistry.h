/*
=============================================================================
	File:	TypeRegistry.h
	Desc:	TypeRegistry - central object type database,
			also serves as an object factory - creates objects by a class name.
			Classes register themselves in the factory
			through the macros DECLARE_CLASS and DEFINE_CLASS.
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/ClassDescriptor.h>

// Forward declarations.
class String;
class AObject;

//
//	TypeRegistry
//	
//	This class maintains a map of class names and GUIDs to factory functions.
//	NOTE: must be class, because it's a friend of TbMetaClass.
//
class TypeRegistry
{
public:
	// must be called after all type infos have been statically initialized
	static ERet Initialize();

	// NOTE: this must be called at the end of the main function to delete the instance of TypeRegistry.
	static void Destroy();

	static bool IsInitialized();

public:
	static const TbMetaClass* FindClassByGuid( const TypeGUID& typeCode );
	static const TbMetaClass* FindClassByName( const char* class_name );

	static AObject* CreateInstance( const TypeGUID& typeCode );

	static void EnumerateDescendants( const TbMetaClass& base_class, TArray<const TbMetaClass*> &OutClasses );
	static void EnumerateConcreteDescendants( const TbMetaClass& base_class, TArray<const TbMetaClass*> &OutClasses );

	// return TRUE to exit from the function, FALSE - to continue.
	typedef bool F_ClassIterator( TbMetaClass & type, void* userData );
	static void ForAllClasses( F_ClassIterator* visitor, void* userData );

private:
	//@todo: move these into .cpp
	enum { TABLE_SIZE = 1024 };	// number of buckets, each bucket is a linked list

	// for fast lookup by class id and name (and for detecting duplicate names)
	static TbMetaClass *	m_heads[TABLE_SIZE];	// heads of linked lists of TbMetaClass
};
