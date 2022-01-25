//
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>

#include <Core/ObjectModel/ClumpInfo.h>

#include <Core/Serialization/Text/TxTSerializers.h>
#include <Core/Serialization/Text/TxTWriter.h>


ClumpInfo::ClumpInfo( AllocatorI & allocator )
	: _instances_map( allocator )
{
}

ERet ClumpInfo::buildFromClump( const NwClump& clump )
{
	const U32	num_object_lists = clump.numAliveObjectLists();

	mxDO(_instances_map.Initialize(
		NextPowerOfTwo( num_object_lists )
		, num_object_lists
		));

	//
	const NwObjectList *	current_object_list = clump._object_lists;

	while( current_object_list )
	{
		const NwObjectList *	next_object_list = current_object_list->_next;

		//
		const TbMetaClass& metaclass = current_object_list->getType();

		const U32	object_list_capacity = current_object_list->capacity();

		//
		const U32* existing_count = _instances_map.FindValue( &metaclass );
		if( existing_count )
		{
			_instances_map.Insert( &metaclass, object_list_capacity + *existing_count );
		}
		else
		{
			_instances_map.Insert( &metaclass, object_list_capacity );
		}

		//
		current_object_list = next_object_list;
	}

	return ALL_OK;
}

ERet ClumpInfo::saveToFile( const char* filename ) const
{
	SON::NodeAllocator	node_allocator( MemoryHeaps::global() );

	//
	SON::Node *	root_object = SON::NewObject( node_allocator );
	chkRET_X_IF_NIL(root_object, ERR_UNKNOWN_ERROR);

	//
	const InstanceCountByClassID::PairsArray& pairs = _instances_map.GetPairs();

	for( U32 i = 0; i < pairs.num(); i++ )
	{
		const InstanceCountByClassID::Pair& pair = pairs[i];
		mxASSERT(pair.key);
		if(pair.key)
		{
			SON::Node* number_node = SON::NewNumber( pair.value, node_allocator );
			SON::AddChild( root_object, pair.key->GetTypeName(), number_node );
		}
	}

	//
	FileWriter	file_stream;
	mxDO(file_stream.Open( filename ));

	//
	mxDO(SON::WriteToStream( root_object, file_stream ));

	return ALL_OK;
}
