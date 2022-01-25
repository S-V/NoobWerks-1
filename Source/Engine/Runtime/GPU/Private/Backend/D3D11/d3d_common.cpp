// Living without D3DX:
// http://blogs.msdn.com/b/chuckw/archive/2013/08/21/living-without-d3dx.aspx
#include <algorithm>	// std::min<>, std::max<>
#include <Base/Base.h>
#include <GPU/Public/graphics_device.h>

#if LLGL_Driver_Is_Direct3D

#include "d3d_common.h"

// These headers are needed for D3DERR_* and D3D11_ERROR_* codes
#include <d3d9.h>
#include <D3D11.h>

/// Returns a string corresponding to the error code.
const char* D3D_GetErrorCodeString( HRESULT errorCode )
{
	switch ( errorCode )
	{
		case D3DERR_INVALIDCALL :
			return "The method call is invalid";

		case D3DERR_WASSTILLDRAWING :
			return "The previous blit operation that is transferring information to or from this surface is incomplete";

		case E_FAIL :
			return "Attempted to create a device with the debug layer enabled and the layer is not installed";

		case E_INVALIDARG :
			return "An invalid parameter was passed to the returning function";

		case E_OUTOFMEMORY :
			return "Direct3D could not allocate sufficient memory to complete the call";

		case S_FALSE :
			return "Alternate success value, indicating a successful but nonstandard completion (the precise meaning depends on context";

		case S_OK :
			return "No error occurred"; 

		// DXGI-specific error codes

		case DXGI_ERROR_DEVICE_HUNG :
			return "DXGI_ERROR_DEVICE_HUNG"; 

		case DXGI_ERROR_DEVICE_REMOVED :
			return "DXGI_ERROR_DEVICE_REMOVED"; 

		case DXGI_ERROR_DEVICE_RESET :
			return "DXGI_ERROR_DEVICE_RESET"; 

		case DXGI_ERROR_DRIVER_INTERNAL_ERROR :
			return "DXGI_ERROR_DRIVER_INTERNAL_ERROR"; 

		case DXGI_ERROR_INVALID_CALL :
			return "DXGI_ERROR_INVALID_CALL"; 

		// Direct3D 11 specific error codes

		case D3D11_ERROR_FILE_NOT_FOUND :
			return "The file was not found";

		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS :
			return "There are too many unique instances of a particular type of state object";

		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS :
			return "There are too many unique instances of a particular type of view object";

		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD :
			return "Deferred context map without initial discard";

		default:
			/* fallthrough */;
	}

	return ::DXGetErrorDescriptionA( errorCode );
}
//--------------------------------------------------------------//
HRESULT dxWARN( HRESULT errorCode, const char* message, ... )
{
	char	buffer[ 2048 ];
	mxGET_VARARGS_A( buffer, message );
	ptWARN( "%s\nReason:\n[%d] %s\n", buffer,errorCode,  D3D_GetErrorCodeString(errorCode) );
	return errorCode;
}
//--------------------------------------------------------------//
HRESULT dxERROR( HRESULT errorCode, const char* message, ... )
{
	char	buffer[ 2048 ];
	mxGET_VARARGS_A( buffer, message );
	ptWARN( "%s\nReason:\n[%d] %s\n", buffer, errorCode, D3D_GetErrorCodeString(errorCode) );
	return errorCode;
}
//--------------------------------------------------------------//
HRESULT dxERROR( HRESULT errorCode )
{
	ptWARN( "Error:\n[%d] %s\n", errorCode, D3D_GetErrorCodeString( errorCode ) );
	return errorCode;
}
//--------------------------------------------------------------//
ERet D3D_HRESULT_To_EResult( HRESULT errorCode )
{
	switch ( errorCode )
	{
		case D3DERR_INVALIDCALL :
			return ERR_INVALID_FUNCTION_CALL;

		case E_INVALIDARG :
			return ERR_INVALID_PARAMETER;

		case E_OUTOFMEMORY :
			return ERR_OUT_OF_MEMORY;

		case S_FALSE :
		case S_OK :
			return ALL_OK;

		// Direct3D 11 specific error codes

		case D3D11_ERROR_FILE_NOT_FOUND :
			return ERR_FAILED_TO_OPEN_FILE;

		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS :
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS :
			return ERR_TOO_MANY_OBJECTS;

		default:
			/* fallthrough */;
	}
	return ERet::ERR_UNKNOWN_ERROR;
}

#endif // LLGL_Driver_Is_Direct3D

#if LLGL_ENABLE_PERF_HUD




// fix for MVC+ 2013
// error LNK2001: unresolved external symbol _WKPDID_D3DDebugObjectName
#if mxARCH_TYPE == mxARCH_64BIT
#pragma comment( lib, "dxguid.lib")
#endif



	void dxSetDebugName( IDXGIObject* o, const char* name )
	{
		//DBGOUT("dxSetDebugName: %s\n", name);
	
//D3D11 WARNING: ID3D11DepthStencilState::SetPrivateData: Existing private data of same name with different size found! [ STATE_SETTING WARNING #55: SETPRIVATEDATA_CHANGINGPARAMS]
//D3D11: **BREAK** enabled for the previous message, which was: [ WARNING STATE_SETTING #55: SETPRIVATEDATA_CHANGINGPARAMS ]

		if( o != NULL && name != NULL ) {
			o->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(name), name );
		}
	}
	void dxSetDebugName( ID3D11Device* o, const char* name )
	{
		//DBGOUT("dxSetDebugName: %s\n", name);
		if( o != NULL && name != NULL ) {
			o->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(name), name );
		}
	}
	void dxSetDebugName( ID3D11DeviceChild* o, const char* name )
	{
		//DBGOUT("dxSetDebugName: %s\n", name);
		if( o != NULL && name != NULL ) {
			o->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(name), name );
		}
	}

#endif // RX_D3D_USE_PERF_HUD
