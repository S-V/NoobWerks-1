#pragma once


class NwAppI;
struct NwTime;
class NwGameStateManager;
class NwRenderSystemI;


class NwGameGUI
{
public:

	ERet Initialize(
		AllocatorI& allocator
		, const size_t max_memory
		);

	/// must be called before destroying Graphics!
	void Shutdown();

public:
	ERet Draw(
		NwGameStateManager& state_mgr
		, const NwTime& game_time
		, NwAppI* app
		);
};
