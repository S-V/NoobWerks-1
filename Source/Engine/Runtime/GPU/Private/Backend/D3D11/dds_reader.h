//--------------------------------------------------------------------------------------
// DDS.h
//
// This header defines constants and structures that are useful when parsing 
// DDS files.  DDS files were originally designed to use several structures
// and constants that are native to DirectDraw and are defined in ddraw.h,
// such as DDSURFACEDESC2 and DDSCAPS2.  This file defines similar 
// (compatible) constants and structures so that one can use DDS files 
// without needing to include ddraw.h.
//--------------------------------------------------------------------------------------
#pragma once

// "DDS " on little-endian machines (" SDD" if big-endian)
enum DDS_Constants: U32
{
	DDS_MAGIC_NUM = ' SDD',	// 0x20534444
};

#pragma pack(push,1)
struct DDS_PIXELFORMAT
{
    U32 	size;		// Structure size; set to 32 (bytes).
    U32 	flags;		// Values which indicate what type of data is in the surface. 
    U32 	fourCC;		// Four-character codes for specifying compressed or custom formats.
    U32 	RGBBitCount;// Number of bits in an RGB (possibly including alpha) format. Valid when dwFlags includes DDPF_RGB, DDPF_LUMINANCE, or DDPF_YUV.
    U32 	RBitMask;	// Red (or luminance or Y) mask for reading color data. For instance, given the A8R8G8B8 format, the red mask would be 0x00ff0000.
    U32 	GBitMask;	// Green (or U) mask for reading color data. For instance, given the A8R8G8B8 format, the green mask would be 0x0000ff00.
    U32 	BBitMask;	// Blue (or V) mask for reading color data. For instance, given the A8R8G8B8 format, the blue mask would be 0x000000ff.
    U32 	ABitMask;	// Alpha mask for reading alpha data. dwFlags must include DDPF_ALPHAPIXELS or DDPF_ALPHA. For instance, given the A8R8G8B8 format, the alpha mask would be 0xff000000.
};
struct DDS_HEADER
{
    U32 			size;	// Size of structure. This member must be set to 124.
    U32 			flags;	// Flags to indicate which members contain valid data.

	U32 			height;	// Surface height (in pixels).
    U32 			width;	// Surface width (in pixels).

	// The pitch or number of bytes per scan line in an uncompressed texture;
	// the total number of bytes in the top level texture for a compressed texture.
	U32 			pitchOrLinearSize;

	// Depth of a volume texture (in pixels) (if DDS_HEADER_FLAGS_VOLUME is set in flags), otherwise unused.
	U32 			depth;

    U32 			mipMapCount;	// Number of mipmap levels, otherwise unused.

    U32 			reserved1[11];	// Unused.

    DDS_PIXELFORMAT	pixelFormat;	// The pixel format (see DDS_PIXELFORMAT).

    U32 			surfaceFlags;	// Specifies the complexity of the surfaces stored.
    U32 			cubemapFlags;	// Additional detail about the surfaces stored.

	U32 			reserved2[3];	// Unused.
};
// DDS header extension to handle resource arrays.
struct DDS_HEADER_DXT10
{
	U32	dxgiFormat;	// DXGI_FORMAT
	U32	resourceDimension;	// D3D10_RESOURCE_DIMENSION
	U32	miscFlag; // see D3D11_RESOURCE_MISC_FLAG
	U32	arraySize;
	U32	reserved; // see DDS_MISC_FLAGS2
};
#pragma pack(pop)

ASSERT_SIZEOF( DDS_HEADER, 124 );
ASSERT_SIZEOF( DDS_HEADER_DXT10, 20 );

// returns a valid pointer to DDS file header and the offset to the start of the texture data
const DDS_HEADER* DDS_ParseHeader( const void* _data, UINT _size, UINT *_offset = NULL );


ERet DDS_Parse( const void* data, UINT size, TextureImage_m &dds_image_ );

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
