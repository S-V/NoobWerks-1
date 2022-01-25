// 
#include <Base/Base.h>
#pragma hdrstop

//
#include <Core/Memory/MemoryHeaps.h>
//
#include <Planets/PlanetsCommon.h>


namespace Planets
{

PlanetHeightmap::PlanetHeightmap( AllocatorI& allocator )
	: pixels_of_all_mipmaps( allocator )
{
}

ERet PlanetHeightmap::loadFromFile( const char* filepath )
{
	FileReader	file;
	mxDO(file.Open(filepath));

	HeightmapInfo	heightmap_info;
	mxDO(file.Get(heightmap_info));

	//
	HeightmapInfo *dst_heightmap_info = this;
	*dst_heightmap_info = heightmap_info;

	file >> pixels_of_all_mipmaps;

	DEVOUT("Loaded heightmap from '%s' (%.2f MiB).",
		filepath, (float)getUsedMemory() / mxMEGABYTE
		);

	return ALL_OK;
}

ERet PlanetHeightmap::saveToFile( const char* filepath ) const
{
	DBGOUT("Saving heightmap to '%s'...",
		filepath
		);

	FileWriter	file;
	mxDO(file.Open(filepath));

	const HeightmapInfo	heightmap_info = *this;
	mxDO(file.Put(heightmap_info));

	file << pixels_of_all_mipmaps;

	return ALL_OK;
}

size_t PlanetHeightmap::getUsedMemory() const
{
	return sizeof(*this) + pixels_of_all_mipmaps.rawSize();
}

void PlanetHeightmap::clear()
{
	HeightmapInfo * heightmap_info = this;
	mxZERO_OUT( *heightmap_info );

	pixels_of_all_mipmaps.clear();
}

}//namespace Planets


NwStar::NwStar()
{
	//radius_in_Solar_radii = 1.0f;
	radius_in_meters = 1000.0f;
	mass_in_Solar_masses = 1.0f;
}


/*
	//
#if 0
	//double distance_from_star_to_planet_in_meters = 1e5;	// 100 km
	double distance_from_star_to_planet_in_meters = 2000 * 1000; // 2 000 km
	_main_star.pos_in_world_space.xyz = CV3d(distance_from_star_to_planet_in_meters);
	_main_star.radius_in_meters = 1000*1000;	// 1000 km
	//_main_star.radius_in_Solar_radii = 0.01f;

#else

	//
	double distance_from_star_to_planet_in_meters = 1.49e11;

	_main_star.pos_in_world_space = CV3d(distance_from_star_to_planet_in_meters);

	//
	double sun_radius_in_meters = 696340*1000;	// 696,340 km

	_main_star.radius_in_meters = sun_radius_in_meters
		* 1//00
		;	// 696,340 km

#endif
	*/