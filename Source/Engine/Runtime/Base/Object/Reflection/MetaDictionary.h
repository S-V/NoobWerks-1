// Key-Value Map Descriptor
// 
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>

class MetaDictionary: public TbMetaType
{
public:
	// The types of stored elements:
	const TbMetaType &	_key_type;
	const TbMetaType &	_value_type;

public:
	inline MetaDictionary(
		const char* type_name
		, const TbTypeSizeInfo& size_info
		, const TbMetaType& key_type
		, const TbMetaType& value_type
		)
		: TbMetaType(
			ETypeKind::Type_Dictionary
			, type_name
			, size_info
		)
		, _key_type( key_type )
		, _value_type( value_type )
	{
	}


	/// for iterating over contiguous ranges of elements
	typedef void IterateMemoryBlockCallback(
		void ** p_memory_pointer	// a pointer to the pointer at the start of the allocated memory
		, const U32 memory_size		// the size of the allocated memory block, in bytes
		, const bool is_dynamically_allocated	// true if the memory is dynamically allocated (i.e. it is not embedded into the structure)
		, const TbMetaType& item_type
		, const U32 live_item_count
		, const U32 item_stride
		, void* user_data
		);

	virtual void iterateMemoryBlocks(
		void * dictionary_instance
		, IterateMemoryBlockCallback* callback
		, void* user_data
		) const = 0;

	virtual void markMemoryAsExternallyAllocated(
		void * dictionary_instance
		) const = 0;
};
