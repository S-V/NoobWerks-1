/*
=============================================================================
ToDo:
	prevent creation of duplicate render states

References:
http://seanmiddleditch.com/journal/2014/02/direct3d-11-debug-api-tricks/
=============================================================================
*/
#if LLGL_Driver == LLGL_Driver_Direct3D_11

//TODO: mxOPTIMIZE("avoid binding resource to VS/GS if it's not used by VS/GS");
#define BIND_VS_RESOURCES	(1)
#define BIND_GS_RESOURCES	(1)

///
#define DBG_DRAW_CALLS	(1)

#define DBG_POOLED_CBs	(0)




/// D3DX APIs are deprecated
#define Use_D3DCompile_instead_of_D3DX11CompileFromMemory	(1)


#include <xutility>
#include <typeinfo.h>	// typeid()

//#include <bx/os.h>
#include <d3dx11.h>

#include <GlobalEngineConfig.h>
#include <Base/Base.h>
#include <Base/Template/Templates.h>	// TCopyBase

#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <GPU/Public/graphics_device.h>

#include <stb/stb_image.h>	// for loading TGA files
#include <Utility/Image/stb_image_Util.h>

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_TextureLoader.h>

#include "Backend_D3D11.h"
#include "DDS_Reader.h"


/// cannot call ReportLiveDeviceObjects() in 32-bit:
/// Run-Time Check Failure #0 - The value of ESP was not properly saved across a function call.
/// This is usually a result of calling a function declared with one calling convention with a function pointer declared with a different calling convention.
#define DEBUG_D3D11_LEAKS (0)//	(MX_DEVELOPER && (mxARCH_TYPE == mxARCH_64BIT))

#if DEBUG_D3D11_LEAKS

	#ifndef _Field_size_
	#define _Field_size_(X)
	#endif

	#ifndef _Out_writes_bytes_opt_
	#define _Out_writes_bytes_opt_(X)
	#endif

	#include <initguid.h>
	#include <dxgidebug.h>

#endif


/// print out each command
#define DBG_COMMANDS	(GFX_CFG_DEBUG_COMMAND_BUFFER)

#define GFX_DBG_LOG_GPU_EVENT_MARKERS	(0)

namespace NGpu
{

#if D3D_USE_DYNAMIC_LIB
	typedef int     (WINAPI* PFN_D3DPERF_BEGIN_EVENT)(DWORD _color, LPCWSTR _wszName);
	typedef int     (WINAPI* PFN_D3DPERF_END_EVENT)();
	typedef void    (WINAPI* PFN_D3DPERF_SET_MARKER)(DWORD _color, LPCWSTR _wszName);
	typedef void    (WINAPI* PFN_D3DPERF_SET_REGION)(DWORD _color, LPCWSTR _wszName);
	typedef BOOL    (WINAPI* PFN_D3DPERF_QUERY_REPEAT_FRAME)();
	typedef void    (WINAPI* PFN_D3DPERF_SET_OPTIONS)(DWORD _options);
	typedef DWORD   (WINAPI* PFN_D3DPERF_GET_STATUS)();
	typedef HRESULT (WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID _riid, void** _factory);
	typedef HRESULT (WINAPI* PFN_GET_DEBUG_INTERFACE)(REFIID _riid, void** _debug);
	typedef HRESULT (WINAPI* PFN_GET_DEBUG_INTERFACE1)(UINT _flags, REFIID _riid, void** _debug);

	static PFN_D3D11_CREATE_DEVICE  D3D11CreateDevice;
	static PFN_CREATE_DXGI_FACTORY  CreateDXGIFactory;
	static PFN_D3DPERF_SET_MARKER   D3DPERF_SetMarker;
	static PFN_D3DPERF_BEGIN_EVENT  D3DPERF_BeginEvent;
	static PFN_D3DPERF_END_EVENT    D3DPERF_EndEvent;
	static PFN_GET_DEBUG_INTERFACE  DXGIGetDebugInterface;
	static PFN_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1;

#if Use_D3DCompile_instead_of_D3DX11CompileFromMemory
	typedef HRESULT (WINAPI *PFN_D3DCompile)(
		LPCVOID                         pSrcData,
		SIZE_T                          SrcDataSize,
		LPCSTR                          pFileName,
		CONST D3D_SHADER_MACRO*         pDefines,
		ID3DInclude*                    pInclude,
		LPCSTR                          pEntrypoint,
		LPCSTR                          pTarget,
		UINT                            Flags1,
		UINT                            Flags2,
		ID3DBlob**                      ppCode,
		ID3DBlob**                      ppErrorMsgs
		);
	static PFN_D3DCompile gs_D3DCompile;
#else
	typedef HRESULT (WINAPI* PFN_D3DX11CompileFromMemory)(
		LPCSTR pSrcData, SIZE_T SrcDataLen,
		LPCSTR pFileName, CONST D3D10_SHADER_MACRO* pDefines, LPD3D10INCLUDE pInclude, 
		LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
		ID3DX11ThreadPump* pPump, ID3D10Blob** ppShader, ID3D10Blob** ppErrorMsgs, HRESULT* pHResult
	);
	static PFN_D3DX11CompileFromMemory gs_D3DX11CompileFromMemory;
#endif

#endif // D3D_USE_DYNAMIC_LIB

ID3D11ShaderResourceView* BackendD3D11::getResourceByHandle( HShaderInput handle )
{
	if( handle.IsNil() ) {
		return nil;
	}
	ID3D11ShaderResourceView* pSRV = nil;
	const UINT type = handle.id & ((1 << SRT_NumBits)-1);
	const UINT index = ((UINT)handle.id >> SRT_NumBits);

	if( type == SRT_Buffer ) {
		pSRV = _buffers[ index ].m_SRV;
	}
	else if( type == SRT_Texture ) {
		pSRV = textures[ index ].m_SRV;
	}
	else if( type == SRT_ColorSurface ) {
		pSRV = color_targets[ index ].m_SRV;
	}
	else if( type == SRT_DepthSurface ) {
		pSRV = depthTargets[ index ].m_SRV;
	}
	mxASSERT_PTR(pSRV);
	return pSRV;
}

ID3D11UnorderedAccessView* BackendD3D11::getUAVByHandle( HShaderOutput handle )
{
	ID3D11UnorderedAccessView* pUAV = nil;
	const UINT type = handle.id & ((1 << SRT_NumBits)-1);
	const UINT index = ((UINT)handle.id >> SRT_NumBits);

	if( type == SRT_Buffer ) {
		pUAV = _buffers[ index ].m_UAV;
	}
	else if( type == SRT_Texture ) {
		pUAV = textures[ index ].m_UAV;
	}
	else if( type == SRT_ColorSurface ) {
		pUAV = color_targets[ index ].m_UAV;
	}
	mxASSERT_PTR(pUAV);
	return pUAV;
}


#if 0
	void BackendD3D11::UpdateVideoMode()
	{
		DXGI_SWAP_CHAIN_DESC	swapChainInfo;
		backend->swap_chain->GetDesc( &swapChainInfo );

		if( backend->resetFlags != 0
		|| swapChainInfo.BufferDesc.Width != (UINT)backend->resolution.width
		|| swapChainInfo.BufferDesc.Height != (UINT)backend->resolution.height )
		{
			swapChainInfo.BufferDesc.Width = (UINT)backend->resolution.width;
			swapChainInfo.BufferDesc.Height = (UINT)backend->resolution.height;

			ReleaseBackBuffer();

			if( backend->resetFlags & RESET_FULLSCREEN ) {
				UNDONE;
			}
			if( backend->resetFlags & RESET_VSYNC ) {
				UNDONE;
			}

			//	A swapchain cannot be resized unless all outstanding references
			//	to its back _buffers have been released.
			//	The application must release all of its direct and indirect references
			//	on the backbuffers in order for ResizeBuffers to succeed.
			//	Direct references are held by the application after calling AddRef on a resource.
			//	Indirect references are held by views to a resource,
			//	binding a view of the resource to a device context,
			//	a command list that used the resource,
			//	a command list that used a view to that resource,
			//	a command list that executed another command list that used the resource, etc.

			//	Before calling ResizeBuffers,
			//	ensure that the application releases all references
			//	(by calling the appropriate number of Release invocations) on the resources,
			//	any views to the resource, any command lists that use either the resources or views,
			//	and ensure that neither the resource, nor a view is still bound to a device context.
			//	ClearState can be used to ensure this.
			//	If a view is bound to a deferred context,
			//	then the partially built command list must be discarded as well
			//	(by calling ClearState, FinishCommandList, then Release on the command list).

			dxCHK(backend->swap_chain->ResizeBuffers(
				swapChainInfo.BufferCount,
				swapChainInfo.BufferDesc.Width,
				swapChainInfo.BufferDesc.Height,
				swapChainInfo.BufferDesc.Format,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			));

			// Recreate render target and depth-stencil.
			CreateBackBuffer( swapChainInfo );

			backend->resetFlags = 0;
		}
	}
#endif

	const D3D_FEATURE_LEVEL minimumRequiredFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	//NativeDevice GetNativeDevice()
	//{
	//	NativeDevice	result;
	//	result.pD3DDevice = backend->device;
	//	return result;
	//}
	//void* GetNativeResourceView( HTexture _handle )
	//{
	//	return backend->textures[ _handle.id ].m_SRV;
	//}

#ifndef DXGI_FORMAT_B4G4R4A4_UNORM
// Win8 only BS
// https://blogs.msdn.com/b/chuckw/archive/2012/11/14/directx-11-1-and-windows-7.aspx?Redirected=true
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb173059%28v=vs.85%29.aspx
#	define DXGI_FORMAT_B4G4R4A4_UNORM DXGI_FORMAT(115)
#endif // DXGI_FORMAT_B4G4R4A4_UNORM

static ERet _SetupDXGI(
	IDXGIFactory  ** outDXGIFactory,
	IDXGIAdapter  ** outDXGIAdapter,
	IDXGIOutput  ** outDXGIOutput,
	DeviceVendor::Enum & outDeviceVendor
	)
{
	HRESULT hr = E_FAIL;

	// create DXGI _dxgi_factory.

	hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), (void**) outDXGIFactory );
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to create DXGIFactory" );
		return ERR_UNKNOWN_ERROR;
	}

	// Enumerate adapters.
	{
		UINT iAdapter = 0;
		IDXGIAdapter * pAdapter = nil;
		while( (*outDXGIFactory)->EnumAdapters( iAdapter, &pAdapter ) != DXGI_ERROR_NOT_FOUND ) {
			pAdapter->Release();
			++iAdapter;
		}
		ptPRINT("Detected %d video _dxgi_adapter(s)",iAdapter);
	}

	hr = (*outDXGIFactory)->EnumAdapters( 0, outDXGIAdapter );
	if( hr == DXGI_ERROR_NOT_FOUND ) {
		ptERROR( "Failed to enumerate video _dxgi_adapter 0" );
		return ERR_UNKNOWN_ERROR;
	}

	DXGI_ADAPTER_DESC  adapterDesc;
	hr = (*outDXGIAdapter)->GetDesc( &adapterDesc );
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to get _dxgi_adapter description" );
		return ERR_UNKNOWN_ERROR;
	}

	{
		ptPRINT("Adapter description: %s",mxTO_ANSI(adapterDesc.Description));
		ptPRINT("VendorId: %u, DeviceId: %u, SubSysId: %u, Revision: %u",
			adapterDesc.VendorId,adapterDesc.DeviceId,adapterDesc.SubSysId,adapterDesc.Revision);

		outDeviceVendor = DeviceVendor::FourCCToVendorEnum( adapterDesc.VendorId );
		if( DeviceVendor::Vendor_Unknown != outDeviceVendor ) {
			ptPRINT("Device vendor: %s", DeviceVendor::GetVendorString( outDeviceVendor ));
		}
		ptPRINT("Dedicated video memory: %u Mb, Dedicated system memory: %u Mb, Shared system memory: %u Mb",
			adapterDesc.DedicatedVideoMemory / (1024*1024),
			adapterDesc.DedicatedSystemMemory / (1024*1024),
			adapterDesc.SharedSystemMemory / (1024*1024)
		);
	}

	//
	hr = (*outDXGIAdapter)->EnumOutputs(
		0 /* The index of the output (monitor) */,
		outDXGIOutput
		);
	if( hr == DXGI_ERROR_NOT_FOUND )
	{
		ptERROR( "Failed to enumerate video card output 0" );
		return ERR_UNKNOWN_ERROR;
	}

	DXGI_OUTPUT_DESC  oDesc;
	hr = (*outDXGIOutput)->GetDesc( &oDesc );
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to get video card output description" );
		return ERR_UNKNOWN_ERROR;
	}

	{
		ptPRINT("Detected output device: %s",mxTO_ANSI(oDesc.DeviceName));
	//	ptPRINT("Desktop coordinates: left(%d),top(%d),right(%d),bottom(%d)\n",
	//		oDesc.DesktopCoordinates.left,oDesc.DesktopCoordinates.top,oDesc.DesktopCoordinates.right,oDesc.DesktopCoordinates.bottom);
		UINT deckstopScrWidth, deckstopScrHeight;
		mxGetCurrentDeckstopResolution( deckstopScrWidth, deckstopScrHeight );
		ptPRINT( "Current desktop resolution: %ux%u", deckstopScrWidth, deckstopScrHeight );
	}

	return ALL_OK;
}
//-------------------------------------------------------------------------------------------------------------//
static ERet _createDirect3DDevice(
								 const AGraphicsBackend::CreationInfo& creation_settings,
								 const ComPtr< IDXGIAdapter >& inDXGIAdapter,
								 ComPtr< ID3D11Device > & outDevice,
								 ComPtr< ID3D11DeviceContext > & outImmediateContext
								 )
{
	UINT  createDeviceFlags = 0;
	// Use this flag if your application will only call methods of Direct3D 11 interfaces from a single thread.
	// By default, the ID3D11Device object is thread-safe. By using this flag, you can increase performance.
	// However, if you use this flag and your application calls methods of Direct3D 11 interfaces from multiple threads,
	// undefined behavior might result.
	createDeviceFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;	// We call Direct3D 11 interface methods only from the render thread.

#if MX_DEBUG //|| MX_DEVELOPER
	if( creation_settings.create_debug_device )
	{
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif // MX_DEBUG || MX_DEVELOPER

	// Required for Direct2D interoperability with Direct3D resources.
	// Needed for supporting DXGI_FORMAT_B8G8R8A8_UNORM. Only works with 10.1 and above.
	//createDeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		//D3D_FEATURE_LEVEL_10_1,
		//D3D_FEATURE_LEVEL_10_0,
		//D3D_FEATURE_LEVEL_9_3,
		//D3D_FEATURE_LEVEL_9_2,
		//D3D_FEATURE_LEVEL_9_1,
	};

	D3D_FEATURE_LEVEL	selectedFeatureLevel;

	HRESULT hr = E_FAIL;

	{
		// If you set the pAdapter parameter to a non-nil value, you must also set the DriverType parameter to the D3D_DRIVER_TYPE_UNKNOWN value.
		// If you set the pAdapter parameter to a non-nil value and the DriverType parameter to the D3D_DRIVER_TYPE_HARDWARE value, D3D11CreateDevice returns an HRESULT of E_INVALIDARG.
		// See: "D3D11CreateDevice function"
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476082(v=vs.85).aspx
		// http://www.gamedev.net/topic/561002-d3d11createdevice---invalid_arg/
		//
		hr = D3D11CreateDevice(
			inDXGIAdapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			( HMODULE )nil,	/* HMODULE Software rasterizer */
			createDeviceFlags,
			featureLevels,	// array of feature levels, null means 'get the greatest feature level available'
			mxCOUNT_OF(featureLevels),
			D3D11_SDK_VERSION,
			&outDevice._ptr,
			&selectedFeatureLevel,
			&outImmediateContext._ptr
		);
	}

	if( FAILED( hr ) )
	{
		dxERROR( hr,
			"Failed to create Direct3D 11 device.\n"
			"This application requires a Direct3D 11 class device "
			"running on Windows Vista or later"
			);
		return ERR_UNKNOWN_ERROR;
	}

	// NOTE: this incurs a significant overhead (frame render time went up from 2 to 15 ms) but helps to find rare driver bugs...
#if 0
	if( 0 && MX_DEBUG )
	{
		ComPtr< ID3D11Debug >	d3dDebug;
		if( SUCCEEDED( outDevice->QueryInterface( IID_ID3D11Debug, (void**)&d3dDebug._ptr ) )
			&& (d3dDebug != nil) )
		{
			dxCHK(d3dDebug->SetFeatureMask( D3D11_DEBUG_FEATURE_FLUSH_PER_RENDER_OP | D3D11_DEBUG_FEATURE_FINISH_PER_RENDER_OP | D3D11_DEBUG_FEATURE_PRESENT_PER_RENDER_OP ));
		}
	}
#endif
	return ALL_OK;
}
//-------------------------------------------------------------------------------------------------------------//
static ERet _checkFeatureLevel( const ComPtr< ID3D11Device >& pD3DDevice )
{
	const D3D_FEATURE_LEVEL selectedFeatureLevel = pD3DDevice->GetFeatureLevel();

	ptPRINT("Selected feature level: %s"
		, D3D_FeatureLevelToStr( selectedFeatureLevel )
		);

	if( selectedFeatureLevel < minimumRequiredFeatureLevel )
	{
		ptERROR(
			"This application requires a Direct3D 11 class device with at least %s support"
			, D3D_FeatureLevelToStr(minimumRequiredFeatureLevel)
			);
		return ERR_UNSUPPORTED_FEATURE;
	}

	return ALL_OK;
}
//-------------------------------------------------------------------------------------------------------------//
static void _GetDeviceCaps(
						   const ComPtr< ID3D11Device >& inD3DDevice
						   , Capabilities &caps_
						   )
{
	// GetGPUThreadPriority() returns E_FAIL when running in PIX
	if(!LLGL_ENABLE_PERF_HUD)
	{
		ComPtr< IDXGIDevice >	pDXGIDevice;
		const HRESULT hr = inD3DDevice->QueryInterface(
			__uuidof( IDXGIDevice ),
			(void**) &pDXGIDevice._ptr
		);
		if( FAILED( hr ) ) {
			dxERROR( hr, "Failed to get IDXGIDevice interface" );
		}
		if( nil != pDXGIDevice )
		{
			INT renderThreadPriority;
			if(FAILED( pDXGIDevice->GetGPUThreadPriority( &renderThreadPriority ) ))
			{
				dxERROR( hr, "GetGPUThreadPriority() failed" );
			}
			ptPRINT("GPU thread priority: %d.",renderThreadPriority);
		}
	}

	D3D11_FEATURE_DATA_THREADING  featureSupportData_Threading;
	if(SUCCEEDED(inD3DDevice->CheckFeatureSupport(
		D3D11_FEATURE_THREADING,
		&featureSupportData_Threading,
		sizeof(featureSupportData_Threading)
		)))
	{
		if( TRUE == featureSupportData_Threading.DriverConcurrentCreates ) {
			ptPRINT("Driver supports concurrent resource creation");
		}
		if( TRUE == featureSupportData_Threading.DriverCommandLists ) {
			ptPRINT("Driver supports command lists");
		}
	}

	D3D11_FEATURE_DATA_DOUBLES  featureSupportData_Doubles;
	if(SUCCEEDED(inD3DDevice->CheckFeatureSupport(
		D3D11_FEATURE_DOUBLES,
		&featureSupportData_Doubles,
		sizeof(featureSupportData_Doubles)
		)))
	{
		if( TRUE == featureSupportData_Doubles.DoublePrecisionFloatShaderOps ) {
			ptPRINT("Driver supports double data types");
		}
	}

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS		hardwareOptions;
	if(SUCCEEDED(inD3DDevice->CheckFeatureSupport(
		D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
		&hardwareOptions,
		sizeof(hardwareOptions)
		)))
	{
		if( TRUE == hardwareOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x ) {
			ptPRINT("Driver supports Compute Shaders Plus Raw And Structured Buffers Via Shader Model 4.x");
		}
	}

	{
		UINT format_support_flags;
		inD3DDevice->CheckFormatSupport(
			DXGI_FORMAT_R32G32B32_FLOAT,
			&format_support_flags
			);

		caps_.supports_RGB_float_render_target = !!(format_support_flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
	}
}
//-------------------------------------------------------------------------------------------------------------//
static bool FindSuitableRefreshRate(
	const ComPtr< IDXGIOutput >& inDXGIOutput,
	const UINT inBackbufferWidth, const UINT inBackbufferHeight,
	const DXGI_FORMAT inBackBufferFormat,
	DXGI_RATIONAL &outRefreshRate )
{
	// Microsoft best practices advises this step.
	outRefreshRate.Numerator = 0;
	outRefreshRate.Denominator = 0;

	const UINT	flags = DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING;

	UINT	numDisplayModes = 0;

	HRESULT	hr = inDXGIOutput->GetDisplayModeList(
		inBackBufferFormat,
		flags,
		&numDisplayModes,
		0
	);
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to get the number of display modes." );
		return false;
	}

	enum { MAX_DISPLAY_MODES = 32 };

	DXGI_MODE_DESC	displayModes[ MAX_DISPLAY_MODES ];

	numDisplayModes = smallest( numDisplayModes, mxCOUNT_OF(displayModes) );

	hr = inDXGIOutput->GetDisplayModeList(
		inBackBufferFormat,
		flags,
		&numDisplayModes,
		displayModes
	);
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to get display mode list" );
		return false;
	}

	ptPRINT( "List of supported (%d) display modes matching the color format '%s':",
			numDisplayModes, DXGI_FORMAT_ToChars( inBackBufferFormat ) );

	for(UINT iDisplayMode = 0; iDisplayMode < numDisplayModes; iDisplayMode++)
	{
		const DXGI_MODE_DESC& displayMode = displayModes[ iDisplayMode ];

		ptPRINT( "[%u]: %ux%u, %u Hz",
			iDisplayMode+1, displayMode.Width, displayMode.Height,
			displayMode.RefreshRate.Numerator / displayMode.RefreshRate.Denominator );

		if( displayMode.Width == inBackbufferWidth
			&& displayMode.Height == inBackbufferHeight )
		{
			outRefreshRate.Numerator = displayMode.RefreshRate.Numerator;
			outRefreshRate.Denominator = displayMode.RefreshRate.Denominator;
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------------------------------//
static bool FindSuitableDisplayMode(
	const ComPtr< IDXGIOutput >& inDXGIOutput,
	const UINT inBackbufferWidth, const UINT inBackbufferHeight,
	const DXGI_FORMAT inBackBufferFormat,
	const DXGI_RATIONAL& inRefreshRate,
	DXGI_MODE_DESC &outBufferDesc
	)
{
	DXGI_MODE_DESC	desiredDisplayMode;
	mxZERO_OUT( desiredDisplayMode );

	desiredDisplayMode.Width	= inBackbufferWidth;
	desiredDisplayMode.Height 	= inBackbufferHeight;
	desiredDisplayMode.Format 	= inBackBufferFormat;
	desiredDisplayMode.RefreshRate = inRefreshRate;

	desiredDisplayMode.ScanlineOrdering	= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desiredDisplayMode.Scaling			= DXGI_MODE_SCALING_UNSPECIFIED;

	const HRESULT hr = inDXGIOutput->FindClosestMatchingMode(
		&desiredDisplayMode,
		&outBufferDesc,
		nil
	);
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to find a supported video mode." );
		return false;
	}

	return true;
}
//-------------------------------------------------------------------------------------------------------------//
UINT getNumMSAASamples(
	const DXGI_FORMAT inBackBufferFormat,
	const ComPtr< ID3D11Device >& inD3DDevice
	)
{
	const bool	bMultisampleAntiAliasing = false;

	if( bMultisampleAntiAliasing )
	{
		UINT	numMsaaSamples = 32;

		// Get the number of quality levels available during multisampling.
		while( numMsaaSamples > 1 )
		{
			UINT	numQualityLevels = 0;
			if(SUCCEEDED( inD3DDevice->CheckMultisampleQualityLevels( inBackBufferFormat, numMsaaSamples, &numQualityLevels ) ))
			{
				if( numQualityLevels > 0 ) {
					// The format and sample count combination are supported for the installed _dxgi_adapter.
					break;
				}
			}
			numMsaaSamples /= 2;
		}

		ptPRINT("Multisample Anti-Aliasing: %d samples.",numMsaaSamples);

		return numMsaaSamples;
	}
	else
	{
		return 1;
	}
}
//-------------------------------------------------------------------------------------------------------------//
static void CreateSwapChainDescription(
	const ComPtr< ID3D11Device >& inD3DDevice,
	const HWND inWindowHandle, const BOOL inWindowedMode,
	const UINT inBackbufferWidth, const UINT inBackbufferHeight,
	const DXGI_FORMAT inBackBufferFormat,
	const DXGI_RATIONAL& inRefreshRate,
	DXGI_SWAP_CHAIN_DESC &outSwapChainDesc
	)
{
	// for switching between windowed/fullscreen modes
	const bool	allowModeSwitch = true;

	const bool	bSampleBackBuffer = false;

	DXGI_USAGE	backBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//backBufferUsage |= DXGI_USAGE_BACK_BUFFER;	//<- this line is optional

	if( bSampleBackBuffer ) {
		backBufferUsage |= DXGI_USAGE_SHADER_INPUT;
	}


	mxZERO_OUT( outSwapChainDesc );

	outSwapChainDesc.BufferCount	= 1;

	outSwapChainDesc.BufferDesc.Width			= inBackbufferWidth;
	outSwapChainDesc.BufferDesc.Height 			= inBackbufferHeight;
	outSwapChainDesc.BufferDesc.Format 			= inBackBufferFormat;
	outSwapChainDesc.BufferDesc.RefreshRate		= inRefreshRate;
	outSwapChainDesc.BufferDesc.ScanlineOrdering= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	outSwapChainDesc.BufferDesc.Scaling			= DXGI_MODE_SCALING_UNSPECIFIED;

	outSwapChainDesc.BufferUsage	= backBufferUsage;

	outSwapChainDesc.OutputWindow	= inWindowHandle;	// the window that the swap chain will use to present images on the screen

	outSwapChainDesc.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;	// the contents of the back buffer are discarded after calling IDXGISwapChain::Present()

	outSwapChainDesc.Windowed	= inWindowedMode;

	if( allowModeSwitch ) {
		outSwapChainDesc.Flags	= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// flag for switching between windowed/fullscreen modes
	}

#if 0
	FindSuitableDisplayMode(
		inDXGIOutput,
		inBackbufferWidth, inBackbufferHeight, inBackBufferFormat, inRefreshRate,
		sd.BufferDesc );
#endif

	outSwapChainDesc.SampleDesc.Count = getNumMSAASamples( outSwapChainDesc.BufferDesc.Format, inD3DDevice );
	outSwapChainDesc.SampleDesc.Quality	= 0;
}
//-------------------------------------------------------------------------------------------------------------//
void _getSwapChainDescription(
							  DXGI_SWAP_CHAIN_DESC &desc_
							  , HWND hWnd
							  , const BOOL windowed
							  , ID3D11Device* device
							  )
{
	RECT	rect;
	::GetClientRect( hWnd, &rect );

	const UINT	windowWidth = rect.right - rect.left;
	const UINT	windowHeight = rect.bottom - rect.top;

	//const BOOL	isFullScreen = !windowed;
	//if( isFullScreen )
	//{
	//	windowWidth = GetSystemMetrics( SM_CXSCREEN );
	//	windowHeight = GetSystemMetrics( SM_CYSCREEN );
	//}

	const DXGI_FORMAT	backBufferFormat =
		//DXGI_FORMAT_R8G8B8A8_UNORM

		// Use sRGB frame-buffer extensions for efficient automatic gamma correction with proper blending.
		// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch24.html
		// When writing to sRGB, colors are transformed from linear space (in pixel shaders) to gamma (pow(2.2) curve).
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		;

	DXGI_RATIONAL refreshRate;

	// Microsoft best practices advises this step.
	refreshRate.Numerator = 0;
	refreshRate.Denominator = 0;

	CreateSwapChainDescription(
		device,
		hWnd, windowed,
		windowWidth, windowHeight, backBufferFormat,
		refreshRate,
		desc_
	);

	// Set this flag to enable an application to render using GDI on a swap chain or a surface.
	// This will allow the application to call IDXGISurface1::GetDC on the 0th back buffer or a surface.
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
}
//-------------------------------------------------------------------------------------------------------------//
static ERet _createDirect3DSwapChain(
	DXGI_SWAP_CHAIN_DESC& sd,
	const ComPtr< IDXGIFactory >& inDXGIFactory,
	const ComPtr< ID3D11Device >& inD3DDevice,
	ComPtr< IDXGISwapChain > &outSwapChain
	)
{
	ptPRINT("Creating a swap chain...");
	ptPRINT("Selected display mode: %ux%u",sd.BufferDesc.Width,sd.BufferDesc.Height);
	ptPRINT("Back buffer format: %s",DXGI_FORMAT_ToChars(sd.BufferDesc.Format));
	if( sd.BufferDesc.RefreshRate.Denominator != 0 ) {
		ptPRINT("Selected refresh rate: %u Hertz", sd.BufferDesc.RefreshRate.Numerator/sd.BufferDesc.RefreshRate.Denominator );
	} else {
		ptPRINT("Selected default refresh rate");
	}
	ptPRINT("Scanline ordering: %s",DXGI_ScanlineOrder_ToStr(sd.BufferDesc.ScanlineOrdering));
	ptPRINT("Scaling: %s",DXGI_ScalingMode_ToStr(sd.BufferDesc.Scaling));

	const HRESULT hr = inDXGIFactory->CreateSwapChain( inD3DDevice, &sd, &outSwapChain._ptr );
	if( FAILED( hr ) ) {
		dxERROR( hr, "Failed to create a swap chain" );
		return ERR_UNKNOWN_ERROR;
	}
/*
	An alternative way to create device and swap chain:

	hr = D3D11CreateDeviceAndSwapChain(
		pDXGIAdapter0,
		BackendD3D11::Type,
		( HMODULE )null,	// HMODULE Software rasterizer
		createDeviceFlags,
		featureLevels,	// array of feature levels, null means 'get the greatest feature level available'
		mxCOUNT_OF(featureLevels),	// numFeatureLevels
		D3D11_SDK_VERSION,
		&swapChainInfo,
		&swap_chain._ptr,
		&device._ptr,
		&selectedFeatureLevel,
		&immediateContext._ptr
	);
	if( FAILED( hr ) ) {
		dxERROR( hr,
			"Failed to create device and swap chain.\n"
			"This application requires a Direct3D 11 class device "
			"running on Windows Vista (or later)\n" );
		RETURN_ON_ERROR;
	}
*/
	return ALL_OK;
}
//-------------------------------------------------------------------------------------------------------------//
static bool DisableAltEnter( const HWND inWindowHandle,
							const ComPtr< IDXGIFactory >& inDXGIFactory )
{
	// Prevent DXGI from monitoring message queue for the Alt-Enter key sequence
	// (which causes the application to switch from windowed to fullscreen or vice versa).
	// IDXGIFactory::MakeWindowAssociation is recommended,
	// because a standard control mechanism for the user is strongly desired.

	const HRESULT hr = inDXGIFactory->MakeWindowAssociation(
		inWindowHandle,
		DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER /*| DXGI_MWA_NO_PRINT_SCREEN*/
	);
	if( FAILED( hr ) ) {
		dxWARN( hr, "MakeWindowAssociation() failed" );
		return false;
	}

	return true;
}

static D3D11_COMPARISON_FUNC D3D11_Convert_ComparisonFunc( NwComparisonFunc::Enum e )
{
	switch( e ) {
		case NwComparisonFunc::Always :		return D3D11_COMPARISON_ALWAYS;
		case NwComparisonFunc::Never :		return D3D11_COMPARISON_NEVER;
		case NwComparisonFunc::Less :			return D3D11_COMPARISON_LESS;
		case NwComparisonFunc::Equal :		return D3D11_COMPARISON_EQUAL;
		case NwComparisonFunc::Greater :		return D3D11_COMPARISON_GREATER;
		case NwComparisonFunc::Not_Equal :	return D3D11_COMPARISON_NOT_EQUAL;
		case NwComparisonFunc::Less_Equal :	return D3D11_COMPARISON_LESS_EQUAL;
		case NwComparisonFunc::Greater_Equal :return D3D11_COMPARISON_GREATER_EQUAL;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_COMPARISON_ALWAYS;
}

static D3D11_STENCIL_OP D3D11_Convert_StencilOp( NwStencilOp::Enum e )
{
	switch( e ) {
		case NwStencilOp::KEEP :		return D3D11_STENCIL_OP_KEEP;
		case NwStencilOp::ZERO :		return D3D11_STENCIL_OP_ZERO;
		case NwStencilOp::INCR_WRAP :	return D3D11_STENCIL_OP_INCR;
		case NwStencilOp::DECR_WRAP :	return D3D11_STENCIL_OP_DECR;
		case NwStencilOp::REPLACE :	return D3D11_STENCIL_OP_REPLACE;
		case NwStencilOp::INCR_SAT :	return D3D11_STENCIL_OP_INCR_SAT;
		case NwStencilOp::DECR_SAT :	return D3D11_STENCIL_OP_DECR_SAT;
		case NwStencilOp::INVERT :	return D3D11_STENCIL_OP_INVERT;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_STENCIL_OP_KEEP;
}

static void D3D11_Convert_DepthStencil_Description( const NwDepthStencilDescription& input, D3D11_DEPTH_STENCIL_DESC &output )
{
	output.DepthEnable = (input.flags & NwDepthStencilFlags::Enable_DepthTest) != 0;

	output.DepthWriteMask
		= (input.flags & NwDepthStencilFlags::Enable_DepthWrite)
		? D3D11_DEPTH_WRITE_MASK_ALL
		: D3D11_DEPTH_WRITE_MASK_ZERO
		;

	output.DepthFunc = D3D11_Convert_ComparisonFunc( input.comparison_function );

	output.StencilEnable			= (input.flags & NwDepthStencilFlags::Enable_Stencil) != 0;
	output.StencilReadMask			= input.stencil_read_mask;
	output.StencilWriteMask			= input.stencil_write_mask;

	output.FrontFace.StencilFailOp			= D3D11_Convert_StencilOp(input.front_face.stencil_fail_op);
	output.FrontFace.StencilDepthFailOp		= D3D11_Convert_StencilOp(input.front_face.depth_fail_op);
	output.FrontFace.StencilPassOp			= D3D11_Convert_StencilOp(input.front_face.stencil_pass_op);
	output.FrontFace.StencilFunc			= D3D11_Convert_ComparisonFunc(input.front_face.stencil_comparison);

	output.BackFace.StencilFailOp			= D3D11_Convert_StencilOp(input.back_face.stencil_fail_op);
	output.BackFace.StencilDepthFailOp		= D3D11_Convert_StencilOp(input.back_face.depth_fail_op);
	output.BackFace.StencilPassOp			= D3D11_Convert_StencilOp(input.back_face.stencil_pass_op);
	output.BackFace.StencilFunc				= D3D11_Convert_ComparisonFunc(input.back_face.stencil_comparison);
}

static D3D11_FILL_MODE D3D11_Convert_FillMode( NwFillMode::Enum e )
{
	switch( e ) {
		case NwFillMode::Solid :		return D3D11_FILL_SOLID;
		case NwFillMode::Wireframe :	return D3D11_FILL_WIREFRAME;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_FILL_SOLID;
}

static D3D11_CULL_MODE D3D11_Convert_CullMode( NwCullMode::Enum e )
{
	switch( e ) {
		case NwCullMode::None :	return D3D11_CULL_NONE;
		case NwCullMode::Back :	return D3D11_CULL_BACK;
		case NwCullMode::Front :	return D3D11_CULL_FRONT;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_CULL_NONE;
}

static void D3D11_Convert_Rasterizer_Description( const NwRasterizerDescription& input, D3D11_RASTERIZER_DESC &output )
{
	output.FillMode					= D3D11_Convert_FillMode( input.fill_mode );
	output.CullMode					= D3D11_Convert_CullMode( input.cull_mode );
	output.FrontCounterClockwise	= TRUE;
	output.DepthBias				= 0;
	output.DepthBiasClamp			= 0.000000f;
	output.SlopeScaledDepthBias		= 0.000000f;
	output.DepthClipEnable			= (input.flags & NwRasterizerFlags::Enable_DepthClip) != 0;
	output.ScissorEnable			= (input.flags & NwRasterizerFlags::Enable_Scissor) != 0;
	output.MultisampleEnable		= (input.flags & NwRasterizerFlags::Enable_Multisample) != 0;
	output.AntialiasedLineEnable	= (input.flags & NwRasterizerFlags::Enable_AntialiasedLine) != 0;
}

static D3D11_TEXTURE_ADDRESS_MODE D3D11_Convert_TextureAddressMode( NwTextureAddressMode::Enum e )
{
	switch( e ) {
		case NwTextureAddressMode::Wrap :			return D3D11_TEXTURE_ADDRESS_WRAP;
		case NwTextureAddressMode::Clamp :		return D3D11_TEXTURE_ADDRESS_CLAMP;
		case NwTextureAddressMode::Border :		return D3D11_TEXTURE_ADDRESS_BORDER;
		case NwTextureAddressMode::Mirror :		return D3D11_TEXTURE_ADDRESS_MIRROR;		
		case NwTextureAddressMode::MirrorOnce :	return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_TEXTURE_ADDRESS_WRAP;
}

static D3D11_FILTER D3D11_Convert_TextureFilter( NwTextureFilter::Enum e, bool enableComparison )
{
	if( enableComparison )
	{
		switch( e )
		{
		case NwTextureFilter::Min_Mag_Mip_Point :               return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		case NwTextureFilter::Min_Mag_Point_Mip_Linear :        return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
		case NwTextureFilter::Min_Point_Mag_Linear_Mip_Point :  return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case NwTextureFilter::Min_Point_Mag_Mip_Linear :        return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
		case NwTextureFilter::Min_Linear_Mag_Mip_Point :        return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		case NwTextureFilter::Min_Linear_Mag_Point_Mip_Linear : return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case NwTextureFilter::Min_Mag_Linear_Mip_Point :        return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		case NwTextureFilter::Min_Mag_Mip_Linear :              return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		case NwTextureFilter::Anisotropic :                     return D3D11_FILTER_COMPARISON_ANISOTROPIC;
			mxNO_SWITCH_DEFAULT;
		}
	}
	else
	{
		switch( e )
		{
		case NwTextureFilter::Min_Mag_Mip_Point :                          return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case NwTextureFilter::Min_Mag_Point_Mip_Linear :                   return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		case NwTextureFilter::Min_Point_Mag_Linear_Mip_Point :             return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case NwTextureFilter::Min_Point_Mag_Mip_Linear :                   return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		case NwTextureFilter::Min_Linear_Mag_Mip_Point :                   return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		case NwTextureFilter::Min_Linear_Mag_Point_Mip_Linear :            return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case NwTextureFilter::Min_Mag_Linear_Mip_Point :                   return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case NwTextureFilter::Min_Mag_Mip_Linear :                         return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case NwTextureFilter::Anisotropic :                                return D3D11_FILTER_ANISOTROPIC;
			mxNO_SWITCH_DEFAULT;
		}
	}
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

static void D3D11_Convert_Sampler_Description( const NwSamplerDescription& input, D3D11_SAMPLER_DESC &output )
{
	bool enableComparison = (input.comparison != NwComparisonFunc::Never && input.comparison != NwComparisonFunc::Always);
	output.Filter	= D3D11_Convert_TextureFilter( input.filter, enableComparison );
	output.AddressU	= D3D11_Convert_TextureAddressMode( input.address_U );
	output.AddressV	= D3D11_Convert_TextureAddressMode( input.address_V );
	output.AddressW	= D3D11_Convert_TextureAddressMode( input.address_W );
	output.MipLODBias		= input.mip_LOD_bias;
	output.MaxAnisotropy	= input.max_anisotropy;
	output.ComparisonFunc	= D3D11_Convert_ComparisonFunc( input.comparison );
	output.BorderColor[0]	= input.border_color[0];
	output.BorderColor[1]	= input.border_color[1];
	output.BorderColor[2]	= input.border_color[2];
	output.BorderColor[3]	= input.border_color[3];
	output.MinLOD			= input.min_LOD;
	output.MaxLOD			= input.max_LOD;
}

static D3D11_BLEND D3D11_Convert_BlendMode( NwBlendFactor::Enum e )
{
	switch( e ) {
	case NwBlendFactor::ZERO :				return D3D11_BLEND_ZERO;
	case NwBlendFactor::ONE :				return D3D11_BLEND_ONE;
	case NwBlendFactor::SRC_COLOR :			return D3D11_BLEND_SRC_COLOR;
	case NwBlendFactor::INV_SRC_COLOR :		return D3D11_BLEND_INV_SRC_COLOR;
	case NwBlendFactor::SRC_ALPHA :			return D3D11_BLEND_SRC_ALPHA;
	case NwBlendFactor::INV_SRC_ALPHA :		return D3D11_BLEND_INV_SRC_ALPHA;
	case NwBlendFactor::DST_ALPHA :		return D3D11_BLEND_DEST_ALPHA;
	case NwBlendFactor::INV_DST_ALPHA :	return D3D11_BLEND_INV_DEST_ALPHA;
	case NwBlendFactor::DST_COLOR :		return D3D11_BLEND_DEST_COLOR;
	case NwBlendFactor::INV_DST_COLOR :	return D3D11_BLEND_INV_DEST_COLOR;
	case NwBlendFactor::SRC_ALPHA_SAT :		return D3D11_BLEND_SRC_ALPHA_SAT;
	case NwBlendFactor::BLEND_FACTOR :		return D3D11_BLEND_BLEND_FACTOR;
	case NwBlendFactor::INV_BLEND_FACTOR :	return D3D11_BLEND_INV_BLEND_FACTOR;
	case NwBlendFactor::SRC1_COLOR :		return D3D11_BLEND_SRC1_COLOR;
	case NwBlendFactor::INV_SRC1_COLOR :	return D3D11_BLEND_INV_SRC1_COLOR;
	case NwBlendFactor::SRC1_ALPHA :		return D3D11_BLEND_SRC1_ALPHA;
	case NwBlendFactor::INV_SRC1_ALPHA :	return D3D11_BLEND_INV_SRC1_ALPHA;
	mxNO_SWITCH_DEFAULT;
	}
	return D3D11_BLEND_ONE;
}

static D3D11_BLEND_OP D3D11_Convert_BlendOp( NwBlendOp::Enum e )
{
	switch( e ) {
	case NwBlendOp::MIN :			return D3D11_BLEND_OP_MIN;
	case NwBlendOp::MAX :			return D3D11_BLEND_OP_MAX;
	case NwBlendOp::ADD :			return D3D11_BLEND_OP_ADD;
	case NwBlendOp::SUBTRACT :	return D3D11_BLEND_OP_SUBTRACT;
	case NwBlendOp::REV_SUBTRACT :return D3D11_BLEND_OP_REV_SUBTRACT;
	mxNO_SWITCH_DEFAULT;
	}
	return D3D11_BLEND_OP_ADD;
}

static U8 D3D11_Convert_ColorWriteMask( NwColorWriteMask8 flags )
{
	U8 result = 0;
	if( flags & NwColorWriteMask::Enable_Red )	result |= D3D11_COLOR_WRITE_ENABLE_RED;
	if( flags & NwColorWriteMask::Enable_Green )	result |= D3D11_COLOR_WRITE_ENABLE_GREEN;
	if( flags & NwColorWriteMask::Enable_Blue )	result |= D3D11_COLOR_WRITE_ENABLE_BLUE;
	if( flags & NwColorWriteMask::Enable_Alpha )	result |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	return result;
}

static void D3D11_Convert_Blend_Description( const NwBlendDescription& input, D3D11_BLEND_DESC &output )
{
	output.AlphaToCoverageEnable	= input.flags & NwBlendFlags::Enable_AlphaToCoverage;
	output.IndependentBlendEnable	= false;//input.enableIndependentBlend;

	output.RenderTarget[0].BlendEnable		= input.flags & NwBlendFlags::Enable_Blending;
	output.RenderTarget[0].SrcBlend			= D3D11_Convert_BlendMode( input.color_channel.source_factor );
	output.RenderTarget[0].DestBlend		= D3D11_Convert_BlendMode( input.color_channel.destination_factor );
	output.RenderTarget[0].BlendOp			= D3D11_Convert_BlendOp( input.color_channel.operation );
	output.RenderTarget[0].SrcBlendAlpha	= D3D11_Convert_BlendMode( input.alpha_channel.source_factor );
	output.RenderTarget[0].DestBlendAlpha	= D3D11_Convert_BlendMode( input.alpha_channel.destination_factor );
	output.RenderTarget[0].BlendOpAlpha		= D3D11_Convert_BlendOp( input.alpha_channel.operation );
	output.RenderTarget[0].RenderTargetWriteMask	= D3D11_Convert_ColorWriteMask( input.write_mask );
}

template< class OBJECT, class DESCRIPTION >
U32 FindObjectIndexByHash( THandleManager< OBJECT >& objects, const DESCRIPTION& info, U32 hash, const char* debugName )
{
	if( !objects.NumValidItems() ) {
		return -1;
	}
	THandleManager< OBJECT >::Iterator	it( objects );
	while( it.IsValid() )
	{
		OBJECT& o = it.Value();
		if( o.m_checksum == hash ) {
			ptWARN("Found duplicate object of dataType '%s' (name: '%s')",
				typeid(OBJECT).name(), debugName ? debugName : "<?>" );
			ptBREAK;//!< remove this
			return objects.GetContainedItemIndex( &o );
		}
		it.MoveToNext();
	}
	return -1;
}

//---------------------------------------------------------------------------

	BackendD3D11::BackendD3D11( AllocatorI & allocator )
		: _allocator( allocator )
	{
		shutdown_was_called = false;
	}

	BackendD3D11::~BackendD3D11()
	{
	}

	ERet BackendD3D11::Initialize(
		const CreationInfo& settings
		, Capabilities &caps_
		, PlatformData &platform_data_
		)
	{

#if D3D_USE_DYNAMIC_LIB

		// Dynamically load the D3D11 DLLs loaded and map the function pointers

		//-------------------------------------------------
		// Load D3D11.dll

		// This may fail if Direct3D 11 isn't installed
		DLL_Holder	d3d11_DLL( ::LoadLibraryA("d3d11.dll") );
		mxENSURE(d3d11_DLL, ERR_FAILED_TO_LOAD_LIBRARY, "Failed to load d3d11.dll.");

		D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)::GetProcAddress(d3d11_DLL, "D3D11CreateDevice");
		mxENSURE(D3D11CreateDevice, ERR_OBJECT_NOT_FOUND, "Function D3D11CreateDevice() not found.");

		//DLL_Holder	amd_ags_DLL( ::LoadLibraryA( (mxARCH_TYPE == mxARCH_32BIT) ? "amd_ags_x86.dll" : "amd_ags_x64.dll" ) );

		//-------------------------------------------------
		// Load D3DX11.dll


#if Use_D3DCompile_instead_of_D3DX11CompileFromMemory

		// first try to load D3DX11CompileFromMemory from DirectX 2010 June
		DLL_Holder	d3dCompiler_DLL( ::LoadLibraryA( "D3DCompiler_47.dll" ) );
		if( d3dCompiler_DLL ) {
			gs_D3DCompile = (PFN_D3DCompile)::GetProcAddress( d3dCompiler_DLL, "D3DCompile" );
		} else {
			ptWARN("Couldn't load D3dcompiler_47.dll !");
			// if absent try to take it from DirectX 2010 June
			d3dCompiler_DLL.ptr = ::LoadLibraryA( "D3dcompiler_43.dll" );
			if( d3dCompiler_DLL ) {
				gs_D3DCompile = (PFN_D3DCompile)::GetProcAddress( d3dCompiler_DLL, "D3DCompile" );
			}
		}

		mxENSURE( gs_D3DCompile, ERR_OBJECT_NOT_FOUND, "Function D3DCompile() not found." );

#else

		// first try to load D3DX11CompileFromMemory from DirectX 2010 June
		DLL_Holder	d3dx11_DLL( ::LoadLibraryA("D3DX11_43.dll") );
		if( d3dx11_DLL ) {
			gs_D3DX11CompileFromMemory = (PFN_D3DX11CompileFromMemory)::GetProcAddress(d3dx11_DLL, "D3DX11CompileFromMemory");
		} else {
			// if absent try to take it from DirectX 2010 Feb
			d3dx11_DLL.ptr = ::LoadLibraryA("D3DX11_42.dll");
			if( d3dx11_DLL ) {
				gs_D3DX11CompileFromMemory = (PFN_D3DX11CompileFromMemory)::GetProcAddress(d3dx11_DLL, "D3DX11CompileFromMemory");
			}
		}
		mxENSURE(gs_D3DX11CompileFromMemory, ERR_OBJECT_NOT_FOUND, "Function D3DX11CompileFromMemory() not found.");

#endif


		//-------------------------------------------------
		// Load D3D9.dll

		// D3D9.dll is needed for PIX support
		DLL_Holder	d3d9_DLL( ::LoadLibraryA("d3d9.dll") );
		if( d3d9_DLL )
		{
			D3DPERF_SetMarker  = (PFN_D3DPERF_SET_MARKER )::GetProcAddress(d3d9_DLL.ptr, "D3DPERF_SetMarker" );
			D3DPERF_BeginEvent = (PFN_D3DPERF_BEGIN_EVENT)::GetProcAddress(d3d9_DLL.ptr, "D3DPERF_BeginEvent");
			D3DPERF_EndEvent   = (PFN_D3DPERF_END_EVENT  )::GetProcAddress(d3d9_DLL.ptr, "D3DPERF_EndEvent"  );
			mxENSURE(NULL != D3DPERF_SetMarker
				&& NULL != D3DPERF_BeginEvent
				&& NULL != D3DPERF_EndEvent
				, ERR_OBJECT_NOT_FOUND
				, "Failed to Initialize PIX events."
				);
		}

		//-------------------------------------------------
		// Load DXGI.dll

		DLL_Holder	dxgi_DLL( ::LoadLibraryA("dxgi.dll") );
		mxENSURE(dxgi_DLL, ERR_FAILED_TO_LOAD_LIBRARY, "Failed to load dxgi.dll.");

		CreateDXGIFactory = (PFN_CREATE_DXGI_FACTORY)::GetProcAddress(dxgi_DLL, "CreateDXGIFactory1");
		if (NULL == CreateDXGIFactory) {
			CreateDXGIFactory = (PFN_CREATE_DXGI_FACTORY)::GetProcAddress(dxgi_DLL, "CreateDXGIFactory");
		}
		mxENSURE(CreateDXGIFactory, ERR_OBJECT_NOT_FOUND, "Function CreateDXGIFactory() not found.");

		//-------------------------------------------------
		// Load dxgidebug.dll

		DLL_Holder	dxgidebug_DLL = ::LoadLibraryA("dxgidebug.dll");
		if (NULL != dxgidebug_DLL)
		{
			DXGIGetDebugInterface  = (PFN_GET_DEBUG_INTERFACE )::GetProcAddress(dxgidebug_DLL, "DXGIGetDebugInterface");
			DXGIGetDebugInterface1 = (PFN_GET_DEBUG_INTERFACE1)::GetProcAddress(dxgidebug_DLL, "DXGIGetDebugInterface1");
			if (NULL == DXGIGetDebugInterface && NULL == DXGIGetDebugInterface1) {
				dxgidebug_DLL.Release();
			} else {
				// Figure out how to access IDXGIInfoQueue on pre Win8...
			}
		}

		m_d3d11_DLL		.ptr = d3d11_DLL;			d3d11_DLL		.Disown();
	#if !Use_D3DCompile_instead_of_D3DX11CompileFromMemory
		m_d3dx11_DLL	.ptr = d3dx11_DLL;			d3dx11_DLL		.Disown();
	#endif
		m_d3d9_DLL		.ptr = d3d9_DLL;			d3d9_DLL		.Disown();
		m_dxgi_DLL		.ptr = dxgi_DLL;			dxgi_DLL		.Disown();
		m_dxgidebug_DLL	.ptr = dxgidebug_DLL;		dxgidebug_DLL	.Disown();

#endif // D3D_USE_DYNAMIC_LIB



		DeviceVendor::Enum		deviceVendor;
		mxDO(_SetupDXGI(
			&_dxgi_factory._ptr,
			&_dxgi_adapter._ptr,
			&_dxgi_output._ptr,
			deviceVendor
			));

		//
		ComPtr< ID3D11Device >			device;
		ComPtr< ID3D11DeviceContext >	deviceContext;

		mxDO(_createDirect3DDevice( settings, _dxgi_adapter, device, deviceContext ));
		mxDO(_checkFeatureLevel( device ));

		_GetDeviceCaps( device, caps_ );

		//
		HWND hWnd = (HWND) settings.window_handle;

		DXGI_SWAP_CHAIN_DESC	swapChainDesc;
		_getSwapChainDescription( swapChainDesc
			, hWnd
			, !settings.device.use_fullscreen_mode
			, this->_d3d_device
			);

		//
		ComPtr< IDXGISwapChain >	swap_chain;
		mxDO(_createDirect3DSwapChain( swapChainDesc, _dxgi_factory, device, swap_chain ));

		this->_d3d_device	= device;
		this->_dxgi_swap_chain	= swap_chain;

		device->GetImmediateContext( &_d3d_device_context._ptr );

		mxZERO_OUT(this->resolution);

		//
		DXGI_SWAP_CHAIN_DESC	swapChainInfo;
		this->_dxgi_swap_chain->GetDesc( &swapChainInfo );
		mxDO(this->createBackBuffer( swapChainInfo, settings.framebuffer_handle ));

//		profiler.Startup(device);

		mxDO(constant_buffer_pool.Initialize( settings.uniform_buffer_pool_handles, this ));

		framesRendered = 0;

		//
		platform_data_.d3d11.device = device;
		platform_data_.d3d11.context = _d3d_device_context;

		return ALL_OK;
	}

	void BackendD3D11::Shutdown()
	{
		mxASSERT(!shutdown_was_called);
		shutdown_was_called = true;

		//
		constant_buffer_pool.Shutdown();

//		profiler.Shutdown();

		// This sets all input/_dxgi_output resource slots, shaders, input layouts, predications, scissor rectangles,
		// depth-stencil state, rasterizer state, blend state, sampler state, and viewports to NULL.
		// The primitive topology is set to UNDEFINED.
		_d3d_device_context->ClearState();

		this->releaseBackBuffer();

		DBGOUT("D3D11: checking live objects...");

#if 0
		TDestroyLiveObjects( this->programs );
		TDestroyLiveObjects( this->shaders );
		TDestroyLiveObjects( this->_buffers );
		TDestroyLiveObjects( this->inputLayouts );
		TDestroyLiveObjects( this->textures );
		TDestroyLiveObjects( this->color_targets );
		TDestroyLiveObjects( this->depthTargets );
		TDestroyLiveObjects( this->_buffers );
		TDestroyLiveObjects( this->blendStates );
		TDestroyLiveObjects( this->samplerStates );
		TDestroyLiveObjects( this->rasterizerStates );
		TDestroyLiveObjects( this->depthStencilStates );
#endif

		for( UINT i = 0; i < mxCOUNT_OF(depthStencilStates); i++ ) {
			depthStencilStates[i].Destroy();
		}
		for( UINT i = 0; i < mxCOUNT_OF(rasterizerStates); i++ ) {
			rasterizerStates[i].Destroy();
		}
		for( UINT i = 0; i < mxCOUNT_OF(samplerStates); i++ ) {
			samplerStates[i].Destroy();
		}
		for( UINT i = 0; i < mxCOUNT_OF(blendStates); i++ ) {
			blendStates[i].Destroy();
		}

		for( UINT i = 0; i < mxCOUNT_OF(color_targets); i++ ) {
			color_targets[i].Destroy();
		}
		for( UINT i = 0; i < mxCOUNT_OF(depthTargets); i++ ) {
			depthTargets[i].Destroy();
		}

		for( UINT i = 0; i < mxCOUNT_OF(inputLayouts); i++ ) {
			inputLayouts[i].Destroy();
		}

		for( UINT i = 0; i < mxCOUNT_OF(_buffers); i++ ) {
			_buffers[i].Destroy();
		}
		for( UINT i = 0; i < mxCOUNT_OF(textures); i++ ) {
			textures[i].Destroy();
		}

		for( UINT i = 0; i < mxCOUNT_OF(shaders); i++ ) {
			shaders[i].destroy( _allocator );
		}
		for( UINT i = 0; i < mxCOUNT_OF(programs); i++ ) {
			programs[i].destroy();
		}

		//
		mxZERO_OUT(this->resolution);

		//
		SAFE_RELEASE(_dxgi_swap_chain);
		SAFE_RELEASE(_d3d_device_context);
		SAFE_RELEASE(_d3d_device);
		

#if D3D_USE_DYNAMIC_LIB
		m_d3d9_DLL		.Release();
		m_d3dx11_DLL	.Release();
		m_d3d11_DLL		.Release();	
#endif // D3D_USE_DYNAMIC_LIB

		_dxgi_output.Release();
		_dxgi_adapter.Release();
		_dxgi_factory.Release();

#if D3D_USE_DYNAMIC_LIB
		m_dxgidebug_DLL	.Release();
		m_dxgi_DLL		.Release();
#endif // D3D_USE_DYNAMIC_LIB



		// Print DXGI leaks.

#if DEBUG_D3D11_LEAKS
		//
		ComPtr<ID3D11Debug>	d3d_debug;
		if(SUCCEEDED(DXGIGetDebugInterface(
			__uuidof(IDXGIDebug), (void**) &d3d_debug._ptr
			)))
		{
			d3d_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		}
		else
		{
			UNDONE;
		}
#endif // MX_DEVELOPER

	}

	bool BackendD3D11::isDeviceRemoved() const
	{
		mxUNDONE
		return false;
	}

#pragma region "Render Targets"

	void BackendD3D11::createColorTarget(
		const unsigned index
		, const NwColorTargetDescription& description
		, const char* debug_name
		)
	{
		ColorTargetD3D11 &	newRT = color_targets[ index ];
		newRT.Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::deleteColorTarget( const unsigned index )
	{
		ColorTargetD3D11 &	renderTarget = color_targets[ index ];
		renderTarget.Destroy();
	}

	void BackendD3D11::createDepthTarget(
		const unsigned index
		, const NwDepthTargetDescription& description
		, const char* debug_name
		)
	{
		DepthTargetD3D11 &	newDT = depthTargets[ index ];
		newDT.Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::deleteDepthTarget( const unsigned index )
	{
		DepthTargetD3D11 &	depth_target = depthTargets[ index ];
		depth_target.Destroy();
	}

#pragma endregion

#pragma region "Textures"

	void BackendD3D11::createTexture(
		const unsigned index
		, const Memory* texture_data
		, const GrTextureCreationFlagsT flags
		, const char* debug_name
		)
	{
		TextureD3D11 &	newTexture = textures[ index ];
		newTexture.CreateFromMemory(
			texture_data->data
			, texture_data->size
			, _d3d_device
			, flags
			, debug_name
			);
	}

	void BackendD3D11::createTexture2D(
		const unsigned index
		, const NwTexture2DDescription& description
		, const Memory* initial_data
		, const char* debug_name
		)
	{
		TextureD3D11 &	newTexture2D = textures[ index ];
		newTexture2D.Create( description
			, _d3d_device
			, initial_data ? initial_data->data : nil
			, debug_name
			);
	}

	void BackendD3D11::createTexture3D(
		const unsigned index
		, const NwTexture3DDescription& description
		, const Memory* initial_data
		, const char* debug_name
		)
	{
		textures[ index ].Create( description
			, _d3d_device
			, initial_data ? initial_data->data : nil
			, debug_name
			);
	}

	void BackendD3D11::deleteTexture( const unsigned index )
	{
		TextureD3D11 &	texture = textures[ index ];
		texture.Destroy();
	}

#pragma endregion

#pragma region "Render States"

	void BackendD3D11::createDepthStencilState(
		const unsigned index
		, const NwDepthStencilDescription& description
		, const char* debug_name
		)
	{
		depthStencilStates[ index ].Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::createRasterizerState(
		const unsigned index
		, const NwRasterizerDescription& description
		, const char* debug_name
		)
	{
		rasterizerStates[ index ].Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::createSamplerState(
		const unsigned index
		, const NwSamplerDescription& description
		, const char* debug_name
		)
	{
		samplerStates[ index ].Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::createBlendState(
		const unsigned index
		, const NwBlendDescription& description
		, const char* debug_name
		)
	{
		blendStates[ index ].Create( description, _d3d_device, debug_name );
	}

	void BackendD3D11::deleteDepthStencilState( const unsigned index )
	{
		depthStencilStates[ index ].Destroy();
	}
	
	void BackendD3D11::deleteRasterizerState( const unsigned index )
	{	
		rasterizerStates[ index ].Destroy();
	}
	
	void BackendD3D11::deleteSamplerState( const unsigned index )
	{
		samplerStates[ index ].Destroy();
	}

	void BackendD3D11::deleteBlendState( const unsigned index )
	{
		blendStates[ index ].Destroy();
	}

#pragma endregion

#pragma region "Input Layouts"

	void BackendD3D11::createInputLayout(
		const unsigned index
		, const NwVertexDescription& description
		, const char* debug_name
		)
	{

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

		TCopyBase( _vertex_descriptions[ index ], description );

#else

		//const char* name = description.name.IsEmpty() ? nil : description.name.c_str();
		//const U32 hash = MurmurHash32( &description, sizeof(description) );
		//for( int i = 0; i < inputLayouts.num(); i++ )
		//{
		//	InputLayoutD3D11& layout = inputLayouts[ i ];
		//	if( layout.m_checksum == hash ) {
		//		DBGOUT("Returning existing input layout '%s'", name ? name : "<?>");
		//		layout.m_refCount++;
		//		const HInputLayout handle = { i };
		//		return handle;
		//	}
		//}
//#if LLGL_VERBOSE
//		DBGOUT("Creating input layout '%s'", name ? name : "<?>");
//#endif
		InputLayoutD3D11 &	layout = inputLayouts[ index ];
		layout.Create( description, _d3d_device );

#endif // !
		
	}

	void BackendD3D11::deleteInputLayout( const unsigned index )
	{
		InputLayoutD3D11 &	layout = inputLayouts[ index ];
		//layout.m_refCount--;
		//if( 0 == layout.m_refCount )
		//{
			layout.Destroy();
		//	inputLayouts.Free( handle.id );
		//}
	}
#pragma endregion

#pragma region "Shaders and Programs"

	void BackendD3D11::createShader( const unsigned index, const Memory* shader_binary )
	{
		const ShaderHeader_d* header = (ShaderHeader_d* ) shader_binary->data;
		const NwShaderType::Enum shaderType = (NwShaderType::Enum) header->type;
		//
		const void* compiled_bytecode_data = header + 1;
		const U32 compiled_bytecode_size = shader_binary->size - sizeof(*header);
		//
		shaders[ index ].create( *header, compiled_bytecode_data, compiled_bytecode_size, _d3d_device, _allocator );
	}

	void BackendD3D11::deleteShader( const unsigned index )
	{
		shaders[ index ].destroy( _allocator );
	}

	void BackendD3D11::createProgram( const unsigned index, const NwProgramDescription& description )
	{
		ProgramD3D11 &program = programs[ index ];
		program.VS = description.shaders[NwShaderType::Vertex];
		program.PS = description.shaders[NwShaderType::Pixel];
		program.HS = description.shaders[NwShaderType::Hull];
		program.DS = description.shaders[NwShaderType::Domain];
		program.GS = description.shaders[NwShaderType::Geometry];
		program.CS = description.shaders[NwShaderType::Compute];
	}

	void BackendD3D11::deleteProgram( const unsigned index )
	{
		programs[ index ].destroy();
	}

#pragma endregion

	void BackendD3D11::updateBuffer( const unsigned index, const void* data, const unsigned size )
	{
		mxASSERT(index < mxCOUNT_OF(_buffers));
		mxASSERT_PTR(data);
		mxASSERT(size > 0);

		//
		BufferD3D11& bufferD3D = _buffers[ index ];
		mxASSERT_PTR( bufferD3D.m_ptr );
		mxASSERT( size <= bufferD3D.m_size );

		const U32 bytes_to_copy = smallest( bufferD3D.m_size, size );
#if 1

		D3D11_MAPPED_SUBRESOURCE	mappedData;
		if(SUCCEEDED(_d3d_device_context->Map( bufferD3D.m_ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData )))
		{
			memcpy( mappedData.pData, data, bytes_to_copy );
			_d3d_device_context->Unmap( bufferD3D.m_ptr, 0 );
		}

#else

		_d3d_device_context->UpdateSubresource(
			bufferD3D.m_ptr,	// ID3D11Resource *pDstResource
			0,		// UINT DstSubresource
			nil,	// const D3D11_BOX *pDstBox
			data,	// const void *pSrcData
			0,		// UINT SrcRowPitch
			0		// UINT SrcDepthPitch
			);
#endif

	}

	void BackendD3D11::UpdateTexture( const unsigned index, const NwTextureRegion& region, const Memory* new_contents )
	{
		const TextureD3D11& texture = textures[ index ];

		const bool is_2D_texture = ( texture.m_depth == 1 );

		//if( texture._is_dynamic )
		//{
		//	//
		//}
		//else
		{
			mxASSERT2( 0 == TEST_BIT( texture._flags, NwTextureCreationFlags::DYNAMIC ), "UpdateSubresource() won't work with Dynamic textures!" );

			// Calculate a subresource index for a texture: the index which equals MipSlice + (ArraySlice * MipLevels).
			// For volume (3D) textures, all slices for a given mipmap level are a single subresource index.
			const UINT iDestSubresource = D3D11CalcSubresource(
				region.mipMapLevel,	// A zero-based index for the mipmap level to address; 0 indicates the first, most detailed mipmap level.
				region.arraySlice,	// The zero-based index for the array level to address; always use 0 for volume (3D) textures.
				texture.m_numMips	// Number of mipmap levels in the resource.
				);

			//
			D3D11_BOX	dstBox;
			dstBox.left   = region.x;
			dstBox.top    = region.y;
			dstBox.right  = region.x + region.width;
			dstBox.bottom = region.y + region.height;
			dstBox.front  = region.z;	// The z position of the front of the box.
			dstBox.back   = region.z + region.depth;	// The z position of the back of the box.

			//
			U32 levelSize = 0;	// Determine the size of this mipmap level, in bytes.
			U32 rowCount = 0;	// Pictures are encoded in blocks (1x1 or 4x4) and this is the height of the block matrix.
			U32 rowPitch = 0;	// And find out the size of a row of blocks, in bytes.
			GetSurfaceInfo( texture.m_format, region.width, region.height, levelSize, rowCount, rowPitch );
			mxUNUSED(rowCount);

			_d3d_device_context->UpdateSubresource(
				texture.m_resource,	// pDstResource
				iDestSubresource,	// DstSubresource
				is_2D_texture ? nil : &dstBox,	// pDstBox - The portion of the destination subresource to copy the resource data into. 
				new_contents->data,	// pSrcData
				rowPitch,		// SrcRowPitch - The size of one row of the source data.
				is_2D_texture ? 0 : levelSize	// SrcDepthPitch - The size of one depth slice of source data.
				);
		}
	}

	template< class COMMAND_TYPE >
	static inline bool memoryWasAllocatedAfterCommand( const COMMAND_TYPE* cmd, const Memory* mem )
	{
		return (void*) mem == (void*) ( cmd + 1 );
	}

	ERet BackendD3D11::releaseBackBuffer()
	{
		//unresolved external symbol:
		//dxCHK(D3DX11UnsetAllDeviceObjects(backend->deviceContext));

		return ALL_OK;
	}

	ERet BackendD3D11::createBackBuffer( const DXGI_SWAP_CHAIN_DESC& scd, HColorTarget render_target_handle )
	{
		// Initialize back buffer. Grab back buffer texture and create main depth-stencil texture.
		// The application can re-query interfaces after calling ResizeBuffers() via IDXGISwapChain::GetBuffer().

		// Get a pointer to the back buffer.
		ID3D11Texture2D *	back_buffer_texture = nil;
		dxTRY(this->_dxgi_swap_chain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**) &back_buffer_texture ));
		//dxSetDebugName(colorTexture, "Main depth-stencil");

		// create a render-target view for accessing the back buffer.
		ID3D11RenderTargetView *	render_target_view = nil;
		dxTRY(this->_d3d_device->CreateRenderTargetView( back_buffer_texture, nil, &render_target_view ));

		this->resolution.width = scd.BufferDesc.Width;
		this->resolution.height = scd.BufferDesc.Height;

		//
		ColorTargetD3D11 &	newRT = color_targets[ render_target_handle.id ];
		newRT.m_colorTexture = back_buffer_texture;
		newRT.m_RTV = render_target_view;
		newRT.m_SRV = nil;
		newRT.m_width = scd.BufferDesc.Width;
		newRT.m_height = scd.BufferDesc.Height;
		newRT.m_format = scd.BufferDesc.Format;
		newRT.m_flags = 0;

		return ALL_OK;
	}

	ColorTargetD3D11::ColorTargetD3D11()
	{
		m_colorTexture = nil;
		m_RTV = nil;
		m_SRV = nil;
		m_UAV = nil;
		m_width = 0;
		m_height = 0;
		m_format = DXGI_FORMAT_UNKNOWN;
		m_flags = 0;
	}

	void ColorTargetD3D11::Create(
		const NwColorTargetDescription& render_target_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		const TextureFormatInfoD3D11& format = gs_textureFormats[ render_target_desc.format ];
		const DXGI_FORMAT nativeFormat = format.tex;

		const bool bUAV = (render_target_desc.flags & NwTextureCreationFlags::RANDOM_WRITES);
		const bool bGenerateMips = (render_target_desc.flags & NwTextureCreationFlags::GENERATE_MIPS) && (render_target_desc.numMips > 1);

		D3D11_TEXTURE2D_DESC	texDesc;
		texDesc.Width				= render_target_desc.width;
		texDesc.Height				= render_target_desc.height;
		texDesc.MipLevels			= render_target_desc.numMips;
		texDesc.ArraySize			= 1;
		texDesc.Format				= nativeFormat;
		texDesc.SampleDesc.Count	= 1;
		texDesc.SampleDesc.Quality	= 0;
		texDesc.Usage				= D3D11_USAGE_DEFAULT;
		texDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
		if( bUAV ) {
			texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		texDesc.CPUAccessFlags		= 0;
		texDesc.MiscFlags			= bGenerateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		dxCHK(device->CreateTexture2D( &texDesc, nil, &m_colorTexture ));
		dxCHK(device->CreateRenderTargetView( m_colorTexture, nil, &m_RTV ));
		dxCHK(device->CreateShaderResourceView( m_colorTexture, nil, &m_SRV ));
		if( bUAV ) {
			dxCHK(device->CreateUnorderedAccessView( m_colorTexture, nil, &m_UAV ));
		}

		dxSetDebugName(m_colorTexture, debug_name);

		m_width = render_target_desc.width;
		m_height = render_target_desc.height;
		m_format = nativeFormat;
		m_flags = 0;
	}

	void ColorTargetD3D11::Destroy()
	{

#if MX_DEVELOPER
		if( m_colorTexture != nil ) {
			DBGOUT("Destroying render target %ux%u", m_width, m_height );
		}
#endif // MX_DEVELOPER

		SAFE_RELEASE(m_colorTexture);
		SAFE_RELEASE(m_RTV);
		SAFE_RELEASE(m_SRV);
		m_width = 0;
		m_height = 0;
		m_format = DXGI_FORMAT_UNKNOWN;
		m_flags = 0;
	}

	DepthTargetD3D11::DepthTargetD3D11()
	{
		m_depthTexture = nil;
		m_DSV = nil;
		m_SRV = nil;
		m_DSV_ReadOnly = nil;

		m_width = 0;
		m_height = 0;
		m_flags = 0;
	}

	void DepthTargetD3D11::Create(
		const NwDepthTargetDescription& depth_target_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		const TextureFormatInfoD3D11& format = gs_textureFormats[ depth_target_desc.format ];
		const DXGI_FORMAT depthStencilFormat = format.tex;
		const DXGI_FORMAT depthStencilViewFormat = format.dsv;
		// NOTE: the backing resource must be created as a typeless surface
		// so that it can be accessed in shaders via a shader resource view
		const DXGI_FORMAT shaderResourceFormat = format.srv;

		UINT sampleCount = 1;

		UINT textureBindFlags = D3D11_BIND_DEPTH_STENCIL;
		if( depth_target_desc.sample ) {
			textureBindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		//
		D3D11_TEXTURE2D_DESC	texDesc;
		texDesc.Width				= depth_target_desc.width;
		texDesc.Height				= depth_target_desc.height;
		texDesc.MipLevels			= 1;
		texDesc.ArraySize			= 1;
		texDesc.Format				= depthStencilFormat;
		texDesc.SampleDesc.Count	= sampleCount;
		texDesc.SampleDesc.Quality	= 0;
		texDesc.Usage				= D3D11_USAGE_DEFAULT;
		texDesc.BindFlags			= textureBindFlags;
		texDesc.CPUAccessFlags		= 0;
		texDesc.MiscFlags			= 0;
		dxCHK(device->CreateTexture2D( &texDesc, nil, &m_depthTexture ));

		//
		D3D11_DEPTH_STENCIL_VIEW_DESC	dsvDesc;
		dsvDesc.Format				= depthStencilViewFormat;
		dsvDesc.ViewDimension		= (sampleCount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags				= 0;
		dsvDesc.Texture2D.MipSlice	= 0;

		dxCHK(device->CreateDepthStencilView( m_depthTexture, &dsvDesc, &m_DSV ));

		//
		if( depth_target_desc.sample )
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			{
				srvDesc.Format = shaderResourceFormat;
				srvDesc.ViewDimension = (sampleCount > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
			}
			dxCHK(device->CreateShaderResourceView( m_depthTexture, &srvDesc, &m_SRV ));


			// Create a read-only DSV.

			// NB: this flag can only be used with feature level D3D11 and above
			dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;

			if( DXGI_FORMAT_D24_UNORM_S8_UINT == depthStencilViewFormat
				|| DXGI_FORMAT_D32_FLOAT_S8X24_UINT == depthStencilViewFormat
				)
			{
				dsvDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
			}

			dxCHK(device->CreateDepthStencilView( m_depthTexture, &dsvDesc, &m_DSV_ReadOnly ));
		}

		dxSetDebugName(m_depthTexture, debug_name);
		dxSetDebugName(m_DSV, debug_name);
		dxSetDebugName(m_SRV, debug_name);
		dxSetDebugName(m_DSV_ReadOnly, debug_name);

		m_width = depth_target_desc.width;
		m_height = depth_target_desc.height;
		m_flags = 0;
	}

	void DepthTargetD3D11::Destroy()
	{
#if LLGL_VERBOSE
		DBGOUT("Destroying depth target %ux%u", m_width, m_height );
#endif
		SAFE_RELEASE(m_depthTexture);
		SAFE_RELEASE(m_DSV);
		SAFE_RELEASE(m_SRV);
		SAFE_RELEASE(m_DSV_ReadOnly);

		m_width = 0;
		m_height = 0;
		m_flags = 0;
	}

	DepthStencilStateD3D11::DepthStencilStateD3D11()
	{
		m_ptr = nil;
	}

	void DepthStencilStateD3D11::Create(
		const NwDepthStencilDescription& depth_stencil_state_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		D3D11_Convert_DepthStencil_Description( depth_stencil_state_desc, depth_stencil_desc );
		dxCHK(device->CreateDepthStencilState( &depth_stencil_desc, &m_ptr ));
		dxSetDebugName(m_ptr, debug_name);
	}

	void DepthStencilStateD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
	}

	RasterizerStateD3D11::RasterizerStateD3D11()
	{
		m_ptr = nil;
	}

	void RasterizerStateD3D11::Create(
		const NwRasterizerDescription& rasterizer_state_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		D3D11_Convert_Rasterizer_Description( rasterizer_state_desc, rasterizerDesc );
		dxCHK(device->CreateRasterizerState( &rasterizerDesc, &m_ptr ));
		dxSetDebugName(m_ptr, debug_name);
	}

	void RasterizerStateD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
	}

	SamplerStateD3D11::SamplerStateD3D11()
	{
		m_ptr = nil;
	}

	void SamplerStateD3D11::Create(
		const NwSamplerDescription& sampler_state_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		D3D11_SAMPLER_DESC samplerDesc;
		D3D11_Convert_Sampler_Description( sampler_state_desc, samplerDesc );

		dxCHK(device->CreateSamplerState( &samplerDesc, &m_ptr ));
	}

	void SamplerStateD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
	}

	BlendStateD3D11::BlendStateD3D11()
	{
		m_ptr = nil;
	}

	void BlendStateD3D11::Create(
		const NwBlendDescription& blend_state_desc
		, ID3D11Device * device
		, const char* debug_name
		)
	{
		D3D11_BLEND_DESC blendDesc;
		D3D11_Convert_Blend_Description( blend_state_desc, blendDesc );
		dxCHK(device->CreateBlendState( &blendDesc, &m_ptr ));
		dxSetDebugName(m_ptr, debug_name);
	}
	void BlendStateD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
	}

	InputLayoutD3D11::InputLayoutD3D11()
	{
		m_ptr = nil;
		m_numSlots = 0;
		mxZERO_OUT(m_strides);
	}

	static const char* gs_semanticName[NwAttributeUsage::Count] = {
		"Position",
		"Color",
		"Normal", "Tangent", "Bitangent",
		"TexCoord",
		"BoneWeights", "BoneIndices",
	};
	// [base_type] [normalized_type]
	static const DXGI_FORMAT gs_attribType[NwAttributeType::Count][2] = {
		// Signed Byte4
		{ DXGI_FORMAT_R8G8B8A8_SINT,      DXGI_FORMAT_R8G8B8A8_SNORM     },
		// Unsigned Byte4
		{ DXGI_FORMAT_R8G8B8A8_UINT,      DXGI_FORMAT_R8G8B8A8_UNORM     },
		// Signed Short2
		{ DXGI_FORMAT_R16G16B16A16_SINT,  DXGI_FORMAT_R16G16B16A16_SNORM },
		// Unsigned Short2
		{ DXGI_FORMAT_R16G16B16A16_UINT,  DXGI_FORMAT_R16G16B16A16_UNORM },

		// Half2
		{ DXGI_FORMAT_R16G16_FLOAT,       DXGI_FORMAT_R16G16_FLOAT       },
		// Half4
		{ DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_R16G16B16A16_FLOAT  },

		// Float1
		{ DXGI_FORMAT_R32_FLOAT,          DXGI_FORMAT_R32_FLOAT          },
		// Float2
		{ DXGI_FORMAT_R32G32_FLOAT,       DXGI_FORMAT_R32G32_FLOAT       },
		// Float3
		{ DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_R32G32B32_FLOAT    },
		// Float4
		{ DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT },

		// Unsigned Int1
		{ DXGI_FORMAT_R32_UINT,           DXGI_FORMAT_R32_UINT          },
		// Unsigned Int2
		{ DXGI_FORMAT_R32G32_UINT,        DXGI_FORMAT_R32G32_UINT       },
		// Unsigned Int3
		{ DXGI_FORMAT_R32G32B32A32_UINT,  DXGI_FORMAT_R32G32B32A32_UINT },
		// Unsigned Int4
		{ DXGI_FORMAT_R32G32B32A32_UINT,  DXGI_FORMAT_R32G32B32A32_UINT },

		// R11G11B10F
		{ DXGI_FORMAT_R11G11B10_FLOAT,    DXGI_FORMAT_R11G11B10_FLOAT   },
		// R10G10B10A2_UNORM
		{ DXGI_FORMAT_R10G10B10A2_UNORM,  DXGI_FORMAT_R10G10B10A2_UNORM   },
	};
	// [attrib_type] [is_normalized?]
	static const char* gs_nameInShader[NwAttributeType::Count][2] = {
		{ "int4",   "float4"  },
		{ "uint4",  "float4"  },
		{ "int2",   "float2"  },
		{ "uint2",  "float2"  },

		{ "half2",  "float2"  },
		{ "half4",  "float4"  },
		
		{ "float",  "float"   },
		{ "float2", "float2"  },
		{ "float3", "float3"  },
		{ "float4", "float4"  },

		{ "uint",   "float"   },	// UInt1
		{ "uint2",  "float2"  },	// UInt2
		{ "uint3",  "float3"  },	// UInt3
		{ "uint4",  "float4"  },	// UInt4

		{ "float3",  "float3" },	// R11G11B10F
		{ "float4",  "float4" },	// R10G10B10A2_UNORM
	};

	namespace
	{
		static UINT convertToD3D11InputElements( const NwVertexDescription& vertex_description, D3D11_INPUT_ELEMENT_DESC *d3d11_element_descriptions_ )
		{
			UINT	numInputSlots = 1;

			for( int i = 0; i < vertex_description.attribCount; i++ )
			{
				const NwVertexElement& element = vertex_description.attribsArray[ i ];

				numInputSlots = largest(numInputSlots, element.inputSlot+1);

				D3D11_INPUT_ELEMENT_DESC &	d3d11_elem_desc = d3d11_element_descriptions_[i];

				d3d11_elem_desc.SemanticName			= gs_semanticName[ element.usageType ];
				d3d11_elem_desc.SemanticIndex			= element.usageIndex;
				d3d11_elem_desc.Format					= gs_attribType[ element.dataType ][ element.normalized ];
				d3d11_elem_desc.InputSlot				= element.inputSlot;
				d3d11_elem_desc.AlignedByteOffset		= vertex_description.attribOffsets[ i ];
				d3d11_elem_desc.InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
				d3d11_elem_desc.InstanceDataStepRate	= 0;
			}

			return numInputSlots;
		}
	}//namespace

	void InputLayoutD3D11::Create(
		const NwVertexDescription& desc
		, ID3D11Device * device
		)
	{
		// Compile a dummy vertex shader and retrieve its input signature.
		typedef TFixedArray< ANSICHAR, 1024 >	CodeSnippet;

		D3D11_INPUT_ELEMENT_DESC	d3d11_element_descriptions[LLGL_MAX_VERTEX_ATTRIBS];

		const UINT	numInputSlots = convertToD3D11InputElements( desc, d3d11_element_descriptions );

		//
		CodeSnippet				dummyVertexShaderCode(_InitZero);
		CodeSnippet::OStream	stream = dummyVertexShaderCode.GetOStream();
		TextStream				tw( stream );
		{
			tw << "void F(\n";
			for( int i = 0; i < desc.attribCount; i++ )
			{
				const NwVertexElement& element = desc.attribsArray[ i ];
				const D3D11_INPUT_ELEMENT_DESC &	d3d11_elem_desc = d3d11_element_descriptions[i];

				char shaderSemantic[64];
				sprintf_s(shaderSemantic, "%s%u", d3d11_elem_desc.SemanticName, d3d11_elem_desc.SemanticIndex );

				char shaderTypeName[32];
				sprintf_s(shaderTypeName, "%s", gs_nameInShader[ element.dataType ][ element.normalized ] );

 				tw.PrintF("%s _%u : %s%s\n"
					,shaderTypeName
					,i
					,shaderSemantic
					, (i != desc.attribCount-1) ? "," : ""
				);
			}
			tw << "){}\n";
			;
		}

		DBGOUT("Input signature: %s", dummyVertexShaderCode.raw());

		for( int i = 0; i < desc.attribCount; i++ )
		{
			const D3D11_INPUT_ELEMENT_DESC& element = d3d11_element_descriptions[i];
			DBGOUT("%s[%u]: %s at %u in slot %u", element.SemanticName, element.SemanticIndex, DXGI_FORMAT_ToChars(element.Format), element.AlignedByteOffset, element.InputSlot);
		}

		const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;
		const char* vertexShaderProfile = "vs_5_0";

		ComPtr< ID3DBlob >	compiledCode;

#if Use_D3DCompile_instead_of_D3DX11CompileFromMemory

		dxCHK((*gs_D3DCompile)(
			dummyVertexShaderCode.raw(),
			dummyVertexShaderCode.num(),
			nil,	// Optional. The name of the shader file. Use either this or pSrcData.
			nil,	// const D3D_SHADER_MACRO* defines
			nil,	// ID3DInclude* includeHandler
			"F",	// const char* entryPoint,
			vertexShaderProfile,
			0,		// Shader compile options
			0,		// Effect compile options.
			&compiledCode._ptr,
			nil	// Optional. A pointer to an ID3D10Blob that contains compiler error messages, or nil if there were no errors.
		));

#else

		dxCHK(D3DX11CompileFromMemory(
			dummyVertexShaderCode.raw(),
			dummyVertexShaderCode.num(),
			nil,	// Optional. The name of the shader file. Use either this or pSrcData.
			nil,	// const D3D_SHADER_MACRO* defines
			nil,	// ID3DInclude* includeHandler
			"F",	// const char* entryPoint,
			vertexShaderProfile,
			0,		// Shader compile options
			0,		// Effect compile options.
			nil,	// ID3DX11ThreadPump* pPump
			&compiledCode._ptr,
			nil,	// Optional. A pointer to an ID3D10Blob that contains compiler error messages, or nil if there were no errors.
			nil		// HRESULT* pHResult
		));

#endif

		mxASSERT(compiledCode.IsValid());

		const void* bytecode = compiledCode->GetBufferPointer();
		size_t bytecodeLength = compiledCode->GetBufferSize();

		mxASSERT_PTR(bytecode);
		mxASSERT(bytecodeLength > 0);

		// create a new vertex declaration.
		dxCHK(device->CreateInputLayout(
			d3d11_element_descriptions,
			desc.attribCount,
			bytecode,
			bytecodeLength,
			&m_ptr
		));

		m_numSlots = numInputSlots;
		TCopyStaticArray( m_strides, desc.streamStrides, numInputSlots );

		dxSetDebugName( m_ptr, debug_name.c_str() );
	}

	void InputLayoutD3D11::Create(
		const NwVertexDescription& desc
		, const NGpu::Memory* vs_bytecode
		, ID3D11Device * device
		)
	{
		D3D11_INPUT_ELEMENT_DESC	d3d11_element_descriptions[ LLGL_MAX_VERTEX_ATTRIBS ];

		const UINT	numInputSlots = convertToD3D11InputElements( desc, d3d11_element_descriptions );

#if MX_DEBUG
		DBGOUT("[GFX] Creating input layout: %u attribs", desc.attribCount);
#endif

		// create a new vertex declaration.
		dxCHK(device->CreateInputLayout(
			d3d11_element_descriptions,
			desc.attribCount,
			vs_bytecode->data,
			vs_bytecode->size,
			&m_ptr
		));

		dxSetDebugName( m_ptr, debug_name.c_str() );

		m_numSlots = numInputSlots;
		TCopyStaticArray( m_strides, desc.streamStrides, numInputSlots );
	}

	void InputLayoutD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
		m_numSlots = 0;
		mxZERO_OUT(m_strides);
	}

	BufferD3D11::BufferD3D11()
	{
		m_ptr = nil;
		m_SRV = nil;
		m_UAV = nil;
		m_size = 0;
	}

	ERet BufferD3D11::Create(
		const NwBufferDescription& description
		, ID3D11Device * device
		, const void* initial_data
		, const char* debug_name
		)
	{
		const bool dynamic = (description.flags & NwBufferFlags::Dynamic) || (description.type == Buffer_Uniform);

		D3D11_BUFFER_DESC	bufferDesc;
		mxZERO_OUT(bufferDesc);

		bufferDesc.ByteWidth		= description.size;
		bufferDesc.BindFlags		= 0;

		if( description.flags & NwBufferFlags::Staging )
		{
			bufferDesc.Usage			= D3D11_USAGE_STAGING;
			bufferDesc.CPUAccessFlags	= D3D11_CPU_ACCESS_READ;
		}
		else
		{
			bufferDesc.Usage			= dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
			bufferDesc.CPUAccessFlags	= dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		}

		bufferDesc.MiscFlags		= 0;
		bufferDesc.StructureByteStride	= 0;

		switch( description.type )
		{
		case Buffer_Uniform:
			bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
			break;

		case Buffer_Vertex:
			bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
			break;

		case Buffer_Index:
			bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
			break;

		case Buffer_Data:
			bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = description.stride;
			if( description.flags & NwBufferFlags::Write ) {
				bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}
			if( description.flags & NwBufferFlags::Sample ) {
				bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			break;

		default:
			mxUNREACHABLE;
			return ERR_INVALID_PARAMETER;
		}

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = initial_data;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;

		dxCHK(device->CreateBuffer(
			&bufferDesc,
			initial_data ? &initData : nil,
			&m_ptr
		));

		mxASSERT_PTR(m_ptr);

		//
#if LLGL_DEBUG_LEVEL >= LLGL_DEBUG_LEVEL_NONE
		dxSetDebugName(m_ptr, debug_name);
#endif

		if( description.flags & NwBufferFlags::Sample )
		{
			dxCHK(device->CreateShaderResourceView( m_ptr, nil, &m_SRV ));
		}

		if( description.flags & NwBufferFlags::Write )
		{
			if( description.flags & NwBufferFlags::Append )
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
				uavDesc.Format				= DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement	= 0;
				uavDesc.Buffer.NumElements	= description.size / description.stride;
				uavDesc.Buffer.Flags		= D3D11_BUFFER_UAV_FLAG_APPEND;

				dxCHK(device->CreateUnorderedAccessView( m_ptr, &uavDesc, &m_UAV ));
			}
			else
			{
				dxCHK(device->CreateUnorderedAccessView( m_ptr, nil, &m_UAV ));
			}
		}

		m_size = description.size;
		//m_name = _d.name.SafeGetPtr();

		return ALL_OK;
	}

	void BufferD3D11::Destroy()
	{
		SAFE_RELEASE(m_ptr);
		SAFE_RELEASE(m_SRV);
		SAFE_RELEASE(m_UAV);
	}

	ERet BackendD3D11::createBuffer(
		const unsigned index
		, const NwBufferDescription& description
		, const Memory* initial_data
		, const char* debug_name
		)
	{
#if LLGL_VERBOSE
		DEVOUT("D3D11: Creating Buffer[%u] of size %u...", (U32)index, (U32)description.size);
#endif // LLGL_VERBOSE

		BufferD3D11& new_buffer = _buffers[ index ];

		mxASSERT( nil == new_buffer.m_ptr );

		mxDO(new_buffer.Create( description
			, _d3d_device
			, initial_data ? initial_data->data : nil
			, debug_name
			));

		return ALL_OK;
	}

	void BackendD3D11::deleteBuffer( const unsigned index )
	{
		_buffers[ index ].Destroy();
	}

	/*
	-----------------------------------------------------------------------------
		ShaderD3D11
	-----------------------------------------------------------------------------
	*/
	ShaderD3D11::ShaderD3D11()
	{
		m_ptr = nil;

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND
		vs_bytecode = nil;
#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND
	}

	ERet ShaderD3D11::create( const ShaderHeader_d& shaderHeader
		, const void* compiledBytecode, UINT bytecodeLength
		, ID3D11Device * device
		, AllocatorI & allocator_for_vs_bytecode
		)
	{
		const NwShaderType::Enum shaderType = (NwShaderType::Enum) shaderHeader.type;
		//U32 disassembleFlags = 0;
		//ID3DBlob* disassembly = nil;
		//D3DDisassemble(
		//	compiledBytecode,
		//	bytecodeLength,
		//	disassembleFlags,
		//	nil,	// LPCSTR szComments
		//	&disassembly
		//	);
		//if(disassembly){
		//	Util_SaveDataToFile(disassembly->GetBufferPointer(), disassembly->GetBufferSize(),
		//		Str::Format<String64>("R:/%s.asm",g_shaderTypeName[shaderType].buffer).raw());
		//	disassembly->Release();
		//	disassembly = nil;
		//}

		ID3D11ClassLinkage* linkage = nil;

		switch( shaderType )
		{
		case NwShaderType::Vertex :
			dxCHK(device->CreateVertexShader( compiledBytecode, bytecodeLength, linkage, &m_VS ));

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

			vs_bytecode = allocateHeaderWithSize( allocator_for_vs_bytecode, bytecodeLength, PREFERRED_ALIGNMENT );

			if( !vs_bytecode ) {
				return ERR_OUT_OF_MEMORY;
			}

			memcpy( vs_bytecode->data, compiledBytecode, bytecodeLength );

#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

			break;

		case NwShaderType::Hull :
			dxCHK(device->CreateHullShader( compiledBytecode, bytecodeLength, linkage, &m_HS ));
			break;

		case NwShaderType::Domain :
			dxCHK(device->CreateDomainShader( compiledBytecode, bytecodeLength, linkage, &m_DS ));
			break;

		case NwShaderType::Geometry :
			dxCHK(device->CreateGeometryShader( compiledBytecode, bytecodeLength, linkage, &m_GS ));
			break;

		case NwShaderType::Pixel :
			dxCHK(device->CreatePixelShader( compiledBytecode, bytecodeLength, linkage, &m_PS ));
			break;

		case NwShaderType::Compute :
			dxCHK(device->CreateComputeShader( compiledBytecode, bytecodeLength, linkage, &m_CS ));
			break;

		mxNO_SWITCH_DEFAULT;
		}

		this->header = shaderHeader;

		return ALL_OK;
	}

	void ShaderD3D11::destroy( AllocatorI & allocator_for_vs_bytecode )
	{

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

		if( vs_bytecode )
		{
			releaseMemory( allocator_for_vs_bytecode, vs_bytecode );
			vs_bytecode = nil;
		}

#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

		SAFE_RELEASE(m_ptr);
	}

	TextureD3D11::TextureD3D11()
	{
		m_resource = nil;
		m_SRV = nil;
		m_UAV = nil;

		//m_type = TextureType::TEXTURE_2D;

		m_width = 0;
		m_height = 0;
		m_depth = 0;

		m_format = NwDataFormat::Unknown;
		m_numMips = 0;

		_flags.raw_value = NwTextureCreationFlags::defaults;
	}

	ERet TextureD3D11::CreateFromMemory(
		const void* data, UINT size,
		ID3D11Device * device,
		GrTextureCreationFlagsT flags
		, const char* debug_name
		)
	{
		const U32 magic_num = *(U32*) data;

		if( magic_num == TEXTURE_FOURCC )
		{
			mxDO(this->_createTextureFromRawImageData( data, size, flags, device ));
		}
		else if( magic_num == DDS_MAGIC_NUM )
		{
			mxDO(this->_createTextureFromDDS( data, size, flags, device ));
		}
		else if( RF1::is_VBM( data, size ) )
		{
			mxDO(this->_createTextureFromVBM( data, size, flags, device ));
		}
		else
		{
			mxDO(this->_createTextureUsingSTBImage( data, size, flags, device ));
		}

		return ALL_OK;
	}

	void TextureD3D11::Create(
		const NwTexture2DDescription& txInfo
		, ID3D11Device * device
		, const void* imageData
		, const char* debug_name
		)
	{
		mxASSERT2(txInfo.numMips > 0, "MipMap levels should be generated offline");
		TextureImage_m image;
		image.data		= imageData;
		image.size		= CalculateTexture2DSize(txInfo.width, txInfo.height, txInfo.format, txInfo.numMips);
		image.width		= txInfo.width;
		image.height	= txInfo.height;
		image.depth		= 1;
		image.format	= txInfo.format;
		image.numMips	= txInfo.numMips;
		image.arraySize	= txInfo.arraySize;
		image.type		= TEXTURE_2D;

		this->_createTexture_internal( image, txInfo.flags, device );
	}

	void TextureD3D11::Create(
		const NwTexture3DDescription& txInfo
		, ID3D11Device * device
		, const void* data
		, const char* debug_name
		)
	{
		mxASSERT(txInfo.depth > 1);
		TextureImage_m image;
		image.data		= data;
		image.size		= 0;//CalculateTexture2DSize(txInfo.width, txInfo.height, txInfo.format, txInfo.numMips) * txInfo.depth;
		image.width		= txInfo.width;
		image.height	= txInfo.height;
		image.depth		= txInfo.depth;
		image.format	= txInfo.format;
		image.numMips	= txInfo.numMips;
		image.arraySize	= 1;
		image.type		= TEXTURE_3D;

		this->_createTexture_internal( image, txInfo.flags, device );

		dxSetDebugName( m_resource, debug_name );
	}

	void TextureD3D11::Destroy()
	{
		SAFE_RELEASE(m_resource);
		SAFE_RELEASE(m_SRV);
		SAFE_RELEASE(m_UAV);
		//m_type = TextureType::TEXTURE_2D;
		m_numMips = 0;
		_flags.raw_value = NwTextureCreationFlags::defaults;
	}

	ERet TextureD3D11::_createTextureFromRawImageData(
		const void* data, UINT size
		,  const GrTextureCreationFlagsT flags
		, ID3D11Device * device
		)
	{
		const TextureHeader_d& header = *(TextureHeader_d*) data;
		const void* imageData = mxAddByteOffset(data, sizeof(TextureHeader_d));

		//
		TextureImage_m image;
		image.Init( imageData, header );

		//
		return this->_createTexture_internal(
			image, flags, device
			);
	}

	ERet TextureD3D11::_createTextureFromDDS(
		const void* data, UINT size
		,  const GrTextureCreationFlagsT flags
		, ID3D11Device * device
		)
	{
		TextureImage_m image;
		mxDO(DDS_Parse( data, size, image ));
		//
		return this->_createTexture_internal(
			image, flags, device
			);
	}

	ERet TextureD3D11::_createTextureFromVBM(
		const void* data, UINT size
		,  const GrTextureCreationFlagsT flags
		, ID3D11Device * device
		)
	{
		TextureImage_m image;

		//
		NwBlob	temp_storage( MemoryHeaps::temporary() );
		mxDO(RF1::load_VBM_from_memory( &image, data, size, temp_storage ));

		//
		return this->_createTexture_internal(
			image, flags, device
			);
	}

	ERet TextureD3D11::_createTextureUsingSTBImage(
		const void* data, UINT size
		,  const GrTextureCreationFlagsT flags
		, ID3D11Device * device
		)
	{
		const int required_components = STBI_rgb_alpha;

		int image_width, image_height;
		int bytes_per_pixel;

		unsigned char* image_data = stbi_load_from_memory(
			(stbi_uc*) data, size,
			&image_width, &image_height, &bytes_per_pixel, required_components
			);
		if( !image_data )
		{
			ptERROR("Unknown texture type, reason: '%s'", stbi_failure_reason());
			return ERR_FAILED_TO_PARSE_DATA;
		}

		//
		stb_image_Util::ImageDeleter	stb_image_deleter( image_data );

		const int size_of_image_data = image_width * image_height * required_components * sizeof(char);

		//
		TextureImage_m image;
		image.data    = image_data;
		image.size    = size_of_image_data;
		image.width   = image_width;
		image.height	= image_height;
		image.depth		= 1;
		image.format	= NwDataFormat::RGBA8;
		image.numMips	= 1;
		image.arraySize = 1;
		image.type		= TEXTURE_2D;

		DEVOUT("Loading texture (R8G8B8A8): %dx%d (%d bytes)",
			image_width, image_height, size_of_image_data
			);
		//
		return this->_createTexture_internal(
			image, flags, device
			);
	}

	ERet TextureD3D11::_createTexture_internal(
		const TextureImage_m& image
		, const GrTextureCreationFlagsT flags
		, ID3D11Device * device
		)
	{
		const NwDataFormat::Enum textureFormat = image.format;
		const TextureFormatInfoD3D11& format = gs_textureFormats[ textureFormat ];

		const DXGI_FORMAT native_format = ( flags & NwTextureCreationFlags::sRGB )
			? DXGI_FORMAT_MAKE_SRGB(format.tex)
			: format.tex
			;

		const UINT arraySize = image.arraySize;
		mxASSERT(arraySize >= 1);

		const UINT numSides = (image.type == TEXTURE_CUBEMAP) ? 6 : 1;
		const UINT numSlices = image.numMips * numSides;
		D3D11_SUBRESOURCE_DATA* initialData = nil;

		if( image.data )
		{

#if MX_DEVELOPER
			if(image.type == TEXTURE_2D || image.type == TEXTURE_1D) {
				mxASSERT2(arraySize <= 1, "creating texture arrays with initial data is not impl");
			}
#endif

			// Each element of pInitialData provides all of the slices that are defined for a given miplevel.

			initialData = (D3D11_SUBRESOURCE_DATA*) alloca( numSlices * sizeof(initialData[0]) );

			MipLevel_m* mipLevels = (MipLevel_m*) alloca( numSlices * sizeof(MipLevel_m) );

			mxDO(ParseMipLevels( image, mipLevels, numSlices ));

			// The distance from the first pixel of one 2D slice of a 3D texture
			// to the first pixel of the next 2D slice in that texture.
			UINT slice_pitch = 0;
			if(image.type == TEXTURE_3D)
			{
				U32 rowCount, rowPitch;

				GetSurfaceInfo(
					image.format,
					image.width,
					image.height,
					slice_pitch,	// total size of this mip level, in bytes
					rowCount,		// number of rows (horizontal blocks)
					rowPitch		// (aligned) size of each row (aka pitch), in bytes
					);
			}

			for( int i = 0; i < numSlices; i++ )
			{
				const MipLevel_m& mip = mipLevels[ i ];
				initialData[ i ].pSysMem	= mip.data;
				initialData[ i ].SysMemPitch	= mip.pitch;
				initialData[ i ].SysMemSlicePitch	= slice_pitch;
			}
		}

		const bool is_dynamic = (flags & NwTextureCreationFlags::DYNAMIC);
		const bool enable_unordered_access = (flags & NwTextureCreationFlags::RANDOM_WRITES);
		const bool enable_hw_mip_map_generation = (flags & NwTextureCreationFlags::GENERATE_MIPS);

		//
		D3D11_SHADER_RESOURCE_VIEW_DESC	srvDesc;
		mxZERO_OUT(srvDesc);


		// Use 1 for a multisampled texture; or 0 to generate a full set of subtextures.
		const UINT num_miplevels_in_initial_data = enable_hw_mip_map_generation ? 0 : image.numMips;

		//
		if( (image.type == TEXTURE_2D) || (image.type == TEXTURE_CUBEMAP) )
		{
			D3D11_TEXTURE2D_DESC	texDesc;
			mxZERO_OUT(texDesc);

			texDesc.Width			= image.width;
			texDesc.Height			= image.height;
			texDesc.MipLevels		= num_miplevels_in_initial_data;
			texDesc.Format			= native_format;
			texDesc.SampleDesc.Count	= 1;
			texDesc.SampleDesc.Quality	= 0;
			texDesc.Usage			= is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
			texDesc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
			if( enable_unordered_access ) {
				texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}
			texDesc.CPUAccessFlags	= is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
			if( enable_hw_mip_map_generation ) {
				texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			}

			// need to set D3D11_BIND_RENDER_TARGET to generate mipmaps for UAV
			if( enable_unordered_access && enable_hw_mip_map_generation ) {
				texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			}

			//
			srvDesc.Format	= texDesc.Format;

			if( image.type == TEXTURE_CUBEMAP )
			{
				mxASSERT(arraySize == 6);
				texDesc.ArraySize	= 6;
				texDesc.MiscFlags	|= D3D11_RESOURCE_MISC_TEXTURECUBE;

				srvDesc.ViewDimension	= D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip	= 0;
				srvDesc.TextureCube.MipLevels = num_miplevels_in_initial_data;
			}
			else
			{
				texDesc.ArraySize	= arraySize;

				if( arraySize == 1 )
				{
					srvDesc.ViewDimension	= D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MostDetailedMip	= 0;
					srvDesc.Texture2D.MipLevels = num_miplevels_in_initial_data;
				}
				else
				{
					srvDesc.ViewDimension	= D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY;	// View the resource as a 2D-texture array
					srvDesc.Texture2DArray.MostDetailedMip	= 0;
					srvDesc.Texture2DArray.MipLevels = num_miplevels_in_initial_data;
					srvDesc.Texture2DArray.FirstArraySlice = 0;
					srvDesc.Texture2DArray.ArraySize = arraySize;
				}
			}

			dxCHK(device->CreateTexture2D( &texDesc, initialData, &m_texture2D ));
		}
		else
		{
			D3D11_TEXTURE3D_DESC	texDesc;
			mxZERO_OUT(texDesc);

			texDesc.Width			= image.width;
			texDesc.Height			= image.height;
			texDesc.Depth			= image.depth;
			texDesc.MipLevels		= num_miplevels_in_initial_data;
			texDesc.Format			= native_format;
			texDesc.Usage			= is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
			texDesc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
			if( enable_unordered_access ) {
				texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}
			texDesc.CPUAccessFlags	= is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
			if( enable_hw_mip_map_generation ) {
				texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			}

			// need to set D3D11_BIND_RENDER_TARGET to generate mipmaps for UAV
			if( enable_unordered_access && enable_hw_mip_map_generation ) {
				texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			}

			//
			srvDesc.Format	= texDesc.Format;
			srvDesc.ViewDimension	= D3D11_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip	= 0;
			srvDesc.Texture3D.MipLevels = image.numMips;

			dxCHK(device->CreateTexture3D( &texDesc, initialData, &m_texture3D ));
		}

		//
		dxCHK(device->CreateShaderResourceView( m_resource, nil, &m_SRV ));

		//
		if( enable_unordered_access )
		{
			if( (image.type == TEXTURE_2D) || (image.type == TEXTURE_CUBEMAP) )
			{
				dxCHK(device->CreateUnorderedAccessView( m_resource, nil, &m_UAV ));
			}
			else
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
				mxZERO_OUT(uav_desc);

				uav_desc.Format = srvDesc.Format;
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.FirstWSlice = 0;
				uav_desc.Texture3D.MipSlice = 0;
				uav_desc.Texture3D.WSize = image.depth;

				dxCHK(device->CreateUnorderedAccessView( m_resource, &uav_desc, &m_UAV ));
			}
		}

		m_width = image.width;
		m_height = image.height;
		m_depth = image.depth;
		m_format = textureFormat;
		m_numMips = image.numMips;

		_flags = flags;

		mxASSERT_PTR(m_resource);
		mxASSERT_PTR(m_SRV);

		return ALL_OK;
	}

	ProgramD3D11::ProgramD3D11()
	{
		VS.SetNil();
		HS.SetNil();
		DS.SetNil();
		GS.SetNil();
		PS.SetNil();
	}
	void ProgramD3D11::destroy()
	{
		// Nothing.
	}

	void SaveScreenshot( const char* _where, EImageFileFormat::Enum _format )
	{
		UNDONE;
#if 0
		ID3D11DeviceContext* deviceContext = backend->m_deviceContext;

		ID3D11Texture2D* pSrcResource = nil;
		dxCHK(backend->swap_chain->GetBuffer( 0, IID_ID3D11Texture2D, (void**)&pSrcResource ));

		static const D3DX11_IMAGE_FILE_FORMAT s_dx_formats[] = {
			D3DX11_IFF_BMP//IFF_BMP,
			,D3DX11_IFF_JPG//IFF_JPG,
			,D3DX11_IFF_PNG//IFF_PNG,
			,D3DX11_IFF_DDS//IFF_DDS,
			,D3DX11_IFF_TIFF//IFF_TIFF,
			,D3DX11_IFF_GIF//IFF_GIF,
			,D3DX11_IFF_WMP//IFF_WMP,
		};
		const D3DX11_IMAGE_FILE_FORMAT dx_format = s_dx_formats[_format];
		mxBUG("GUI text is not visible when saved in PNG");
		::D3DX11SaveTextureToFileA( deviceContext, pSrcResource, dx_format, _where );
#endif
	}

#if 0
            case RC_ReadPixels :
                {
                    const ReadPixelsCommand* command = commands.ReadNoCopy< ReadPixelsCommand >();

                    ColorTargetD3D11& source = backend->color_targets[ command->source.id ];

                    ID3D11Texture2D* pSrcResource = source.m_colorTexture;

                    D3D11_TEXTURE2D_DESC srcDesc;
                    pSrcResource->GetDesc(&srcDesc);

                    D3D11_TEXTURE2D_DESC dstDesc;
                    dstDesc.Width = source.m_width;
                    dstDesc.Height = source.m_height;
                    dstDesc.MipLevels = 1;
                    dstDesc.ArraySize = 1;
                    dstDesc.Format = srcDesc.Format;
                    dstDesc.SampleDesc.Count = 1;
                    dstDesc.SampleDesc.Quality = 0;
                    dstDesc.Usage = D3D11_USAGE_STAGING;
                    dstDesc.BindFlags = 0;
                    dstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    dstDesc.MiscFlags = 0;

                    ID3D11Texture2D* pDstResource = nil;
                    if(SUCCEEDED(backend->device->CreateTexture( &dstDesc, nil, &pDstResource )))
                    {
                        backend->deviceContext->CopyResource( pDstResource, pSrcResource );

                        ScopedLock_D3D11 lockedTexture( backend->deviceContext, pDstResource, D3D11_MAP_READ );

                        //const UINT rowPitchInBytes = lockedTexture.GetMappedData().RowPitch;
                        const UINT depthPitchInBytes = lockedTexture.GetMappedData().DepthPitch;
                        const UINT bytesToCopy = smallest(command->bufferSize, depthPitchInBytes);
                        memcpy(command->destination, lockedTexture.ToVoidPtr(), bytesToCopy);

                        SAFE_RELEASE(pDstResource);
                    }
                } break;
#endif



/*
-----------------------------------------------------------------------------
	ConstantBufferPool_D3D11
-----------------------------------------------------------------------------
*/
ConstantBufferPool_D3D11::ConstantBufferPool_D3D11()
{

}

ConstantBufferPool_D3D11::~ConstantBufferPool_D3D11()
{

}

ERet ConstantBufferPool_D3D11::Initialize(
	const UniformBufferPoolHandles& uniform_buffer_pool_handles
	, BackendD3D11 * backend
	)
{
	DEVOUT("D3D11: Creating pooled Constant Buffers...");

	//
	NwBufferDescription	buffer_description(_InitDefault);
	buffer_description.type = Buffer_Uniform;

	mxOPTIMIZE("allocate larger buffers first to reduce fragmentation");
	//
	for( UINT slot = 0; slot < LLGL_MAX_BOUND_UNIFORM_BUFFERS; slot++ )
	{
		for( UINT bin_index = 0; bin_index < UniformBufferPoolHandles::NUM_SIZE_BINS; bin_index++ )
		{
			const HBuffer buffer_handle = uniform_buffer_pool_handles.handles[ slot ][ bin_index ];
			mxASSERT(buffer_handle.IsValid());

			buffer_description.size = UniformBufferPoolHandles::getBufferSizeByBinIndex( bin_index );

			//
			backend->createBuffer(
				buffer_handle.id
				, buffer_description
				, nil/*initial_data*/
				
				, (MX_DEVELOPER ? "PooledCB" : nil)

				);

			//
			slot_entries[ slot ].handles[ bin_index ] = buffer_handle;
		}
		slot_entries[ slot ].used_bins = 0;
	}

	return ALL_OK;
}

void ConstantBufferPool_D3D11::Shutdown()
{

}

bool ConstantBufferPool_D3D11::isBufferUsed( U32 slot, U32 bin_index ) const
{
	return slot_entries[ slot ].used_bins & BIT(bin_index);
}

HBuffer ConstantBufferPool_D3D11::allocateConstantBuffer( U32 slot, U32 size )
{
	mxASSERT(slot >= 0 && slot < mxCOUNT_OF(slot_entries));
	mxASSERT(size >= LLGL_MIN_UNIFORM_BUFFER_SIZE && size <= LLGL_MAX_UNIFORM_BUFFER_SIZE);

	const UINT bin_index = UniformBufferPoolHandles::getBinIndexForBufferOfSize( size );

	mxASSERT(!isBufferUsed( slot, bin_index ));

	slot_entries[ slot ].used_bins |= BIT(bin_index);

	return slot_entries[ slot ].handles[ bin_index ];
}

void ConstantBufferPool_D3D11::releaseConstantBuffer( U32 slot, U32 size )
{
	mxASSERT(slot >= 0 && slot < mxCOUNT_OF(slot_entries));

	const UINT bin_index = UniformBufferPoolHandles::getBinIndexForBufferOfSize( size );

	mxASSERT(isBufferUsed( slot, bin_index ));

	slot_entries[ slot ].used_bins &= ~BIT(bin_index);
}

void ConstantBufferPool_D3D11::markAllAsFree()
{
	for( int i = 0; i < mxCOUNT_OF(slot_entries); i++ ) {
		slot_entries[i].used_bins = 0;
	}
}

BackEndCounters g_BackEndStats;mxTEMP

GpuProfiler::GpuProfiler()
{
	mxZERO_OUT(*this);
}

void GpuProfiler::Startup(ID3D11Device* device)
{
    for (int i=0; i<2; i++)
    {
        D3D11_QUERY_DESC querydesc;
        querydesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
        querydesc.MiscFlags = 0;
        device->CreateQuery(&querydesc, &pipeline_statistics[i]);

        querydesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        querydesc.MiscFlags = 0;
        device->CreateQuery(&querydesc, &timestamp_disjoint_query[i]);

        querydesc.Query = D3D11_QUERY_TIMESTAMP;
        querydesc.MiscFlags = 0;
        device->CreateQuery(&querydesc, &timestamp_begin_frame[i]);
        device->CreateQuery(&querydesc, &timestamp_end_frame[i]);
    }
}

void GpuProfiler::Shutdown()
{
    for (int i=0; i<2; i++)
    {
        SAFE_RELEASE(pipeline_statistics[i]);
        SAFE_RELEASE(timestamp_disjoint_query[i]);
        SAFE_RELEASE(timestamp_begin_frame[i]);
        SAFE_RELEASE(timestamp_end_frame[i]);
    }
}

void GpuProfiler::BeginFrame(ID3D11DeviceContext* ctxt, UINT framecount)
{
    // Pipeline statistics are gathered between the query begin/end
    ctxt->Begin(pipeline_statistics[framecount%2]);

    // All timestamp queries must be inside a timestamp disjoint begin/end bracket
    ctxt->Begin(timestamp_disjoint_query[framecount%2]);

    // To issue a timestamp, only the End() call needs to be made (Begin() is ignored)
    ctxt->End(timestamp_begin_frame[framecount%2]);
}

void GpuProfiler::EndFrame(ID3D11DeviceContext* ctxt, UINT framecount, GPUCounters &_stats)
{
    ctxt->End(timestamp_end_frame[framecount%2]);

    ctxt->End(timestamp_disjoint_query[framecount%2]);

    ctxt->End(pipeline_statistics[framecount%2]);

    // We double-buffer the timestamp queries. If this is the zeroth frame, then
    // we haven't flushed any queries to the command buffer yet.
    if (framecount > 0)
    {
        // Determine which frame has completed queries
        int profiling_frame = (framecount-1) % 2;

        // Get the timestamp disjoint, which tells us the frequency and if the timestamps are valid
        // We loop until the results are ready, which has the result of syncing the CPU with the previous
        // frame on the GPU
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampinfo;
        timestampinfo.Frequency = 0;
        while (timestampinfo.Frequency == 0)
        {
            ctxt->GetData(timestamp_disjoint_query[profiling_frame], &timestampinfo, sizeof(timestampinfo), 0);
        }

        // Once the timestamp disjoint is finished, then all the actual timestamps are also ready
        UINT64 begin = 0;
        UINT64 end = 0;
        ctxt->GetData(timestamp_begin_frame[profiling_frame], &begin, sizeof(begin), 0);
        ctxt->GetData(timestamp_end_frame[profiling_frame], &end, sizeof(end), 0);

        // Print results
        if (timestampinfo.Disjoint == TRUE)
        {
//            Text::Print(0, 10, "Disjoint");
        } else {
//           Text::Print(0, 10, "GPU: %3.1f ms", float((end - begin) / (double)timestampinfo.Frequency) * 1000.0f);
			_stats.GPU_Time_Milliseconds = float((end - begin) / (double)timestampinfo.Frequency) * 1000.0f;
        }

        D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
        ctxt->GetData( pipeline_statistics[profiling_frame], &stats, sizeof(stats), 0 );

        //if (show_pipeline_stats)
        //{
        //    Text::Print(10, 20,  "IAVertices: %llu\n", stats.IAVertices);
        //    Text::Print(10, 30,  "IAPrimitives: %llu\n", stats.IAPrimitives);
        //    Text::Print(10, 40,  "VSInvocations: %llu\n", stats.VSInvocations);
        //    Text::Print(10, 50,  "GSInvocations: %llu\n", stats.GSInvocations);
        //    Text::Print(10, 60,  "GSPrimitives: %llu\n", stats.GSPrimitives);
        //    Text::Print(10, 70,  "CInvocations: %llu\n", stats.CInvocations);
        //    Text::Print(10, 80,  "CPrimitives: %llu\n", stats.CPrimitives);
        //    Text::Print(10, 90,  "PSInvocations: %llu\n", stats.PSInvocations);
        //    Text::Print(10, 100, "HSInvocations: %llu\n", stats.HSInvocations);
        //    Text::Print(10, 110, "DSInvocations: %llu\n", stats.DSInvocations);
        //    Text::Print(10, 120, "CSInvocations: %llu\n", stats.CSInvocations);
        //}
        _stats.IAVertices = stats.IAVertices;
		_stats.IAPrimitives = stats.IAPrimitives;
		_stats.VSInvocations	= stats.VSInvocations;
		_stats.GSInvocations	= stats.GSInvocations;
		_stats.GSPrimitives	= stats.GSPrimitives;
		_stats.CInvocations	= stats.CInvocations;
		_stats.CPrimitives		= stats.CPrimitives;
		_stats.PSInvocations	= stats.PSInvocations;
		_stats.HSInvocations	= stats.HSInvocations;
		_stats.DSInvocations	= stats.DSInvocations;
		_stats.CSInvocations	= stats.CSInvocations;
    }
}

//-------------------------------------------------------------------------------------------------------------//
mxBEGIN_REFLECT_ENUM( DXGI_FORMAT_ENUM )
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_UNKNOWN					  ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32A32_TYPELESS       ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32A32_FLOAT		  ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32A32_UINT           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32A32_SINT           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32_TYPELESS          ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32_FLOAT             ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32_UINT              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32B32_SINT              ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_TYPELESS       ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_FLOAT          ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_UNORM          ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_UINT           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_SNORM          ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16B16A16_SINT           ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32_TYPELESS             ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32_FLOAT                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32_UINT                 ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G32_SINT                 ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32G8X24_TYPELESS           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_D32_FLOAT_S8X24_UINT        ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R10G10B10A2_TYPELESS        ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R10G10B10A2_UNORM           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R10G10B10A2_UINT            ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R11G11B10_FLOAT             ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_TYPELESS           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_UNORM              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_UINT               ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_SNORM              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8B8A8_SINT               ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_TYPELESS             ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_FLOAT                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_UNORM                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_UINT                 ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_SNORM                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16G16_SINT                 ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_D32_FLOAT                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32_FLOAT                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32_UINT                    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R32_SINT                    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R24G8_TYPELESS              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_D24_UNORM_S8_UINT           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R24_UNORM_X8_TYPELESS       ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_X24_TYPELESS_G8_UINT        ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_TYPELESS               ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_UNORM                  ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_UINT                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_SNORM                  ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_SINT                   ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_FLOAT                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_D16_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_UINT                    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_SNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R16_SINT                    ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8_TYPELESS                 ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8_UNORM                    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8_UINT                     ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8_SNORM                    ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8_SINT                     ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_A8_UNORM                    ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R1_UNORM                    ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R9G9B9E5_SHAREDEXP          ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R8G8_B8G8_UNORM             ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_G8R8_G8B8_UNORM             ),

	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC1_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC1_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC1_UNORM_SRGB              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC2_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC2_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC2_UNORM_SRGB              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC3_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC3_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC3_UNORM_SRGB              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC4_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC4_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC4_SNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC5_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC5_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC5_SNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B5G6R5_UNORM                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B5G5R5A1_UNORM              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8A8_UNORM              ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8X8_UNORM              ),
#if 0
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8A8_TYPELESS           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8X8_TYPELESS           ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC6H_TYPELESS               ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC6H_UF16                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC6H_SF16                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC7_TYPELESS                ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC7_UNORM                   ),
	mxREFLECT_ENUM_ITEM1( DXGI_FORMAT_BC7_UNORM_SRGB              ),
#endif
mxEND_REFLECT_ENUM;

//-------------------------------------------------------------------------------------------------------------//

const char* DXGI_FORMAT_ToChars( DXGI_FORMAT format )
{
	return mxGET_ENUM_TYPE( DXGI_FORMAT_ENUM ).FindString( format );
}

// Texture format, SRV, DSV
const TextureFormatInfoD3D11 gs_textureFormats[NwDataFormat::MAX] =
{
	{ DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,               DXGI_FORMAT_UNKNOWN           },

	// Uncompressed formats:
	{ DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_UNKNOWN           }, // RGBA8
	{ DXGI_FORMAT_R11G11B10_FLOAT,    DXGI_FORMAT_R11G11B10_FLOAT,       DXGI_FORMAT_UNKNOWN           }, // R11G11B10_FLOAT
	{ DXGI_FORMAT_R10G10B10A2_UNORM,  DXGI_FORMAT_R10G10B10A2_UNORM,     DXGI_FORMAT_UNKNOWN           }, // R10G10B10A2_UNORM
	{ DXGI_FORMAT_R16_FLOAT,          DXGI_FORMAT_R16_FLOAT,             DXGI_FORMAT_UNKNOWN           }, // R16F
	{ DXGI_FORMAT_R32_FLOAT,          DXGI_FORMAT_R32_FLOAT,             DXGI_FORMAT_UNKNOWN           }, // R32F

	// R16G16_FLOAT
	{ DXGI_FORMAT_R16G16_FLOAT,       DXGI_FORMAT_R16G16_FLOAT,          DXGI_FORMAT_UNKNOWN           },
	// R16G16_UNORM
	{ DXGI_FORMAT_R16G16_UNORM,       DXGI_FORMAT_R16G16_UNORM,          DXGI_FORMAT_UNKNOWN           },

	{ DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_UNKNOWN           }, // RGBA16F
	{ DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,    DXGI_FORMAT_UNKNOWN           }, // RGBA32F

	{ DXGI_FORMAT_R8_UNORM,           DXGI_FORMAT_R8_UNORM,              DXGI_FORMAT_UNKNOWN           }, // R8_UNORM
	{ DXGI_FORMAT_R16_UNORM,          DXGI_FORMAT_R16_UNORM,             DXGI_FORMAT_UNKNOWN           }, // R16_UNORM
	{ DXGI_FORMAT_R32_UINT,           DXGI_FORMAT_R32_UINT,              DXGI_FORMAT_UNKNOWN           }, // R32_UINT

	// RGB32F
	{ DXGI_FORMAT_R32G32B32_FLOAT,		DXGI_FORMAT_R32G32B32_FLOAT,		DXGI_FORMAT_UNKNOWN           },


	// Block-compressed formats:
	{ DXGI_FORMAT_BC1_UNORM,          DXGI_FORMAT_BC1_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC1
	{ DXGI_FORMAT_BC2_UNORM,          DXGI_FORMAT_BC2_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC2
	{ DXGI_FORMAT_BC3_UNORM,          DXGI_FORMAT_BC3_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC3
	{ DXGI_FORMAT_BC4_UNORM,          DXGI_FORMAT_BC4_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC4
	{ DXGI_FORMAT_BC5_UNORM,          DXGI_FORMAT_BC5_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC5
#ifdef Direct3D_11_1_Plus
	{ DXGI_FORMAT_BC6H_SF16,          DXGI_FORMAT_BC6H_SF16,             DXGI_FORMAT_UNKNOWN           }, // BC6H
	{ DXGI_FORMAT_BC7_UNORM,          DXGI_FORMAT_BC7_UNORM,             DXGI_FORMAT_UNKNOWN           }, // BC7
#else
	{ DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,				 DXGI_FORMAT_UNKNOWN           }, // BC6H
	{ DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,				 DXGI_FORMAT_UNKNOWN           }, // BC7
#endif

	// Depth-stencil formats:
	{ DXGI_FORMAT_R16_TYPELESS,       DXGI_FORMAT_R16_UNORM,             DXGI_FORMAT_D16_UNORM,         }, // D16
	{ DXGI_FORMAT_R24G8_TYPELESS,     DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT  }, // D24
	{ DXGI_FORMAT_R24G8_TYPELESS,     DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT  }, // D24S8
	{ DXGI_FORMAT_R32_TYPELESS,       DXGI_FORMAT_R32_UINT,              DXGI_FORMAT_D32_FLOAT  }, // D32
};

//--------------------------------------------------------------------------------------
// Returns the BPP for a particular format
//--------------------------------------------------------------------------------------

UINT DXGI_FORMAT_BitsPerPixel( DXGI_FORMAT format )
{
	switch( format )
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT: 
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return 32;

#ifdef DXGI_1_1_FORMATS
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return 32;
#endif // DXGI_1_1_FORMATS

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 16;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return 8;

#ifdef DXGI_1_1_FORMATS
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;
#endif // DXGI_1_1_FORMATS

		mxNO_SWITCH_DEFAULT; // unhandled format
	}
	return 0;
}

//--------------------------------------------------------------------------------------
// Helper functions to create SRGB formats from typeless formats and vice versa
//--------------------------------------------------------------------------------------

/// Find the appropriate SRGB format for the input texture format.
DXGI_FORMAT DXGI_FORMAT_MAKE_SRGB( DXGI_FORMAT format )
{
	switch( format )
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
	};
	return format;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT DXGI_FORMAT_MAKE_TYPELESS( DXGI_FORMAT format )
{
	switch( format )
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_TYPELESS;
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_TYPELESS;
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_TYPELESS;
	};
	return format;
}

//-------------------------------------------------------------------------------------------------------------//

bool DXGI_FORMAT_IsBlockCompressed( DXGI_FORMAT format )
{
	switch (format)
	{
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return true;
		break;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return true;
		break;

#ifdef DXGI_1_1_FORMATS
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
		break;
#endif // DXGI_1_1_FORMATS

	default:
		// Nothing.
		break;
	}
	return false;
}
//-------------------------------------------------------------------------------------------------------------//
void DXGI_FORMAT_GetSurfaceInfo( size_t width, size_t height, DXGI_FORMAT fmt, size_t* pNumBytes, size_t* pRowBytes, size_t* pNumRows )
{
	size_t numBytes = 0;
	size_t rowBytes = 0;
	size_t numRows = 0;

	bool bc = false;
	bool packed = false;
	bool planar = false;
	size_t bpe = 0;
	switch (fmt)
	{
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		bc=true;
		bpe = 8;
		break;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		bc = true;
		bpe = 16;
		break;

	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	//case DXGI_FORMAT_YUY2:
		packed = true;
		bpe = 4;
		break;

	//case DXGI_FORMAT_Y210:
	//case DXGI_FORMAT_Y216:
	//	packed = true;
	//	bpe = 8;
	//	break;

	//case DXGI_FORMAT_NV12:
	//case DXGI_FORMAT_420_OPAQUE:
	//	planar = true;
	//	bpe = 2;
	//	break;

	//case DXGI_FORMAT_P010:
	//case DXGI_FORMAT_P016:
	//	planar = true;
	//	bpe = 4;
	//	break;
	}

	if (bc)
	{
		size_t numBlocksWide = 0;
		if (width > 0)
		{
			numBlocksWide = std::max<size_t>( 1, (width + 3) / 4 );
		}
		size_t numBlocksHigh = 0;
		if (height > 0)
		{
			numBlocksHigh = std::max<size_t>( 1, (height + 3) / 4 );
		}
		rowBytes = numBlocksWide * bpe;
		numRows = numBlocksHigh;
		numBytes = rowBytes * numBlocksHigh;
	}
	else if (packed)
	{
		rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
		numRows = height;
		numBytes = rowBytes * height;
	}
	//else if ( fmt == DXGI_FORMAT_NV11 )
	//{
	//	rowBytes = ( ( width + 3 ) >> 2 ) * 4;
	//	numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
	//	numBytes = rowBytes * numRows;
	//}
	else if (planar)
	{
		rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
		numBytes = ( rowBytes * height ) + ( ( rowBytes * height + 1 ) >> 1 );
		numRows = height + ( ( height + 1 ) >> 1 );
	}
	else
	{
		size_t bpp = DXGI_FORMAT_BitsPerPixel( fmt );
		rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
		numRows = height;
		numBytes = rowBytes * height;
	}

	if (pNumBytes)
	{
		*pNumBytes = numBytes;
	}
	if (pRowBytes)
	{
		*pRowBytes = rowBytes;
	}
	if (pNumRows)
	{
		*pNumRows = numRows;
	}
}

HRESULT FillInitData( _In_ size_t width,
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
					 _Out_writes_(mipCount*arraySize) D3D11_SUBRESOURCE_DATA* initData )
{
	if ( !bitData || !initData )
	{
		return E_POINTER;
	}

	skipMip = 0;
	twidth = 0;
	theight = 0;
	tdepth = 0;

	size_t NumBytes = 0;
	size_t RowBytes = 0;
	const U8* pSrcBits = bitData;
	const U8* pEndBits = bitData + bitSize;

	size_t index = 0;
	for( size_t j = 0; j < arraySize; j++ )
	{
		size_t w = width;
		size_t h = height;
		size_t d = depth;
		for( size_t i = 0; i < mipCount; i++ )
		{
			DXGI_FORMAT_GetSurfaceInfo(
				w,
				h,
				format,
				&NumBytes,
				&RowBytes,
				nullptr
				);

			if ( (mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize) )
			{
				if ( !twidth )
				{
					twidth = w;
					theight = h;
					tdepth = d;
				}
				mxASSUME(index < mipCount * arraySize);
				initData[index].pSysMem = ( const void* )pSrcBits;
				initData[index].SysMemPitch = static_cast<UINT>( RowBytes );
				initData[index].SysMemSlicePitch = static_cast<UINT>( NumBytes );
				++index;
			}
			else if ( !j )
			{
				// Count number of skipped mipmaps (first item only)
				++skipMip;
			}

			if (pSrcBits + (NumBytes*d) > pEndBits)
			{
				return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
			}

			pSrcBits += NumBytes * d;

			w = w >> 1;
			h = h >> 1;
			d = d >> 1;
			if (w == 0)
			{
				w = 1;
			}
			if (h == 0)
			{
				h = 1;
			}
			if (d == 0)
			{
				d = 1;
			}
		}
	}

	return (index > 0) ? S_OK : E_FAIL;
}


//-------------------------------------------------------------------------------------------------------------//
const char* DXGI_ScanlineOrder_ToStr( DXGI_MODE_SCANLINE_ORDER scanlineOrder )
{
	switch( scanlineOrder )
	{
	case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED :			return "DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED";
	case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE :			return "DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE";
	case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST :	return "DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST";
	case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST :	return "DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST";
	mxNO_SWITCH_DEFAULT;
	}
	return "Unknown";
}
//-------------------------------------------------------------------------------------------------------------//
const char* DXGI_ScalingMode_ToStr( DXGI_MODE_SCALING scaling )
{
	switch( scaling )
	{
	case DXGI_MODE_SCALING_UNSPECIFIED :return "DXGI_MODE_SCALING_UNSPECIFIED";
	case DXGI_MODE_SCALING_CENTERED :	return "DXGI_MODE_SCALING_CENTERED";
	case DXGI_MODE_SCALING_STRETCHED :	return "DXGI_MODE_SCALING_STRETCHED";
	mxNO_SWITCH_DEFAULT;
	}
	return "Unknown";
}

DXGI_FORMAT DXGI_GetDepthStencil_Typeless_Format( DXGI_FORMAT depthStencilFormat )
{
	DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
	if( depthStencilFormat == DXGI_FORMAT_D16_UNORM ) {
		textureFormat = DXGI_FORMAT_R16_TYPELESS;
	} else if( depthStencilFormat == DXGI_FORMAT_D24_UNORM_S8_UINT ) {
		textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT ) {
		textureFormat = DXGI_FORMAT_R32_TYPELESS;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ) {
		textureFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	} else {
		String64 message;
		Str::Format(message,"not a valid depth format: %d", depthStencilFormat);
		mxUNREACHABLE2(message.raw());
	}
	return textureFormat;
}

DXGI_FORMAT DXGI_GetDepthStencilView_Format( DXGI_FORMAT depthStencilFormat )
{
	DXGI_FORMAT depthStencilViewFormat = DXGI_FORMAT_UNKNOWN;
	if( depthStencilFormat == DXGI_FORMAT_D16_UNORM ) {
		depthStencilViewFormat = DXGI_FORMAT_R16_FLOAT;
	} else if( depthStencilFormat == DXGI_FORMAT_D24_UNORM_S8_UINT ) {
		depthStencilViewFormat = DXGI_FORMAT_R24G8_TYPELESS;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT ) {
		depthStencilViewFormat = DXGI_FORMAT_R32_FLOAT;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ) {
		depthStencilViewFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	} else {
		String64 message;
		Str::Format(message,"not a valid depth format: %d", depthStencilFormat);
		mxUNREACHABLE2(message.raw());
	}
	return depthStencilViewFormat;
}

DXGI_FORMAT DXGI_GetDepthStencil_SRV_Format( DXGI_FORMAT depthStencilFormat )
{
	DXGI_FORMAT shaderResourceViewFormat = DXGI_FORMAT_UNKNOWN;
	if( depthStencilFormat == DXGI_FORMAT_D16_UNORM ) {
		shaderResourceViewFormat = DXGI_FORMAT_R16_UNORM;
	} else if( depthStencilFormat == DXGI_FORMAT_D24_UNORM_S8_UINT ) {
		shaderResourceViewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT ) {
		shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT;
	} else if( depthStencilFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ) {
		shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	} else {
		String64 message;
		Str::Format(message,"not a valid depth format: %d", depthStencilFormat);
		mxUNREACHABLE2(message.raw());
	}
	return shaderResourceViewFormat;
}

//-------------------------------------------------------------------------------------------------------------//
mxBEGIN_REFLECT_ENUM( D3D_FEATURE_LEVEL_ENUM )
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_9_1, D3D_FEATURE_LEVEL_9_1	),
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_2	),
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_3	),
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_0	),
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_1	),
	mxREFLECT_ENUM_ITEM( D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_0	),
mxEND_REFLECT_ENUM;

const char* D3D_FeatureLevelToStr( D3D_FEATURE_LEVEL featureLevel )
{
	return mxGET_ENUM_TYPE( D3D_FEATURE_LEVEL_ENUM ).FindString( featureLevel );
}

D3D11_MAP ConvertMapModeD3D( EMapMode mapMode )
{
	switch( mapMode )
	{
	case Map_Read :	return D3D11_MAP_READ;
	case Map_Write :	return D3D11_MAP_WRITE;
	case Map_Read_Write :	return D3D11_MAP_READ_WRITE;
	case Map_Write_Discard :	return D3D11_MAP_WRITE_DISCARD;
	case Map_Write_DiscardRange :	return D3D11_MAP_WRITE_NO_OVERWRITE;
		mxNO_SWITCH_DEFAULT;
	}
	return D3D11_MAP_WRITE_DISCARD;
}


static const U32 RGBAi_to_D3DPERF_ARGB(const RGBAi& colorRGBA)
{
	// Convert RGBA to ARGB, because D3DPERF_BeginEvent takes D3DCOLOR_ARGB.
	//return rotateRight( colorRGBA.u, 8 );
	return colorRGBA.u;
}


static void __SetMarker( const char* markerName, const RGBAi& colorRGBA )
{
#if GFX_DBG_LOG_GPU_EVENT_MARKERS
	ptPRINT("=== __SetMarker: %s", markerName);
#endif
	D3DPERF_SetMarker(
		RGBAi_to_D3DPERF_ARGB(colorRGBA),
		MarkerNameToWChars(markerName).wideName
		);
}

static void __PushMarker( const char* markerName, const U32 colorRGBA )
{
#if GFX_DBG_LOG_GPU_EVENT_MARKERS
	ptPRINT("=== __PushMarker: %s", markerName);
#endif

	D3DPERF_BeginEvent(
		RGBAi_to_D3DPERF_ARGB(RGBAi(colorRGBA)),
		MarkerNameToWChars(markerName).wideName
		);
}

static void __PopMarker()
{
#if GFX_DBG_LOG_GPU_EVENT_MARKERS
	ptPRINT("=== __PopMarker");
#endif
	D3DPERF_EndEvent();
}

//( Topology::Enum topology )
static const D3D11_PRIMITIVE_TOPOLOGY gs_primitive_topology[NwTopology::MAX] =
{
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,		// Topology::TriangleList
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,		// Topology::TriangleStrip
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST,			// Topology::LineList
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,			// Topology::LineStrip
	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,			// Topology::PointList
	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,			// Topology::TriangleFan
	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,			// Topology::Undefined
};
//struct PrimInfo
//{
//	D3D11_PRIMITIVE_TOPOLOGY m_type;
//	U32 m_min;
//	U32 m_div;
//	U32 m_sub;
//};
//static const PrimInfo s_primInfo[] =
//{
//	{ D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,     0, 0, 0 },
//	{ D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,     1, 1, 0 },
//	{ D3D11_PRIMITIVE_TOPOLOGY_LINELIST,      2, 2, 0 },
//	{ D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,     2, 1, 1 },
//	{ D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,  3, 3, 0 },
//	{ D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 3, 1, 2 },		
//	{ D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,     0, 0, 0 },	// 	TriangleFan	
//};

/// redundancy checking
template< UINT SLOT_COUNT >
static inline
void CalculateRange( const U32 _dirtySlots, DWORD &_iFirstSlot, DWORD &_iLastSlot )
{
	mxSTATIC_ASSERT_ISPOW2( SLOT_COUNT );
	mxSTATIC_ASSERT( SLOT_COUNT <= BYTES_TO_BITS(sizeof(U32)) );
	_BitScanForward( &_iFirstSlot, _dirtySlots );
	_BitScanReverse( &_iLastSlot, _dirtySlots );
	_iLastSlot &= SLOT_COUNT - 1;	// clamp to [0..MaxCount], where MaxCount must be a power of two
}

///
struct ShaderState
{
	// currently bound shader programs
	HProgram				program;
	HShader					stages[NwShaderType::MAX];

	U32		dirtyCBs;
	U32		dirtySSs;
	U32		dirtySRs;
	U32		dirtyUAVs;

	HBuffer			CBs[LLGL_MAX_BOUND_UNIFORM_BUFFERS];	//!< constant _buffers
	HSamplerState	SSs[LLGL_MAX_BOUND_SHADER_SAMPLERS];	//!< sampler states
	HShaderInput	SRs[LLGL_MAX_BOUND_SHADER_TEXTURES];	//!< shader resources
	HShaderOutput	UAVs[LLGL_MAX_BOUND_OUTPUT_BUFFERS];	//!< Unordered Access Views

public:
	void reset()
	{
		memset( this, LLGL_NIL_HANDLE, sizeof(*this) );
		dirtyCBs = 0;
		dirtySSs = 0;
		dirtySRs = 0;
		dirtyUAVs = 0;
	}

	mxFORCEINLINE void setConstantBuffer( const HBuffer buffer_handle, const UINT input_slot )
	{
		mxASSERT(input_slot < LLGL_MAX_BOUND_UNIFORM_BUFFERS);
		if( this->CBs[ input_slot ] != buffer_handle ) {
			this->CBs[ input_slot ] = buffer_handle;
			this->dirtyCBs |= (1u << input_slot);
		}
	}

	void flushDirtyState( BackendD3D11* backend, ID3D11DeviceContext *d3d_device_context, U32 _usedShadersMask = ~0 )
	{
		mxSTATIC_ASSERT(BYTES_TO_BITS(sizeof(dirtyCBs)) >= LLGL_MAX_BOUND_UNIFORM_BUFFERS);
		if( dirtyCBs )
		{
			DWORD	iFirstSlot, iLastSlot;
			CalculateRange< LLGL_MAX_BOUND_UNIFORM_BUFFERS >( this->dirtyCBs, iFirstSlot, iLastSlot );

			ID3D11Buffer *	constantBuffers[ LLGL_MAX_BOUND_UNIFORM_BUFFERS ] = { nil };
			for( UINT iCB = iFirstSlot; iCB <= iLastSlot; iCB++ )
			{
				const HBuffer hCB = this->CBs[ iCB ];
				if( hCB.IsValid() ) {
					constantBuffers[ iCB ] = backend->_buffers[ hCB.id ].m_ptr;
				}
			}

			const UINT numBuffers = iLastSlot - iFirstSlot + 1;
			ID3D11Buffer **	changedConstantBuffers = constantBuffers + iFirstSlot;
			d3d_device_context->VSSetConstantBuffers( iFirstSlot, numBuffers, changedConstantBuffers );
			if( _usedShadersMask & BIT(NwShaderType::Geometry) ) {
				d3d_device_context->GSSetConstantBuffers( iFirstSlot, numBuffers, changedConstantBuffers );
			}
			d3d_device_context->PSSetConstantBuffers( iFirstSlot, numBuffers, changedConstantBuffers );
			if( _usedShadersMask & BIT(NwShaderType::Compute) ) {
				d3d_device_context->CSSetConstantBuffers( iFirstSlot, numBuffers, changedConstantBuffers );
			}

			this->dirtyCBs = 0;
		}//If had any 'dirty' constant _buffers.

		//

		mxSTATIC_ASSERT(BYTES_TO_BITS(sizeof(dirtySSs)) >= LLGL_MAX_BOUND_SHADER_SAMPLERS);
		if( this->dirtySSs )
		{
			DWORD	iFirstSlot, iLastSlot;
			CalculateRange< LLGL_MAX_BOUND_SHADER_SAMPLERS >( this->dirtySSs, iFirstSlot, iLastSlot );

			ID3D11SamplerState *	samplerStates[ LLGL_MAX_BOUND_SHADER_SAMPLERS ] = { nil };
			for( UINT iSS = iFirstSlot; iSS <= iLastSlot; iSS++ )
			{
				const HSamplerState hSS = this->SSs[ iSS ];
				if( hSS.IsValid() ) {
					samplerStates[ iSS ] = backend->samplerStates[ hSS.id ].m_ptr;
				}
			}

			const UINT numChangedItems = iLastSlot - iFirstSlot + 1;
			ID3D11SamplerState **	changedItems = samplerStates + iFirstSlot;

#if BIND_VS_RESOURCES
			d3d_device_context->VSSetSamplers( iFirstSlot, numChangedItems, changedItems );
#endif
#if BIND_GS_RESOURCES
			if( _usedShadersMask & BIT(NwShaderType::Geometry) ) {
				d3d_device_context->GSSetSamplers( iFirstSlot, numChangedItems, changedItems );
			}
#endif
			d3d_device_context->PSSetSamplers( iFirstSlot, numChangedItems, changedItems );
			if( _usedShadersMask & BIT(NwShaderType::Compute) ) {
				d3d_device_context->CSSetSamplers( iFirstSlot, numChangedItems, changedItems );
			}

			this->dirtySSs = 0;
		}

		//

		mxSTATIC_ASSERT(BYTES_TO_BITS(sizeof(dirtySRs)) >= LLGL_MAX_BOUND_SHADER_TEXTURES);
		if( this->dirtySRs )
		{
			DWORD	iFirstSlot, iLastSlot;
			CalculateRange< LLGL_MAX_BOUND_SHADER_TEXTURES >( this->dirtySRs, iFirstSlot, iLastSlot );

			ID3D11ShaderResourceView *	shaderResourceViews[ LLGL_MAX_BOUND_SHADER_TEXTURES ] = { nil };
			for( UINT iSR = iFirstSlot; iSR <= iLastSlot; iSR++ )
			{
				const HShaderInput hSR = this->SRs[ iSR ];
				if( hSR.IsValid() ) {
					shaderResourceViews[ iSR ] = backend->getResourceByHandle( hSR );
				}
			}

			const UINT numChangedItems = iLastSlot - iFirstSlot + 1;
			ID3D11ShaderResourceView **	changedItems = shaderResourceViews + iFirstSlot;

#if BIND_VS_RESOURCES
			d3d_device_context->VSSetShaderResources( iFirstSlot, numChangedItems, changedItems );
#endif
#if BIND_GS_RESOURCES
			if( _usedShadersMask & BIT(NwShaderType::Geometry) ) {
				d3d_device_context->GSSetShaderResources( iFirstSlot, numChangedItems, changedItems );
			}
#endif
			d3d_device_context->PSSetShaderResources( iFirstSlot, numChangedItems, changedItems );
			if( _usedShadersMask & BIT(NwShaderType::Compute) ) {
				d3d_device_context->CSSetShaderResources( iFirstSlot, numChangedItems, changedItems );
			}
			this->dirtySRs = 0;
		}

		//

		mxSTATIC_ASSERT(BYTES_TO_BITS(sizeof(dirtyUAVs)) >= LLGL_MAX_BOUND_OUTPUT_BUFFERS);
		if( this->dirtyUAVs )
		{
			DWORD	iFirstSlot, iLastSlot;
			CalculateRange< LLGL_MAX_BOUND_OUTPUT_BUFFERS >( this->dirtyUAVs, iFirstSlot, iLastSlot );

			ID3D11UnorderedAccessView *	unorderedAccessViews[D3D11_PS_CS_UAV_REGISTER_COUNT] = { nil };
			UINT						initialCounts[D3D11_PS_CS_UAV_REGISTER_COUNT] = { 0 };	// reset UAV counters (e.g. with AppendStructuredBuffer)

			for( UINT iUAV = iFirstSlot; iUAV <= iLastSlot; iUAV++ )
			{
				const HShaderOutput hUAV = this->UAVs[ iUAV ];
				if( hUAV.IsValid() ) {
					unorderedAccessViews[ iUAV ] = backend->getUAVByHandle( hUAV );
				}
			}
			const UINT numUAVsToBind = iLastSlot - iFirstSlot + 1;

			//
			if( _usedShadersMask & BIT(NwShaderType::Compute) ) {
				d3d_device_context->CSSetUnorderedAccessViews( iFirstSlot, numUAVsToBind, unorderedAccessViews, initialCounts );
			} else {
				mxASSERT( _usedShadersMask & BIT(NwShaderType::Pixel) );
				d3d_device_context->OMSetRenderTargetsAndUnorderedAccessViews(
					0,	// NumRTVs (NOTE: you can also set D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
					NULL,	// ID3D11RenderTargetView *const *ppRenderTargetViews
					NULL,	// ID3D11DepthStencilView *pDepthStencilView
					iFirstSlot,	// UAVStartSlot
					numUAVsToBind,	// NumUAVs
					unorderedAccessViews,	// ID3D11UnorderedAccessView *const *ppUnorderedAccessViews
					initialCounts
					);
			}

			this->dirtyUAVs = 0;
		}
	}
};


/// tracks the contents of all constant buffers
struct CBuffersState
{
	const void *	contents[LLGL_MAX_BOUND_UNIFORM_BUFFERS];
	U16				sizes[LLGL_MAX_BOUND_UNIFORM_BUFFERS];
	HBuffer			handles[LLGL_MAX_BOUND_UNIFORM_BUFFERS];

	/// a bitmask which indicates the constant buffer slots that were bound to with pooled constant buffers
	U32				slots_with_pooled_cbuffers;

public:
	CBuffersState()
	{
		this->reset();
	}

	void reset()
	{
		for( int i = 0; i < LLGL_MAX_BOUND_UNIFORM_BUFFERS; i++ )
		{
			contents[i] = nil;
			sizes[i] = 0;
			handles[i].SetNil();
		}
		slots_with_pooled_cbuffers = 0;
	}

	void releasePooledConstantBuffers( BackendD3D11 * backend )
	{
		slots_with_pooled_cbuffers = 0;
		backend->constant_buffer_pool.markAllAsFree();
	}

	///
	const HBuffer BindUniformBufferData(
		const TSpan<BYTE>& src_data
		, const UINT input_slot
		, HBuffer buffer_handle
		, BackendD3D11 * backend
		)
	{
		const void* desired_contents_ptr = src_data._data;
		const U32 src_data_size = src_data.rawSize();

#if DBG_POOLED_CBs
DBGOUT("slot=%d, size=%d, slots_with_pooled_cbuffers=%d (frame=%ld)",
	   input_slot, src_data_size, slots_with_pooled_cbuffers, getNumRenderedFrames());
#endif

		// If the current contents of the constant buffer differ from the desired contents:
		if(mxUNLIKELY( contents[ input_slot ] != desired_contents_ptr ))
		{
			// If the currently bound constant buffer was allocated from the pool...
			if( slots_with_pooled_cbuffers & BIT(input_slot) )
			{
				//...release it.
				_ReleasePooledConstantBuffer(
					input_slot,
					sizes[ input_slot ],
					backend
					);
			}

			// If the user did not provide a constant buffer...
			if( buffer_handle.IsNil() )
			{
				// ...Allocate a constant buffer from the pool.
				buffer_handle = this->_AcquirePooledConstantBuffer(
					input_slot,
					src_data_size,
					backend
					);
			}
			else
			{
				// use the user-supplied constant buffer
			}

			mxASSERT(buffer_handle.IsValid());
mxASSERT( buffer_handle.id < mxCOUNT_OF(backend->_buffers) );
mxASSERT_PTR( backend->_buffers[ buffer_handle.id ].m_ptr );

			contents[ input_slot ] = desired_contents_ptr;
			sizes[ input_slot ] = src_data_size;
			handles[ input_slot ] = buffer_handle;

			// Fill the constant buffer with the specified data.

//#if MX_DEVELOPER
//			if( cmd.dbgname ) {
//				String64	tmp;
//				Str::Format( tmp, "BindUniformBufferData: %s", cmd.dbgname );
//				__PushMarker( tmp.c_str(), RGBAf::cadetblue.ToRGBAi().u );
//			}
//#endif // MX_DEVELOPER

			backend->updateBuffer(
				buffer_handle.id,
				desired_contents_ptr,
				src_data_size
				);

//#if MX_DEVELOPER
//			if( cmd.dbgname ) {
//				__PopMarker();
//			}
//#endif // MX_DEVELOPER
		}

		return handles[ input_slot ];
	}

	/// invalidates the cached state after binding a constant buffer
	void onCommand_SetCBuffer(
		const UINT input_slot
		, const HBuffer handle
		, BackendD3D11 * backend
		)
	{
		// If there is a constant buffer bound to this slot and it is different from the specified buffer...
		if( handles[ input_slot ].IsValid() && handles[ input_slot ] != handle )
		{
			//...release the currently bound constant buffer and clear the slot.

			// If the currently bound constant buffer was allocated from the pool...
			if( slots_with_pooled_cbuffers & BIT(input_slot) )
			{
				//...release it.
				_ReleasePooledConstantBuffer( input_slot, sizes[ input_slot ], backend );
			}

			handles[ input_slot ].SetNil();
			contents[ input_slot ] = nil;
			sizes[ input_slot ] = 0;
		}
	}

	/// 
	void onCommand_UpdateBuffer(
		const GfxCommand_UpdateBuffer_OLD* cmd
		, const UINT input_slot
		, const HBuffer handle
		)
	{
		// TODO: should invalidate the cached state after updating a constant buffer,
		// but it is too costly to track all buffer changes: VB,IB,UAV,CB,...
		//this->invalidate_CB_contents_with_handle( handle );
	}

private:
	HBuffer _AcquirePooledConstantBuffer(
		const UINT input_slot
		, const UINT buffer_size
		, BackendD3D11 * backend
		)
	{
		mxASSERT(buffer_size <= LLGL_MAX_UNIFORM_BUFFER_SIZE);
		mxASSERT(0 == (slots_with_pooled_cbuffers & BIT(input_slot)));

#if DBG_POOLED_CBs
DBGOUT("slot=%d, size=%d, slots_with_pooled_cbuffers=%d (frame=%ld)",
	   input_slot, buffer_size, slots_with_pooled_cbuffers, getNumRenderedFrames());
#endif

		const HBuffer buffer_handle = backend->constant_buffer_pool.allocateConstantBuffer(
			input_slot, buffer_size
			);

		slots_with_pooled_cbuffers |= BIT(input_slot);

		return buffer_handle;
	}

	void _ReleasePooledConstantBuffer(
		const UINT input_slot
		, const UINT buffer_size
		, BackendD3D11 * backend
		)
	{
#if DBG_POOLED_CBs
DBGOUT("slot=%d, size=%d, slots_with_pooled_cbuffers=%d (frame=%ld)",
	   input_slot, buffer_size, slots_with_pooled_cbuffers, getNumRenderedFrames());
#endif

		mxASSERT(0 != (slots_with_pooled_cbuffers & BIT(input_slot)));

		backend->constant_buffer_pool.releaseConstantBuffer(
			input_slot, buffer_size
			);

		slots_with_pooled_cbuffers &= ~BIT(input_slot);
	}

	void invalidate_CB_contents_with_handle( const HBuffer handle )
	{
		for( int i = 0; i < LLGL_MAX_BOUND_UNIFORM_BUFFERS; i++ ) {
			if( handles[i] == handle ) {
				contents[i] = nil;
				return;
			}
		}
	}
};


/// contains everything needed for issuing a draw call;
/// keeps only lightweight handles to quickly find differences between old and new state.
struct D3D11State
{
	// Programmable parts:

	ShaderState		shader;

	// for tracking contents of constant buffers
	CBuffersState	CBs_state;


	// Non-programmable parts (configurable render state objects):

	NwRenderState32	states;	//!< currently bound render state objects

	// Geometry:
	HInputLayout	inputLayout;	//!< currently bound input layout

	// Array of offset values; one offset value for each buffer in the vertex-buffer array. Each offset is the number of bytes between the first element of a vertex buffer and the first element that will be used.
	// NOTE: currently, we always fetch vertices starting from zero.
	UINT    streamOffsets[LLGL_MAX_VERTEX_STREAMS];	//!< vertex stream offsets
	// Array of stride values; one stride value for each buffer in the vertex-buffer array. Each stride is the size (in bytes) of the elements that are to be used from that vertex buffer.
	UINT    streamStrides[LLGL_MAX_VERTEX_STREAMS];	//!< vertex stream strides
	// The number of vertex _buffers in the array.
	UINT    numStreams;	//!< number of vertex streams

#if LLGL_MAX_VERTEX_STREAMS == 1
	HBuffer	vertexBuffer;
#endif

	// currently bound index buffer
	HBuffer	indexBuffer;
	bool	using32bitIndices;

	UINT	primitive_topology;	//!< current primitive topology (Topology::Enum)

	//
	NwViewport		viewport;
	NwRectangle64		scissorRect;
	bool			scissorEnabled;

	HColorTarget	color_targets[LLGL_MAX_BOUND_TARGETS];//!< bound render targets
	HDepthTarget	depth_target;	//!< bound depth-stencil surface
	U8				target_count;	//!< number of bound color targets

public:
	D3D11State()
	{
		shader.reset();

		states.SetNil();

		CBs_state.reset();

		inputLayout.SetNil();

		mxZERO_OUT(streamOffsets);
		mxZERO_OUT(streamStrides);
		numStreams = 0;

#if LLGL_MAX_VERTEX_STREAMS == 1
		vertexBuffer.SetNil();
#endif

		indexBuffer.SetNil();
		using32bitIndices = false;

		primitive_topology = NwTopology::Undefined;

		mxZERO_OUT(viewport);
		mxZERO_OUT(scissorRect);
		scissorEnabled = false;

		memset(color_targets, LLGL_NIL_HANDLE, sizeof(color_targets));
		depth_target.SetNil();
		target_count = 0;
	}
};

static void __UnbindShaders( ID3D11DeviceContext *d3d_device_context )
{
	d3d_device_context->VSSetShader( nil, nil, 0 );
	d3d_device_context->HSSetShader( nil, nil, 0 );
	d3d_device_context->DSSetShader( nil, nil, 0 );
	d3d_device_context->GSSetShader( nil, nil, 0 );
	d3d_device_context->PSSetShader( nil, nil, 0 );
	d3d_device_context->CSSetShader( nil, nil, 0 );
}

static void __UnbindShaderResources( ID3D11DeviceContext *d3d_device_context )
{
	ID3D11ShaderResourceView *	SRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT] = { nil };
	ID3D11UnorderedAccessView *	UAVs[D3D11_PS_CS_UAV_REGISTER_COUNT] = { nil };
	UINT						UAV_InitialCounts[D3D11_PS_CS_UAV_REGISTER_COUNT] = { 0 };
	d3d_device_context->VSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->HSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->DSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->GSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->PSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->CSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	d3d_device_context->CSSetUnorderedAccessViews( 0, mxCOUNT_OF(UAVs), UAVs, UAV_InitialCounts );
}

// a lightweight version of ID3D11DeviceContext::ClearState() which mirrors D3D11State::Reset()
static void __ResetState( ID3D11DeviceContext *d3d_device_context )
{
	__UnbindShaders( d3d_device_context );
	__UnbindShaderResources( d3d_device_context );

	d3d_device_context->RSSetState( nil );
	d3d_device_context->OMSetBlendState( nil, nil, ~0 );
	d3d_device_context->OMSetDepthStencilState( nil, 0 );

	d3d_device_context->IASetInputLayout( nil );

	// Commented out because of [ STATE_SETTING INFO #240: DEVICE_IASETVERTEXBUFFERS_BUFFERS_EMPTY]
	// D3D11 INFO: ID3D11DeviceContext::IASetVertexBuffers:
	// Since NumBuffers is 0, the operation effectively does nothing.
	// This is probably not intentional, nor is the most efficient way to achieve this operation. Avoid calling the routine at all.
	//d3d_device_context->IASetVertexBuffers( 0, 0, nil, nil, nil );

	d3d_device_context->IASetIndexBuffer( nil, DXGI_FORMAT_UNKNOWN, 0 );

	// Commented out because of [ STATE_SETTING INFO #237: DEVICE_IASETPRIMITIVETOPOLOGY_TOPOLOGY_UNDEFINED]
	// D3D11 INFO: ID3D11DeviceContext::IASetPrimitiveTopology:
	// Topology value D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED is OK to set during IASetPrimitiveTopology; but will not be valid for any Draw routine.
	//d3d_device_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED );

	d3d_device_context->RSSetViewports( 0, nil );
	d3d_device_context->RSSetScissorRects( 0, nil );

	d3d_device_context->OMSetRenderTargets( 0, nil, nil );
}


static void __SetRenderTargets(
					  const RenderTargets& render_targets
					  
					  //!< the value to clear the depth buffer with
					  , float depth_clear_value

					  //!< GfxViewStateFlags
					  , U16 flags

					  //!< the value to clear the stencil buffer with
					  , U8 stencil_clear_value	

					  , D3D11State & _state
					  , BackendD3D11* backend
					  , ID3D11DeviceContext *d3d_device_context
					  )
{
	// Unbind shader resources
	// avoid D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets:
	// Resource being set to OM DepthStencil is still bound on input!
	// [ STATE_SETTING WARNING #9: DEVICE_OMSETRENDERTARGETS_HAZARD]
	// Forcing VS/GS/PS/CS shader resource slot 0/1/... to NULL.
	if( render_targets.target_count || render_targets.depth_target.IsValid() )
	{
		//__UnbindShaderResources( d3d_device_context );
		ID3D11ShaderResourceView *	SRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT] = { nil };
		d3d_device_context->VSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
		d3d_device_context->PSSetShaderResources( 0, mxCOUNT_OF(SRVs), SRVs );
	}

	ID3D11DepthStencilView* depthStencilView = nil;
	if( render_targets.depth_target.IsValid() )
	{
		const DepthTargetD3D11& depth_target = backend->depthTargets[ render_targets.depth_target.id ];

		depthStencilView = ( flags & NwViewStateFlags::ReadOnlyDepthStencil )
			? depth_target.m_DSV_ReadOnly
			: depth_target.m_DSV
			;

		mxASSERT_PTR(depthStencilView);

		//
		UINT clear_depth_stencil_flags = 0;

		if( flags & NwViewStateFlags::ClearDepth ) {
			clear_depth_stencil_flags |= D3D11_CLEAR_DEPTH;
		}
		if( flags & NwViewStateFlags::ClearStencil ) {
			clear_depth_stencil_flags |= D3D11_CLEAR_STENCIL;
		}

		if( clear_depth_stencil_flags )
		{
			d3d_device_context->ClearDepthStencilView(
				depthStencilView
				, clear_depth_stencil_flags
				, depth_clear_value
				, stencil_clear_value
				);
		}
	}

	// Only set the render targets and the depth stencil if they are different from the currently bound ones.
	bool changed_render_targets_or_depth_stencil
		= (_state.target_count != render_targets.target_count)
		|| (_state.depth_target != render_targets.depth_target)
		;

	ID3D11RenderTargetView *	pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nil };
	for( UINT iRT = 0; iRT < render_targets.target_count; iRT++ )
	{
		const HColorTarget hRT = render_targets.color_targets[ iRT ];
		const ColorTargetD3D11& rRT = backend->color_targets[ hRT.id ];
		pRTVs[ iRT ] = rRT.m_RTV;
		if( flags & (1UL << iRT) ) {
			d3d_device_context->ClearRenderTargetView( pRTVs[ iRT ], render_targets.clear_colors[ iRT ] );
		}
		changed_render_targets_or_depth_stencil |= (_state.color_targets[iRT] != hRT);
		_state.color_targets[ iRT ] = hRT;
	}

	//
	if( changed_render_targets_or_depth_stencil ) {
		_state.depth_target = render_targets.depth_target;
		_state.target_count = render_targets.target_count;
		d3d_device_context->OMSetRenderTargets( render_targets.target_count, pRTVs, depthStencilView );
	}
}

static void __SetView(
					  const ViewState& view,
					  D3D11State & _state,
					  BackendD3D11* backend,
					  ID3D11DeviceContext *d3d_device_context
					  )
{
	__SetRenderTargets(
		view
		, view.depth_clear_value
		, view.flags
		, view.stencil_clear_value	
		, _state
		, backend
		, d3d_device_context
		);

	if( _state.viewport.u != view.viewport.u )
	{
		mxASSERT(!isInvalidViewport( view.viewport ));

		_state.viewport = view.viewport;

		D3D11_VIEWPORT    viewport11;
		viewport11.TopLeftX    = view.viewport.x;
		viewport11.TopLeftY    = view.viewport.y;
		viewport11.Width       = view.viewport.width;
		viewport11.Height      = view.viewport.height;
		viewport11.MinDepth    = 0.0f;
		viewport11.MaxDepth    = 1.0f;
		d3d_device_context->RSSetViewports( 1, &viewport11 );
	}

	//
	_state.CBs_state.reset();
	_state.CBs_state.releasePooledConstantBuffers( backend );
}

static void
__setRenderStates(
				  const NwRenderState32& new_render_states
				  , D3D11State & current_state
				  , BackendD3D11* backend
				  , ID3D11DeviceContext *d3d_device_context
				  )
{
	if(mxUNLIKELY( current_state.states.u != new_render_states.u ))
	{
		if( current_state.states.rasterizer_state != new_render_states.rasterizer_state )
		{
			current_state.states.rasterizer_state = new_render_states.rasterizer_state;
			const RasterizerStateD3D11& rasterizerState = backend->rasterizerStates[ new_render_states.rasterizer_state.id ];
			d3d_device_context->RSSetState( rasterizerState.m_ptr );
		}
		if( current_state.states.depth_stencil_state != new_render_states.depth_stencil_state || current_state.states.stencil_reference_value != new_render_states.stencil_reference_value )
		{
			current_state.states.depth_stencil_state = new_render_states.depth_stencil_state;
			current_state.states.stencil_reference_value = new_render_states.stencil_reference_value;
			const DepthStencilStateD3D11& depthStencilState = backend->depthStencilStates[ new_render_states.depth_stencil_state.id ];
			d3d_device_context->OMSetDepthStencilState( depthStencilState.m_ptr, new_render_states.stencil_reference_value );
		}
		if( current_state.states.blend_state != new_render_states.blend_state )
		{
			current_state.states.blend_state = new_render_states.blend_state;
			const BlendStateD3D11& blendState = backend->blendStates[ new_render_states.blend_state.id ];
			d3d_device_context->OMSetBlendState( blendState.m_ptr, nil, ~0 );
		}
	}
}

static void __SetScissorRect( bool scissorEnabled, const NwRectangle64& scissorRect, D3D11State & _current, BackendD3D11* backend, ID3D11DeviceContext *d3d_device_context )
{
	if( _current.scissorEnabled != scissorEnabled || _current.scissorRect.u != scissorRect.u )
	{
		_current.scissorEnabled = scissorEnabled;
		_current.scissorRect = scissorRect;

		if( scissorEnabled )
		{
			D3D11_RECT	rect;
			rect.left = scissorRect.left;
			rect.top = scissorRect.top;
			rect.right = scissorRect.right;
			rect.bottom = scissorRect.bottom;
			d3d_device_context->RSSetScissorRects( 1, &rect );
		}
		else
		{
			d3d_device_context->RSSetScissorRects( 0, nil );
			_current.scissorEnabled = false;
		}
	}
}

static void __Set_Shader_And_Commit_Constants( const HProgram program_handle, ShaderState & _current, BackendD3D11* backend, ID3D11DeviceContext *d3d_device_context )
{
	mxASSERT(program_handle.IsValid());
	if( _current.program != program_handle )
	{
		_current.program = program_handle;

		const ProgramD3D11& program = backend->programs[ program_handle.id ];

		if( _current.stages[NwShaderType::Vertex] != program.VS )
		{
			_current.stages[NwShaderType::Vertex] = program.VS;
			if( program.VS.IsValid() ) {
				const ShaderD3D11& vertexShader = backend->shaders[ program.VS.id ];
				d3d_device_context->VSSetShader( vertexShader.m_VS, nil, 0 );
			} else {
				d3d_device_context->VSSetShader( nil, nil, 0 );
			}
		}

		if( _current.stages[NwShaderType::Geometry] != program.GS )
		{
			_current.stages[NwShaderType::Geometry] = program.GS;
			if( program.GS.IsValid() ) {
				const ShaderD3D11& geometryShader = backend->shaders[ program.GS.id ];
				d3d_device_context->GSSetShader( geometryShader.m_GS, nil, 0 );
				_current.dirtyCBs = ~0;
				_current.dirtySSs = ~0;
				_current.dirtySRs = ~0;
			} else {
				d3d_device_context->GSSetShader( nil, nil, 0 );
			}
		}

		if( _current.stages[NwShaderType::Pixel] != program.PS )
		{
			_current.stages[NwShaderType::Pixel] = program.PS;
			if( program.PS.IsValid() ) {
				const ShaderD3D11& pixelShader = backend->shaders[ program.PS.id ];
				d3d_device_context->PSSetShader( pixelShader.m_PS, nil, 0 );
			} else {
				d3d_device_context->PSSetShader( nil, nil, 0 );
			}
		}

		if( _current.stages[NwShaderType::Compute] != program.CS )
		{
			_current.stages[NwShaderType::Compute] = program.CS;
			if( program.CS.IsValid() ) {
				const ShaderD3D11& computeShader = backend->shaders[ program.CS.id ];
				d3d_device_context->CSSetShader( computeShader.m_CS, nil, 0 );
				_current.dirtyCBs = ~0;
				_current.dirtySSs = ~0;
				_current.dirtySRs = ~0;
				_current.dirtyUAVs = ~0;
			} else {
				d3d_device_context->CSSetShader( nil, nil, 0 );
			}
		}
	}//If program changed.

	const U32 usedShadersMask = 0
		|(_current.stages[NwShaderType::Pixel].IsValid() << NwShaderType::Pixel)
		|(_current.stages[NwShaderType::Vertex].IsValid() << NwShaderType::Vertex)
		|(_current.stages[NwShaderType::Compute].IsValid() << NwShaderType::Compute)
		|(_current.stages[NwShaderType::Geometry].IsValid() << NwShaderType::Geometry)
		|(_current.stages[NwShaderType::Hull].IsValid() << NwShaderType::Hull)
		|(_current.stages[NwShaderType::Domain].IsValid() << NwShaderType::Domain)
		;
	_current.flushDirtyState( backend, d3d_device_context, usedShadersMask );
}

static void __ExecuteDrawCall(
						  const Cmd_Draw& cmd
						  , const UINT instance_count
						  , D3D11State & current_state
						  , BackendD3D11* backend
						  , ID3D11DeviceContext *d3d_device_context
						  )
{
	const HProgram program_handle = cmd.program;

	// Bind shader program and set its inputs.
	__Set_Shader_And_Commit_Constants(
		program_handle
		, current_state.shader
		, backend
		, d3d_device_context
		);


	// Set input layout.

	const HInputLayout	input_layout = cmd.input_layout;

	bool input_layout_changed = false;

	if(mxUNLIKELY( current_state.inputLayout != input_layout ))
	{
		current_state.inputLayout = input_layout;
		input_layout_changed = true;

		if( input_layout.IsValid() )
		{
			/*const*/ InputLayoutD3D11 & input_layout_obj = backend->inputLayouts[ input_layout.id ];

#if LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

			if( !input_layout_obj.m_ptr )
			{
				const HShader current_vertex_shader_handle = current_state.shader.stages[ NwShaderType::Vertex ];
				const ShaderD3D11& current_vertex_shader = backend->shaders[ current_vertex_shader_handle.id ];

				const NwVertexDescription& vertex_description = backend->_vertex_descriptions[ input_layout.id ];

				input_layout_obj.Create(
					vertex_description
					, current_vertex_shader.vs_bytecode
					, backend->_d3d_device
					);
			}

#endif // LLGL_CONFIG_BACKEND_D3D11_CREATE_INPUT_LAYOUTS_ON_DEMAND

			mxASSERT( input_layout_obj.m_numSlots <= LLGL_MAX_VERTEX_STREAMS );
			TCopyStaticArray( current_state.streamStrides, input_layout_obj.m_strides );
			current_state.numStreams = input_layout_obj.m_numSlots;

			d3d_device_context->IASetInputLayout( input_layout_obj.m_ptr );
		}
		else
		{
			mxZERO_OUT(current_state.streamStrides);
			current_state.numStreams = 0;
			d3d_device_context->IASetInputLayout( nil );
		}
	}

	// Set vertex _buffers.
#if LLGL_MAX_VERTEX_STREAMS == 1
	if( input_layout_changed || mxUNLIKELY( current_state.vertexBuffer != cmd.VB ) )
	{
		if( input_layout.IsValid() )
		{
			mxASSERT(current_state.numStreams == 1);
			mxASSERT(cmd.VB.IsValid());

			current_state.vertexBuffer = cmd.VB;

			const BufferD3D11& vertexBuffer = backend->_buffers[ cmd.VB.id ];

			ID3D11Buffer *    vertexBuffers[LLGL_MAX_VERTEX_STREAMS] = { nil };
			vertexBuffers[ 0 ] = vertexBuffer.m_ptr;
			d3d_device_context->IASetVertexBuffers( 0, 1, vertexBuffers, current_state.streamStrides, current_state.streamOffsets );
		}
	}
#else
	ID3D11Buffer *    vertexBuffers[LLGL_MAX_VERTEX_STREAMS] = { nil };
	if( input_layout.IsValid() )
	{
		mxASSERT(current_state.numStreams > 0);

		for( UINT iVB = 0; iVB < current_state.numStreams; iVB++ )
		{
			const HBuffer handle = cmd.VB[ iVB ];
			const BufferD3D11& vertexBuffer = backend->_buffers[ handle.id ];
			vertexBuffers[ iVB ] = vertexBuffer.m_ptr;
		}
		d3d_device_context->IASetVertexBuffers( 0, current_state.numStreams, vertexBuffers, current_state.streamStrides, current_state.streamOffsets );
	}
	else
	{
		// D3D11 INFO: ID3D11DeviceContext::IASetVertexBuffers:
		// Since NumBuffers is 0, the operation effectively does nothing.
		// This is probably not intentional, nor is the most efficient way to achieve this operation.
		// Avoid calling the routine at all.
		// [ STATE_SETTING INFO #240: DEVICE_IASETVERTEXBUFFERS_BUFFERS_EMPTY]
		//d3d_device_context->IASetVertexBuffers( 0, 0, nil, nil, nil );
	}
#endif

	// Set index buffer.

	const bool use32bitIndices = cmd.use_32_bit_indices;

	if( current_state.indexBuffer != cmd.IB || current_state.using32bitIndices != use32bitIndices )
	{
		current_state.indexBuffer = cmd.IB;
		current_state.using32bitIndices = use32bitIndices;
		if( cmd.IB.IsValid() )
		{
			const BufferD3D11& indexBuffer = backend->_buffers[ cmd.IB.id ];
			const DXGI_FORMAT indexFormat = use32bitIndices ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			d3d_device_context->IASetIndexBuffer( indexBuffer.m_ptr, indexFormat, 0 );
		}
		else
		{
			d3d_device_context->IASetIndexBuffer( nil, DXGI_FORMAT_UNKNOWN, 0 );
		}
	}

	// Set primitive topology.
	const NwTopology::Enum primitive_topology = (NwTopology::Enum) cmd.primitive_topology;
	if( current_state.primitive_topology != primitive_topology )
	{
		current_state.primitive_topology = primitive_topology;
		d3d_device_context->IASetPrimitiveTopology( gs_primitive_topology[ primitive_topology ] );
	}

	// Execute the draw call.

	if( instance_count )
	{
		if( cmd.index_count ) {
			d3d_device_context->DrawIndexedInstanced(
				cmd.index_count// UINT IndexCountPerInstance
				, instance_count// UINT InstanceCount
				, cmd.start_index// UINT StartIndexLocation
				, cmd.base_vertex// INT BaseVertexLocation
				, 0// UINT StartInstanceLocation
				);
		} else {
			d3d_device_context->Draw(
				cmd.vertex_count, cmd.base_vertex
				);
		}
	}
	else
	{
		if( cmd.index_count ) {
			d3d_device_context->DrawIndexed( cmd.index_count, cmd.start_index, cmd.base_vertex );
		} else {
			d3d_device_context->Draw( cmd.vertex_count, cmd.base_vertex );
		}
	}
}

static void __Dispatch_Compute_Shader( const Cmd_DispatchCS& _command, D3D11State & _current, BackendD3D11* backend, ID3D11DeviceContext *d3d_device_context )
{
	// Bind shader program and set its inputs.
	__Set_Shader_And_Commit_Constants( _command.program, _current.shader, backend, d3d_device_context );

	//
	d3d_device_context->Dispatch( _command.groupsX, _command.groupsY, _command.groupsZ );

	// Unbind outputs.

	ID3D11UnorderedAccessView *	unorderedAccessViews[D3D11_PS_CS_UAV_REGISTER_COUNT] = { nil };
	d3d_device_context->CSSetUnorderedAccessViews( 0, mxCOUNT_OF(unorderedAccessViews), unorderedAccessViews, nil );

	ID3D11ShaderResourceView *	nullShaderResourceViews[LLGL_MAX_BOUND_SHADER_TEXTURES] = { nil };
	d3d_device_context->CSSetShaderResources( 0, mxCOUNT_OF(nullShaderResourceViews), nullShaderResourceViews );

//	d3d_device_context->CSSetShader(nil, nil, 0);
}

void BackendD3D11::renderFrame( Frame & frame, const RunTimeSettings& settings )
{
	LLGL_PROFILER_SCOPE( "BackendD3D11::Render", RGBAi::OrangeRed );
#if DBG_COMMANDS
DBGOUT("\nBackendD3D11::Render");
#endif
	//
	{
#if LLGL_ENABLE_PERF_HUD
		__PushMarker( "Update Dynamic Transient Buffers", RGBAi::GREEN );
#endif // LLGL_ENABLE_PERF_HUD

		frame.transient_vertex_buffer.Flush( this );

		frame.transient_index_buffer.Flush( this );

#if LLGL_ENABLE_PERF_HUD
		__PopMarker();
#endif // LLGL_ENABLE_PERF_HUD
	}

	NwRenderContext & main_context = frame.main_context;

	const NwRenderContext::SortItem* __restrict	items = main_context._sort_items.raw();
	const U32									num_items = main_context._sort_items.num();
	const char* __restrict						command_buffer_start = main_context._command_buffer.getStart();

	U32 currentViewId = ~0;
	U32 currentTag = ~0;	// for debugging

	D3D11State	current_state;	// initialized with default settings (as if ID3D11DeviceContext::ClearState() were called)

	//const Uniforms* currentUniforms = nil;	// per-instance data (e.g. local-to-world & skinning matrices)

	for( U32 i = 0; i < num_items; i++ )
	{
		const NwRenderContext::SortItem& item = items[ i ];

		U32 viewId;
		const HProgram program = DecodeSortKey( item.sortkey, viewId );
		// the program handle is valid only if this is a Draw/Dispatch command

		// for inspecting the view's contents in the watch window
		IF_DEBUG const ViewState& dbgViewState = frame.view_states[ viewId ];
#if LLGL_ENABLE_PERF_HUD
		const ViewNameT& dbgViewName = frame.view_names[ viewId ];
#endif

		const bool viewChanged = (viewId != currentViewId);
		if( viewChanged )
		{
			mxASSERT(viewId < LLGL_MAX_VIEWS);
			const ViewState& viewSettings = frame.view_states[ viewId ];

#if LLGL_ENABLE_PERF_HUD

			if( currentViewId != ~0 ) {
				__PopMarker();
			}

			const ViewNameT& viewName = frame.view_names[ viewId ];

			#if 0
				__PushMarker( viewName.c_str(), ~0 );
			#else
				String64	viewNameWithIndex;
				Str::Format( viewNameWithIndex, "%s (%d)", viewName.c_str(), viewId );
				__PushMarker( viewNameWithIndex.c_str(), ~0 );
			#endif

#endif // LLGL_ENABLE_PERF_HUD

			// Bind default shader resources.
			const ShaderInputs& defaultParams = frame.view_inputs[ viewId ];
			TCopyStaticArray( current_state.shader.CBs, defaultParams.CBs );
			TCopyStaticArray( current_state.shader.SSs, defaultParams.SSs );
			TCopyStaticArray( current_state.shader.SRs, defaultParams.SRs );

			current_state.shader.dirtyCBs = ~0;
			current_state.shader.dirtySSs = ~0;
			current_state.shader.dirtySRs = ~0;
			// shader inputs will be set before the first draw call
			//			current_state.shader.FlushDirtyState( _d3d_device_context, ~0 );

			__SetView( viewSettings, current_state, this, _d3d_device_context );

			currentViewId = viewId;
		}

		const char* command_list_start = (char*) mxAddByteOffset( command_buffer_start, item.start );
		const char* command_list_end = mxAddByteOffset( command_list_start, item.size );
		const char* current_command = command_list_start;
#if DBG_COMMANDS
DBGOUT("\nStart View = %s", frame.view_names[ viewId ].c_str());
#endif
		do
		{
			const RenderCommand* command_header = (RenderCommand*) current_command;
			mxASSERT(CommandBuffer::IsProperlyAligned( command_header ));

			U32					input_slot;
			U32					resource_handle_value;
			const ECommandType	command_type = command_header->decode(
				&input_slot, &resource_handle_value
				);
#if DBG_COMMANDS
const U32 curr_cmd_offset_from_beginning_of_cmdbuf
= mxGetByteOffset32(command_buffer_start,command_header)
;
DBGOUT("READ CMD = '%s' @ %u",
	   dbg_commandToString(command_type), curr_cmd_offset_from_beginning_of_cmdbuf
	   );
#endif
			switch( command_type )
			{

#pragma region "Draw* Commands"

			case CMD_DRAW:
				{
					const Cmd_Draw& draw_cmd = *(Cmd_Draw*) current_command;

#if DBG_COMMANDS
					DBGOUT("\tDIP: %s, prog: %u, baseVtx: %u, vtxCnt: %u, startIdx: %u, idxCnt: %u",
						draw_cmd.dbgname,
						draw_cmd.program.id,
						draw_cmd.base_vertex, draw_cmd.vertex_count, draw_cmd.start_index, draw_cmd.index_count
						);
#endif

#if MX_DEBUG && DBG_DRAW_CALLS
					//if(!strcmp(draw_cmd.dbgname,"vox chunk")){
					//	printf("");
					//}
					if(draw_cmd.debug_callback)
					{
						(*draw_cmd.debug_callback)();
					}
#endif // DBG_DRAW_CALLS

					__ExecuteDrawCall(
						draw_cmd
						, resource_handle_value // instance_count
						, current_state
						, this
						, _d3d_device_context
						);

					frame.perf_counters.c_draw_calls++;
					frame.perf_counters.c_vertices += draw_cmd.vertex_count;
					////if( draw_cmd.topology == Topology::TriangleList )
					//stats.c_triangles += draw_cmd.index_count / 3u;mxTEMP
					current_command += sizeof(Cmd_Draw);
				} break;

#pragma endregion



			case CMD_DISPATCH_CS:
				{
					const Cmd_DispatchCS& cmd_DispatchCS = *(Cmd_DispatchCS*) current_command;
					__Dispatch_Compute_Shader( cmd_DispatchCS, current_state, this, _d3d_device_context );
					current_command += sizeof(Cmd_DispatchCS);
				} break;



#pragma region "Set* Commands"

			case CMD_SET_CBUFFER:
				{
					const HBuffer buffer_handle( resource_handle_value );
					current_state.shader.setConstantBuffer( buffer_handle, input_slot );
					current_state.CBs_state.onCommand_SetCBuffer( input_slot, buffer_handle, this );
					current_command += sizeof(Cmd_BindResource);
				} break;

			case CMD_SET_RESOURCE :
				{
					const HShaderInput handle( resource_handle_value );
					mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_TEXTURES);
					if( current_state.shader.SRs[ input_slot ] != handle ) {
						current_state.shader.dirtySRs |= (1u << input_slot);
						current_state.shader.SRs[ input_slot ] = handle;
					}
					current_command += sizeof(Cmd_BindResource);
				} break;

			case CMD_SET_SAMPLER :
				{
					const HSamplerState handle( resource_handle_value );
					mxASSERT(input_slot < LLGL_MAX_BOUND_SHADER_SAMPLERS);
					if( current_state.shader.SSs[ input_slot ] != handle ) {
						current_state.shader.dirtySSs |= (1u << input_slot);
						current_state.shader.SSs[ input_slot ] = handle;
					}
					current_command += sizeof(Cmd_BindResource);
				} break;

			case CMD_SET_UAV :
				{
					const HShaderOutput handle( resource_handle_value );
					mxASSERT(input_slot < LLGL_MAX_BOUND_OUTPUT_BUFFERS);
					if( current_state.shader.UAVs[ input_slot ] != handle ) {
						current_state.shader.dirtyUAVs |= (1u << input_slot);
						current_state.shader.UAVs[ input_slot ] = handle;
					}
					current_command += sizeof(Cmd_BindResource);
				} break;

			case CMD_SET_RENDER_TARGETS:
				{
					const Cmd_SetRenderTargets& cmd_set_render_targets = *(Cmd_SetRenderTargets*) current_command;

					__SetRenderTargets(
						cmd_set_render_targets.render_targets
						, 0//view.depth_clear_value
						, 0//view.flags
						, 0//view.stencil_clear_value	
						, current_state
						, this
						, _d3d_device_context
						);

					current_command += sizeof(Cmd_SetRenderTargets);
				} break;

			case CMD_SET_VIEWPORT:
				{
					const Cmd_SetViewport& cmd_set_viewport = *(Cmd_SetViewport*) current_command;

					current_state.viewport.width = cmd_set_viewport.width;
					current_state.viewport.height = cmd_set_viewport.height;
					current_state.viewport.x = cmd_set_viewport.top_left_x;
					current_state.viewport.y = cmd_set_viewport.top_left_y;

					D3D11_VIEWPORT    viewport11;
					viewport11.TopLeftX    = cmd_set_viewport.top_left_x;
					viewport11.TopLeftY    = cmd_set_viewport.top_left_y;
					viewport11.Width       = cmd_set_viewport.width;
					viewport11.Height      = cmd_set_viewport.height;
					viewport11.MinDepth    = cmd_set_viewport.min_depth;
					viewport11.MaxDepth    = cmd_set_viewport.max_depth;
					_d3d_device_context->RSSetViewports( 1, &viewport11 );

					current_command += sizeof(Cmd_SetViewport);
				} break;

			case CMD_SET_SCISSOR :
				{
					const Cmd_SetScissor& cmd_SetScissor = *(Cmd_SetScissor*) current_command;
					const bool scissorRectEnabled = !!resource_handle_value;
					__SetScissorRect( scissorRectEnabled, cmd_SetScissor.rect, current_state, this, _d3d_device_context );
					current_command += sizeof(Cmd_SetScissor);
				} break;

			case CMD_SET_RENDER_STATE :
				{
					const Cmd_SetRenderState& cmd = *(Cmd_SetRenderState*) current_command;
					__setRenderStates( cmd, current_state, this, _d3d_device_context );
					current_command += sizeof(Cmd_SetRenderState);
				} break;

#pragma endregion

				//

#pragma region "Update* Commands"

			case CMD_UPDATE_BUFFER_OLD :
				{
					const GfxCommand_UpdateBuffer_OLD* cmd_UpdateBuffer = (GfxCommand_UpdateBuffer_OLD*) current_command;
					const Memory *	new_contents = cmd_UpdateBuffer->new_contents;

					this->updateBuffer(
						resource_handle_value,
						new_contents->getDataPtr(), new_contents->size
						);

					//
					current_state.CBs_state.onCommand_UpdateBuffer(
						cmd_UpdateBuffer
						, input_slot
						, HBuffer::MakeHandle( resource_handle_value )
						);

					current_command += sizeof(GfxCommand_UpdateBuffer_OLD) + cmd_UpdateBuffer->bytes_to_skip_after_this_command;

					if( !memoryFollowsCommand( new_contents, cmd_UpdateBuffer ) )
					{
						NGpu::releaseMemory( new_contents );
					}
					
				} break;

			case CMD_UPDATE_BUFFER :
				{
					const Cmd_UpdateBuffer* update_buffer_cmd = (Cmd_UpdateBuffer*) current_command;

					const void* src_data = update_buffer_cmd->data;

					this->updateBuffer(
						resource_handle_value,
						src_data, update_buffer_cmd->size
						);

					current_command += sizeof(*update_buffer_cmd);

					if( update_buffer_cmd->deallocator ) {
						update_buffer_cmd->deallocator->Deallocate( (void*)src_data );
					}
					
				} break;

			case CMD_BIND_CONSTANTS:
				{
					const Cmd_BindConstants& cmd = *(Cmd_BindConstants*) current_command;

					const TSpan<BYTE> src_data = cmd.GetConstantsData();

					const HBuffer buffer_handle = current_state.CBs_state.BindUniformBufferData(
						src_data
						, input_slot
						, HBuffer::MakeNilHandle()//TODO
						, this
						);

					current_state.shader.setConstantBuffer(
						buffer_handle
						, input_slot
						);

					current_command = (char*) cmd.GetNextCommandPtr();
				} break;

			case CMD_BIND_PUSH_CONSTANTS:
				{
					const Cmd_BindPushConstants& cmd = *(Cmd_BindPushConstants*) current_command;

					const TSpan<BYTE> src_data = cmd.GetConstantsData();

					const HBuffer buffer_handle = current_state.CBs_state.BindUniformBufferData(
						src_data
						, input_slot
						, HBuffer::MakeHandle( resource_handle_value )
						, this
						);

					current_state.shader.setConstantBuffer(
						buffer_handle
						, input_slot
						);

					current_command = (char*) cmd.GetNextCommandPtr();
				} break;

			case CMD_UPDATE_TEXTURE :
				{
					UNDONE;
					//const Cmd_UpdateTexture& cmd_UpdateTexture = *(Cmd_UpdateTexture*) current_command;
					//const HTexture resourceHandle = { commandHeader >> 16u };
					//this->UpdateTexture(
					//	resourceHandle,
					//	cmd_UpdateTexture.updateRegion,
					//	cmd_UpdateTexture.data
					//	);
					//current_command += sizeof(Cmd_UpdateTexture);
					//if( cmd_UpdateTexture.deallocator ) {
					//	cmd_UpdateTexture.deallocator->Deallocate( (void*) cmd_UpdateTexture.data );
					//} else {
					//	mxUNDONE;
					//	//current_command += cmd_DrawDynGeom.size;
					//}
				} break;
#pragma endregion

			case CMD_GENERATE_MIPS :
				{
					const Cmd_GenerateMips& cmd_GenerateMips = *(Cmd_GenerateMips*) current_command;
					const HShaderInput resourceHandle( resource_handle_value );
					ID3D11ShaderResourceView* pSRV = getResourceByHandle( resourceHandle );
					_d3d_device_context->GenerateMips( pSRV );
					current_command += sizeof(Cmd_GenerateMips);
				} break;

			case CMD_CLEAR_RENDER_TARGETS :
				{
					const Cmd_ClearRenderTargets& cmdClearRenderTargets = *reinterpret_cast< const Cmd_ClearRenderTargets* >( current_command );

					for( UINT iRT = 0; iRT < cmdClearRenderTargets.target_count; iRT++ )
					{
						const HColorTarget hRT = cmdClearRenderTargets.color_targets[ iRT ];
						mxASSERT(hRT.IsValid());
						if(hRT.IsValid())
						{
							const ColorTargetD3D11& rRT = this->color_targets[ hRT.id ];
							_d3d_device_context->ClearRenderTargetView(
								rRT.m_RTV,
								cmdClearRenderTargets.clear_colors[ iRT ]
							);
						}
					}

					current_command += sizeof(Cmd_ClearRenderTargets);
				} break;

			case CMD_CLEAR_DEPTH_STENCIL_VIEW :
				{
					const Cmd_ClearDepthStencilView& cmdClearDepthStencilView = *reinterpret_cast< const Cmd_ClearDepthStencilView* >( current_command );

					const DepthTargetD3D11& rDT = this->depthTargets[ cmdClearDepthStencilView.depth_target.id ];

					_d3d_device_context->ClearDepthStencilView(
						rDT.m_DSV
						, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL
						, cmdClearDepthStencilView.depth_clear_value
						, cmdClearDepthStencilView.stencil_clear_value
						);

					current_command += sizeof(Cmd_ClearDepthStencilView);
				} break;

			case CMD_COPY_TEXTURE:
				{
					const Cmd_CopyTexture& cmd = *reinterpret_cast< const Cmd_CopyTexture* >( current_command );

					ID3D11Resource *pDstResource = this->textures[ cmd.dst_texture_handle.id ].m_resource;
					ID3D11Resource *pSrcResource = this->textures[ cmd.src_texture_handle.id ].m_resource;

					_d3d_device_context->CopyResource(
						pDstResource, pSrcResource
						);

					current_command += sizeof(Cmd_CopyTexture);
				} break;

			case CMD_COPY_RESOURCE:
				{
					const Cmd_CopyResource& copyResourceCommand = *reinterpret_cast< const Cmd_CopyResource* >( current_command );

					const BufferD3D11& sourceBuffer		= _buffers[ copyResourceCommand.source.id ];
					const BufferD3D11& destinationBuffer= _buffers[ copyResourceCommand.destination.id ];

					ID3D11Resource *pSrcResource = sourceBuffer.m_ptr;
					ID3D11Resource *pDstResource = destinationBuffer.m_ptr;

					_d3d_device_context->CopyResource( pDstResource, pSrcResource );

					current_command += sizeof(Cmd_CopyResource);
				} break;

				//

#pragma region "Retrieve* Commands"

			case CMD_MAP_READ:
				{
					const Cmd_MapRead& mapReadCommand = *reinterpret_cast< const Cmd_MapRead* >( current_command );

					const BufferD3D11& buffer = _buffers[ mapReadCommand.buffer.id ];

					D3D11_MAPPED_SUBRESOURCE	mappedData;

					const HRESULT hResult = _d3d_device_context->Map(
						buffer.m_ptr	// ID3D11Resource *pResource
						, 0	// UINT Subresource
						, D3D11_MAP_READ	// D3D11_MAP MapType
						, D3D11_MAP_FLAG_DO_NOT_WAIT	// D3D11_MAP_FLAG
						, &mappedData	// D3D11_MAPPED_SUBRESOURCE *pMappedResource
						);

					if(SUCCEEDED( hResult ))
					{
						(*mapReadCommand.callback)( mapReadCommand.userData, mappedData.pData );

						_d3d_device_context->Unmap( buffer.m_ptr, 0 );
					}

					current_command += sizeof(Cmd_MapRead);
				} break;

#pragma endregion

				//

			case CMD_PUSH_MARKER :
				{
#if LLGL_ENABLE_PERF_HUD
					const Cmd_PushMarker& cmd_PushMarker = *(Cmd_PushMarker*) current_command;

					__PushMarker( cmd_PushMarker.text, cmd_PushMarker.rgba );

					current_command += sizeof(Cmd_PushMarker) + cmd_PushMarker.skip;
#endif
				} break;

			case CMD_POP_MARKER :
				{
#if LLGL_ENABLE_PERF_HUD
					__PopMarker();
					current_command += sizeof(Cmd_PopMarker);
#endif
				} break;

			case CMD_SET_MARKER :
				{
#if LLGL_ENABLE_PERF_HUD
					const Cmd_SetMarker& cmd_SetMarker = *(Cmd_SetMarker*) current_command;
					__SetMarker( cmd_SetMarker.text, cmd_SetMarker.rgba );
					current_command += sizeof(Cmd_SetMarker) + cmd_SetMarker.skip;
#endif // LLGL_ENABLE_PERF_HUD
				} break;



			case CMD_DBG_PRINT :
				{
					const Cmd_DbgPrint& cmd_DbgPrint = *(Cmd_DbgPrint*) current_command;
					currentTag = cmd_DbgPrint.tag;
					//if( cmd_DbgPrint.tag == 7 ) {
					//	ptPRINT("%s", cmd_DbgPrint.text);
					//}
					current_command += sizeof(Cmd_DbgPrint) + cmd_DbgPrint.skip;
				} break;

			case CMD_NOP :
				{
					const Cmd_NOP& cmd_NOP = *(Cmd_NOP*) current_command;

					DEVOUT("%s", cmd_NOP.dbgmsg.getDataPtr());

					current_command += sizeof(Cmd_NOP) + cmd_NOP.bytes_to_skip_after_this_command;
				} break;

				mxDEFAULT_UNREACHABLE(;);
			}//switch( command_type )
		}
		while( current_command < command_list_end );
	}//For each command list.

#if LLGL_ENABLE_PERF_HUD
	if( currentViewId != ~0 ) {
		__PopMarker();
	}
#endif

	// Cleanup (aka make the runtime happy)
	//_d3d_device_context->ClearState();
	__ResetState( _d3d_device_context );	// mirrors D3D11State::Reset()

//		profiler.EndFrame( _d3d_device_context, framesRendered, stats.gpu );

	//		BackendD3D11::UpdateVideoMode();

	framesRendered++;



	// Present the contents of the swap chain to the screen.

	// An integer that specifies the how to synchronize presentation of a frame with the vertical blank.
	// Values are:
	// 0 - The presentation occurs immediately, there is no synchronization.
	// 1,2,3,4 - Synchronize presentation after the n'th vertical blank.
	const UINT sync_interval = settings.device.enable_VSync ? 1 : 0;

	// If VSync is enabled:
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.

	dxCHK(_dxgi_swap_chain->Present(
		sync_interval,	// UINT SyncInterval
		0		// UINT Flags
		));

	const U64 backEndFinishTime = mxGetTimeInMicroseconds();

//		g_BackEndStats = stats;
}

//TIncompleteType< sizeof(ShaderState) >	s;

}//namespace NGpu

#endif // LLGL_Driver == LLGL_Driver_Direct3D_11


/*
Reference code:
https://github.com/walbourn/directxtk-samples/blob/master/SimpleSampleWin32/DeviceResources.cpp

Debugging:
https://seanmiddleditch.com/direct3d-11-debug-api-tricks/
https://walbourn.github.io/dxgi-debug-device/
*/
