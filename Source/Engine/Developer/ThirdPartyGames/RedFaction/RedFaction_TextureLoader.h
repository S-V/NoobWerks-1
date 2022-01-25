//
// Loader for Volition's animated texture files (*.vbm) used in Red Faction 1.
// 
// Based on work done by Rafal Harabien:
// https://github.com/rafalh/rf-reversed
//
#pragma once

#include <Base/Template/Containers/Blob.h>

struct TextureImage_m;


namespace RF1
{
	bool is_VBM( const void* mem, UINT size );

	ERet load_VBM_from_memory(
		TextureImage_m *image_
		, const void* data, UINT size
		, NwBlob & temp_storage
		);

}//namespace RF1
