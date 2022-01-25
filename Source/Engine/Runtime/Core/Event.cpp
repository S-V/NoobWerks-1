#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/Event.h>

mxSTOLEN("this is based on Quake2/Quake3/Doom3 code");
namespace EventSystem
{
	// circular buffer of system events
	enum Constants: U32
	{
		EVENT_QUEUE_SIZE = 512,
		EVENT_QUEUE_MASK = EVENT_QUEUE_SIZE - 1,

		MAX_EVENT_HANDLERS = 8,
	};

	struct PrivateData
	{
		TbGameEvent	ringBuffer[ EVENT_QUEUE_SIZE ];	// FIFO queue

		// these never decrease
		U32			eventHead;	// write cursor
		U32			eventTail;	// read cursor

		TFixedArray< TbGameEventHandlerI*, MAX_EVENT_HANDLERS >	event_handlers;

	public:
		PrivateData()
		{
			eventHead = 0;
			eventTail = 0;
		}

		void  reset()
		{
			eventHead = 0;
			eventTail = 0;
		}
	};
	static TPtr< PrivateData >	gs_data;

	ERet Initialize()
	{
		ptPRINT("EventSystem:Initialize(): sizeof(TbGameEvent)=%d\n",sizeof(TbGameEvent));
		gs_data.ConstructInPlace();
		return ALL_OK;
	}

	void Shutdown()
	{
		gs_data.Destruct();
	}

	void addEventHandler( TbGameEventHandlerI* new_handler )
	{
		mxASSERT( !containsEventHandler( new_handler ) );

		if( !containsEventHandler( new_handler ) )
		{
			gs_data->event_handlers.add( new_handler );
		}
	}

	void removeEventHandler( TbGameEventHandlerI* old_handler )
	{
		mxASSERT( containsEventHandler( old_handler ) );

		gs_data->event_handlers.removeFirst( old_handler );
	}

	bool containsEventHandler( TbGameEventHandlerI* handler )
	{
		return gs_data->event_handlers.contains( handler );
	}

	void PostEvent( const TbGameEvent& new_event )
	{
		U32 eventHead = gs_data->eventHead;
		U32 eventTail = gs_data->eventTail;

		TbGameEvent * alloced_event = &gs_data->ringBuffer[ eventHead & EVENT_QUEUE_MASK ];

		if ( eventHead - eventTail >= EVENT_QUEUE_SIZE )
		{
			mxASSERT(0&&"PostEvent(): overflow\n");
			//// we are discarding an event, but don't leak memory
			//if ( alloced_event->evPtr ) {
			//	mxFREE( alloced_event->evPtr );
			//}
			eventTail++;
		}

		eventHead++;

		//
		gs_data->eventHead = eventHead;
		gs_data->eventTail = eventTail;

		//
		*alloced_event = new_event;
		alloced_event->timestamp = Testbed::realTime();
	}

	void ClearEvents()
	{
		gs_data->reset();
	}

	void processEvents()
	{
		const	U32 eventHead = gs_data->eventHead;
				U32 eventTail = gs_data->eventTail;

		const	U32	num_queued_events = eventHead - eventTail;

		if( num_queued_events )
		{
			const TbGameEvent* events = gs_data->ringBuffer;

			const UINT num_event_handlers = gs_data->event_handlers.num();
			TbGameEventHandlerI** event_handlers = gs_data->event_handlers.raw();

			for( UINT iEvent = 0; iEvent < num_queued_events; iEvent++ )
			{
				const TbGameEvent& game_event = events[ eventTail++ & EVENT_QUEUE_MASK ];

				for( UINT iHandler = 0; iHandler < num_event_handlers; iHandler++ )
				{
					event_handlers[iHandler]->handleGameEvent( game_event );
				}
			}

			gs_data->eventTail = eventTail;
			mxASSERT(eventHead == eventTail);
		}
	}

}//namespace EventSystem

/*
References:

The Urho3D event system allows for data transport and function invocation without the sender and receiver having to explicitly know of each other.
https://urho3d.github.io/documentation/1.5/_events.html

http://seanballais.github.io/blog/implementing-a-simple-message-bus-in-cpp/
*/
