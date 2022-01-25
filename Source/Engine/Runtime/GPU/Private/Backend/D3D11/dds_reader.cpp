#include <Base/Base.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_utilities.h>
#include "DDS_Reader.h"
//#include "image.h"

// "DDS " or ' SDD' on little-endian machines
#define DDS_MAGIC	0x20534444

#define DDS_MAGIC_SIZE			sizeof(U32)
#define DDS_HEADER_SIZE			sizeof(DDS_HEADER)
#define DDS_IMAGE_DATA_OFFSET	(DDS_HEADER_SIZE + DDS_MAGIC_SIZE)


#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#define DDS_DXT1 MAKEFOURCC('D', 'X', 'T', '1')
#define DDS_DXT2 MAKEFOURCC('D', 'X', 'T', '2')
#define DDS_DXT3 MAKEFOURCC('D', 'X', 'T', '3')
#define DDS_DXT4 MAKEFOURCC('D', 'X', 'T', '4')
#define DDS_DXT5 MAKEFOURCC('D', 'X', 'T', '5')
#define DDS_ATI1 MAKEFOURCC('A', 'T', 'I', '1')
#define DDS_BC4U MAKEFOURCC('B', 'C', '4', 'U')
#define DDS_ATI2 MAKEFOURCC('A', 'T', 'I', '2')
#define DDS_BC5U MAKEFOURCC('B', 'C', '5', 'U')

/// The DOOM 3 engine and its derivatives use the so called RXGB format, that is just like DXT5,
/// but with the Red and Alpha components swapped, which means that the normal is given by the (a, g, b) components.
#define DDS_RXGB MAKEFOURCC('R', 'X', 'G', 'B')

#define DDS_A8R8G8B8	21
#define D3DFMT_A16B16G16R16  36
#define D3DFMT_A16B16G16R16F 113

// Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
enum DDS_RESOURCE_DIMENSION
{
    DDS_DIMENSION_TEXTURE1D	= 2,
    DDS_DIMENSION_TEXTURE2D	= 3,
    DDS_DIMENSION_TEXTURE3D	= 4,
};

// Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
enum DDS_RESOURCE_MISC_FLAG
{
   DDS_RESOURCE_MISC_TEXTURECUBE = 0x4L,
};

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA

const DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT2 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT4 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };

const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };

const DDS_PIXELFORMAT DDSPF_R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

const DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

// This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
const DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

// DDS_header.dwFlags
#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_ALPHA                  0x00000002
#define DDPF_FOURCC                 0x00000004
#define DDPF_INDEXED                0x00000020
#define DDPF_RGB                    0x00000040
#define DDPF_YUV                    0x00000200
#define DDPF_LUMINANCE              0x00020000

#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000

#define DDSCAPS2_CUBEMAP_ALLFACES	(DDSCAPS2_CUBEMAP_POSITIVEX|DDSCAPS2_CUBEMAP_NEGATIVEX \
									 |DDSCAPS2_CUBEMAP_POSITIVEY|DDSCAPS2_CUBEMAP_NEGATIVEY \
									 |DDSCAPS2_CUBEMAP_POSITIVEZ|DDSCAPS2_CUBEMAP_NEGATIVEZ)

#define DDSCAPS2_VOLUME             0x00200000

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME


//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------

#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )


const DDS_HEADER* DDS_ParseHeader( const void* _data, UINT _size, UINT *_offset /*= NULL*/ )
{
	// Need at least enough data to fill the header and magic number to be a valid DDS
	if( _size < (sizeof(DDS_HEADER)+sizeof(U32)) ) {
		return NULL;
	}

	// DDS files always start with the same magic number ("DDS ")
	U32 magicNumber = *(U32*) _data;
	if( magicNumber != DDS_MAGIC ) {
		return NULL;
	}

	const DDS_HEADER* header = c_cast(DDS_HEADER* ) mxAddByteOffset( _data, sizeof(U32) );

	// Verify header to validate DDS file
	if( (header->size != sizeof(DDS_HEADER)) || (header->pixelFormat.size != sizeof(DDS_PIXELFORMAT)) )
	{
		return NULL;
	}

	if( (header->surfaceFlags & DDSCAPS_TEXTURE) == 0 ) {
		return NULL;
	}

	// Check for DX10 extension
	bool bDXT10Header = false;
	if( (header->pixelFormat.flags & DDS_FOURCC) && (MAKEFOURCC( 'D', 'X', '1', '0' ) == header->pixelFormat.fourCC) )
	{
		// Must be long enough for both headers and magic value
		if( _size < (sizeof(DDS_HEADER)+sizeof(U32)+sizeof(DDS_HEADER_DXT10)) )
		{
			return false;
		}
		bDXT10Header = true;
	}

	// setup the pointers in the process request
	if( _offset )
	{
		*_offset = sizeof(U32)
			+ sizeof(DDS_HEADER)
			+ (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0)
			;
	}

//	const void* textureData = mxAddByteOffset( _data, _offset );
//	U32 dataSize = _size - _offset;

	return header;
}

///
struct DDS_Format_Mapping
{
	U32				DDS_format;
	NwDataFormat::Enum	engine_format;
};
static const DDS_Format_Mapping gs_DDS_Format_Mappings[] =
{
	{ DDS_DXT1,                  NwDataFormat::BC1     },
	{ DDS_DXT2,                  NwDataFormat::BC2     },
	{ DDS_DXT3,                  NwDataFormat::BC2     },
	{ DDS_DXT4,                  NwDataFormat::BC3     },
	{ DDS_DXT5,                  NwDataFormat::BC3     },
	{ DDS_ATI1,                  NwDataFormat::BC4     },
	{ DDS_BC4U,                  NwDataFormat::BC4     },
	{ DDS_ATI2,                  NwDataFormat::BC5     },
	{ DDS_BC5U,                  NwDataFormat::BC5     },
	//{ D3DFMT_A16B16G16R16,       NwDataFormat::RGBA16  },
	{ D3DFMT_A16B16G16R16F,      NwDataFormat::RGBA16F },
	//{ DDPF_RGB|DDPF_ALPHAPIXELS, NwDataFormat::BGRA8   },
	//{ DDPF_INDEXED,              NwDataFormat::L8      },
	//{ DDPF_LUMINANCE,            NwDataFormat::L8      },
	//{ DDPF_ALPHA,                NwDataFormat::L8      },
	//{ DDS_A8R8G8B8,				NwDataFormat::BGRA8     },
	{ DDS_RXGB,					 NwDataFormat::BC3 },
};

static NwDataFormat::Enum FindEngineFormat( U32 _DDS_format )
{
	for( UINT i = 0; i < mxCOUNT_OF(gs_DDS_Format_Mappings); i++ )
	{
		const DDS_Format_Mapping& mapping = gs_DDS_Format_Mappings[ i ];
		if( mapping.DDS_format == _DDS_format ) {
			return mapping.engine_format;
		}
	}
	return NwDataFormat::Unknown;
}

ERet DDS_Parse( const void* data, UINT size, TextureImage_m &dds_image_ )
{
	U32 offset = 0;
	const DDS_HEADER* header = DDS_ParseHeader( data, size, &offset );
	chkRET_X_IF_NIL(header, ERR_INVALID_PARAMETER);

	const bool bDXT10Header = ( offset > sizeof(U32) + sizeof(DDS_HEADER) );

	//
	const U32 width = header->width;
	const U32 height = header->height;
	const U32 depth = header->depth;

	// > 1 if (header->surfaceFlags & DDS_SURFACE_FLAGS_MIPMAP)
	const U32 mipCount = largest(header->mipMapCount, 1);

	const bool isCubeMap = ((header->cubemapFlags & DDSCAPS2_CUBEMAP) != 0);
	if( isCubeMap )
	{
		if((header->cubemapFlags & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
		{
			// partial cube maps are not supported - we require all six faces to be defined
			ptWARN("partial cube maps are not supported");
			return ERR_UNSUPPORTED_FEATURE;
		}
	}

	if( depth > 0 ) {
		ptWARN("volume textures are not supported");
		return ERR_UNSUPPORTED_FEATURE;
	}

	const U32 pixelFormatFlags = header->pixelFormat.flags;

	//const bool hasAlpha = ((pixelFormatFlags & DDPF_ALPHAPIXELS) != 0);
	const U32 ddsFormat = (pixelFormatFlags & DDPF_FOURCC) ? header->pixelFormat.fourCC : pixelFormatFlags;

	const NwDataFormat::Enum engineFormat = FindEngineFormat( ddsFormat );
	if( engineFormat == NwDataFormat::Unknown ) {
		ptWARN("Unknown DDS format: '%d'", ddsFormat);
		return ERR_UNSUPPORTED_FEATURE;
	}

	const DataFormat_t& pixelFormat = g_DataFormats[ engineFormat ];

	const U32 bitsPerPixel = pixelFormat.bitsPerPixel;
	const U32 texelsInBlock = pixelFormat.blockWidth * pixelFormat.blockHeight;
	const U32 blockSizeBytes = texelsInBlock * bitsPerPixel / BITS_IN_BYTE;

	mxUNUSED(bitsPerPixel);
	mxUNUSED(texelsInBlock);
	mxUNUSED(blockSizeBytes);

	dds_image_.data		= mxAddByteOffset( data, offset );
	dds_image_.size		= size - offset;
	dds_image_.format		= engineFormat;
	dds_image_.width		= width;
	dds_image_.height		= height;
	dds_image_.depth		= depth;
	dds_image_.numMips		= mipCount;
	dds_image_.arraySize	= 1;

	dds_image_.type
		= isCubeMap
		? TEXTURE_CUBEMAP
		: ((depth > 1)
		? TEXTURE_3D
		: TEXTURE_2D
		);

	if( bDXT10Header  )
	{
		const DDS_HEADER_DXT10* header10 = (DDS_HEADER_DXT10*) (header + 1);
		dds_image_.arraySize = header10->arraySize;
	}
	return ALL_OK;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
