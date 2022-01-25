//
// Support for Nuklear:
// https://github.com/Immediate-Mode-UI/Nuklear
//
#pragma once

#define NK_ASSERT(expr) mxASSERT(expr)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#include <Nuklear/nuklear.h>


// NOTE: depends on Graphics!

ERet Nuklear_initialize(
						AllocatorI& allocator
						, const size_t max_memory
						);
void Nuklear_shudown();

nk_context* Nuklear_getContext();

bool Nuklear_isMouseHoveringOverGUI();

void Nuklear_processInput(
						  float delta_time_seconds
						  );


ERet Nuklear_precache_Fonts_if_needed();

ERet Nuklear_render(
					const unsigned int scene_pass_id
					);

//
class NwNuklearScoped: NonCopyable
{
public:
	NwNuklearScoped();
	~NwNuklearScoped();
};

void Nuklear_unhide_window(
						   const char* window_name
						   );

///
#define nwNUKLEAR_GEN_ID	(TO_STR(__FUNCTION__)TO_STR(__COUNTER__))


class NkFileBrowserI
{
public:
	virtual ~NkFileBrowserI() {}

	virtual ERet drawUI() = 0;

	static NkFileBrowserI* create( AllocatorI & allocator );
};
