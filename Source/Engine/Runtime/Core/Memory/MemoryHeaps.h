//
#pragma once

#include <Core/Memory.h>


namespace MemoryHeaps
{
	ERet initialize();
	void shutdown();

	/// untracked memory heap
	AllocatorI& process();

	/// default, generic heap
	ProxyAllocator& global();

	ProxyAllocator& strings();

	/// for background loading
	ProxyAllocator& backgroundQueue();

	/// for parallel jobs
	//ProxyAllocator& taskScheduler();

	/// textures, meshes, sounds, ...
	ProxyAllocator& resources();

	/// game object storage
	ProxyAllocator& clumps();


	/// for Lua
	ProxyAllocator& scripting();


	/// low-level graphics
	ProxyAllocator& graphics();

	/// high-level graphics
	ProxyAllocator& renderer();

	///
	ProxyAllocator& animation();

	/// GUI
	ProxyAllocator& dearImGui();

	/// voxel terrain
	ProxyAllocator& voxels();

	ProxyAllocator& physics();

	ProxyAllocator& audio();

	ProxyAllocator& network();


	//
	AllocatorI& debug();

	/// thread-safe ring buffer allocator, can be used for passing data between threads
	AllocatorI& temporary();


	/// returns a pointer to the global thread-safe allocator
	inline AllocatorI& jobs() {
		//return MemoryHeaps::global();
		return MemoryHeaps::temporary();
	}


	//ProxyAllocator& resources();

	/// always fails upon allocation
	//ProxyAllocator& error();


	ERet initialize();
	void shutdown();

}//namespace MemoryHeaps
