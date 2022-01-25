//
// Support for Nuklear:
// https://github.com/Immediate-Mode-UI/Nuklear
//
#include <Base/Base.h>
#pragma hdrstop

#include <Core/Memory/MemoryHeaps.h>

#include <Engine/WindowsDriver.h>	// InputState
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <Rendering/Public/Core/VertexFormats.h>

#include <Utility/GUI/GUI_Common.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>




namespace Nuklear
{
	AllocatorI& getMemHeap()
	{ return MemoryHeaps::global(); }
}


static void* nk_malloc(nk_handle unused, void *old,nk_size size)
{
	(void)unused;
	(void)old;
	return Nuklear::getMemHeap().Allocate( size, EFFICIENT_ALIGNMENT );
}

static void nk_mfree(nk_handle unused, void *ptr)
{
	(void)unused;
	Nuklear::getMemHeap().Deallocate( ptr );
}



//
//#define STB_RECT_PACK_IMPLEMENTATION
//#include <stb/stb_rect_pack.h>
//
//#define STB_TRUETYPE_IMPLEMENTATION
//#include <stb/stb_truetype.h>

//
#define NK_ASSERT(expr) mxASSERT(expr)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
//
#define NK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include <Nuklear/nuklear.h>









#define nwCONSTRUCT_IN_PLACE( PTR, TYPE, ... )\
	{\
		mxASSERT( PTR == nil );\
		static TStaticBlob16<TYPE>	TYPE##storage;\
		PTR = new( &TYPE##storage ) TYPE( ## __VA_ARGS__ );\
	}


namespace
{
	typedef Vertex_Nuklear	VertexTypeT;
	typedef U16				IndexTypeT;

	const size_t MAX_VERTICES_SIZE = mxKiB(512);
	const size_t MAX_INDICES_SIZE = mxKiB(128);

	struct NuklearState: TSingleInstance< NuklearState >
	{
		nk_context	ctx;

		HTexture				font_texture;
		nk_font_atlas			font_atlas;
		nk_draw_null_texture	null_texture;

		nk_allocator			custom_allocator;

		//
		DynamicArray< BYTE >	allocated_memory_owner;

		///
		ByteSpanT	core_mem;

		/// converted vertex draw commands
		ByteSpanT	converted_commands_mem;

		/// holds all produced vertices
		ByteSpanT	converted_vertices_mem;

		/// holds all produced indices
		ByteSpanT	converted_indices_mem;

		enum
		{
			MEM_BLOCK_NUKLEAR_CORE,
			MEM_BLOCK_COMMANDS,
			MEM_BLOCK_VERTICES,
			MEM_BLOCK_INDICES,

			MEM_BLOCK_COUNT
		};

		//
		AllocatorI &	allocator;

	public:
		NuklearState( AllocatorI& allocator )
			: allocator( allocator )
			, allocated_memory_owner( allocator )
		{
			font_texture.SetNil();
		}
	};

	static TPtr< NuklearState >	gs_nuklear_state;
}//namespace

///
struct RequiredMemBlockInfo
{
	U32		size;
	U32		alignment;	// must be a power of two

public:
	void setSizeAndAlignment(
		U32 size
		, U32 alignment =  DEFAULT_MEMORY_ALIGNMENT
		)
	{
		this->size = size;
		this->alignment = alignment;
	}
};

///
static ERet
preallocate(
			DynamicArray< BYTE > * owning_array
			, const RequiredMemBlockInfo* required_mem_blocks_infos
			, const UINT num_mem_blocks
			, U32 *aligned_mem_block_offsets
			)
{
	size_t	current_offset = 0;

	for( UINT i = 0; i < num_mem_blocks; i++ )
	{
		const RequiredMemBlockInfo& required_mem_block_info = required_mem_blocks_infos[ i ];

		//
		const size_t alignment = required_mem_block_info.alignment;

		const U32	aligned_offset	= AlignUp( current_offset, alignment );
		const U32	aligned_size	= AlignUp( required_mem_block_info.size, alignment );
		current_offset = aligned_offset + aligned_size;

		//
		aligned_mem_block_offsets[ i ] = aligned_offset;
	}

	//
	const size_t	total_size_to_allocate = current_offset;

	mxOPTIMIZE("virtual memory (uncommitted)");
	//
	void* allocated_mem = owning_array->allocator().Allocate(
		total_size_to_allocate,
		required_mem_blocks_infos[ 0 ].alignment
		);
	//
	mxENSURE( allocated_mem, ERR_OUT_OF_MEMORY, "" );

	//
	owning_array->takeOwnershipOfAllocatedData( allocated_mem, total_size_to_allocate );

	//
	return ALL_OK;
}

static
void initMemorySpans(
					 ByteSpanT *spans_
					 , void* mem_base_ptr
					 , const UINT mem_block_count
					 , const U32* mem_block_offsets
					 , const RequiredMemBlockInfo* mem_blocks_infos
					 )
{
	for( UINT i = 0; i < mem_block_count; i++ )
	{
		//
		spans_[ i ]._data = (BYTE*) mxAddByteOffset(
			mem_base_ptr
			, mem_block_offsets[ i ]
		);
		//
		const RequiredMemBlockInfo& mem_block_info = mem_blocks_infos[ i ];
		spans_[ i ]._count = mem_block_info.size;
	}
}

///
static ERet
Nuklear_preallocateMemory(
			NuklearState & nuklear_state
			)
{
	enum { NUM_MEM_BLOCKS = NuklearState::MEM_BLOCK_COUNT };

	RequiredMemBlockInfo	mem_blocks_infos[ NUM_MEM_BLOCKS ];
	U32						mem_block_offsets[ NUM_MEM_BLOCKS ];

	//
	mem_blocks_infos[
		NuklearState::MEM_BLOCK_NUKLEAR_CORE
	].setSizeAndAlignment(mxKiB(4));

	mem_blocks_infos[
		NuklearState::MEM_BLOCK_COMMANDS
	].setSizeAndAlignment(mxKiB(2));

	mem_blocks_infos[
		NuklearState::MEM_BLOCK_VERTICES
	].setSizeAndAlignment(MAX_VERTICES_SIZE);

	mem_blocks_infos[
		NuklearState::MEM_BLOCK_INDICES
	].setSizeAndAlignment( MAX_INDICES_SIZE, 4 );

	//
	mxDO(preallocate(
		&nuklear_state.allocated_memory_owner
		, mem_blocks_infos
		, NUM_MEM_BLOCKS
		, mem_block_offsets
		));

	//
	initMemorySpans(
		&nuklear_state.core_mem
		, nuklear_state.allocated_memory_owner.raw()
		, NUM_MEM_BLOCKS
		, mem_block_offsets
		, mem_blocks_infos
		);

	return ALL_OK;
}

ERet Nuklear_initialize(
						AllocatorI& allocator
						, const size_t max_memory
						)
{
	nwCONSTRUCT_IN_PLACE(
		gs_nuklear_state._ptr
		, NuklearState, allocator
		);

	//
	mxDO(Nuklear_preallocateMemory( *gs_nuklear_state ));

	//
	mxENSURE(
		1 == nk_init_fixed(
			&gs_nuklear_state->ctx
			, gs_nuklear_state->core_mem.raw()
			, gs_nuklear_state->core_mem.rawSize()
			, nil	// font
			)
		, ERR_UNKNOWN_ERROR
		, ""
		);

	//TODO: set style
	//nk_style_default()

	//
	gs_nuklear_state->custom_allocator.userdata = nk_handle_ptr(nil);
	gs_nuklear_state->custom_allocator.alloc = &nk_malloc;
	gs_nuklear_state->custom_allocator.free = &nk_mfree;

	return ALL_OK;
}

void Nuklear_shudown()
{
	if( gs_nuklear_state.IsValid() )
	{
		NGpu::DeleteTexture( gs_nuklear_state->font_texture );

		nk_free( &gs_nuklear_state->ctx );

		//
		gs_nuklear_state.Destruct();
	}
}

nk_context* Nuklear_getContext()
{
	return &gs_nuklear_state->ctx;
}

bool Nuklear_isMouseHoveringOverGUI()
{
	return nk_item_is_any_active( &gs_nuklear_state->ctx );
}

void Nuklear_processInput(
						  float delta_time_seconds
						  )
{
	const InputState& input_state = NwInputSystemI::Get().getState();
	if( !input_state.window.has_focus ) {
		return;
	}

	//
	nk_context* ctx = Nuklear_getContext();

	nk_input_begin( ctx );

	ctx->delta_time_seconds = delta_time_seconds;

	//
	const MouseState& mouse_state = input_state.mouse;
	nk_input_motion( ctx, mouse_state.x, mouse_state.y );
	nk_input_scroll( ctx, nk_vec2( 0, mouse_state.wheel_scroll ) );
	nk_input_button(ctx, NK_BUTTON_LEFT,   mouse_state.x, mouse_state.y, (mouse_state.held_buttons_mask & BIT(MouseButton_Left)) );
	nk_input_button(ctx, NK_BUTTON_RIGHT,  mouse_state.x, mouse_state.y, (mouse_state.held_buttons_mask & BIT(MouseButton_Right)) );
	nk_input_button(ctx, NK_BUTTON_MIDDLE, mouse_state.x, mouse_state.y, (mouse_state.held_buttons_mask & BIT(MouseButton_Middle)) );
	//nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)glfw.double_click_pos.x, (int)glfw.double_click_pos.y, glfw.is_double_click_down);

	//
	const KeyboardState& keyboard_state = input_state.keyboard;

	//
 //   for (i = 0; i < glfw.text_len; ++i)
 //       nk_input_unicode(ctx, glfw.text[i]);

	//{
		nk_input_key( ctx, NK_KEY_CTRL, (input_state.modifiers & KeyModifier_Ctrl) );
		nk_input_key( ctx, NK_KEY_SHIFT, (input_state.modifiers & KeyModifier_Shift) );
	
		nk_input_key(ctx, NK_KEY_DEL, keyboard_state.held[KEY_Delete] );
		nk_input_key(ctx, NK_KEY_ENTER, keyboard_state.held[KEY_Enter] );
		nk_input_key(ctx, NK_KEY_TAB, keyboard_state.held[KEY_Tab] );
		nk_input_key(ctx, NK_KEY_BACKSPACE, keyboard_state.held[KEY_Backspace] );

		nk_input_key(ctx, NK_KEY_UP, keyboard_state.held[KEY_Up] );
		nk_input_key(ctx, NK_KEY_DOWN, keyboard_state.held[KEY_Down] );
		nk_input_key(ctx, NK_KEY_LEFT, keyboard_state.held[KEY_Left] );
		nk_input_key(ctx, NK_KEY_RIGHT, keyboard_state.held[KEY_Right] );


	//	nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);

	//}

	//if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
	//	glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
	//		nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
	//		nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
	//} else {
	//	nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
	//	nk_input_key(ctx, NK_KEY_COPY, 0);
	//	nk_input_key(ctx, NK_KEY_PASTE, 0);
	//	nk_input_key(ctx, NK_KEY_CUT, 0);
	//	nk_input_key(ctx, NK_KEY_SHIFT, 0);
	//}

	nk_input_end( ctx );
}

static void
load_Fonts_if_needed()
{
	if( gs_nuklear_state->font_texture.IsValid() ) {
		return;
	}

	/* Load Fonts: if none of these are loaded a default font will be used  */
	/* Load Cursor: if you uncomment cursor loading please hide the cursor */
	{
		nk_font_atlas * font_atlas = &gs_nuklear_state->font_atlas;

		//
		nk_font_atlas_init( font_atlas, &gs_nuklear_state->custom_allocator );
		nk_font_atlas_begin( font_atlas );

		/*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../extra_font/DroidSans.ttf", 14, 0);*/
		/*struct nk_font *robot = nk_font_atlas_add_from_file(atlas, "../../extra_font/Roboto-Regular.ttf", 14, 0);*/
		/*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
		/*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../extra_font/ProggyClean.ttf", 12, 0);*/
		/*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../extra_font/ProggyTiny.ttf", 10, 0);*/
		/*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../extra_font/Cousine-Regular.ttf", 13, 0);*/


		// bake font (rasterize) to a big texture containing all glyphs needed
		int w, h;
		const void* image = nk_font_atlas_bake( font_atlas, &w, &h, NK_FONT_ATLAS_RGBA32 );

		/* upload font to texture and create texture view */

		//
		const size_t font_texture_size = w * h * sizeof(UByte4);

		NwTexture2DDescription	font_texture_desc;
		font_texture_desc.format = NwDataFormat::RGBA8;
		font_texture_desc.width = w;
		font_texture_desc.height = h;
		font_texture_desc.numMips = 1;
		gs_nuklear_state->font_texture = NGpu::createTexture2D(
			font_texture_desc
			, NGpu::copy( font_atlas->pixel, font_texture_size )
			IF_DEVELOPER , "NuklearFontTexture"
			);

		//
		nk_font_atlas_end(
			font_atlas
			, nk_handle_id( NGpu::AsInput( gs_nuklear_state->font_texture ).id )
			, &gs_nuklear_state->null_texture
			);

		if( font_atlas->default_font ) {
			nk_style_set_font( &gs_nuklear_state->ctx, &font_atlas->default_font->handle );
		}

		/*nk_style_load_all_cursors(ctx, atlas->cursors);*/
	}

	/* style.c */
#ifdef INCLUDE_STYLE
	/*set_style(ctx, THEME_WHITE);*/
	/*set_style(ctx, THEME_RED);*/
	/*set_style(ctx, THEME_BLUE);*/
	/*set_style(ctx, THEME_DARK);*/
#endif
}

static ERet
execute_draw_command(
					 const nk_draw_command* cmd
					 , const U32 start_index_offset
					 , NGpu::Cmd_Draw & draw_indexed_cmd
					 , NGpu::RenderCommandWriter &cmd_writer
					 )
{
	//
	NwRectangle64	scissor_rectangle;
	const struct nk_rect& clip_rect = cmd->clip_rect;
    scissor_rectangle.left		= (U16)minf( maxf( clip_rect.x,               0 ), MAX_UINT16 );
    scissor_rectangle.right		= (U16)minf( maxf( clip_rect.x + clip_rect.w, 0 ), MAX_UINT16 );
    scissor_rectangle.top		= (U16)minf( maxf( clip_rect.y,               0 ), MAX_UINT16 );
    scissor_rectangle.bottom	= (U16)minf( maxf( clip_rect.y + clip_rect.h, 0 ), MAX_UINT16 );

	mxDO(cmd_writer.setScissor( true, scissor_rectangle ));

	//
	draw_indexed_cmd.start_index = start_index_offset;
	draw_indexed_cmd.index_count = cmd->elem_count;

	cmd_writer.Draw( draw_indexed_cmd );

	return ALL_OK;
}

ERet Nuklear_precache_Fonts_if_needed()
{
	//
	load_Fonts_if_needed();

	return ALL_OK;
}

ERet Nuklear_render(
					const unsigned int scene_pass_id
					)
{
	//
	const UInt2 window_size = WindowsDriver::getWindowSize();

	//
	NwShaderEffect* gui_shader;
	mxDO(Resources::Load( gui_shader, MakeAssetID("gui_nuklear") ));

	//
	mxDO(gui_shader->SetInput(
		nwNAMEHASH("t_texture")
		, NGpu::AsInput( gs_nuklear_state->font_texture )
		));

	//
	const NwShaderEffect::Variant shader_variant = gui_shader->getDefaultVariant();

	//
	NGpu::NwRenderContext& render_context = NGpu::getMainRenderContext();

	// Update shader uniform constants.
	M44f *	proj_matrix;
	{
		mxDO(gui_shader->AllocatePushConstants(
			&proj_matrix
			, render_context._command_buffer
			));
		build_orthographic_projection_matrix_D3D11(
			proj_matrix->a
			, window_size.x, window_size.y
			);
	}

	// Render command lists.

	//
	NGpu::RenderCommandWriter	cmd_writer( render_context );
	//
	cmd_writer.SetRenderState( shader_variant.render_state );
	//
	mxDO(gui_shader->pushAllCommandsInto( render_context._command_buffer ));

	//
	mxDO(gui_shader->BindConstants(
		proj_matrix
		, render_context._command_buffer
		));


	//
	nk_context *	ctx = &gs_nuklear_state->ctx;

	//
	nk_buffer tmp_nk_cmds_buf;
	nk_buffer_init_fixed(
		&tmp_nk_cmds_buf
		, gs_nuklear_state->converted_commands_mem.raw()
		, gs_nuklear_state->converted_commands_mem.rawSize()
		);

	//
	NwTransientBuffer	transient_VB;
	NwTransientBuffer	transient_IB;

	const U32	max_vertices = MAX_VERTICES_SIZE / sizeof(VertexTypeT);
	const U32	max_indices = MAX_INDICES_SIZE / sizeof(IndexTypeT);

	mxDO(NGpu::AllocateTransientBuffers(
		transient_VB
		, max_vertices
		, sizeof(VertexTypeT),

		transient_IB
		, max_indices
		, sizeof(IndexTypeT)
		));

	//
	{/* fill converting configuration */
		nk_convert_config convert_cfg;

		static const nk_draw_vertex_layout_element vertex_layout[] = {
			{NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    NK_OFFSETOF(VertexTypeT, pos)},
			{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    NK_OFFSETOF(VertexTypeT, uv)},
			{NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, NK_OFFSETOF(VertexTypeT, col)},
			{NK_VERTEX_LAYOUT_END}
		};
		memset(&convert_cfg, 0, sizeof(convert_cfg));

		convert_cfg.vertex_layout = vertex_layout;
		convert_cfg.vertex_size = sizeof(VertexTypeT);
		convert_cfg.vertex_alignment = NK_ALIGNOF(VertexTypeT);
		convert_cfg.global_alpha = 1.0f;
		//convert_cfg.shape_AA = AA;
		//convert_cfg.line_AA = AA;
		//convert_cfg.circle_segment_count = 22;
		//convert_cfg.curve_segment_count = 22;
		//convert_cfg.arc_segment_count = 22;
		//convert_cfg.null = d3d11.null;

		{/* setup buffers to load vertices and elements */
			nk_buffer	tmp_vbuf;
			nk_buffer	tmp_ibuf;
			nk_buffer_init_fixed( &tmp_vbuf, transient_VB.data, transient_VB.size );
			nk_buffer_init_fixed( &tmp_ibuf, transient_IB.data, transient_IB.size );
			//
			nk_convert( ctx, &tmp_nk_cmds_buf, &tmp_vbuf, &tmp_ibuf, &convert_cfg );
		}
	}

	//
	NGpu::Cmd_Draw	draw_indexed_cmd;
	draw_indexed_cmd.program = shader_variant.program_handle;
	draw_indexed_cmd.input_layout = VertexTypeT::metaClass().input_layout_handle;
	draw_indexed_cmd.primitive_topology = NwTopology::TriangleList;
	draw_indexed_cmd.use_32_bit_indices = false;

	draw_indexed_cmd.VB = transient_VB.buffer_handle;
	draw_indexed_cmd.IB = transient_IB.buffer_handle;

	draw_indexed_cmd.base_vertex = transient_VB.base_index;
	draw_indexed_cmd.vertex_count = max_vertices;

	//
	U32	first_index_offset = transient_IB.base_index;

	const nk_draw_command* draw_cmd = nil;

	nk_draw_foreach( draw_cmd, ctx, &tmp_nk_cmds_buf )
	{
		if( draw_cmd->elem_count )
		{
			execute_draw_command(
				draw_cmd
				, first_index_offset
				, draw_indexed_cmd
				, cmd_writer
				);

			first_index_offset += draw_cmd->elem_count;
		}
	}

	//
	mxASSERT( cmd_writer.didWriteAnything() );
	cmd_writer.SubmitCommandsWithSortKey(
		NGpu::buildSortKey( scene_pass_id, shader_variant.program_handle.id )
		nwDBG_CMD_SRCFILE_STRING
		);

	// Reset the context state at the end of the frame.
	nk_clear( ctx );

	return ALL_OK;
}

NwNuklearScoped::NwNuklearScoped()
{
	Nuklear_initialize(
		MemoryHeaps::global()
		, mxMiB(1)
		);
}

NwNuklearScoped::~NwNuklearScoped()
{
	Nuklear_shudown();
}

void Nuklear_unhide_window(
						   const char* window_name
						   )
{
	nk_context* nk_ctx = Nuklear_getContext();

	nk_window_show(
		nk_ctx
		, window_name
		, NK_SHOWN
		);
}
