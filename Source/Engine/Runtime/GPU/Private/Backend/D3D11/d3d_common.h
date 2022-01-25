#pragma once

/// Dynamically load DirectX libraries instead of statically linking with them.
/// This allows to launch apps even when SDK is not installed.
#define D3D_USE_DYNAMIC_LIB	(1)

// Use DirectX utility library
//#define USE_D3DX	(1)

// Enable extra D3D debugging in debug builds if using the debug DirectX runtime.  
// This makes D3D objects work well in the debugger watch window, but slows down 
// performance slightly.

//#if MX_DEBUG && (!defined( D3D_DEBUG_INFO ))
//#	define D3D_DEBUG_INFO
//#endif

// DirectX error codes.
#include <DxErr.h>
#if MX_AUTOLINK
	#pragma comment( lib, "DxErr.lib" )
#endif

#include <D3Dcommon.h>
#include <D3Dcompiler.h>

#include <DXGI.h>

#include <GPU/Public/graphics_device.h>


//------------------------------------------------------------------------
//	Useful macros
//------------------------------------------------------------------------

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p )		{ if( (p) != NULL ) { (p)->Release(); (p) = NULL; } }
#endif

#define SAFE_ADDREF( p )		{ if( (p) != NULL ) { (p)->AddRef(); } }
#define SAFE_ACQUIRE( dst, p )	{ if( (dst) != NULL ) { SAFE_RELEASE(dst); } if( p != NULL ) { (p)->AddRef(); } dst = (p); }

#define RELEASE( p )			(p)->Release()


//------------------------------------------------------------------------
//	Debugging and run-time checks - they slow down a lot!
//------------------------------------------------------------------------

#if MX_DEBUG

	#define dxCHK( expr )		for( HRESULT hr = (expr); ; )\
								{\
									if( FAILED( hr ) )\
									{\
										dxERROR( hr );\
										ptBREAK;\
									}\
									break;\
								}

	#define dxTRY( expr )		for( HRESULT hr = (expr); ; )\
								{\
									if( FAILED( hr ) )\
									{\
										dxERROR( hr );\
										ptBREAK;\
										return D3D_HRESULT_To_EResult( hr );\
									}\
									break;\
								}

#else

	#define dxCHK( expr )		(expr)

	#define dxTRY( expr )		(expr)

#endif // RX_DEBUG_RENDERER



//------------------------------------------------------------------------
//	Declarations
//------------------------------------------------------------------------

template< class T >
mxFORCEINLINE
void SafeRelease( T *&p )
{
	if( p != NULL ) {
		p->Release();
		p = NULL;
	}
}

mxFORCEINLINE
bool dxCheckedRelease( IUnknown* pInterface )
{
	const ULONG refCount = pInterface->Release();
	mxASSERT( refCount == 0 );
	return( refCount == 0 );
}

template< class TYPE, UINT COUNT >
inline
void dxSafeReleaseArray( TYPE* (&elements) [COUNT] )
{
	for( UINT iElement = 0; iElement < COUNT; ++iElement )
	{
		if( elements[ iElement ] != NULL )
		{
			elements[ iElement ]->Release();
		}
	}
}

// Verify that a pointer to a COM object is still valid
//
// Usage:
//   VERIFY_COM_INTERFACE(pFoo);
//
template< class Q >
void VERIFY_COM_INTERFACE( Q* p )
{
#if MX_DEBUG
	p->AddRef();
	p->Release();
#endif // MX_DEBUG
}

template< class Q >
void FORCE_RELEASE_COM_INTERFACE( Q*& p )
{
	mxPLATFORM_PROBLEM("used to get around the live device warning - this is possibly a bug in SDKLayer");
	while( p->Release() )
		;
	p = NULL;
}


template< class Q >
void DbgPutNumRefs( Q* p, const char* msg = NULL )
{
#if MX_DEBUG
	// the new reference count
	const UINT numRefs = p->AddRef();
	DBGOUT( "%sNumRefs = %u\n", (msg != NULL) ? msg : "", numRefs );
	p->Release();
#endif // MX_DEBUG
}

///
///	ComPtr< T > - smart pointer for safe and automatic handling of DirectX resources.
///
template< class T >
struct ComPtr 
{
	T *	_ptr;

public:
	mxFORCEINLINE ComPtr()
		: _ptr( NULL )
	{}

	mxFORCEINLINE ComPtr( T * pointer )
		: _ptr( pointer )
	{
		if( this->_ptr ) {
			this->AcquirePointer();
		}
	}

	mxFORCEINLINE ComPtr( const ComPtr<T> & other )
	{
		this->_ptr = other._ptr;
		if( this->_ptr ) {
			this->AcquirePointer();
		}
	}

	mxFORCEINLINE ~ComPtr()
	{
		if( this->_ptr ) {
			this->ReleasePointer();
		}
	}

	mxFORCEINLINE void Release()
	{
		if( this->_ptr ) {
			this->ReleasePointer();
			this->_ptr = NULL;
		}
	}

	mxFORCEINLINE void operator = ( T * pointer )
	{
		if( this->_ptr ) {
			this->ReleasePointer();
		}
		
		this->_ptr = pointer;
		
		if( this->_ptr ) {
			this->AcquirePointer();
		}
	}

	mxFORCEINLINE void operator = ( const ComPtr<T> & other )
	{
		if( this->_ptr ) {
			this->ReleasePointer();
		}

		this->_ptr = other._ptr;

		if( this->_ptr ) {
			this->AcquirePointer();
		}
	}

	mxFORCEINLINE operator T* () const
	{
		return this->_ptr;
	}

	mxFORCEINLINE T * operator -> () const
	{
		mxASSERT_PTR( this->_ptr );
		return this->_ptr;
	}

	mxFORCEINLINE bool operator == ( const T * pointer )
	{
		return this->_ptr == pointer;
	}
	mxFORCEINLINE bool operator == ( const ComPtr<T> & other )
	{
		return this->_ptr == other._ptr;
	}

	mxFORCEINLINE bool IsNil() const
	{
		return ( NULL == this->_ptr );
	}
	mxFORCEINLINE bool IsValid() const
	{
		return ( NULL != this->_ptr );
	}

private:
	// assumes that the pointer is not null
	mxFORCEINLINE void AcquirePointer()
	{
		mxASSERT_PTR( this->_ptr );
		this->_ptr->AddRef();
	}

	// assumes that the pointer is not null
	mxFORCEINLINE void ReleasePointer()
	{
		mxASSERT_PTR( this->_ptr );
		this->_ptr->Release();
	}
};


#define mxDECLARE_COM_PTR( className )\
	typedef ComPtr< className > className ## _ptr



//------------------------------------------------------------------------
//	Logging and error reporting / error handling
//------------------------------------------------------------------------

HRESULT dxWARN( HRESULT errorCode, const char* message, ... );
HRESULT dxERROR( HRESULT errorCode, const char* message, ... );
HRESULT dxERROR( HRESULT errorCode );

/// Returns a string corresponding to the given error code.
const char* D3D_GetErrorCodeString( HRESULT hErrorCode );

/// Converts an HRESULT to a meaningful message
#define dxLAST_ERROR_STRING		::DXGetErrorStringA(::GetLastError())

ERet D3D_HRESULT_To_EResult( HRESULT errorCode );


/// used when D3D_USE_DYNAMIC_LIB is enabled
struct DLL_Holder: NonCopyable {
	HMODULE	ptr;
public:
	DLL_Holder( HMODULE _ptr = NULL )
		: ptr( _ptr ) {
	}
	~DLL_Holder() {
		Release();
	}
	void Release() {
		if( ptr ) {
			::FreeLibrary( ptr );
			ptr = NULL;
		}
	}
	void Disown() {
		ptr = NULL;
	}
	operator HMODULE () const {
		return ptr;
	}
};

//--------------------------------------------------------------------------------------
// Profiling/instrumentation support
//--------------------------------------------------------------------------------------

/// converts a narrow multibyte character string to wide string
struct MarkerNameToWChars {
	wchar_t	wideName[128];
	MarkerNameToWChars( const char* name ) {
		mbstowcs( wideName, name, mxCOUNT_OF(wideName) );
	}
};

//--------------------------------------------------------------------------------------
// Debugging support
//--------------------------------------------------------------------------------------

// Use dxSetDebugName() to attach names to D3D objects for use by 
// SDKDebugLayer, PIX's object table, etc.

//NOTE: i had to disable this functionality
// because of bugs in MS functions leading to
// D3D11 WARNING: ID3D11DepthStencilState::SetPrivateData:
// Existing private data of same name with different size found!
// [ STATE_SETTING WARNING #55: SETPRIVATEDATA_CHANGINGPARAMS]
//
#if LLGL_ENABLE_PERF_HUD

	void dxSetDebugName( class IDXGIObject* o, const char* name );
	void dxSetDebugName( class ID3D11Device* o, const char* name );
	void dxSetDebugName( class ID3D11DeviceChild* o, const char* name );

#else

	inline void dxSetDebugName( const void* o, const char* name ) {}

#endif // RX_D3D_USE_PERF_HUD

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
