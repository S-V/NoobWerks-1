// 
#pragma once

#include <Rendering/Public/Scene/CameraView.h>	// NwLargePosition


namespace Planets
{


enum HeightmapConstants
{
	MAX_HEIGHTMAP_MIPS = 16,

	// the planetary voxel engine has minimum resolution of 8 cells per chunk,
	// no need to generate the full mip chain down to 1x1
	COARSEST_MIP_RESOLUTION = 8,
};


#pragma pack (push,1)
struct HeightmapInfo
{
	// the resolution of the most detailed mip level
	U8	log2_of_mip0_width;
	U8	log2_of_mip1_height;

	//
	U8	num_mipmaps;
	U8	padding;

	//
	struct MipInfo
	{
		U32		data_offset_from_start;
		U16		width;
		U16		height;
	};

	// NB: the coarsest mip is first
	MipInfo		mipmaps[ MAX_HEIGHTMAP_MIPS ];
};
#pragma pack (pop)


/// cylindrical/equirectangular projection
class PlanetHeightmap: NonCopyable, public HeightmapInfo
{
public:
	typedef U8 PixelType;

	DynamicArray< PixelType >	pixels_of_all_mipmaps;

public:
	PlanetHeightmap( AllocatorI& allocator );

	ERet loadFromFile( const char* filepath );
	ERet saveToFile( const char* filepath ) const;

	size_t getUsedMemory() const;

	void clear();

public:
	float sample_Slow( float u, float v, U8 mip_index ) const
	{
		mxASSERT(mip_index < num_mipmaps);
		const MipInfo& mip_info = mipmaps[ mip_index ];
		const U32 iU = U32( u * mip_info.width ) & ( mip_info.width - 1 );
		const U32 iV = U32( v * mip_info.height ) & ( mip_info.height - 1 );
		const U32 pixel_index = ( iV * mip_info.width ) + iU;
		mxASSERT( pixel_index < mip_info.width * mip_info.height );
		const PixelType pixel = pixels_of_all_mipmaps[ mip_info.data_offset_from_start + pixel_index ];
		return 1.0f - pixel * (1.0f/255.0f);
	}
};



//
struct PlanetParametersA
{
	V3d		center_in_world_space;
	double	radius_in_world_units;
};








//
struct PrecomputedPlanetConsts
{
	V3d		center_in_world_space;

	double	radius_squared_in_world_units;
	double	radius_in_world_units;

	U16		default_solid_material;

public:
	PrecomputedPlanetConsts()
	{
		center_in_world_space = CV3d(0);
		radius_squared_in_world_units = 0;
		radius_in_world_units = 0;
		default_solid_material = 1;
	}

	void setRadius( double new_radius_in_world_units )
	{
		radius_in_world_units = new_radius_in_world_units;
		radius_squared_in_world_units = Square( new_radius_in_world_units );
	}
};

///
struct PlanetProcGenData: NonCopyable
{
	BSphereD	sphere_zero_height;	// 32
	//BSphereD	sphere_max_height;

	///
	double	max_height;	// 8

	//PrecomputedPlanetConsts		parameters;
	PlanetHeightmap				heightmap;

	/// heightmap resolution at most detailed LoD := equator length / texture width
	double	texel_size;

	void *	user_data;

public:
	PlanetProcGenData( AllocatorI & allocator = MemoryHeaps::global() )
		: heightmap( allocator )
	{
		texel_size = 1;
		max_height = 0;
		user_data = nil;
	}

	void updateHeightmapResolution(
		double planet_radius_in_meters
		, int texture_width
		)
	{
		const double equator_length_in_meters = ( mxTWO_PI * planet_radius_in_meters );
		texel_size = equator_length_in_meters / texture_width;
	}
};



/// Cached band-limited noise values.
struct CachedNoise
{
	DynamicArray< float >	noise_values;
};


}//namespace Planets






/*
Useful:
http://spaceengine.org/manual/making-addons/creating-a-star/
*/
class NwStar
{
public:
	Rendering::NwLargePosition	pos_in_world_space;

	/// The Sun's radius is about 696,340 km
	//float	radius_in_Solar_radii;
	double	radius_in_meters;

	/// The Sun's radius is about 1.989 x 10^30 kg
	float	mass_in_Solar_masses;

	/// 5,778 K
	//float	temp;	//surface temperature in Kelvin

public:
	NwStar();
};
