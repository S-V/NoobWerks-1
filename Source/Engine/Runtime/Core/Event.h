/*
==========================================================
	Event/Message System

	https://gameprogrammingpatterns.com/event-queue.html

	Chapter 26 - The Game State Observer Pattern 
	Game Engine Gems, Volume One [2011]
==========================================================
*/
#pragma once

#include <Core/EntitySystem.h>	// EntityID


enum TbGameEventTypeE: U32
{
	EV_PROJECTILE_FIRED,
	EV_PROJECTILE_COLLIDED,
	EV_PROJECTILE_EXPLODED,
};

//struct TbGameEventBase: CStruct
//{
//	TbGameEventTypeE	type;
//};

struct TbGameEvent_ShotFired: CStruct
{
	V3f		position;
	F32		col_radius;
	V3f		direction;
	F32		lin_velocity;

	F32		projectile_mass;
};

struct TbGameEvent_ProjectileCollided: CStruct
{
	EntityID	projectile_id;
	V3f			position;
};
struct TbGameEvent_ProjectileExploded: CStruct
{
	EntityID	projectile_id;
	V3f			position;
};



struct TbGameEvent: CStruct
{
	TbGameEventTypeE	type;
	U32					pad;
	U64					timestamp;

	union
	{
		TbGameEvent_ShotFired			shot_fired;
		TbGameEvent_ProjectileCollided	projectile_collided;
		TbGameEvent_ProjectileExploded	projectile_exploded;
	};
};

struct TbGameEventHandlerI: TDoublyLinkedList< TbGameEventHandlerI >
{
	virtual void handleGameEvent(
		const TbGameEvent& game_event
	) = 0;
};

namespace Events
{


}//namespace Events


namespace EventSystem
{
	ERet Initialize();
	void Shutdown();

	void addEventHandler( TbGameEventHandlerI* new_handler );
	void removeEventHandler( TbGameEventHandlerI* old_handler );
	bool containsEventHandler( TbGameEventHandlerI* handler );

	// event generation
	void	PostEvent( const TbGameEvent& new_event );
	void	ClearEvents();

	//SystemEvent	GetEvent();

	void	processEvents();

}//namespace EventSystem
