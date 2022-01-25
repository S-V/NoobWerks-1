//
#pragma once

struct SystemTable;

namespace RunTimeCompiledCpp
{

ERet initialize( SystemTable* system_table = NULL );
void shutdown();

//
// The RCC++ RuntimeObjectSystem file change notifier
// must have its update function called regularly
// so it can detect changed files.
// Additionally the code needs to load any compiled modules
// when a compile is complete.
//
void update( float delta_seconds_elapsed );


struct ListenerI: TDoublyLinkedList< ListenerI >
{
	// Called after a full serialization of objects is done when a new
	// object constructor is added, so listeners can update any object
	// pointers they're holding
	virtual void RTCPP_OnConstructorsAdded() = 0;

protected:
	virtual ~ListenerI() {}
};

void AddListener(ListenerI* new_listener);
void RemoveListener(ListenerI* listener);

}//namespace RunTimeCompiledCpp
