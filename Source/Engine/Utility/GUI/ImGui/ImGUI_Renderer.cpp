#include <bx/uint32_t.h>
#include <bx/timer.h>
#include <Base/Base.h>

#include <Core/Core.h>
#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/AssetReference.h>

#if MX_DEVELOPER
#include <Core/Util/Tweakable.h>
#endif // MX_EDITOR

#include <Engine/WindowsDriver.h>
#include <Scripting/Scripting.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Public/graphics_shader_system.h>

#if ENGINE_CONFIG_USE_BGFX
#include <Renderer/renderer_bgfx.h>
#endif
#include <Rendering/Public/Core/VertexFormats.h>

//#include <Renderer/resources/Texture.h>
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/GUI/GUI_Common.h>

#include "ImGUI_Renderer.h"

// these are used in GetClipboardTextFn_MyImpl():
extern IMGUI_API int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end);
extern int ImTextStrToUtf8(char* buf, int buf_size, const ImWchar* in_text, const ImWchar* in_text_end);

namespace ImGui_Renderer
{
struct PrivateData
{
	HTexture		fontTexture;
	int				lastWheelScroll;

	ImGuiTextFilter	consoleFilter;
	ImVector<char>	clipboard_buf;
};
static TPtr< PrivateData >		me;
static TPtr< TrackingHeap >	heap;

static void ImImpl_RenderDrawLists( ImDrawData* draw_data );

static void* MyMalloc(size_t sz, void* userData)
{
	return MemoryHeaps::dearImGui().Allocate(sz,4);
}
static void MyFree(void* ptr, void* userData)
{
	return MemoryHeaps::dearImGui().Deallocate(ptr);
}

/// because the function GetClipboardTextFn_DefaultImpl() contains
/// a static ImVector<char> for storing clipboard data
/// which causes a memory crash upon Shutdown
static const char* GetClipboardTextFn_MyImpl(void* userData)
{
	me->clipboard_buf.clear();
	if (!OpenClipboard(NULL))
		return NULL;
	HANDLE wbuf_handle = GetClipboardData(CF_UNICODETEXT);
	if (wbuf_handle == NULL)
		return NULL;
	if (ImWchar* wbuf_global = (ImWchar*)GlobalLock(wbuf_handle))
	{
		int buf_len = ImTextCountUtf8BytesFromStr(wbuf_global, NULL) + 1;
		me->clipboard_buf.resize(buf_len);
		ImTextStrToUtf8(me->clipboard_buf.Data, buf_len, wbuf_global, NULL);
	}
	GlobalUnlock(wbuf_handle);
	CloseClipboard();
	return me->clipboard_buf.Data;
}

ERet _CreateFontsTexture()
{
	ImGuiIO& io = ImGui::GetIO();

	 // Build texture atlas
	unsigned char* pixels;
	int width, height;
	int	bytes_per_pixel;
	// Load font texture
	io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height, &bytes_per_pixel );

	// Upload texture to graphics system
	const size_t font_texture_size = width * height * bytes_per_pixel;

	NwTexture2DDescription	font_texture_desc;
	font_texture_desc.format = NwDataFormat::RGBA8;
	font_texture_desc.width = width;
	font_texture_desc.height = height;
	font_texture_desc.numMips = 1;
	me->fontTexture = NGpu::createTexture2D(
		font_texture_desc
		, NGpu::copy( pixels, font_texture_size )
		IF_DEVELOPER , "FontTexture"
		);

	// Store our identifier
    io.Fonts->SetTexID(
		(ImTextureID) NGpu::AsInput( me->fontTexture ).id
		);

	// Release raw texture atlas data
	io.Fonts->ClearTexData();

#if 0
    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
#endif

	return ALL_OK;
}

/// _storage - container for storing allocated resources
ERet Initialize(
				const Config& cfg /*= Config()*/
				)
{
	me.ConstructInPlace();
	mxCONSTRUCT_IN_PLACE_X( heap, TrackingHeap, "ImGui", MemoryHeaps::global() );

	me->lastWheelScroll = 0;

	//
	ImGui::SetAllocatorFunctions( &MyMalloc, &MyFree, nil );

	// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
    ImGui::StyleColorsDark();

	//
	ImGuiIO& io = ImGui::GetIO();

	if( !cfg.save_to_ini ) {
		io.IniFilename = nil;
	}

	io.GetClipboardTextFn = &GetClipboardTextFn_MyImpl;   // Platform dependent default implementations

	io.DisplaySize = ImVec2( 1280.0f, 720.0f );	// Display size, in pixels. For clamping windows positions.
	io.DeltaTime = 1.0f/60.0f;	// Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	io.KeyMap[ImGuiKey_Tab] = KEY_Tab;
	io.KeyMap[ImGuiKey_LeftArrow] = KEY_Left;
	io.KeyMap[ImGuiKey_RightArrow] = KEY_Right;
	io.KeyMap[ImGuiKey_UpArrow] = KEY_Up;
	io.KeyMap[ImGuiKey_DownArrow] = KEY_Down;
	io.KeyMap[ImGuiKey_Home] = KEY_Home;
	io.KeyMap[ImGuiKey_End] = KEY_End;
	io.KeyMap[ImGuiKey_Delete] = KEY_Delete;
	io.KeyMap[ImGuiKey_Backspace] = KEY_Backspace;
	io.KeyMap[ImGuiKey_Enter] = KEY_Enter;
	io.KeyMap[ImGuiKey_Escape] = KEY_Escape;
	io.KeyMap[ImGuiKey_A] = KEY_A;
	io.KeyMap[ImGuiKey_C] = KEY_C;
	io.KeyMap[ImGuiKey_V] = KEY_V;
	io.KeyMap[ImGuiKey_X] = KEY_X;
	io.KeyMap[ImGuiKey_Y] = KEY_Y;
	io.KeyMap[ImGuiKey_Z] = KEY_Z;

	io.BackendRendererUserData = me;

	mxDO(_CreateFontsTexture());

	return ALL_OK;
}

void Shutdown()
{
	ImGui::DestroyContext();

	heap.Destruct();
	me.Destruct();
}

void UpdateInputs()
{
	const InputState& input_state = NwInputSystemI::Get().getState();
	if( !input_state.window.has_focus ) {
		return;
	}

	// Setup inputs
	ImGuiIO& io = ImGui::GetIO();

	io.MousePos = ImVec2( (float)input_state.mouse.x, (float)input_state.mouse.y );

	io.MouseDown[0] = (input_state.mouse.held_buttons_mask & BIT(MouseButton_Left)) != 0;
	io.MouseDown[1] = (input_state.mouse.held_buttons_mask & BIT(MouseButton_Right)) != 0;
	io.MouseDown[2] = (input_state.mouse.held_buttons_mask & BIT(MouseButton_Middle)) != 0;
	io.MouseDown[3] = (input_state.mouse.held_buttons_mask & BIT(MouseButton_X1)) != 0;
	io.MouseDown[4] = (input_state.mouse.held_buttons_mask & BIT(MouseButton_X2)) != 0;
	io.MouseWheel = (float)(input_state.mouse.wheel_scroll - me->lastWheelScroll);
	me->lastWheelScroll = input_state.mouse.wheel_scroll;

	for( int i = 0; i < MAX_KEYS; i++ ) {
		io.KeysDown[i] = (input_state.keyboard.held.Get(i)) != 0;
	}
	io.KeyAlt = (input_state.modifiers & KeyModifier_Alt) != 0;
	io.KeyCtrl = (input_state.modifiers & KeyModifier_Ctrl) != 0;
	io.KeyShift = (input_state.modifiers & KeyModifier_Shift) != 0;

	{
		const char* inputChar = input_state.input_text;
		while( *inputChar != '\0' )
		{
			ImWchar c = (ImWchar) *inputChar++;
			if (c < 0x7f)
			{
				DBGOUT("%d", c);
				io.AddInputCharacter(c); // ASCII or GTFO! :(
			}
		}
	}

	//
	if( input_state.keyboard.held.Get(KEY_Backspace) )
	{
		const unsigned int BACKSPACE = 9;
		io.AddInputCharacter( BACKSPACE );
	}
}

void BeginFrame( const float delta_time_seconds )
{
	const InputState& input_state = NwInputSystemI::Get().getState();

	// Update time

	ImGuiIO& io = ImGui::GetIO();

	io.DeltaTime = delta_time_seconds;
	if( io.DeltaTime < 1e-4f ) {
		io.DeltaTime = 1e-4f;
	}

	io.DisplaySize = ImVec2(
		(float)input_state.window.width, (float)input_state.window.height
		);

	// Start the frame
	ImGui::NewFrame();
}

static
ERet _RenderDrawData(
					 ImDrawData* draw_data
					 , const U32 viewId
					 )
{
    // Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) {
        return ALL_OK;
	}

	//
	PrivateData* pd = ImGui::GetCurrentContext()
		? (PrivateData*)ImGui::GetIO().BackendRendererUserData
		: NULL
		;
	mxENSURE(pd, ERR_INVALID_FUNCTION_CALL, "");

//	rmt_ScopedCPUSample(ImGui_RenderDrawLists, RMTSF_None);
//	PROFILE;
	NGpu::NwRenderContext& render_context = NGpu::getMainRenderContext();

	NGpu::SortKey	sortKey = 0;

	// Setup orthographic projection matrix
	const ImVec2& screenSize = ImGui::GetIO().DisplaySize;


	//
	NwShaderEffect* gui_shader;
	mxDO(Resources::Load( gui_shader, MakeAssetID("DearImGui") ));

	const NwShaderEffect::Variant shader_variant = gui_shader->getDefaultVariant();


	// Update shader uniform constants.
	M44f *	mvp;
	mxDO(gui_shader->AllocatePushConstants(
		&mvp
		, render_context._command_buffer
		));

	build_orthographic_projection_matrix_D3D11(
		mvp->a
		, screenSize.x, screenSize.y
		);

	// Render command lists.
	for( int iCmdList = 0; iCmdList < draw_data->CmdListsCount; iCmdList++ )
	{
		const ImDrawList* cmdList = draw_data->CmdLists[ iCmdList ];
		const ImDrawVert* srcVertices = cmdList->VtxBuffer.Data;
		const ImDrawIdx* srcIndices = cmdList->IdxBuffer.Data;
		const U32 numVertices = cmdList->VtxBuffer.size();
		const U32 numIndices = cmdList->IdxBuffer.size();
		const U32 vertexStride = sizeof(srcVertices[0]);
		const U32 indexStride = sizeof(srcIndices[0]);

		NwTransientBuffer	VB;
		NwTransientBuffer	IB;
		mxDO(NGpu::AllocateTransientBuffers(
			VB, numVertices, vertexStride,
			IB, numIndices, indexStride
		));
		memcpy( VB.data, srcVertices, numVertices * vertexStride );
		memcpy( IB.data, srcIndices, numIndices * indexStride );

		U32	indexOffset = 0;
		for( int iDrawCmd = 0; iDrawCmd < cmdList->CmdBuffer.size(); iDrawCmd++ )
		{
			const ImDrawCmd& drawCmd = cmdList->CmdBuffer[ iDrawCmd ];
			// Typically, 1 command = 1 gpu draw call (unless command is a callback).
			if( drawCmd.UserCallback )
			{
				drawCmd.UserCallback( cmdList, &drawCmd );
			}
			else
			{
				const HShaderInput texture = (drawCmd.TextureId != nil)
					? HShaderInput::MakeHandle( (U16)drawCmd.TextureId )
					: NGpu::AsInput( me->fontTexture )// HShaderInput::MakeNilHandle()
					;
				mxDO(gui_shader->SetInput(
					nwNAMEHASH("t_texture"),
					texture
					));

				{
					NGpu::RenderCommandWriter	cmd_writer( render_context );
//cmd_writer.dbgPrintf(0,"ImGui shader params");
					//
					mxDO(gui_shader->pushAllCommandsInto( render_context._command_buffer ));

					//
					mxDO(gui_shader->BindConstants(
						mvp
						, render_context._command_buffer
						));

					//
					NwRectangle64	clipping_rectangle;
					clipping_rectangle.left		= drawCmd.ClipRect.x;
					clipping_rectangle.top		= drawCmd.ClipRect.y;
					clipping_rectangle.right	= drawCmd.ClipRect.z;
					clipping_rectangle.bottom	= drawCmd.ClipRect.w;

					mxDO(cmd_writer.setScissor( true, clipping_rectangle ));

					NGpu::Cmd_Draw	cmd_Draw;
					cmd_Draw.program = shader_variant.program_handle;

					cmd_Draw.SetMeshState(
						Vertex_ImGui::metaClass().input_layout_handle,
						VB.buffer_handle,
						IB.buffer_handle,
						NwTopology::TriangleList,
						sizeof(ImDrawIdx) == sizeof(U32)
						);

					cmd_Draw.base_vertex = VB.base_index;
					cmd_Draw.vertex_count = numVertices;
					cmd_Draw.start_index = IB.base_index + indexOffset;
					cmd_Draw.index_count = drawCmd.ElemCount;

					IF_DEBUG sprintf( cmd_Draw.dbgname, "ImGui" );

					IF_DEBUG cmd_Draw.src_loc = GFX_SRCFILE_STRING;

					cmd_writer.SetRenderState(shader_variant.render_state);
//cmd_writer.dbgPrintf(0,GFX_SRCFILE_STRING);
					/*mxDO*/(cmd_writer.Draw( cmd_Draw ));

					//
					cmd_writer.SubmitCommandsWithSortKey(
						NGpu::buildSortKey( viewId, NGpu::buildSortKey(shader_variant.program_handle, sortKey++) )
						nwDBG_CMD_SRCFILE_STRING
						);
				}
			}//If this is a Render! command.
			indexOffset += drawCmd.ElemCount;

		}//For each Draw command.
	}//For each command list.

	return ALL_OK;
}

ERet EndFrame(const U32 viewId)
{
	mxPROFILE_SCOPE("RenderGUI");
	ImGui::Render();

	//
	return _RenderDrawData(
		ImGui::GetDrawData()
		, viewId
		);
}

}//namespace ImGui_Renderer




static bool ImGui_EnumGetter( void* data, int idx, const char** out_text )
{
	const MetaEnum& enumType = *(const MetaEnum*) data;
	mxASSERT( idx >= 0 && idx < enumType.m_items.count );
	const MetaEnum::Item& enumItem = enumType.m_items.items[ idx ];
	*out_text = enumItem.name;
	return true;
}

void ImGui_drawEnumComboBox( void * enum_value, const MetaEnum& enum_type, const char* label )
{
	const MetaEnum::ValueT currValue = enum_type.GetValue( enum_value );
	const char* valueName = enum_type.FindString( currValue );

	int	itemIndex = enum_type.m_items.GetItemIndexByValue( currValue );
	itemIndex = Clamp<int>( itemIndex, 0, enum_type.m_items.count );
	ImGui::Combo( label, &itemIndex, &ImGui_EnumGetter, (void*) &enum_type, enum_type.m_items.count );

	if( itemIndex >= 0 && itemIndex < enum_type.m_items.count )
	{
		const MetaEnum::ValueT newValue = enum_type.m_items.items[ itemIndex ].value;
		enum_type.SetValue( enum_value, newValue );
	}
}


void ImGui_DrawFlagsCheckboxes(
							   void * flags_value
							   , const MetaFlags& flags_type
							   , const char* label
							   )
{
	const MetaFlags::ValueT oldFlags = flags_type.GetValue( flags_value );

	const bool flatten_hierarchy = true;
	const ImGuiTreeNodeFlags imgui_tree_node_flags = flatten_hierarchy ? ImGuiTreeNodeFlags_DefaultOpen : 0;
	const bool opened_tree_node = ImGui::TreeNodeEx( label, imgui_tree_node_flags );

	if( opened_tree_node )
	{
		MetaFlags::ValueT newFlags = oldFlags;

		for( int itemIdx = 0; itemIdx < flags_type.m_items.count; itemIdx++ )
		{
			const MetaFlags::Item& item = flags_type.m_items.items[ itemIdx ];

			bool is_checked = oldFlags & item.value;
			if(ImGui::Checkbox( item.name, &is_checked ))
			{
				if( is_checked ) {
					newFlags |= item.value;
				} else {
					newFlags &= ~item.value;
				}
			}
		}

		if( oldFlags != newFlags ) {
			flags_type.SetValue( flags_value, newFlags );
		}

		ImGui::TreePop();
	}
}


static
bool isLowLevelEngineType( const TbMetaType& type )
{
	if( type.m_kind >= Type_Void && type.m_kind <= Type_Flags )
	{
		return true;
	}

	//if( type.IsClass() )
	//{
	//	const TbMetaClass& metaClass = type.UpCast< TbMetaClass >();
	//	return metaClass == TypeDeducer< V3f >::GetClass();
	//}

	return false;
}

static void ImGui_EditAssetID(
							  AssetID & asset_id
							  , const char* field_name
							  )
{
	char	buf[256];
	strcpy_s(buf, asset_id.c_str());

	if(ImGui::InputText(field_name, buf, sizeof(buf)))
	{
		asset_id.d = NameID(buf);
	}
	//_log << AssetId_ToChars( asset_id );
}

static
void ImGui_DrawPropertyGrid_recursive( void * object_ptr
									  , const TbMetaType& meta_type
									  , const char* name
									  , int depth
									  , bool force_unfold_nodes = false
									  )
{
	const bool flatten_hierarchy = force_unfold_nodes || ( 0 == depth ) || isLowLevelEngineType( meta_type );
	const ImGuiTreeNodeFlags imgui_tree_node_flags = flatten_hierarchy ? ImGuiTreeNodeFlags_DefaultOpen : 0;

	switch( meta_type.m_kind )
	{
	case ETypeKind::Type_Integer :
		{
			INT32 value = GetInteger( object_ptr, meta_type.m_size );
			ImGui::InputInt( name, &value );
			PutInteger( object_ptr, meta_type.m_size, value );
		}
		break;

	case ETypeKind::Type_Float :
		{
			float value = GetDouble( object_ptr, meta_type.m_size );
			ImGui::InputFloat( name, &value );
			PutDouble( object_ptr, meta_type.m_size, value );
		}
		break;
	case ETypeKind::Type_Bool :
		{
			bool value = TPODCast<bool>::GetConst( object_ptr );
			ImGui::Checkbox( name, &value );
			*((bool*)object_ptr) = value;
		}
		break;

	case ETypeKind::Type_Enum :
		{
			const MetaEnum& enumType = meta_type.UpCast< MetaEnum >();

			ImGui_drawEnumComboBox( object_ptr, enumType, name );
		}
		break;

	case ETypeKind::Type_Flags :
		{
			const MetaFlags& flags_type = meta_type.UpCast< MetaFlags >();

			ImGui_DrawFlagsCheckboxes(
				object_ptr
				, flags_type
				, name
				);

			//const MetaFlags::ValueT oldFlags = flags_type.GetValue( object_ptr );

			//const bool opened_tree_node = ImGui::TreeNodeEx( name, imgui_tree_node_flags );

			//if( opened_tree_node )
			//{
			//	MetaFlags::ValueT newFlags = 0;
			//	for( int itemIdx = 0; itemIdx < flags_type.m_items.count; itemIdx++ )
			//	{
			//		const MetaFlags::Item& item = flags_type.m_items.items[ itemIdx ];

			//		bool value = oldFlags & item.value;
			//		ImGui::Checkbox( item.name, &value );

			//		if( value ) {
			//			newFlags |= item.value;
			//		}
			//	}

			//	if( oldFlags != newFlags ) {
			//		flags_type.SetValue( object_ptr, newFlags );
			//	}

			//	ImGui::TreePop();
			//}
		}
		break;

	case ETypeKind::Type_String :
		{
			//const String & stringReference = TPODCast< String >::GetConst( object_ptr );
			//_log << stringReference;
		}
		break;

	case ETypeKind::Type_Class :
		{
			const TbMetaClass& classType = meta_type.UpCast< TbMetaClass >();

			//
			{
				const bool opened_tree_node = ImGui::TreeNodeEx( name, imgui_tree_node_flags );

				if( opened_tree_node )
				{
					{
						const TbMetaClass* parentType = classType.GetParent();

						while( parentType != nil )
						{
							if( parentType->GetLayout().numFields )
							{
								ImGui_DrawPropertyGrid_recursive(
									object_ptr
									, *parentType
									, parentType->GetTypeName()
									, depth+1
									, force_unfold_nodes
									);
							}

							parentType = parentType->GetParent();
						}
					}

					//
					const TbClassLayout& layout = classType.GetLayout();
					for( int fieldIndex = 0; fieldIndex < layout.numFields; fieldIndex++ )
					{
						const MetaField& field = layout.fields[ fieldIndex ];
						void* memberVarPtr = mxAddByteOffset( object_ptr, field.offset );

						ImGui_DrawPropertyGrid_recursive(
							memberVarPtr
							, field.type
							, field.name
							, depth+1
							, force_unfold_nodes
							);
					}

					ImGui::TreePop();
				}//if tree node is open
			}
		}
		break;

	case ETypeKind::Type_Pointer :
		{
			//_log << "(Pointers not impl)";
		}
		break;

	case ETypeKind::Type_AssetReference:
		{
			NwAssetRef& asset_ref = *static_cast< NwAssetRef* >( object_ptr );
			ImGui_EditAssetID(asset_ref.id, name);
		}
		break;

	case ETypeKind::Type_AssetId :
		{
			AssetID& asset_id = *static_cast< AssetID* >( object_ptr );
			ImGui_EditAssetID(asset_id, name);
		}
		break;

	case ETypeKind::Type_ClassId :
		{
			//_log << "(ClassId not impl)";
		}
		break;

	case ETypeKind::Type_UserData :
		{
			//_log << "(UserData not impl)";
		}
		break;

	case ETypeKind::Type_Blob :
		{
			//_log << "(Blobs not impl)";
		}
		break;

	case ETypeKind::Type_Array :
		{
			const MetaArray& meta_array = meta_type.UpCast< MetaArray >();

			const bool opened_tree_node = ImGui::TreeNodeEx( name, imgui_tree_node_flags );

			if( opened_tree_node )
			{
				const TbMetaType& item_type = meta_array.m_itemType;
				const UINT item_count = meta_array.Generic_Get_Count(object_ptr);
				void* items_start = meta_array.Generic_Get_Data( object_ptr );

				for( int item_index = 0; item_index < item_count; item_index++ )
				{
					void* item_ptr = mxAddByteOffset( items_start, item_type.m_size * item_index );

					String128	item_name;
					Str::Format(item_name, "%s[%d]", item_type.m_name, item_index);

					ImGui_DrawPropertyGrid_recursive(
						item_ptr
						, item_type
						, item_name.c_str()
						, depth+1
						, force_unfold_nodes
						);
				}

				ImGui::TreePop();

			}//if tree node is open
		}
		break;

		mxNO_SWITCH_DEFAULT;
	}//switch
}

EditPropertyGridResult ImGui_DrawPropertyGrid( void * _memory
							, const TbMetaType& _type
							, const char* name )
{
	const U64 struct_hash_before = MurmurHash64(_memory, _type.m_size);

	ImGui_DrawPropertyGrid_recursive( _memory, _type, name, 0 );

	const U64 struct_hash_after = MurmurHash64(_memory, _type.m_size);

	return (struct_hash_before == struct_hash_after)
		? PropertiesDidNotChange : PropertiesDidChange
		;
}
//
//void ImGui_DrawPropertyGrid( void * object_ptr
//							, const TbMetaType& meta_type
//							, const char* name
//							, const EdTypeDatabase& typeDb )
//{
//	switch( meta_type.m_kind )
//	{
//	case ETypeKind::Type_Integer :
//		{
//			INT32 value = GetInteger( object_ptr, meta_type.m_size );
//			ImGui::InputInt( name, &value );
//			PutInteger( object_ptr, meta_type.m_size, value );
//		}
//		break;
//
//	case ETypeKind::Type_Float :
//		{
//			float value = GetDouble( object_ptr, meta_type.m_size );
//			ImGui::InputFloat( name, &value );
//			PutDouble( object_ptr, meta_type.m_size, value );
//		}
//		break;
//	case ETypeKind::Type_Bool :
//		{
//			bool value = TPODCast<bool>::GetConst( object_ptr );
//			ImGui::Checkbox( name, &value );
//			*((bool*)object_ptr) = value;
//		}
//		break;
//
//	case ETypeKind::Type_Enum :
//		{
//			const MetaEnum& enumType = meta_type.UpCast< MetaEnum >();
//			const int enumValue = enumType.GetValue( object_ptr );
//			const char* valueName = enumType.FindString( enumValue );
//UNDONE;
////			int newValue = enumValue;
////			ImGui::Combo( name, &newValue, ? );
//
//			//_log << valueName;
//		}
//		break;
//
//	case ETypeKind::Type_Flags :
//		{
//			//const MetaFlags& flags_type = meta_type.UpCast< MetaFlags >();
//			//String512 temp;
//			//Dbg_FlagsToString( object_ptr, flags_type, temp );
//			//_log << temp;
//		}
//		break;
//
//	case ETypeKind::Type_String :
//		{
//			//const String & stringReference = TPODCast< String >::GetConst( object_ptr );
//			//_log << stringReference;
//		}
//		break;
//
//	case ETypeKind::Type_Class :
//		{
//			const TbMetaClass& classType = meta_type.UpCast< TbMetaClass >();
//			const TbMetaClass* parentType = classType.GetParent();
//			while( parentType != nil )
//			{
//				if( parentType->GetLayout().numFields
//					&& ImGui::TreeNode(parentType->GetTypeName()) )
//				{
//					ImGui_DrawPropertyGrid( object_ptr, *parentType, parentType->GetTypeName() );
//					ImGui::TreePop();
//				}
//				parentType = parentType->GetParent();
//			}
//
//			if( ImGui::TreeNode(name) )
//			{
//				const EdClassInfo* metaClassInfo = typeDb.FindClassInfo(classType);
//				const TbClassLayout& layout = classType.GetLayout();
//				for( int fieldIndex = 0 ; fieldIndex < layout.numFields; fieldIndex++ )
//				{
//					const MetaField& field = layout.fields[ fieldIndex ];
//					void* memberVarPtr = mxAddByteOffset( object_ptr, field.offset );
//					ImGui_DrawPropertyGrid( memberVarPtr, field.type, field.name );
//				}
//				ImGui::TreePop();
//			}
//		}
//		break;
//
//	case ETypeKind::Type_Pointer :
//		{
//			//_log << "(Pointers not impl)";
//		}
//		break;
//
//	case ETypeKind::Type_AssetId :
//		{
//			const AssetID& asset_id = *static_cast< const AssetID* >( object_ptr );
//			//_log << AssetId_ToChars( asset_id );
//		}
//		break;
//
//	case ETypeKind::Type_ClassId :
//		{
//			//_log << "(ClassId not impl)";
//		}
//		break;
//
//	case ETypeKind::Type_UserData :
//		{
//			//_log << "(UserData not impl)";
//		}
//		break;
//
//	case ETypeKind::Type_Blob :
//		{
//			//_log << "(Blobs not impl)";
//		}
//		break;
//
//	case ETypeKind::Type_Array :
//		{
//			//_log << "(Arrays not impl)";
//		}
//		break;
//
//		mxNO_SWITCH_DEFAULT;
//	}//switch
//}

bool ImGui_GetScreenSpacePosition(
							 ImVec2 *screenPosition_,
							 const V3f& worldPosition,
							 const M44f& view_projection_matrix,
							 float screen_width, float screen_height
							 )
{
	const V4f posH = M44_Project( view_projection_matrix, V4f::set(worldPosition, 1.0f) );
	if( posH.w < 0.0f ) {
		return false;	// behind the eye
	}
	// [-1..+1] -> [0..W]
	float x = (posH.x + 1.0f) * (screen_width * 0.5f);
	// [-1..+1] -> [H..0]
	float y = (1.0f - posH.y) * (screen_height * 0.5f);
	*screenPosition_ = ImVec2( x, y );
	return true;
}

void ImGui_Window_Text_Float3( const V3f& xyz )
{
	String128 buffer;
	StringStream writer(buffer);
	writer << xyz;

	ImGui::Text(writer.GetString().c_str());
}

#if MX_DEVELOPER

void ImGui_DrawTweakablesWindow()
{
	if( ImGui::Begin("Tweakables") )
	{
		const Tweaking::TweakableVars& tweakables = Tweaking::GetAllTweakables();
		const Tweaking::TweakableVars::PairsArray& pairs = tweakables.GetPairs();

		for( UINT i = 0; i < pairs.num(); i++ )
		{
			const Tweaking::TweakableVar& tweakable = pairs[i].value;

			switch( tweakable.type )
			{
			case Tweaking::TweakableVar::Bool :
				ImGui::Checkbox( tweakable.name, tweakable.b );
				break;

			case Tweaking::TweakableVar::Int :
				ImGui::InputInt( tweakable.name, tweakable.i );
				break;

			case Tweaking::TweakableVar::Float :
				ImGui::InputFloat( tweakable.name, tweakable.f );
				break;

			case Tweaking::TweakableVar::Double :
				{
					double temp = *tweakable.d;
					ImGui::InputDouble( tweakable.name, &temp );
					*tweakable.d = temp;
				}
				break;

			case Tweaking::TweakableVar::Double3 :
				{
					V3d temp = *tweakable.v3d;

					String64	txtbuf;

					Str::Format(txtbuf, "%s.X", tweakable.name, temp.x);
					ImGui::InputDouble( txtbuf.c_str(), &temp.x );

					Str::Format(txtbuf, "%s.Y", tweakable.name, temp.y);
					ImGui::InputDouble( txtbuf.c_str(), &temp.y );

					Str::Format(txtbuf, "%s.Z", tweakable.name, temp.z);
					ImGui::InputDouble( txtbuf.c_str(), &temp.z );

					*tweakable.v3d = temp;
				}
				break;

				mxNO_SWITCH_DEFAULT;
			}
		}
	}

	ImGui::End();
}

#endif // MX_DEVELOPER


static
void formatByteCount( String &dst_, const U32 num_bytes )
{
	if( num_bytes > mxMEBIBYTE ) {
		Str::Format( dst_, "%.3f Mb", ((float)num_bytes / mxMEBIBYTE) );
	} else if( num_bytes > mxKIBIBYTE ) {
		Str::Format( dst_, "%.3f Kb", ((float)num_bytes / mxKIBIBYTE) );
	} else {
		Str::Format( dst_, "%u bytes", num_bytes );
	}
}

#if MX_DEBUG_MEMORY

static
void ImGui_drawMemoryAllocatorInfo_Recursive( const ProxyAllocator* allocator )
{
	const MemoryAllocatorStats& stats = allocator->_stats;

	String32	currently_allocated_string;
	formatByteCount( currently_allocated_string, stats.currently_allocated );

	String32	peak_memory_usage_string;
	formatByteCount( peak_memory_usage_string, stats.peak_memory_usage );

	//
	String128	temp;
	Str::Format( temp, "'%s': %s allocated, %s max, %u allocs, %u frees",
			allocator->_name, currently_allocated_string.c_str(), peak_memory_usage_string.c_str(), stats.total_allocations, stats.total_frees );

	if( ImGui::TreeNode(allocator->_name, temp.c_str()) )
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("[%s]", allocator->_name);
			ImGui::Text("Currently allocated: %u bytes", stats.currently_allocated);
			ImGui::Text("Peak memory usage: %u bytes", stats.peak_memory_usage);
			ImGui::EndTooltip();
		}

		for( const ProxyAllocator* child = allocator->_first_child; child != nil; child = child->_sibling )
		{
			ImGui_drawMemoryAllocatorInfo_Recursive( child );
		}

		ImGui::TreePop();
	}
}

void ImGui_DrawMemoryAllocatorsTree( const ProxyAllocator* root_allocator )
{
	if( ImGui::Begin("Memory Allocators") )
	{
		ImGui_drawMemoryAllocatorInfo_Recursive( root_allocator );
	}

	ImGui::End();
}

#endif // MX_DEBUG_MEMORY

/* References:
 * Custom UI widgets:
 * 
 * Bezier widget #786
 * https://github.com/ocornut/imgui/issues/786
*/
