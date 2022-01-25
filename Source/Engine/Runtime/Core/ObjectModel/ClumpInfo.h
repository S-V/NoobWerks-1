//
#pragma once

#include <Core/ObjectModel/Clump.h>


/// used for saving capacity to file to minimize reallocations on next launch
class ClumpInfo: NonCopyable
{
	typedef THashMap< const TbMetaClass*, U32 >	 InstanceCountByClassID;

	InstanceCountByClassID	_instances_map;

public:
	ClumpInfo( AllocatorI & allocator );

	ERet buildFromClump( const NwClump& clump );

	ERet saveToFile( const char* filename ) const;
	ERet loadFromFile( const char* filename );
};
