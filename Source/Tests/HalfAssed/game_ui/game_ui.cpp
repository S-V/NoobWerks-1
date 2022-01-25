//
#include "stdafx.h"
#pragma hdrstop

#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "game_ui/game_ui.h"


ERet initialize_game_GUI(
						 AllocatorI& allocator
						 , const size_t max_memory
						 )
{
	mxDO(Nuklear_initialize(
		allocator
		, max_memory
		));

	return ALL_OK;
}

void shutdown_game_GUI()
{
	Nuklear_shudown();
}
