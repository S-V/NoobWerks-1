#include "EditorSupport_PCH.h"
#pragma hdrstop
#include <Core/ObjectModel/Clump.h>
//#include <Rendering/Public/Core/Mesh.h>
#include <EditorSupport/EditorSupport.h>

namespace EditorUtil
{

	//-----------------------------------------------------------------------
	// Reference tracking
	//-----------------------------------------------------------------------


	void DBG_PrintReferences( const ListOfReferences& references )
	{
		for( UINT iRef = 0; iRef < references.num(); iRef++ )
		{
			const EditorUtil::ObjectReference& objectReference = references[ iRef ];
			DBGOUT("--- PrintReferences[%u]: %s at 0x%x\n", iRef, objectReference.type->GetTypeName(), objectReference.pointer );
		}
	}

	//-----------------------------------------------------------------------

#if 0
	UINT CollectPointersFromObject( CStruct* o, const TbMetaClass& type, ListOfReferences &references )
	{
		chkRET_X_IF_NIL(o, 0);

		references.RemoveAll();

		struct CollectPointers : public Reflection::AVisitor
		{
			void *	m_object;
			ListOfReferences &	m_references;

		public:
			CollectPointers( CStruct* o, ListOfReferences &references )
				: m_object( o )
				, m_references( references )
			{
			}
			virtual void Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* _userData ) override
			{
				if( !type.m_pointeeType.IsClass() ) {
					ptWARN("CollectPointersFromObject(): pointee type is not a class.\n");
					return;
				}
				const TbMetaClass& pointeeBaseClass = type.m_pointeeType.UpCast< TbMetaClass >();

				const void* pointerValue = p.o;
				if( pointerValue == nil ) {
					return;
				}
				if( pointerValue == m_object ) {
					ptWARN("CollectPointersFromObject(): Object has a pointer to itself!\n");
					return;
				}

				ObjectReference & newReference = m_references.add();

				newReference.o = c_cast(CStruct*) pointerValue;
				newReference.type = &pointeeBaseClass;
				newReference.pointer = &p;
			}
		};

		CollectPointers		collectPointers( o, references );
		collectPointers.Visit_Element( o, type, nil );

		return references.num();
	}

	//-----------------------------------------------------------------------

	UINT GetNumReferencesToObject( CStruct* o, const Clump& clump )
	{
		chkRET_X_IF_NIL(o, 0);

		struct CountReferences : public Reflection::AVisitor
		{
			void *	m_object;
			UINT	m_refCount;

		public:
			CountReferences( CStruct* o )
			{
				m_object = o;
				m_refCount = 0;
			}
			virtual void Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* _userData ) override
			{
				if( p.o == m_object ) {
					++m_refCount;
				}
			}
		};

		CountReferences		countReferences( o );

		NwObjectList* current = clump.GetObjectLists();
		while(PtrToBool( current ))
		{
			current->IterateItems( &countReferences, nil );

			current = current->_next;
		}

		return countReferences.m_refCount;
	}
#endif

	//-----------------------------------------------------------------------

	//UINT CollectReferencesToObject( CStruct* o, const Clump& clump, ListOfReferences &references )
	//{
	//	chkRET_X_IF_NIL(o, 0);

	//	references.RemoveAll();

	//	struct CollectReferences : public Reflection::AVisitor
	//	{
	//		void *	m_object;
	//		ListOfReferences & m_references;

	//	public:
	//		CollectReferences( CStruct* o, ListOfReferences &references )
	//			: m_object( o )
	//			, m_references( references )
	//		{
	//		}
	//		virtual void Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* _userData ) override
	//		{
	//			if( p.o == m_object )
	//			{
	//				ObjectReference & newReference = m_references.add();

	//				AVisitor::SContext	topmostContext;
	//				GetTopLevelContext( context, topmostContext );

	//				newReference.o = c_cast(CStruct*) topmostContext.start;
	//				newReference.type = topmostContext.struc;
	//				newReference.pointer = &p;
	//			}
	//		}
	//	};

	//	CollectReferences		collectReferences( o, references );

	//	NwObjectList* current = clump.GetObjectLists();
	//	while(PtrToBool( current ))
	//	{
	//		current->IterateItems( &collectReferences, nil );

	//		current = current->_next;
	//	}

	//	return references.num();
	//}

	//-----------------------------------------------------------------------

	void PatchReferences( ListOfReferences & references, void* newValue )
	{
		for( UINT iRef = 0; iRef < references.num(); iRef++ )
		{
			const EditorUtil::ObjectReference& objectReference = references[ iRef ];
			DBGOUT("PatchReferences(): %s at 0x%x\n", objectReference.type->GetTypeName(), objectReference.pointer );
			*(const void**)objectReference.pointer = newValue;
		}
	}


	//void GetListOfAssetTypeInfo( AssetTypeInfo &output )
	//{
	//	output.empty();

	//	{
	//		AssetTypeInfo& newItem = output.add;
	//		newItem.className = "Rendering::NwMesh";
	//		newItem.assetType = "Mesh";
	//		newItem.fileExtension = ".mesh";
	//	}
	//	{
	//		AssetTypeInfo& newItem = output.add;
	//		newItem.className = "NwTexture";
	//		newItem.assetType = "Texture";
	//		newItem.fileExtension = ".texture";
	//	}
	//}

}//namespace EditorUtil

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
