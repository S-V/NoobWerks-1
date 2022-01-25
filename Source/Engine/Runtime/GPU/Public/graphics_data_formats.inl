// References:
// Texture Block Compression in Direct3D 11:
// https://msdn.microsoft.com/en-us/library/windows/desktop/hh308955(v=vs.85).aspx

#ifdef DECLARE_DATA_FORMAT

//                                       +------------------------------- Bits Per Pixel
//                                       |  +---------------------------- Block width (texels)
//                                       |  |  +------------------------- Block height (texels)
//                                       |  |  |   +--------------------- Block size (in bytes)
//                                       |  |  |   |  +------------------ Min blocks x
//                                       |  |  |   |  |  +--------------- Min blocks y
//                                       |  |  |   |  |  |   +----------- Depth bits
//                                       |  |  |   |  |  |   |  +-------- Stencil bits
//                                       |  |  |   |  |  |   |  |
//                                       |  |  |   |  |  |   |  |
//                                       |  |  |   |  |  |   |  |
//                                       |  |  |   |  |  |   |  |
//                                       |  |  |   |  |  |   |  |
// Uncompressed formats:
DECLARE_DATA_FORMAT( Unknown,            0, 0, 0,  0, 0, 0,  0, 0 )
DECLARE_DATA_FORMAT( RGBA8,             32, 1, 1,  4, 1, 1,  0, 0 )
DECLARE_DATA_FORMAT( R11G11B10_FLOAT,   32, 1, 1,  4, 1, 1,  0, 0 )	// DXGI_FORMAT_R11G11B10_FLOAT
DECLARE_DATA_FORMAT( R10G10B10A2_UNORM, 32, 1, 1,  4, 1, 1,  0, 0 )	// DXGI_FORMAT_R10G10B10A2_UNORM
DECLARE_DATA_FORMAT( R16F,              16, 1, 1,  2, 1, 1,  0, 0 )
DECLARE_DATA_FORMAT( R32F,              32, 1, 1,  4, 1, 1,  0, 0 )
DECLARE_DATA_FORMAT( R16G16_FLOAT,      32, 1, 1,  4, 1, 1,  0, 0 )	// DXGI_FORMAT_R16G16_FLOAT
DECLARE_DATA_FORMAT( R16G16_UNORM,      32, 1, 1,  4, 1, 1,  0, 0 )
DECLARE_DATA_FORMAT( RGBA16F,           64, 1, 1,  8, 1, 1,  0, 0 )	// 4 16-bit floats (s10e5 format, 16-bits per channel)
DECLARE_DATA_FORMAT( RGBA32F,          128, 1, 1, 16, 1, 1,  0, 0 )
// A single-component, 8-bit unsigned-normalized-integer format that supports 8 bits for the red channel.
DECLARE_DATA_FORMAT( R8_UNORM,           8, 1, 1,  1, 1, 1,  0, 0 )
// A single-component, 16-bit unsigned-normalized-integer format.
DECLARE_DATA_FORMAT( R16_UNORM,         16, 1, 1,  2, 1, 1,  0, 0 )
// A single-component, 32-bit unsigned-integer format that supports 32 bits for the red channel. (DXGI_FORMAT_R32_UINT)
DECLARE_DATA_FORMAT( R32_UINT,          32, 1, 1,  4, 1, 1,  0, 0 )
/// R32G32B32_FLOAT - not all hardware supports it
DECLARE_DATA_FORMAT( RGB32F,            96, 1, 1, 12, 1, 1,  0, 0 )

// Block-compressed formats organize texture data in 4x4 texel blocks:
// DXT1 (RGB compression: 8:1, 8 bytes per block)
DECLARE_DATA_FORMAT( BC1,                4, 4, 4,  8, 1, 1,  0, 0 )
// DXT3 (RGBA compression: 4:1, 16 bytes per block)
DECLARE_DATA_FORMAT( BC2,                8, 4, 4, 16, 1, 1,  0, 0 )
// DXT5 (RGBA compression: 4:1, 16 bytes per block)
DECLARE_DATA_FORMAT( BC3,                8, 4, 4, 16, 1, 1,  0, 0 )
// LATC1/ATI1 1 component texture compression (also 3DC+/ATI1N, 8 bytes per block)
DECLARE_DATA_FORMAT( BC4,                4, 4, 4,  8, 1, 1,  0, 0 )
// LATC2/ATI2 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also 3DC/ATI2N, 16 bytes per block)
DECLARE_DATA_FORMAT( BC5,                8, 4, 4, 16, 1, 1,  0, 0 )
// BC6H
DECLARE_DATA_FORMAT( BC6H,               8, 4, 4, 16, 1, 1,  0, 0 )
// BC7
DECLARE_DATA_FORMAT( BC7,                8, 4, 4, 16, 1, 1,  0, 0 )

// Depth-stencil formats:
DECLARE_DATA_FORMAT( D16,               16, 1, 1,  2, 1, 1, 16, 0 )
DECLARE_DATA_FORMAT( D24,               24, 1, 1,  3, 1, 1, 24, 0 )
DECLARE_DATA_FORMAT( D24S8,             32, 1, 1,  3, 1, 1, 24, 8 )

/// A single-component, 32-bit floating-point format that supports 32 bits for depth.
DECLARE_DATA_FORMAT( D32,               32, 1, 1,  4, 1, 1, 32, 0 )


#endif // DECLARE_DATA_FORMAT

// NOTE: make sure gs_textureFormats remains in sync!
