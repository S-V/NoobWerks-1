//
#pragma once

#include <set>	// std::set<>
#include <map>	// std::map<>
#include <string>	// std::string


/// used to detect conflicting names and hashes
class NwUniqueNameHashChecker: NonCopyable
{
	typedef std::set< std::string >				NameSet;
	typedef std::map< NameHash32, const char* >	NameHashMap;

	NameSet		unique_names_set;
	NameHashMap	name_by_hash_map;

public:
	ERet InsertUniqueName(const char* name, NameHash32 name_hash = 0)
	{
		mxASSERT_PTR(name);
		//
		{
			NameSet::const_iterator existing_name_it = unique_names_set.find( name );

			if( existing_name_it != unique_names_set.end() )
			{
				ptWARN("Name '%s' is not unique!", name);
				return ERR_DUPLICATE_OBJECT;
			}

			unique_names_set.insert( name );
		}

		//
		if( 0 == name_hash ) {
			name_hash = GetDynamicStringHash(name);
		}

		//
		{
			NameHashMap::const_iterator existing_it = name_by_hash_map.find( name_hash );

			if( existing_it != name_by_hash_map.end() )
			{
				ptWARN("'%s' has the same hash as '%s' (hash: 0x%X)",
					name, existing_it->second, name_hash
					);
				return ERR_DUPLICATE_OBJECT;
			}
		}

		name_by_hash_map.insert( std::make_pair( name_hash, name ) );

		return ALL_OK;
	}
};




template< class CONTAINER >
static
ERet EnsureItemsHaveUniqueNameHashes( const CONTAINER& items )
{
	// ensure names have non-conflicting hashes
	NwUniqueNameHashChecker	name_hash_checker;

	CONTAINER::ConstIterator it( items );
	
	while( it.IsValid() )
	{
		const CONTAINER::ItemType& item = *it;

		const NameHash32 nameHash = GetDynamicStringHash( item.name.c_str() );

		mxTRY(name_hash_checker.InsertUniqueName(
			item.name.c_str(), nameHash
			));

		it.MoveToNext();
	}

	return ALL_OK;
}
