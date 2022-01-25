// We use several special memory heaps to reduce heap fragmentation and improve performance.
//
//	EMemHeap - a category under which the memory used by objects is placed.
//
//	(Also known as memory heap, memory arena/zone, etc.)
//	Each object in the engine should be allocated in a dedicated heap.
//	The main purpose for the different heap
//	types is to decrease memory fragmentation and to improve cache
//	usage by grouping similar data together. Platform ports may define
//	platform-specific heap types, as long as only platform specific
//	code uses those new heap types.
//

//------- DECLARE_MEMORY_HEAP( name ) ------------------------

#ifdef DECLARE_MEMORY_HEAP

// General-purpose - this is the memory heap used (by global 'new' and 'delete') by default.
DECLARE_MEMORY_HEAP( Generic ),

//// Geometry held in main memory ( e.g. raw vertex buffers ).
//DECLARE_MEMORY_HEAP( HeapGeometry, Geometry ),
//// Scene graph structures, scene entities, etc.
//DECLARE_MEMORY_HEAP( HeapSceneData,	SceneData ),
// objects now live in clumps
DECLARE_MEMORY_HEAP( Clumps ),

// Low-level graphics abstraction layer.
DECLARE_MEMORY_HEAP( Graphics ),

// High-level rendering system.
DECLARE_MEMORY_HEAP( Renderer ),

DECLARE_MEMORY_HEAP( Physics ),

// FMOD
DECLARE_MEMORY_HEAP( Audio ),

DECLARE_MEMORY_HEAP( Network ),

// String data should be allocated from specialized string pools.
DECLARE_MEMORY_HEAP( String ),

// Scripting system (material scripts, UI, cinematics, etc).
DECLARE_MEMORY_HEAP( Scripting ),


// Special heap for stream data like memory streams, zip file streams, etc.
DECLARE_MEMORY_HEAP( Streaming ),
// General-purpose memory manager for temporary allocations.
// (e.g. it is used to quickly provide temporary storage when loading raw resource data.)
DECLARE_MEMORY_HEAP( Temporary ),

// Fast stack-based memory manager for quick temporary allocations.
// Make sure that data is freed exactly in reverse order of allocation.
//DECLARE_MEMORY_HEAP( HeapStack,	Stack ),

// In-game editor.
DECLARE_MEMORY_HEAP( Editor ),

// Special memory heap for debugging memory leaks/buffer overruns/etc.
DECLARE_MEMORY_HEAP( Debug ),


//#if mxPLATFORM == mxPLATFORM_PS3
//// Temporary storage for quick calculations (thread-local memory).
//DECLARE_MEMORY_HEAP( Scratchpad ),
//#endif

// System memory manager for internal use by the engine.
DECLARE_MEMORY_HEAP( Process ),

#endif // defined(DECLARE_MEMORY_HEAP)
