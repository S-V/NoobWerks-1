/*
=============================================================================
	File:	Reflection.cpp
	Desc:	Reflection - having access at runtime
			to information about the C++ classes in your program.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/BaseType.h>
#include <Base/Object/Reflection/Reflection.h>
#include <Base/Text/String.h>

mxREFACTOR("shouldn't be here!");
#include <Core/Assets/AssetReference.h>

#define MX_DEBUG_REFLECTION		(0)

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//static
TbClassLayout TbClassLayout::dummy = { nil, 0 };

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace Reflection
{

/*
-------------------------------------------------------------------------
	AVisitor
-------------------------------------------------------------------------
*/
void* AVisitor::Visit_Field( void * _o, const MetaField& _field, void* user_data )
{
	return Walker::Visit( _o, _field.type, this, user_data );
}

void* AVisitor::Visit_Aggregate( void * _o, const TbMetaClass& _type, void* user_data )
{
	return Walker::VisitAggregate( _o, _type, this, user_data );
}

void* AVisitor::Visit_Array( void * _array, const MetaArray& _type, void* user_data )
{
	return Walker::VisitArray( _array, _type, this, user_data );
}

void* AVisitor::Visit_Blob( void * _blob, const mxBlobType& _type, void* user_data )
{
	Unimplemented;
	return user_data;
}

//=====================================================================

void* Walker::Visit( void * _object, const TbMetaType& _type, AVisitor* _visitor, void *user_data )
{
	mxASSERT_PTR(_object);

	switch( _type.m_kind )
	{
	case ETypeKind::Type_Integer :
	case ETypeKind::Type_Float :
	case ETypeKind::Type_Bool :
	case ETypeKind::Type_Enum :
	case ETypeKind::Type_Flags :
		{
			return _visitor->Visit_POD( _object, _type, user_data );
		}
		break;

	case ETypeKind::Type_String :
		{
			String & rString = TPODCast< String >::GetNonConst( _object );
			return _visitor->Visit_String( rString, user_data );
		}
		break;

	case ETypeKind::Type_Class :
		{
			const TbMetaClass& classType = _type.UpCast< TbMetaClass >();
			return _visitor->Visit_Aggregate( _object, classType, user_data );
		}
		break;

	case ETypeKind::Type_Pointer :
		{
			const MetaPointer& pointerType = _type.UpCast< MetaPointer >();
			VoidPointer& pointer = *static_cast< VoidPointer* >( _object );
			return _visitor->Visit_Pointer( pointer, pointerType, user_data );
		}
		break;

	case ETypeKind::Type_AssetReference :
		{
			mxREFACTOR("shouldn't be here!");
			NwAssetRef& asset_ref = *static_cast< NwAssetRef* >( _object );
			return _visitor->Visit_AssetId( asset_ref.id, user_data );
		}
		break;


	case ETypeKind::Type_AssetId :
		{
			AssetID& assetId = *static_cast< AssetID* >( _object );
			return _visitor->Visit_AssetId( assetId, user_data );
		}
		break;

	case ETypeKind::Type_ClassId :
		{
			SClassId * classId = static_cast< SClassId* >( _object );
			return _visitor->Visit_TypeId( classId, user_data );
		}
		break;

	case ETypeKind::Type_UserData :
		{
			const mxUserPointerType& userPointerType = _type.UpCast< mxUserPointerType >();
			return _visitor->Visit_UserPointer( _object, userPointerType, user_data );
		}
		break;

	case ETypeKind::Type_Blob :
		{
			const mxBlobType& blobType = _type.UpCast< mxBlobType >();
			return _visitor->Visit_Blob( _object, blobType, user_data );
		}
		break;

	case ETypeKind::Type_Array :
		{
			const MetaArray& arrayType = _type.UpCast< MetaArray >();
			return _visitor->Visit_Array( _object, arrayType, user_data );
		}
		break;

		mxNO_SWITCH_DEFAULT;
	}//switch

	return nil;
}

void* Walker::VisitArray( void * _array, const MetaArray& _type, AVisitor* _visitor, void* user_data )
{
	mxASSERT_PTR(_array);
	const UINT numObjects = _type.Generic_Get_Count( _array );
	const void* arrayBase = _type.Generic_Get_Data( _array );

	const TbMetaType& itemType = _type.m_itemType;
	const UINT32 itemStride = _type.m_itemSize;

#if MX_DEBUG_REFLECTION
	DBGOUT("[Reflector]: Visit array of '%s' (%u items)\n", itemType.m_name, numObjects);
#endif // MX_DEBUG_REFLECTION

#if MX_DEBUG
	enum { DBG_MAX_ARRAY_CONTENTS_SIZE = 4*mxMEGABYTE };
	const UINT arrayContentsSize = itemStride * numObjects;
	mxASSERT( arrayContentsSize < DBG_MAX_ARRAY_CONTENTS_SIZE );
#endif // MX_DEBUG

	for( UINT iObject = 0; iObject < numObjects; iObject++ )
	{
		const MetaOffset itemOffset = iObject * itemStride;
		void* itemPtr = mxAddByteOffset( c_cast(void*)arrayBase, itemOffset );

		Visit( itemPtr, itemType, _visitor, user_data );
	}
	return user_data;
}

void* Walker::VisitAggregate( void * _struct, const TbMetaClass& _type, AVisitor* _visitor, void *user_data )
{
	mxASSERT_PTR(_struct);
#if MX_DEBUG_REFLECTION
	DBGOUT("[Reflector]: Visit object of class '%s'\n", type.m_name);
#endif // MX_DEBUG_REFLECTION

	// First recursively visit the parent classes.
	const TbMetaClass* parentType = _type.GetParent();
	while( parentType != nil )
	{
		VisitStructFields( _struct, *parentType, _visitor, user_data );
		parentType = parentType->GetParent();
	}

	// Now visit the members of this class.
	VisitStructFields( _struct, _type, _visitor, user_data );

	return user_data;
}

void* Walker::VisitStructFields( void * _struct, const TbMetaClass& _type, AVisitor* _visitor, void* user_data )
{
	const TbClassLayout& layout = _type.GetLayout();
	for( UINT fieldIndex = 0 ; fieldIndex < layout.numFields; fieldIndex++ )
	{
		const MetaField& field = layout.fields[ fieldIndex ];

#if MX_DEBUG_REFLECTION
		DBGOUT("[Reflector]: Visit field '%s' of type '%s' in '%s' at offset '%u'\n",
			field.name, field.type.m_name.raw(), _type.m_name.raw(), field.offset);
#endif // MX_DEBUG_REFLECTION

		void* memberVarPtr = mxAddByteOffset( _struct, field.offset );

		_visitor->Visit_Field( memberVarPtr, field, user_data );
	}
	return user_data;
}

static void ValidatePointer( void* ptr )
{
	if( ptr != nil )
	{
		mxASSERT(mxIsValidHeapPointer(ptr));
		//mxASSERT(_CrtIsValidHeapPointer(ptr));
		// try to read a value from the pointer
		void* value = *(void**)ptr;
		(void)value;
	}
}

void* PointerChecker::Visit_Array( void * arrayObject, const MetaArray& arrayType, void* user_data )
{
	ValidatePointer(arrayType.Generic_Get_Data(arrayObject));
	return user_data;
}

void* PointerChecker::Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* user_data )
{
	ValidatePointer(p.o);
	return user_data;
}

void ValidatePointers( const void* o, const TbMetaClass& type )
{
	PointerChecker	pointerChecker;
	Walker::Visit( const_cast<void*>(o), type, &pointerChecker, nil );
}

void CheckAllPointersAreInRange( const void* o, const TbMetaClass& type, const void* start, UINT32 size )
{
	class PointerRangeValidator: public Reflection::AVisitor
	{
		const void *	m_start;
		const UINT		m_size;

	public:
		PointerRangeValidator( const void* start, UINT size )
			: m_start( start ), m_size( size )
		{}
		virtual void* Visit_String( String & s, void* user_data ) override
		{
			if( s.nonEmpty() ) {
				this->ValidatePointer( s.getBufferAddress() );
			}
			return user_data;
		}
		virtual void* Visit_Array( void * arrayObject, const MetaArray& arrayType, void* user_data ) override
		{
			if( arrayType.IsDynamic() )
			{
				const UINT32 capacity = arrayType.Generic_Get_Capacity( arrayObject );
				if( capacity > 0 )
				{
					const void* arrayBase = arrayType.Generic_Get_Data( arrayObject );
					this->ValidatePointer( arrayBase );
				}
			}
			return Reflection::AVisitor::Visit_Array( arrayObject, arrayType, user_data );
		}
		virtual void* Visit_Pointer( VoidPointer& p, const MetaPointer& type, void* user_data ) override
		{
			if( p.o != nil )
			{
				this->ValidatePointer( p.o );
			}
			return user_data;
		}

	private:
		void ValidatePointer( const void* pointer )
		{
			if( !mxPointerInRange( pointer, m_start, m_size ) )
			{
				ptERROR("Invalid pointer\n");
			}
		}
	};
	PointerRangeValidator	pointerChecker( start, size );
	Reflection::Walker::Visit( const_cast<void*>(o), type, &pointerChecker, nil );
}

//=====================================================================

void Walker2::Visit( void * _memory, const TbMetaType& field_type, AVisitor2* _visitor, const AVisitor2::Context& _context )
{
	switch( field_type.m_kind )
	{
	case ETypeKind::Type_Integer :
	case ETypeKind::Type_Float :
	case ETypeKind::Type_Bool :
	case ETypeKind::Type_Enum :
	case ETypeKind::Type_Flags :
		{
			return _visitor->Visit_POD( _memory, field_type, _context );
		}
		break;

	case ETypeKind::Type_String :
		{
			String & stringReference = TPODCast< String >::GetNonConst( _memory );
			return _visitor->Visit_String( stringReference, _context );
		}
		break;

	case ETypeKind::Type_Class :
		{
			const TbMetaClass& classType = field_type.UpCast< TbMetaClass >();
			return VisitAggregate( _memory, classType, _visitor, _context );
		}
		break;

	case ETypeKind::Type_Pointer :
		{
			const MetaPointer& pointerType = field_type.UpCast< MetaPointer >();
			VoidPointer& pointerReference = *static_cast< VoidPointer* >( _memory );
			return _visitor->Visit_Pointer( pointerReference, pointerType, _context );
		}
		break;

	case ETypeKind::Type_AssetReference :
		{
			NwAssetRef * asset_ref = static_cast< NwAssetRef* >( _memory );

			const MetaAssetReference& asset_reference_type = field_type.UpCast< MetaAssetReference >();

			return _visitor->visit_AssetReference(
				asset_ref
				, asset_reference_type
				, _context
				);
		}
		break;

	case ETypeKind::Type_ClassId :
		{
			SClassId * classId = static_cast< SClassId* >( _memory );
			return _visitor->Visit_TypeId( classId, _context );
		}
		break;

	case ETypeKind::Type_AssetId :
		{
			AssetID& assetId = *static_cast< AssetID* >( _memory );
			return _visitor->Visit_AssetId( assetId, _context );
		}
		break;

	case ETypeKind::Type_UserData :
		{
			const mxUserPointerType& userPointerType = field_type.UpCast< mxUserPointerType >();
			return _visitor->Visit_UserPointer( _memory, userPointerType, _context );
		}
		break;

	case ETypeKind::Type_Blob :
		{
			Unimplemented;
		}
		break;

	case ETypeKind::Type_Array :
		{
			const MetaArray& arrayType = field_type.UpCast< MetaArray >();
			return VisitArray( _memory, arrayType, _visitor, _context );
		}
		break;


	case ETypeKind::Type_Dictionary :
		{
			const MetaDictionary& dictionary_type = field_type.UpCast< MetaDictionary >();

			_visitor->visit_Dictionary( _memory, dictionary_type, _context );
		}
		break;

		mxNO_SWITCH_DEFAULT;
	}//switch
}

void Walker2::Visit( void * _memory, const TbMetaType& _type, AVisitor2* _visitor, void *user_data )
{
	mxASSERT_PTR(_memory);

	AVisitor2::Context	context;
	context.userData = user_data;

	Visit(_memory,_type,_visitor, context);
}

void Walker2::VisitArray( void * _array, const MetaArray& _type, AVisitor2* _visitor, const AVisitor2::Context& _context )
{
	mxASSERT_PTR(_array);
	if( _visitor->Visit_Array(_array,_type,_context) )
	{
		const UINT numObjects = _type.Generic_Get_Count( _array );
		const void* arrayBase = _type.Generic_Get_Data( _array );

		const TbMetaType& itemType = _type.m_itemType;
		const UINT32 itemStride = _type.m_itemSize;

#if MX_DEBUG_REFLECTION
		DBGOUT("[Reflector]: Visit array of '%s' (%u items)\n", itemType.m_name, numObjects);
#endif // MX_DEBUG_REFLECTION

#if MX_DEBUG
		enum { DBG_MAX_ARRAY_CONTENTS_SIZE = 4*mxMEGABYTE };
		const UINT arrayContentsSize = itemStride * numObjects;
		mxASSERT( arrayContentsSize < DBG_MAX_ARRAY_CONTENTS_SIZE );
#endif // MX_DEBUG

		AVisitor2::Context	itemContext( _context.depth + 1 );
		itemContext.parent = &_context;
		itemContext.userData = _context.userData;

		for( UINT32 iObject = 0; iObject < numObjects; iObject++ )
		{
			const MetaOffset itemOffset = iObject * itemStride;
			void* itemPtr = mxAddByteOffset( c_cast(void*)arrayBase, itemOffset );

			//_visitor->Visit_Array_Item( itemPtr, itemType, iObject, itemContext );

			Visit( itemPtr, itemType, _visitor, itemContext );
		}
	}
}

void Walker2::visit_Items_stored_in_Dictionary(
							   void * instance
							   , const MetaDictionary& dictionary_type
							   , AVisitor2* visitor
							   , const AVisitor2::Context& context
							   )
{
	mxASSERT_PTR(instance);

	AVisitor2::Context	itemContext( context.depth + 1 );
	itemContext.parent = &context;
	itemContext.userData = context.userData;

	struct CustomParameters {
		AVisitor2 *				visitor;
		AVisitor2::Context *	context;
	} parameters = {
		visitor,
		&itemContext,
	};

	struct Callbacks
	{
		static void iterateMemoryBlock(
			void ** p_memory_pointer
			, const U32 memory_size
			, const bool is_dynamically_allocated
			, const TbMetaType& item_type
			, const U32 live_item_count
			, const U32 item_stride
			, void* user_data
			)
		{
			CustomParameters* params = (CustomParameters*) user_data;

			void *const pointer_to_stored_items = *p_memory_pointer; 

			void * instance = pointer_to_stored_items;
			for( U32 i = 0; i < live_item_count; i++ )
			{
				Walker2::Visit( instance, item_type, params->visitor, *params->context );
				instance = mxAddByteOffset( instance, item_stride );
			}
		}
	};

	dictionary_type.iterateMemoryBlocks(
		instance
		, &Callbacks::iterateMemoryBlock
		, &parameters
		);
}

void Walker2::VisitAggregate( void * _struct, const TbMetaClass& _type, AVisitor2* _visitor, const AVisitor2::Context& _context )
{
	mxASSERT_PTR(_struct);
#if MX_DEBUG_REFLECTION
	DBGOUT("[Reflector]: Visit object of class '%s'\n", type.m_name);
#endif // MX_DEBUG_REFLECTION

	if( _visitor->Visit_Class(_struct,_type,_context) )
	{
		// First recursively visit the parent classes.
		const TbMetaClass* parentType = _type.GetParent();
		while( parentType != nil )
		{
			VisitStructFields( _struct, *parentType, _visitor, _context );
			parentType = parentType->GetParent();
		}

		// Now visit the members of this class.
		VisitStructFields( _struct, _type, _visitor, _context );
	}
}

void Walker2::VisitStructFields( void * _struct, const TbMetaClass& _type, AVisitor2* _visitor, const AVisitor2::Context& _context )
{
	const TbClassLayout& layout = _type.GetLayout();
	for( UINT fieldIndex = 0 ; fieldIndex < layout.numFields; fieldIndex++ )
	{
		const MetaField& field = layout.fields[ fieldIndex ];
		
		AVisitor2::Context	fieldContext( _context.depth + 1 );
		fieldContext.parent = &_context;
		fieldContext.member = &field;
		fieldContext.userData = _context.userData;

#if MX_DEBUG_REFLECTION
		DBGOUT("[Reflector]: Visit field '%s' of type '%s' in '%s' at offset '%u'\n",
			field.name, field.type.m_name.raw(), _type.m_name.raw(), field.offset);
#endif // MX_DEBUG_REFLECTION

		void* memberVarPtr = mxAddByteOffset( _struct, field.offset );

		if( _visitor->Visit_Field( memberVarPtr, field, fieldContext ) )
		{
			Visit( memberVarPtr, field.type, _visitor, fieldContext );
		}
	}
}


void AVisitor2::visit_Dictionary(
	void * dictionary_instance
	, const MetaDictionary& dictionary_type
	, const Context& context
	)
{
	Walker2::visit_Items_stored_in_Dictionary( dictionary_instance, dictionary_type, this, context );
}

bool TellNotToFreeMemory::Visit_Array( void * _array, const MetaArray& _type, const Context& _context )
{
	void* allocated_data = _type.Generic_Get_Data( _array );
	if(allocated_data != nil) {
		_type.SetDontFreeMemory( _array );
	}
	return true;
}
void TellNotToFreeMemory::Visit_String( String & _string, const Context& _context )
{
	_string.DoNotFreeMemory();
}

void TellNotToFreeMemory::visit_Dictionary(
		void * dictionary_instance
		, const MetaDictionary& dictionary_type
		, const Context& context
		)
{
	dictionary_type.markMemoryAsExternallyAllocated( dictionary_instance );

	Super::visit_Dictionary(
		dictionary_instance
		, dictionary_type
		, context
		);
}

void MarkMemoryAsExternallyAllocated( void* _memory, const TbMetaClass& _type )
{
	// Tell the objects not to deallocated the buffer memory.
	// (As a side effect it also warms up the data cache.)
	TellNotToFreeMemory		markMemoryAsExternallyAllocated;
	Reflection::Walker2::Visit( _memory, _type, &markMemoryAsExternallyAllocated );
}

}//namespace Reflection

/*
Smallest Possible Attribute System, Part 3
http://nickporcino.com/meshula-net-archive/posts/post184.html
*/
