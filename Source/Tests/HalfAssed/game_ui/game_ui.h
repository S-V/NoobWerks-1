#pragma once


ERet initialize_game_GUI(
						 AllocatorI& allocator
						 , const size_t max_memory
						 );

/// must be called before destroying Graphics!
void shutdown_game_GUI();
