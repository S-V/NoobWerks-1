#pragma once

#include <Core/Tasking/TaskSchedulerInterface.h>

namespace VX
{

enum TaskPriorities
{
	/// The priority for node refinement jobs - should be less than priorities of frame jobs like frustum culling, etc.
	NODE_SPLIT_PRIORITY = JobPriority_Low,
	/// The priority for node coarsening jobs - should be less than priorities of frame jobs like frustum culling, etc.
	NODE_MERGE_PRIORITY = JobPriority_Normal,

	CHUNK_REMESH_PRIORITY = JobPriority_High,
	CHUNK_EDIT_PRIORITY = JobPriority_High,

	CHUNK_UPDATE_LOD_PRIORITY = JobPriority_Normal,
};

/// returns a pointer to the global thread-safe allocator
inline AllocatorI& getJobAllocator() {
	//return MemoryHeaps::global();
	return MemoryHeaps::temporary();
}

}//namespace VX
