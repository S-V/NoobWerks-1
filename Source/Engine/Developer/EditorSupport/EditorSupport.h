#pragma once

#include <Core/Assets/AssetManagement.h>

namespace EditorUtil
{
	//-----------------------------------------------------------------------
	// Reference tracking
	//-----------------------------------------------------------------------

	struct ObjectInfo
	{
		CStruct *		o;
		const TbMetaClass *	type;
		UINT			numReferences;
	};
	struct ObjectReference
	{
		CStruct *		o;
		const TbMetaClass *	type;
		void *			pointer;	// we may want to patch this pointer (e.g. upon object deletion)

	public:
		ObjectReference()
		{
			o = nil;
			type = nil;
			pointer = nil;
		}
		ObjectReference( const ObjectReference& other )
		{
			o = other.o;
			type = other.type;
			pointer = other.pointer;
		}
		bool operator == ( const ObjectReference& other ) const
		{
			return o == other.o
				&& type == other.type
				&& pointer == other.pointer
				;
		}
		bool operator != ( const ObjectReference& other ) const
		{
			return !(*this == other);
		}
		bool DbgCheckValid() const
		{
			chkRET_FALSE_IF_NIL(o);
			chkRET_FALSE_IF_NIL(type);
			chkRET_FALSE_IF_NIL(pointer);
			return true;
		}
	};

	typedef TArray< ObjectReference >	ListOfReferences;

	void DBG_PrintReferences( const ListOfReferences& references );

	UINT CollectPointersFromObject( CStruct* o, const TbMetaClass& type, ListOfReferences &references );

	UINT GetNumReferencesToObject( CStruct* o, const NwClump& clump );
	UINT CollectReferencesToObject( CStruct* o, const NwClump& clump, ListOfReferences &references );

	void PatchReferences( ListOfReferences & references, void* newValue );

	bool TryDeleteAndCleanupGarbage( CStruct* o, const TbMetaClass& type, NwClump & clump );

	void DeleteUnreferencedObjects( NwClump& clump );

	//-----------------------------------------------------------------------
	// Asset management
	//-----------------------------------------------------------------------

	//struct AssetTypeInfo
	//{
	//	String	className;		// name of C++ class representing the asset (e.g. Rendering::NwMesh, NwTexture)
	//	String	assetType;		// name of asset type (e.g. Mesh, Texture)
	//	String	fileExtension;	// exported asset file extension with dot, e.g. '.mesh', '.texture'
	//};
	//void GetListOfAssetTypeInfo( AssetTypeInfo &output );

}//namespace EditorUtil

//namespace AssetPipeline
//{
//	extern const Chars	ASSET_TYPE_MESH;
//	extern const Chars	ASSET_TYPE_TEXTURE;
//	extern const Chars	ASSET_TYPE_MATERIAL;
//	extern const Chars	ASSET_TYPE_SCRIPT;
//	extern const Chars	ASSET_TYPE_SHADER;
//	extern const Chars	ASSET_TYPE_PHYSICS_MODEL;
//	extern const Chars	ASSET_TYPE_COLLISION_HULL;
//	extern const Chars	ASSET_TYPE_MESH_ANIMATION;
//	extern const Chars	ASSET_TYPE_WAY_POINT_DEF;
//	extern const Chars	ASSET_TYPE_LEVEL_CHUNK;
//
//}//namespace AssetPipeline

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
