#pragma once

#if LLGL_Driver == LLGL_Driver_Direct3D_11

/// This define needs to be enabled in order to be able to launch the app do on other machines,
/// where D3DX11CompileFromMemory is not available.
/// This incurs a small memory and performance cost as we need to store the vertex shader bytecode in memory.
#define LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND	(1)

// Include headers for DirectX 11 programming.
#include "d3d_common.h"

#include <D3D11.h>
#include <D3DX11.h>


#if !D3D_USE_DYNAMIC_LIB
// DirectX 9 headers are needed for PIX support and some D3DERR_* codes
#include <d3d9.h>
#pragma comment( lib, "d3d9.lib" )
#endif // !D3D_USE_DYNAMIC_LIB

#if !D3D_USE_DYNAMIC_LIB && MX_AUTOLINK
	#pragma comment( lib, "d3d11.lib" )
	#if MX_DEBUG
		#pragma comment( lib, "d3dx11d.lib" )
	#else
		#pragma comment( lib, "d3dx11.lib" )
	#endif

	#pragma comment (lib, "dxgi.lib")
	#pragma comment( lib, "dxguid.lib" )
	//#pragma comment( lib, "d3dcompiler.lib" )
	#pragma comment( lib, "winmm.lib" )
	#pragma comment( lib, "comctl32.lib" )
#endif // !D3D_USE_DYNAMIC_LIB && MX_AUTOLINK


#include <GPU/Private/Frontend/frontend.h>
#include <GPU/Private/Backend/backend.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Base/Template/THandleManager.h>


/*
=====================================================================
	CORE RUN-TIME
=====================================================================
*/
namespace NGpu
{
	struct BackendD3D11;

	///
	struct ColorTargetD3D11
	{
		ID3D11Texture2D *		m_colorTexture;
		ID3D11RenderTargetView *	m_RTV;
		ID3D11ShaderResourceView *	m_SRV;
		ID3D11UnorderedAccessView *	m_UAV;
		U16		m_width;
		U16		m_height;
		U8		m_format;	// DXGI_FORMAT
		U8		m_flags;
	public:
		ColorTargetD3D11();

		void Create(
			const NwColorTargetDescription& render_target_desc
			, ID3D11Device * device
			, const char* debug_name
			);

		void Destroy();
	};

	///
	struct DepthTargetD3D11
	{
		ID3D11Texture2D *			m_depthTexture;
		ID3D11DepthStencilView *	m_DSV;
		ID3D11ShaderResourceView *	m_SRV;

		// D3D11_DSV_READ_ONLY_DEPTH|D3D11_DSV_READ_ONLY_STENCIL
		ID3D11DepthStencilView *	m_DSV_ReadOnly;

		U16		m_width;
		U16		m_height;
		U32		m_flags;

	public:
		DepthTargetD3D11();
		void Create(
			const NwDepthTargetDescription& depth_target_desc
			, ID3D11Device * device
			, const char* debug_name
			);
		void Destroy();
	};

	///
	struct TextureD3D11
	{
		union {
			ID3D11Resource *		m_resource;
			ID3D11Texture2D *		m_texture2D;
			ID3D11Texture3D *		m_texture3D;
		};

		ID3D11ShaderResourceView *	m_SRV;
		ID3D11UnorderedAccessView *	m_UAV;
		//TextureTypeT	m_type;

		U16				m_width;
		U16				m_height;
		U16				m_depth;

		DataFormatT		m_format;
		U8				m_numMips;	//!< number of mipmap levels in each array slice

		mxOPTIMIZE("pack into flags?")
		GrTextureCreationFlagsT	_flags;	// CPU-writable?

	public:
		TextureD3D11();

		ERet CreateFromMemory(
			const void* data, UINT size
			, ID3D11Device * device
			, GrTextureCreationFlagsT flags
			, const char* debug_name
			);

		void Create(
			const NwTexture2DDescription& txInfo
			, ID3D11Device * device
			, const void* imageData = NULL
			, const char* debug_name = NULL
			);

		void Create(
			const NwTexture3DDescription& txInfo
			, ID3D11Device * device
			, const void* data = NULL
			, const char* debug_name = NULL
			);

		void Destroy();

	private:
		ERet _createTextureFromRawImageData(
			const void* data, UINT size
			,  const GrTextureCreationFlagsT flags
			, ID3D11Device * device
			);

		ERet _createTextureFromDDS(
			const void* data, UINT size
			,  const GrTextureCreationFlagsT flags
			, ID3D11Device * device
			);

		ERet _createTextureFromVBM(
			const void* data, UINT size
			,  const GrTextureCreationFlagsT flags
			, ID3D11Device * device
			);

		ERet _createTextureUsingSTBImage(
			const void* data, UINT size
			,  const GrTextureCreationFlagsT flags
			, ID3D11Device * device
			);

		ERet _createTexture_internal(
			const TextureImage_m& _image
			,  const GrTextureCreationFlagsT flags
			, ID3D11Device * device
			);
	};

	///
	struct DepthStencilStateD3D11
	{
		ID3D11DepthStencilState *	m_ptr;
		//U32						m_checksum;
	public:
		DepthStencilStateD3D11();
		void Create(
			const NwDepthStencilDescription& depth_stencil_state_desc
			, ID3D11Device * device
			, const char* debug_name
			);
		void Destroy();
	};

	///
	struct RasterizerStateD3D11// : DbgNamedObject<>
	{
		ID3D11RasterizerState *		m_ptr;
		//U32						m_checksum;
	public:
		RasterizerStateD3D11();
		void Create(
			const NwRasterizerDescription& rasterizer_state_desc
			, ID3D11Device * device
			, const char* debug_name
			);
		void Destroy();
	};

	///
	struct SamplerStateD3D11
	{
		ID3D11SamplerState *	m_ptr;
		//U32					m_checksum;
	public:
		SamplerStateD3D11();
		void Create(
			const NwSamplerDescription& sampler_state_desc
			, ID3D11Device * device
			, const char* debug_name
			);
		void Destroy();
	};

	///
	struct BlendStateD3D11
	{
		ID3D11BlendState *	m_ptr;
		//U32				m_checksum;
	public:
		BlendStateD3D11();
		void Create(
			const NwBlendDescription& blend_state_desc
			, ID3D11Device * device
			, const char* debug_name
			);
		void Destroy();
	};

	///
	struct InputLayoutD3D11
	{
		ID3D11InputLayout *	m_ptr;
		U8	m_numSlots;	// number of input slots (vertex streams)
		U8	m_strides[LLGL_MAX_VERTEX_STREAMS];	// strides of each vertex buffer stream
		String32	debug_name;

	public:
		InputLayoutD3D11();

		void Create(
			const NwVertexDescription& desc
			, ID3D11Device * device
			);

		void Create(
			const NwVertexDescription& desc
			, const NGpu::Memory* vs_bytecode
			, ID3D11Device * device
			);

		void Destroy();
	};

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

	struct PaddedVertexDescription: NwVertexDescription
	{
		U32		pad[2];
	};
	//mxSTATIC_ASSERT(sizeof(PaddedVertexDescription) == 64*sizeof(char));
  //TIncompleteType<sizeof(PaddedVertexDescription)>  s;  // 80 in 64-bit

#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

	///
	struct BufferD3D11
	{
		ID3D11Buffer *				m_ptr;
		ID3D11ShaderResourceView *	m_SRV;	//!< valid only if this buffer can be sampled
		ID3D11UnorderedAccessView *	m_UAV;	//!< valid only if this is a structured buffer
		U32		m_size;
		//const char *	m_name;
	public:
		BufferD3D11();
		ERet Create(
			const NwBufferDescription& description
			, ID3D11Device * device
			, const void* initial_data = nil
			, const char* debug_name = nil
			);
		void Destroy();
	};
	mxSTATIC_ASSERT_ISPOW2(sizeof(BufferD3D11));

	///
	struct ShaderD3D11
	{
		// pointer to created shader object (e.g.: ID3D11VertexShader, ID3D11PixelShader, etc)
		union {
			ID3D11VertexShader *	m_VS;
			ID3D11HullShader *		m_HS;
			ID3D11DomainShader *	m_DS;
			ID3D11GeometryShader *	m_GS;
			ID3D11PixelShader *		m_PS;
			ID3D11ComputeShader *	m_CS;
			ID3D11DeviceChild *		m_ptr;
		};

		ShaderHeader_d		header;

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND
		mxOPTIMIZE("avoid duplicate vs bytecode");
		const NGpu::Memory	*	vs_bytecode;
#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

	public:
		ShaderD3D11();

		ERet create( const ShaderHeader_d& shaderHeader
			, const void* compiledBytecode, UINT bytecodeLength
			, ID3D11Device * device
			, AllocatorI & allocator_for_vs_bytecode
			);

		void destroy( AllocatorI & allocator_for_vs_bytecode );
	};

	///
	struct ProgramD3D11 : CStruct
	{
		HShader	VS;
		HShader	PS;
		HShader	HS;
		HShader	DS;
		HShader	GS;
		HShader	CS;
		U32	_pad16;	// pad to 16 bytes
	public:
		mxDECLARE_CLASS(ProgramD3D11, CStruct);
		mxDECLARE_REFLECTION;
		ProgramD3D11();
		void destroy();
	};
	mxSTATIC_ASSERT(sizeof(ProgramD3D11) == 16*sizeof(char));

	///
	class ScopedLock_D3D11
	{
		D3D11_MAPPED_SUBRESOURCE		m_mappedData;
		ComPtr< ID3D11DeviceContext >	m_pD3DContext;
		ComPtr< ID3D11Resource >			m_pDstResource;
		const UINT						m_iSubResource;
	public:
		inline ScopedLock_D3D11( ID3D11DeviceContext* pDeviceContext, ID3D11Resource* pDstResource, D3D11_MAP eMapType, UINT iSubResource = 0, UINT nMapFlags = 0 )
			: m_iSubResource( iSubResource )
		{
			m_pD3DContext = pDeviceContext;
			m_pDstResource = pDstResource;
			dxCHK(pDeviceContext->Map( pDstResource, iSubResource, eMapType, nMapFlags, &m_mappedData ));
		}
		inline ~ScopedLock_D3D11()
		{
			m_pD3DContext->Unmap( m_pDstResource, m_iSubResource );
		}
		template< typename TYPE >
		inline TYPE As()
		{
			return static_cast< TYPE >( m_mappedData.pData );
		}
		inline void* ToVoidPtr()
		{
			return m_mappedData.pData;
		}
		inline D3D11_MAPPED_SUBRESOURCE& GetMappedData()
		{
			return m_mappedData;
		}
	};

	struct PerfCounters
	{
		// time taken by the render thread to wait for the submitter thread, in milliseconds
		U64	waitForSubmitMsec;

		U64	waitForRenderMsec;
	};

	mxSTOLEN("https://github.com/nosferalatu/gage/");
	///
	class GpuProfiler 
	{
		// We double buffer the queries
		ID3D11Query* pipeline_statistics[2];
		ID3D11Query* timestamp_disjoint_query[2];
		ID3D11Query* timestamp_begin_frame[2];
		ID3D11Query* timestamp_end_frame[2];

	public:
		GpuProfiler();

		void Startup(ID3D11Device*);
		void Shutdown();

		void BeginFrame(ID3D11DeviceContext*, UINT framecount);
		void EndFrame(ID3D11DeviceContext*, UINT framecount, GPUCounters &_stats);
	};

	///
	class ConstantBufferPool_D3D11: NonCopyable
	{
		struct SlotEntry
		{
			HBuffer		handles[UniformBufferPoolHandles::NUM_SIZE_BINS];
			U16			used_bins;	// a bitmask indicating which handles are used
		};
		SlotEntry	slot_entries[LLGL_MAX_BOUND_UNIFORM_BUFFERS];

	private:
		bool isBufferUsed( U32 slot, U32 bin_index ) const;

	public:
		ConstantBufferPool_D3D11();
		~ConstantBufferPool_D3D11();

		ERet Initialize(
			const UniformBufferPoolHandles& uniform_buffer_pool_handles
			, BackendD3D11 * backend
			);

		void Shutdown();

		HBuffer allocateConstantBuffer( U32 slot, U32 size );
		void releaseConstantBuffer( U32 slot, U32 size );

		void markAllAsFree();
	};

	///
	struct BackendD3D11: AGraphicsBackend, TSingleInstance< BackendD3D11 >
	{
		ComPtr< ID3D11DeviceContext >	_d3d_device_context;	//!< immediate context
		ComPtr< ID3D11Device >			_d3d_device;
		ComPtr< IDXGISwapChain >		_dxgi_swap_chain;

		ComPtr< IDXGIFactory >		_dxgi_factory;
		ComPtr< IDXGIAdapter >		_dxgi_adapter;
		ComPtr< IDXGIOutput >		_dxgi_output;

		//
		NwResolution    resolution;	// back buffer size

		ConstantBufferPool_D3D11	constant_buffer_pool;

		// Async queues: used for compute tasks that can run async with graphics

//		GpuProfiler	profiler;
		UINT		framesRendered;

		// allocator for persistent data: e.g. vertex shader bytecode for creating input layouts on-demand
		AllocatorI &		_allocator;

		// all created graphics resources:

		DepthStencilStateD3D11	depthStencilStates[LLGL_MAX_DEPTH_STENCIL_STATES];
		RasterizerStateD3D11	rasterizerStates[LLGL_MAX_RASTERIZER_STATES];
		SamplerStateD3D11		samplerStates[LLGL_MAX_SAMPLER_STATES];
		BlendStateD3D11			blendStates[LLGL_MAX_BLEND_STATES];

		ColorTargetD3D11		color_targets[LLGL_MAX_DEPTH_TARGETS];
		DepthTargetD3D11		depthTargets[LLGL_MAX_COLOR_TARGETS];

		InputLayoutD3D11	inputLayouts[LLGL_MAX_INPUT_LAYOUTS];

		BufferD3D11		_buffers[LLGL_MAX_BUFFERS];
		TextureD3D11	textures[LLGL_MAX_TEXTURES];

		ShaderD3D11		shaders[LLGL_MAX_SHADERS];
		ProgramD3D11	programs[LLGL_MAX_PROGRAMS];

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

		LLGL_DECLARE_STATIC_ARRAY( PaddedVertexDescription, _vertex_descriptions, LLGL_MAX_INPUT_LAYOUTS );

#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND


#if D3D_USE_DYNAMIC_LIB
		DLL_Holder	m_d3d11_DLL;
		DLL_Holder	m_d3dx11_DLL;	//!< for D3DX11CompileFromMemory() in CreateInputLayout()
		DLL_Holder	m_d3d9_DLL;		//!< for PIX
		DLL_Holder	m_dxgi_DLL;
		DLL_Holder	m_dxgidebug_DLL;
#endif // D3D_USE_DYNAMIC_LIB

		bool	shutdown_was_called;
		
	public:
		BackendD3D11( AllocatorI & allocator );
		~BackendD3D11();

		//
		// AGraphicsBackend
		//

		virtual ERet Initialize(
			const CreationInfo& settings
			, Capabilities &caps_
			, PlatformData &platform_data_
			) override;

		virtual void Shutdown() override;

		virtual bool isDeviceRemoved() const override;

		//
		virtual void createColorTarget(
			const unsigned index
			, const NwColorTargetDescription& description
			, const char* debug_name
			) override;
		virtual void deleteColorTarget( const unsigned index ) override;

		virtual void createDepthTarget(
			const unsigned index
			, const NwDepthTargetDescription& description
			, const char* debug_name
			) override;
		virtual void deleteDepthTarget( const unsigned index ) override;

		//
		virtual void createTexture(
			const unsigned index
			, const Memory* initial_data
			, const GrTextureCreationFlagsT flags
			, const char* debug_name
			) override;

		virtual void createTexture2D(
			const unsigned index
			, const NwTexture2DDescription& description
			, const Memory* initial_data
			, const char* debug_name
			) override;

		virtual void createTexture3D(
			const unsigned index
			, const NwTexture3DDescription& description
			, const Memory* initial_data
			, const char* debug_name
			) override;

		virtual void deleteTexture( const unsigned index ) override;

		//
		virtual void createDepthStencilState(
			const unsigned index
			, const NwDepthStencilDescription& description
			, const char* debug_name
			) override;
		virtual void deleteDepthStencilState( const unsigned index ) override;

		virtual void createRasterizerState(
			const unsigned index
			, const NwRasterizerDescription& description
			, const char* debug_name
			) override;
		virtual void deleteRasterizerState( const unsigned index ) override;

		virtual void createSamplerState(
			const unsigned index
			, const NwSamplerDescription& description
			, const char* debug_name
			) override;
		virtual void deleteSamplerState( const unsigned index ) override;

		virtual void createBlendState(
			const unsigned index
			, const NwBlendDescription& description
			, const char* debug_name
			) override;
		virtual void deleteBlendState( const unsigned index ) override;

		//
		virtual void createInputLayout(
			const unsigned index
			, const NwVertexDescription& description
			, const char* debug_name
			) override;
		virtual void deleteInputLayout( const unsigned index ) override;

		virtual ERet createBuffer(
			const unsigned index
			, const NwBufferDescription& description
			, const Memory* initial_data
			, const char* debug_name
			) override;
		virtual void deleteBuffer( const unsigned index ) override;

		//
		virtual void createShader( const unsigned index, const Memory* shader_binary ) override;
		virtual void deleteShader( const unsigned index ) override;

		virtual void createProgram( const unsigned index, const NwProgramDescription& description ) override;
		virtual void deleteProgram( const unsigned index ) override;

		//
		virtual void updateBuffer( const unsigned index, const void* data, const unsigned size ) override;
		virtual void UpdateTexture( const unsigned index, const NwTextureRegion& region, const Memory* new_contents ) override;

		//
		virtual void renderFrame( Frame & frame, const RunTimeSettings& settings ) override;

	public:
		ID3D11ShaderResourceView* getResourceByHandle( HShaderInput handle );
		ID3D11UnorderedAccessView* getUAVByHandle( HShaderOutput handle );

	private:
		ERet createBackBuffer( const DXGI_SWAP_CHAIN_DESC& scd, HColorTarget render_target_handle );
		ERet releaseBackBuffer();
	};

//------------------------------------------------------------------------
//	DXGI formats
//------------------------------------------------------------------------

#ifndef DXGI_FORMAT_DEFINED
	#error DXGI_FORMAT must be defined!
#endif

mxDECLARE_ENUM( DXGI_FORMAT, U32, DXGI_FORMAT_ENUM );

mxSTOLEN("bgfx");
struct TextureFormatInfoD3D11
{
	DXGI_FORMAT tex;
	DXGI_FORMAT srv;
	DXGI_FORMAT dsv;
};
extern const TextureFormatInfoD3D11 gs_textureFormats[NwDataFormat::MAX];

const char* DXGI_FORMAT_ToChars( DXGI_FORMAT format );
DXGI_FORMAT String_ToDXGIFormat( const char* _str );

UINT DXGI_FORMAT_BitsPerPixel( DXGI_FORMAT format );
UINT DXGI_FORMAT_GetElementSize( DXGI_FORMAT format );

UINT DXGI_FORMAT_GetElementCount( DXGI_FORMAT format );
bool DXGI_FORMAT_HasAlphaChannel( DXGI_FORMAT format );

// Helper functions to create SRGB formats from typeless formats and vice versa.
DXGI_FORMAT DXGI_FORMAT_MAKE_SRGB( DXGI_FORMAT format );
DXGI_FORMAT DXGI_FORMAT_MAKE_TYPELESS( DXGI_FORMAT format );

bool DXGI_FORMAT_IsBlockCompressed( DXGI_FORMAT format );

/// Get surface information for a particular format
void DXGI_FORMAT_GetSurfaceInfo( size_t width, size_t height, DXGI_FORMAT fmt, size_t* pNumBytes, size_t* pRowBytes, size_t* pNumRows );

HRESULT FillInitData(
					 _In_ size_t width,
					 _In_ size_t height,
					 _In_ size_t depth,
					 _In_ size_t mipCount,
					 _In_ size_t arraySize,
					 _In_ DXGI_FORMAT format,
					 _In_ size_t maxsize,
					 _In_ size_t bitSize,
					 _In_reads_bytes_(bitSize) const U8* bitData,
					 _Out_ size_t& twidth,
					 _Out_ size_t& theight,
					 _Out_ size_t& tdepth,
					 _Out_ size_t& skipMip,
					 _Out_writes_(mipCount*arraySize) D3D11_SUBRESOURCE_DATA* initData
					 );

const char* DXGI_ScanlineOrder_ToStr( DXGI_MODE_SCANLINE_ORDER scanlineOrder );
const char* DXGI_ScalingMode_ToStr( DXGI_MODE_SCALING scaling );

DXGI_FORMAT DXGI_GetDepthStencil_Typeless_Format( DXGI_FORMAT depthStencilFormat );
DXGI_FORMAT DXGI_GetDepthStencilView_Format( DXGI_FORMAT depthStencilFormat );
DXGI_FORMAT DXGI_GetDepthStencil_SRV_Format( DXGI_FORMAT depthStencilFormat );

mxFORCEINLINE
DXGI_FORMAT DXGI_GetIndexBufferFormat( const UINT indexStride )
{
	mxASSERT((indexStride == sizeof(UINT16)) || (indexStride == sizeof(U32)));
	return (indexStride == sizeof(UINT16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

//------------------------------------------------------------------------
//	Shader profiles
//------------------------------------------------------------------------

mxDECLARE_ENUM( D3D_FEATURE_LEVEL, U32, D3D_FEATURE_LEVEL_ENUM );

const char* D3D_FeatureLevelToStr( D3D_FEATURE_LEVEL featureLevel );

D3D11_MAP ConvertMapModeD3D( EMapMode mapMode );

}//namespace NGpu

#endif // LLGL_Driver == LLGL_Driver_Direct3D_11
