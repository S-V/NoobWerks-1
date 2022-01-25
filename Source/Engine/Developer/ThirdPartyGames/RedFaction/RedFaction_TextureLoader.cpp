//
// Loader for Volition's animated texture files (*.vbm) used in Red Faction 1.
// http://www.levels4you.com/forum.php?do=view&t=3388&viewfrom=1
// 
// Based on work done by Rafal Harabien:
// https://github.com/rafalh/rf-reversed
// 
// NOTE: only the first frame is loaded.
//
#include <Base/Base.h>
#pragma hdrstop

#include <stdint.h>

#include <Base/Util/Color.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include "RedFaction_TextureLoader.h"

/*****************************************************************************
*
*  PROJECT:     Open Faction
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        include/vbm_format.h
*  PURPOSE:     VBM format documentation
*  DEVELOPERS:  Rafal Harabien
*
*****************************************************************************/

/**
 * VBM (Volition Bitmap file)
 *
 * VBM is a proprietary Volition image file format used in Red Faction game.
 * VBM files are either static (only one frame) or animated (multiple frames are included).
 * Each frame contains pixel data for one or more mipmap levels.
 * 
 * Note: all fields are little-endian. On big-endian architecture bytes needs to be swapped.
**/

#pragma pack(push, 1)

#define VBM_SIGNATURE 0x6D62762E // ".vbm"

enum vbm_color_format_t
{
    VBM_CF_1555 = 0,
    VBM_CF_4444 = 1,
    VBM_CF_565  = 2,
};

struct vbm_header_t
{
    uint32_t signature;    // should be equal to VBM_SIGNATURE
    uint32_t version;      // RF uses 1 and 2, makeVBM tool always creates files with version 1 */
    uint32_t width;        // nominal image width
    uint32_t height;       // nominal image height
    uint32_t format;       // pixel data format, see vbm_color_format_t
    uint32_t fps;          // frames per second, ignored if frames_count == 1
    uint32_t num_frames;   // number of frames, always 1 for not animated VBM
    uint32_t num_mipmaps;  // number of mipmap levels except for the full size (level 0)
};

#if 0 // pseudocode

struct vbm_file_t
{
    vbm_header_t  vbm_header;                        // file header
    vbm_frame_t   frames[vbm_header_t::num_frames];  // sequence of frames (at least one)
};

struct vbm_frame_t
{
    vbm_mipmap_t  mipmaps[vbm_header_t::num_mipmaps + 1];  // A frame is made of one or more mipmaps
};

struct vbm_mipmap_t
{
    vbm_pixel_t pixels[];  // Mipmaps are 16bpp images in one of three pixel formats (see format field in header)
};

union vbm_pixel_t
{
    uint16_t    raw_pixel_data; // Pixels are 16 bits wide in "little endian" byte order
    struct
    {
        uint8_t blue:   5;
        uint8_t green:  5;
        uint8_t red:    5;
        uint8_t nalpha: 1; // in version 1 inverted alpha (0 - opaque, 1 - transparent), in version 2 field is not inverted anymore
    } cf_1555;
    struct
    {
        uint8_t blue:  4;
        uint8_t green: 4;
        uint8_t red:   4;
        uint8_t alpha: 4;
    } cf_4444;
    struct
    {
        uint8_t blue:  5;
        uint8_t green: 6;
        uint8_t red:   5;
    } cf_565;
};
ASSERT_SIZEOF(vbm_pixel_t, sizeof(uint16_t));
#endif // pseudocode

#pragma pack(pop)


namespace RF1
{
	static void convert_VBM_CF_4444_to_R8G8B8A8(
		const U16* src_image_data
		, const vbm_header_t& header
		, R8G8B8A8 *dst_image_data
		)
	{
		for( UINT iY = 0; iY < header.height; iY++ )
		{
			for( UINT iX = 0; iX < header.width; iX++ )
			{
				const UINT pixel_index = iY * header.width + iX;

				const U16 src_pixel = src_image_data[ pixel_index ];
				R8G8B8A8 &dst_pixel = dst_image_data[ pixel_index ];

				//
				float b = ( ( src_pixel       ) & 15 ) * (1.0f / 16.0f);
				float g = ( ( src_pixel >> 4  ) & 15 ) * (1.0f / 16.0f);
				float r = ( ( src_pixel >> 8  ) & 15 ) * (1.0f / 16.0f);
				float a = ( ( src_pixel >> 12 ) & 15 ) * (1.0f / 16.0f);

				//
				dst_pixel.R = r * 255.0f;
				dst_pixel.G = g * 255.0f;
				dst_pixel.B = b * 255.0f;
				dst_pixel.A = a * 255.0f;
			}//X
		}//Y
	}

	static void convert_VBM_CF_565_to_R8G8B8A8(
		const U16* src_image_data
		, const vbm_header_t& header
		, R8G8B8A8 *dst_image_data
		)
	{
		for( UINT iY = 0; iY < header.height; iY++ )
		{
			for( UINT iX = 0; iX < header.width; iX++ )
			{
				const UINT pixel_index = iY * header.width + iX;

				const U16 src_pixel = src_image_data[ pixel_index ];
				R8G8B8A8 &dst_pixel = dst_image_data[ pixel_index ];

				//
				float b = ( ( src_pixel       ) & 31 ) * (1.0f / 32.0f);
				float g = ( ( src_pixel >> 5  ) & 63 ) * (1.0f / 64.0f);
				float r = ( ( src_pixel >> 11 ) & 31 ) * (1.0f / 32.0f);

				//
				dst_pixel.R = r * 255.0f;
				dst_pixel.G = g * 255.0f;
				dst_pixel.B = b * 255.0f;
				dst_pixel.A = 0xFF;
			}//X
		}//Y
	}

	bool is_VBM( const void* mem, const U32 size )
	{
		if( size < sizeof(vbm_header_t) ) {
			return false;
		}

		const vbm_header_t* header = (vbm_header_t*) mem;
		return header->signature == VBM_SIGNATURE;
	}

	//
	ERet load_VBM_from_memory(
		TextureImage_m *image_
		, const void* data, UINT size
		, NwBlob & temp_storage
		)
	{
		mxASSERT(is_VBM( data, size ));

		const vbm_header_t* header = (vbm_header_t*) data;

		mxENSURE(
			header->num_frames == 1,
			ERR_UNSUPPORTED_FEATURE,
			"Animated VBMs are not supported!"
			);

		const vbm_color_format_t format = (vbm_color_format_t) header->format;

		//
		const U32		num_pixels = header->width * header->height;
		const size_t	dst_image_data_size = num_pixels * sizeof(R8G8B8A8);

		mxDO(temp_storage.setNum( dst_image_data_size ));

		R8G8B8A8 *dst_image_data = (R8G8B8A8*) temp_storage.raw();

		//
		DEVOUT("Loading VBM: %dx%d, format: %d",
			(int) header->width, (int) header->height, (int) format
			);

		//
		const U16* src_image_data = (U16*) ( header + 1 );

		switch( format )
		{
		case VBM_CF_1555 :
			UNDONE;
			break;

		case VBM_CF_4444 :
			convert_VBM_CF_4444_to_R8G8B8A8(
				src_image_data
				, *header
				, dst_image_data
				);
			break;

		case VBM_CF_565  :
			convert_VBM_CF_565_to_R8G8B8A8(
				src_image_data
				, *header
				, dst_image_data
				);
			break;

		default:
			UNDONE;
			return ERR_UNEXPECTED_TOKEN;
		}

		//
		image_->data = dst_image_data;
		image_->size    = num_pixels * sizeof(R8G8B8A8);
		image_->width   = header->width;
		image_->height	= header->height;
		image_->depth	= 1;
		image_->format	= NwDataFormat::RGBA8;
		image_->numMips	= 1;
		image_->arraySize = 1;
		image_->type = TEXTURE_2D;

		return ALL_OK;
	}
}//namespace RF1
