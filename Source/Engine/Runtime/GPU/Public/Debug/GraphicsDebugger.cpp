#include <Base/Base.h>
#pragma hdrstop
#include <GPU/Public/graphics_system.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Public/Debug/GraphicsDebugger.h>


namespace Graphics
{
namespace DebuggerTool
{

namespace
{
	struct DebuggerToolData
	{
		DynamicArray< Surface >		surfaces;
		AllocatorI &						_allocator;
	public:
		DebuggerToolData( ProxyAllocator & allocator )
			: _allocator( allocator )
			, surfaces( allocator )
		{
		}
	};

	static DebuggerToolData *	gs_data = nil;
}//namespace

ERet Initialize( ProxyAllocator & allocator )
{
	mxASSERT(nil == gs_data);
	gs_data = mxNEW( allocator, DebuggerToolData, allocator );
	mxDO(gs_data->surfaces.reserve(64));
	return ALL_OK;
}

void Shutdown()
{
	mxDELETE( gs_data, gs_data->_allocator );
	gs_data = nil;
}

const TSpan< Surface >	getSurfaces()
{
	return Arrays::GetSpan(gs_data->surfaces);
}

void addColorTarget(
					HColorTarget handle
					, const NwColorTargetDescription& description
					, const char* debug_name
					)
{
	Surface	newSurface;
	newSurface.handle = NGpu::AsInput( handle );
	newSurface.format = description.format;

	Str::Format( newSurface.name, "%s: fmt=%s, %ux%u (%.3f MiB)"
		, debug_name
		, DataFormatT_Type().FindString( description.format )
		, description.width, description.height
		, BITS_TO_BYTES( NwDataFormat::BitsPerPixel( description.format ) * description.width * description.height ) / (float)mxMEGABYTE
		);

	newSurface.is_being_displayed_in_ui = false;

	gs_data->surfaces.add( newSurface );
}

void addDepthTarget(
					HDepthTarget handle
					, const NwDepthTargetDescription& description
					, const char* debug_name
					)
{
	Surface	newSurface;
	newSurface.handle = NGpu::AsInput( handle );
	newSurface.format = description.format;

	Str::Format( newSurface.name, "%s: fmt=%s, %ux%u (%.3f MiB)"
		, debug_name
		, DataFormatT_Type().FindString( description.format )
		, description.width, description.height
		, BITS_TO_BYTES( NwDataFormat::BitsPerPixel( description.format ) * description.width * description.height ) / (float)mxMEGABYTE
		);

	newSurface.is_being_displayed_in_ui = false;

	gs_data->surfaces.add( newSurface );
}

static int findSurfaceIndexByHandle( HShaderInput handle )
{
	for( UINT i = 0; i < gs_data->surfaces.num(); i++ )
	{
		if( gs_data->surfaces[i].handle == handle )
		{
			return i;
		}
	}

	return INDEX_NONE;
}

void removeColorTarget( HColorTarget handle )
{
	int surfaceIndex = findSurfaceIndexByHandle( NGpu::AsInput( handle ) );

	if( surfaceIndex != INDEX_NONE )
	{
		gs_data->surfaces.RemoveAt( surfaceIndex );
	}
}

void removeDepthTarget( HDepthTarget handle )
{
	int surfaceIndex = findSurfaceIndexByHandle( NGpu::AsInput( handle ) );

	if( surfaceIndex != INDEX_NONE )
	{
		gs_data->surfaces.RemoveAt( surfaceIndex );
	}
}

void AddTexture2D(
	HTexture texture_handle
	, const NwTexture2DDescription& description
	, const char* debug_name
	)
{
	Surface	newSurface;
	newSurface.handle = NGpu::AsInput( texture_handle );
	newSurface.format = description.format;

	Str::Format( newSurface.name, "%s: fmt=%s, %ux%u (%.3f MiB)"
		, debug_name
		, DataFormatT_Type().FindString( description.format )
		, description.width, description.height
		, BITS_TO_BYTES( NwDataFormat::BitsPerPixel( description.format ) * description.width * description.height ) / (float)mxMEGABYTE
		);

	newSurface.is_being_displayed_in_ui = false;

	gs_data->surfaces.add( newSurface );
}

}//namespace DebuggerTool
}//namespace Graphics
