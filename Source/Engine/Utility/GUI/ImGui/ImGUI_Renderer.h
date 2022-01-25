#pragma once

#include <Core/Memory.h>	// ProxyAllocator

#include <ImGui/imgui.h>
#if MX_AUTOLINK
#pragma comment( lib, "ImGui.lib" )
#endif //MX_AUTOLINK

class NwClump;

namespace ImGui_Renderer
{
	struct Config
	{
		bool	save_to_ini;
	public:
		Config()
		{
			save_to_ini = true;
		}
	};

	ERet Initialize(
		const Config& cfg = Config()
		);
	void Shutdown();

	// don't call if ImGui is not in focus
	void UpdateInputs();

	// begin GUI rendering
	void BeginFrame(const float delta_time_seconds);

	// all of GUI rendering code should be here, between Begin() and End() calls

	// flush and present
	ERet EndFrame(const U32 viewId);

}//namespace ImGui_Renderer




enum EditPropertyGridResult
{
	PropertiesDidNotChange,
	PropertiesDidChange,
};


EditPropertyGridResult ImGui_DrawPropertyGrid( void * _memory
							, const TbMetaType& _type
							, const char* name
							);

//class EdTypeDatabase;
//void ImGui_DrawPropertyGrid( void * _memory
//							, const TbMetaType& _type
//							, const char* name
//							, const EdTypeDatabase& typeDb );

template< class STRUCT >
EditPropertyGridResult ImGui_DrawPropertyGridT( STRUCT * struct_ptr, const char* window_name )
{
	return ImGui_DrawPropertyGrid( struct_ptr, STRUCT::metaClass(), window_name );
}


bool ImGui_GetScreenSpacePosition(
							 ImVec2 *screenPosition_,
							 const V3f& worldPosition,
							 const M44f& view_projection_matrix,
							 float screen_width, float screen_height
							 );

void ImGui_Window_Text_Float3( const V3f& xyz );

void ImGui_drawEnumComboBox( void * enum_value, const MetaEnum& enum_type, const char* label );

template< typename ENUM, typename ENUM_STORAGE >
void ImGui_drawEnumComboBoxT( TEnum< typename ENUM, ENUM_STORAGE > * enum_value
							, const char* label )
{
	const MetaEnum& enum_type = static_cast< const MetaEnum& >( TypeDeducer< TEnum< typename ENUM, ENUM_STORAGE > >::GetType() );
	ImGui_drawEnumComboBox( enum_value, enum_type, label );
}





void ImGui_DrawFlagsCheckboxes(
							void * flags_value
							, const MetaFlags& flags_type
							, const char* label
							);

template< typename FLAGS, typename STORAGE >
void ImGui_DrawFlagsCheckboxesT(
								TBits< typename FLAGS, STORAGE > * flags_value
								, const char* label
								)
{
	const MetaFlags& flags_type = static_cast< const MetaFlags& >(
		TypeDeducer< TBits< typename FLAGS, STORAGE > >::GetType()
		);
	ImGui_DrawFlagsCheckboxes(
		flags_value
		, flags_type
		, label
		);
}


#if MX_DEVELOPER
void ImGui_DrawTweakablesWindow();
#endif // MX_DEVELOPER

#if MX_DEBUG_MEMORY
void ImGui_DrawMemoryAllocatorsTree( const ProxyAllocator* root_allocator );
#endif


//namespace NwImGui
//{
//	void LabelTextAABB(
//		const char* prefix
//		);
//}//namespace NwImGui
