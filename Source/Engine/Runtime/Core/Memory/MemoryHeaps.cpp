/*
=============================================================================

=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Base/Memory/DefaultHeap/DefaultHeap.h>
#include <Base/Memory/DebugHeap/DebugHeap.h>
#include <Core/Memory/MemoryHeaps.h>

namespace MemoryHeaps
{
	static DefaultHeap		gs_defaultAllocator;
	static IGDebugHeap		gs_debugHeap;

	class TemporaryHeapData
	{
		ProxyAllocator		temporaryHeapProxy;
		ScratchAllocator	temporaryAllocator;
		ThreadSafeProxyHeap	threadSafeWrapper;

	public:
		TemporaryHeapData( AllocatorI & parent_allocator )
			: temporaryHeapProxy( "temporary", parent_allocator )
			, temporaryAllocator( &temporaryHeapProxy )
			, threadSafeWrapper( temporaryAllocator )
		{
		}

		ERet initialize( U32 temporaryHeapSize )
		{
			void* temporaryHeapBuffer = temporaryHeapProxy.Allocate( temporaryHeapSize, EFFICIENT_ALIGNMENT );
			mxENSURE( temporaryHeapBuffer, ERR_OUT_OF_MEMORY, "" );

			temporaryAllocator.initialize(
				temporaryHeapBuffer
				, temporaryHeapSize
#if MX_DEBUG
				, temporaryHeapProxy._name
#else
				, nil
#endif
				);

			threadSafeWrapper.Initialize();

			return ALL_OK;
		}

		void shutdown()
		{
			threadSafeWrapper.Shutdown();

			if( temporaryAllocator.isInitialized() )
			{
				temporaryHeapProxy.Deallocate( temporaryAllocator.getBufferStart() );

				temporaryAllocator.shutdown();
			}
		}

		AllocatorI& getAllocator()
		{
			AllocatorI* result = temporaryAllocator.isInitialized() ? (AllocatorI*)&threadSafeWrapper : (AllocatorI*)&temporaryHeapProxy;
			return *result;
		}
	};

	///
	struct MemoryHeapsPrivateData
	{
		AllocatorI &				rootAllocator;	// the heap where this structure was allocated from

		//

		TemporaryHeapData	temporaryHeap;

		//

		ProxyAllocator		rootHeapProxy;

		ProxyAllocator		stringsHeapProxy;
		ProxyAllocator		arraysHeapProxy;

		ProxyAllocator		backgroundQueueHeapProxy;
		ProxyAllocator		taskSchedulerHeapProxy;

		ProxyAllocator		resourcesHeapProxy;
		ProxyAllocator		clumpsHeapProxy;

		ProxyAllocator		scriptingHeapProxy;

		ProxyAllocator		graphicsHeapProxy;
		ProxyAllocator		rendererHeapProxy;
		ProxyAllocator		animationHeapProxy;

		ProxyAllocator		dearImGuiHeapProxy;

		ProxyAllocator		voxels_heap_proxy;

		ProxyAllocator		physicsHeapProxy;

		ProxyAllocator		audioHeapProxy;

		ProxyAllocator		networkHeapProxy;

	public:
		MemoryHeapsPrivateData( AllocatorI & rootAllocator )
			: rootAllocator( rootAllocator )

			, temporaryHeap( rootAllocator )

			, rootHeapProxy				( "Root",			rootAllocator )

			, stringsHeapProxy			( "Strings",		rootHeapProxy )
			, arraysHeapProxy			( "Arrays",			rootHeapProxy )

			, backgroundQueueHeapProxy	( "BackgroundQueue", rootHeapProxy )
			, taskSchedulerHeapProxy	( "TaskScheduler",	rootHeapProxy )

			, resourcesHeapProxy		( "Resources",		rootHeapProxy )
			, clumpsHeapProxy			( "Clumps",			rootHeapProxy )

			, scriptingHeapProxy		( "Scripting",		rootHeapProxy )

			, graphicsHeapProxy			( "Graphics",		rootHeapProxy )
			, rendererHeapProxy			( "Renderer",		rootHeapProxy )
			, animationHeapProxy		( "Animation",		rootHeapProxy )

			, dearImGuiHeapProxy		( "DearImGui",		rootHeapProxy )

			, voxels_heap_proxy			( "Voxels",		rootHeapProxy )

			, physicsHeapProxy			( "Physics",		rootHeapProxy )

			, audioHeapProxy			( "Audio",			rootHeapProxy )

			, networkHeapProxy			( "Network",		rootHeapProxy )
		{}
	};
	static TPtr< MemoryHeapsPrivateData >	gs_privateData;

	//

	ERet initialize()
	{
		const size_t debugHeapSizeMiB = Testbed::g_InitMemorySettings.debugHeapSizeMiB;

		if( debugHeapSizeMiB )
		{
			gs_debugHeap.Initialize( mxMiB(debugHeapSizeMiB) );
		}

		AllocatorI* rootAllocator = debugHeapSizeMiB ? (AllocatorI*)&gs_debugHeap : (AllocatorI*)&gs_defaultAllocator;

		//

		gs_privateData = mxNEW( *rootAllocator, MemoryHeapsPrivateData, *rootAllocator );
		mxENSURE( gs_privateData, ERR_OUT_OF_MEMORY, "" );

		//

		if( const size_t temporaryHeapSizeMiB = Testbed::g_InitMemorySettings.temporaryHeapSizeMiB )
		{
			mxDO(gs_privateData->temporaryHeap.initialize( mxMiB(temporaryHeapSizeMiB) ));
		}
		else
		{
			DEVOUT("Warning: not using a special heap for temporary memory allocations - the app will be slower than usual.");
		}

		//

		setStringAllocator( &gs_privateData->stringsHeapProxy );

		return ALL_OK;
	}

	void shutdown()
	{
		mxDELETE_AND_NIL( gs_privateData._ptr, gs_privateData->rootAllocator );
	}


	//

	AllocatorI& process()					{ return gs_defaultAllocator; }

	ProxyAllocator& global()			{ return gs_privateData->rootHeapProxy; }

	ProxyAllocator& strings()			{ return gs_privateData->stringsHeapProxy; }

	ProxyAllocator& backgroundQueue()	{ return gs_privateData->backgroundQueueHeapProxy; }
	//ProxyAllocator& taskScheduler()		{ return gs_privateData->taskSchedulerHeapProxy; }

	ProxyAllocator& resources()			{ return gs_privateData->resourcesHeapProxy; }

	ProxyAllocator& clumps()			{ return gs_privateData->clumpsHeapProxy; }

	ProxyAllocator& scripting()			{ return gs_privateData->scriptingHeapProxy; }

	ProxyAllocator& graphics()			{ return gs_privateData->graphicsHeapProxy; }
	ProxyAllocator& renderer()			{ return gs_privateData->rendererHeapProxy; }
	ProxyAllocator& animation()			{ return gs_privateData->animationHeapProxy; }

	ProxyAllocator& dearImGui()			{ return gs_privateData->dearImGuiHeapProxy; }

	ProxyAllocator& voxels()			{ return gs_privateData->voxels_heap_proxy; }

	ProxyAllocator& physics()			{ return gs_privateData->physicsHeapProxy; }

	ProxyAllocator& audio()				{ return gs_privateData->audioHeapProxy; }

	ProxyAllocator& network()			{ return gs_privateData->networkHeapProxy; }

	AllocatorI& debug()
	{ return gs_debugHeap; }

	AllocatorI& temporary()
	{ return gs_privateData->temporaryHeap.getAllocator(); }

}//namespace MemoryHeaps


namespace Arrays
{
	AllocatorI& defaultAllocator()
	{
		return MemoryHeaps::gs_privateData->arraysHeapProxy;
	}
}//namespace Arrays
