/*
=============================================================================
	File:	Device.h
	Desc:	Abstract low-level graphics API, header file.
	Note:	based on Branimir Karadzic's bgfx library
	ToDo:	merge all booleans into bit masks
			cmd - command buffer API? LibCMD?
=============================================================================
*/
#pragma once

#include <GPU/Public/graphics_types.h>
#include <GPU/Public/render_context.h>


/*
=====================================================================
LOW LEVEL GRAPHICS LIBRARY (or LAYER)
=====================================================================
*/
namespace NGpu
{
	struct DeviceSettings
	{
		/// Sets the display state to windowed or full screen.
		bool	use_fullscreen_mode;

		///
		bool	enable_VSync;

	public:
		DeviceSettings()
		{
			use_fullscreen_mode = false;
			enable_VSync = false;
		}
	};

	//enum RuntimeFlags
	//{
	//	/// 
	//	SingleThreaded	= BIT(0),

	//	///
	//	FullScreen		= BIT(1),

	//	///
	//	EnableVSync		= BIT(2),
	//};

	struct Settings
	{
		//!< Window handle (HWND on Windows)
		const void *	window_handle;

		/// D3D11_CREATE_DEVICE_DEBUG
		bool	create_debug_device;

		DeviceSettings	device;

		// hints to minimize memory (re-)allocations
		U32			max_gfx_cmds;
		U32			cmd_buf_size;	//!< size of command buffer
		U32		estimated_render_command_count;
		U32		estimated_command_buffer_size;	//!< size of command buffer

		AllocatorI *		scratch_allocator;
		AllocatorI *		object_allocator;

	public:
		Settings();
	};

	ERet Initialize( const Settings& settings );
	void Shutdown();

	bool isInitialized();

	// returns the immediate device window
	NGpu::NwRenderContext& getMainRenderContext();

	//
	NGpu::NwRenderContext* CreateContext( U32 _size );
	void DeleteContext( NGpu::NwRenderContext *& _context );


	// MEMORY MANAGEMENT

	struct RunTimeSettings
	{
		U32 width;
		U32	height;
		DeviceSettings	device;

	public:
		RunTimeSettings()
		{
			width = height = 0;
		}
	};
	void modifySettings( const RunTimeSettings& settings );


	// MEMORY MANAGEMENT

	typedef void (*ReleaseFn)( void* _ptr, void* _userData );

	/// Allocates a buffer to pass to NGpu calls. Data will be freed inside NGpu.
	const Memory* Allocate( U32 size, U32 alignment = 4 );

	/// Allocate buffer and copy data into it. Data will be freed inside NGpu.
	///
	/// @param[in] _data Pointer to data to be copied.
	/// @param[in] _size Size of data to be copied.
	///
	/// @attention C99 equivalent is `bgfx_copy`.
	///
	const Memory* copy(
		  const void* source_data
		, U32 size
		);

	/// Make reference to data to pass to bgfx. Unlike `bgfx::alloc`, this call
	/// doesn't allocate memory for data. It just copies the _data pointer. You
	/// can pass `ReleaseFn` function pointer to release this memory after it's
	/// consumed, otherwise you must make sure _data is available for at least 2
	/// `bgfx::frame` calls. `ReleaseFn` function must be able to be called
	/// from any thread.
	///
	/// @param[in] _data Pointer to data.
	/// @param[in] _size Size of data.
	/// @param[in] _releaseFn Callback function to release memory after use.
	/// @param[in] _userData User data to be passed to callback function.
	///
	/// @attention Data passed must be available for at least 2 `bgfx::frame` calls.
	/// @attention C99 equivalent are `bgfx_make_ref`, `bgfx_make_ref_release`.
	///
	const Memory* makeRef(
		const void* source_data
		, U32 data_size
		, ReleaseFn release_callback = nil
		, void* user_data = nil
		);


	// resource management

	// render targets
	HColorTarget CreateColorTarget(
		const NwColorTargetDescription& description
		, const char* debug_name = nil
		);
	void DeleteColorTarget( HColorTarget handle );

	HDepthTarget CreateDepthTarget(
		const NwDepthTargetDescription& description
		, const char* debug_name = nil
		);
	void DeleteDepthTarget( HDepthTarget handle );

	// Textures

	/// Creates a texture stored in a native GPU texture format
	HTexture CreateTexture(
		const Memory* texture_data
		, const GrTextureCreationFlagsT flags = NwTextureCreationFlags::defaults
		, const char* debug_name = nil
		);
	/// Creates an array of 2D textures.
	HTexture createTexture2D(
		const NwTexture2DDescription& description
		, const Memory* initial_data = nil
		, const char* debug_name = nil
		);
	HTexture createTexture3D(
		const NwTexture3DDescription& description
		, const Memory* initial_data = nil
		, const char* debug_name = nil
		);
	void DeleteTexture( HTexture handle );

	// render states
	HDepthStencilState CreateDepthStencilState(
		const NwDepthStencilDescription& description
		, const char* debug_name = nil
		);
	HRasterizerState CreateRasterizerState(
		const NwRasterizerDescription& description
		, const char* debug_name = nil
		);
	HSamplerState CreateSamplerState(
		const NwSamplerDescription& description
		, const char* debug_name = nil
		);
	HBlendState CreateBlendState(
		const NwBlendDescription& description
		, const char* debug_name = nil
		);

	void DeleteDepthStencilState( HDepthStencilState handle );
	void DeleteRasterizerState( HRasterizerState handle );
	void DeleteSamplerState( HSamplerState handle );
	void DeleteBlendState( HBlendState handle );

	// Geometry
	HInputLayout createInputLayout(
		const NwVertexDescription& description
		, const char* debug_name = nil
		);
	void destroyInputLayout( HInputLayout handle );

	/// Creates a buffer.
	/// @param data[in] data	Initial data. Pass nil to create a dynamic buffer.
	HBuffer CreateBuffer(
		const NwBufferDescription& description
		, const Memory* initial_data = nil
		, const char* name = nil
		);

	void DeleteBuffer( HBuffer handle );

	HShader CreateShader( const Memory* shader_binary );
	void DeleteShader( HShader handle );

	// Programmable (Shader) Stages
	HProgram CreateProgram( const NwProgramDescription& description );
	void DeleteProgram( HProgram & handle );

	// Cast operations
	// Shader resources (e.g. for binding textures as shader inputs) (@todo: move creation to initialization time?)
	HShaderInput AsInput( HBuffer br );
	HShaderInput AsInput( HTexture tx );
	HShaderInput AsInput( HColorTarget rt );
	HShaderInput AsInput( HDepthTarget rt );

	HShaderOutput AsOutput( HBuffer br );
	HShaderOutput AsOutput( HTexture tx );
	HShaderOutput AsOutput( HColorTarget rt );mxTEMP


	// Update* commands will be executed at the beginning of the frame

	void updateBuffer( HBuffer handle, const Memory* new_contents );

	ERet updateGlobalBuffer(
		HBuffer handle
		, const U32 size
		, void **new_contents_	// the returned memory will be 16-byte aligned
		, const U32 debug_tag = 0	// for setting debug breakpoints
		);

	template< typename TYPE >
	ERet updateGlobalBuffer(
		HBuffer buffer_handle
		, TYPE **o_
		, const U32 tag = 0
		)
	{
		TYPE *	o;
		mxDO(updateGlobalBuffer(
			buffer_handle, sizeof(*o), (void**) &o
			, tag
			));
		*o_ = o;
		return ALL_OK;
	}

	/// Updates a region of a texture from system memory.
	mxTODO("add priority, e.g. so that brick won't get overwritten by debug values");
	void UpdateTexture(
		HTexture handle
		, const NwTextureRegion& region
		, const Memory* new_contents
		);





	// Dynamic geometry rendering.
	// Transient memory lasts one frame at most.
	/// 'transient' mesh buffers for 'immediate-mode' rendering: dynamic geometry, debug lines and HUD/GUI
	bool EnoughSpaceInTransientVertexBuffer( U32 count, U32 stride );

	bool EnoughSpaceInTransientIndexBuffer( U32 count, U32 stride );

	ERet allocateTransientVertexBuffer(
		NwTransientBuffer &vertex_buffer_
		, U32 count, U32 stride
		);

	ERet allocateTransientIndexBuffer(
		NwTransientBuffer &index_buffer_
		, U32 count, U32 stride
		);

	ERet AllocateTransientBuffers(
		NwTransientBuffer &vertex_buffer, U32 vertex_count, U32 vertex_stride,
		NwTransientBuffer &index_buffer, U32 index_count, U32 index_stride
	);



	// Frame submission

	void SetViewParameters(
		const LayerID view_id,
		const ViewState& view_state
	);
	void SetViewShaderInputs(
		const LayerID view_id,
		const ShaderInputs& shader_inputs
	);
	void SetLayerConstantBuffer(
		const LayerID layer_id
		, const HBuffer constant_buffer
		, const U8 constant_buffer_slot
		);
	/// (only if LLGL_ENABLE_PERF_HUD is on)
	void SetViewDebugName(
		const LayerID view_id,
		const char* profiling_scope
	);

	/// Waits for the rendering thread to finish executing all pending rendering commands. Should only be used from the game thread.
	void Flush();

	/// Render the whole frame and call Present/SwapBuffers
	ERet NextFrame();

	U64 getNumRenderedFrames();

	void SaveScreenshot( const char* _where, EImageFileFormat::Enum _format );




	void getLastFrameCounters(
		BackEndCounters *last_rendered_frame_counters_
		);


	/// View stats.
	///
	/// @attention C99 equivalent is `bgfx_view_stats_t`.
	///
	struct ViewStats
	{
		char    name[256];      //!< View name.
		LayerID  view;           //!< View id.
		I64 cpuTimeElapsed; //!< CPU (submit) time elapsed.
		I64 gpuTimeElapsed; //!< GPU time elapsed.
	};

	struct FrontEndStats
	{
		U16	num_created_textures;
		U16	num_created_buffers;
		U16	num_created_shaders;
		U16	num_created_programs;
	};
	void getFrontEndStats( FrontEndStats* front_end_stats_ );

	/// Renderer statistics data.
	///
	/// @attention C99 equivalent is `bgfx_stats_t`.
	///
	/// @remarks All time values are high-resolution timestamps, while
	///   time frequencies define timestamps-per-second for that hardware.
	struct Stats
	{
		I64 cpuTimeFrame;               //!< CPU time between two `bgfx::frame` calls.
		I64 cpuTimeBegin;               //!< Render thread CPU submit begin time.
		I64 cpuTimeEnd;                 //!< Render thread CPU submit end time.
		I64 cpuTimerFreq;               //!< CPU timer frequency. Timestamps-per-second

		I64 gpuTimeBegin;               //!< GPU frame begin time.
		I64 gpuTimeEnd;                 //!< GPU frame end time.
		I64 gpuTimerFreq;               //!< GPU timer frequency.

		I64 waitRender;                 //!< Time spent waiting for render backend thread to finish issuing
		                                    //!  draw commands to underlying graphics API.
		I64 waitSubmit;                 //!< Time spent waiting for submit thread to advance to next frame.

		U32 numDraw;                   //!< Number of draw calls submitted.
		U32 numCompute;                //!< Number of compute calls submitted.
		U32 numBlit;                   //!< Number of blit calls submitted.
		U32 maxGpuLatency;             //!< GPU driver latency.

		U16 numDynamicIndexBuffers;    //!< Number of used dynamic index buffers.
		U16 numDynamicVertexBuffers;   //!< Number of used dynamic vertex buffers.
		U16 numFrameBuffers;           //!< Number of used frame buffers.
		U16 numIndexBuffers;           //!< Number of used index buffers.
		U16 numOcclusionQueries;       //!< Number of used occlusion queries.
		U16 numPrograms;               //!< Number of used programs.
		U16 numShaders;                //!< Number of used shaders.
		U16 numTextures;               //!< Number of used textures.
		U16 numUniforms;               //!< Number of used uniforms.
		U16 numVertexBuffers;          //!< Number of used vertex buffers.
		U16 numVertexDecls;            //!< Number of used vertex declarations.

		I64 textureMemoryUsed;          //!< Estimate of texture memory used.
		I64 rtMemoryUsed;               //!< Estimate of render target memory used.
		I32 transientVbUsed;            //!< Amount of transient vertex buffer used.
		I32 transientIbUsed;            //!< Amount of transient index buffer used.

		U32 numPrims[NwTopology::MAX]; //!< Number of primitives rendered.

		I64 gpuMemoryMax;               //!< Maximum available GPU memory for application.
		I64 gpuMemoryUsed;              //!< Amount of GPU memory used by the application.

		U16 width;                     //!< Backbuffer width in pixels.
		U16 height;                    //!< Backbuffer height in pixels.
		U16 textWidth;                 //!< Debug text width in characters.
		U16 textHeight;                //!< Debug text height in characters.

		U16   numViews;                //!< Number of view stats.
		ViewStats* viewStats;               //!< Array of View stats.

		//uint8_t       numEncoders;          //!< Number of encoders used during frame.
		//EncoderStats* encoderStats;         //!< Array of encoder stats.
	};


	struct Capabilities
	{
		/// Not all hardware supports using R32G32B32_FLOAT as a render-target and shader-resource (it's optional).
		bool	supports_RGB_float_render_target : 1;
	};

	const Capabilities getCapabilities();




	/*
	==========================================================
	|	LOW-LEVEL ACCESS
	==========================================================
	*/
	PlatformData GetPlatformData();

	/// returns a pointer to the corresponding ID3D11ShaderResourceView
	void* GetNativeResourceView( HTexture handle );


}//namespace NGpu
