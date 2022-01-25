// A tool For inspecting the contents of textures using Dear ImGui.
#pragma once

#include <Base/Base.h>
#include <Core/Memory.h>
#include <GPU/Public/graphics_types.h>


namespace Graphics
{
namespace DebuggerTool
{
	ERet Initialize( ProxyAllocator & allocator );
	void Shutdown();

	//

	struct Surface
	{
		HShaderInput	handle;
		DataFormatT		format;
		String64		name;
		bool			is_being_displayed_in_ui;
	};
	const TSpan< Surface >	getSurfaces();

	void addColorTarget(
		HColorTarget handle
		, const NwColorTargetDescription& description
		, const char* debug_name
		);
	void addDepthTarget(
		HDepthTarget handle
		, const NwDepthTargetDescription& description
		, const char* debug_name
		);

	// these are used when textures get deallocated and then resized, etc.
	void removeColorTarget( HColorTarget handle );
	void removeDepthTarget( HDepthTarget handle );

	void AddTexture2D(
		HTexture texture_handle
		, const NwTexture2DDescription& description
		, const char* debug_name
		);

}//namespace DebuggerTool
}//namespace Graphics
