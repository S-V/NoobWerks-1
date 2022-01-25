#pragma once

#include <Utility/Meshok/SDF.h>

///////////////////////////////////////////////////////////////////////////////
//	Copyright (C) 2011 Robert Engeln (engeln@gmail.com) All rights reserved.
//	See accompanying LICENSE file for full license information.
///////////////////////////////////////////////////////////////////////////////

//
//  Simplex noise implementation.
//
//  Based on http://webstaff.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf
//
class Noise : public NonCopyable {
public:
    //
    //  Constructor.
    //
    Noise();

    //
    //  Destructor.
    //
    ~Noise();

    //
    //  Samples noise in 3D space.
    //
    float Sample(float x, float y, float z) const;

    //
    //  Samples noise in 3D space and normalizes the result to a given range.
    //
    float SampleRange(float x, float y, float z, float a, float b) const;

    //
    //  Fills a 3D grid with noise.
    //
    //  Parameters:
    //      [in] ptr
    //          Pointer to the destination grid.
    //      [in] x, y, z
    //          Grid dimensions.
    //      [in] xf, yf, zf
    //          Frequency to sample in each direction.
    //
    void Fill(float* ptr, size_t x, size_t y, size_t z, float xf, float yf, float zf) const;

private:
    //
    //  Properties.
    //
    static const V3f GradientVectors[];
    static const int Permutations[];

    int m_permutationTable[512];
};

/*
http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
  function IntNoise(32-bit integer: x)			 

    x = (x<<13) ^ x;
    return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 7fffffff) / 1073741824.0);    

  end IntNoise function
*/


mxSTOLEN("https://github.com/azarus/TerrainGenerator/blob/master/Source/TerrainGenerator/SimplexNoise.h")
namespace SimplexNoise
{
	// Simplex noise
	float OctaveNoise2D(const float octaves, const float persistence, const float scale, const float x, const float y);
	// Simplex noise
	float OctaveNoise3D(const float octaves, const float persistence, const float scale, const float x, const float y, const float z);
	// Simplex noise
	float OctaveNoise4D(const float octaves, const float persistence, const float scale, const float x, const float y, const float z, const float w);

	// Scaled Simplex noise
	float ScaledNoise2D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y);
	// Scaled Simplex noise
	float ScaledNoise3D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y, const float z);
	// Scaled Simplex noise
	float ScaledNoise4D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y, const float z, const float w);

	// Scaled Raw Simplex noise
	float ScaledRawNoise2D(const float loBound, const float hiBound, const float x, const float y);
	// Scaled Raw Simplex noise
	float ScaledRawNoise3D(const float loBound, const float hiBound, const float x, const float y, const float z);
	// Scaled Raw Simplex noise
	float ScaledRawNoise4D(const float loBound, const float hiBound, const float x, const float y, const float z, const float w);


	// Raw Simplex noise - a single noise value.
	float RawNoise2D(const float x, const float y);
	// Raw Simplex noise - a single noise value.
	float RawNoise3D(const float x, const float y, const float z);
	// Raw Simplex noise - a single noise value.
	float RawNoise4D(const float x, const float y, const float z, const float w);

	void CreatePermutationTable(int seed);

	int fastfloor(const float x);

	float dot(const int* g, const float x, const float y);
	float dot(const int* g, const float x, const float y, const float z);
	float dot(const int* g, const float x, const float y, const float z, const float w);
}


// "Microsoft Visual C++ 12.0" (2013)
#if ( _MSC_VER >= 1800 )

// github.com/nsf/mc

struct Noise3D {
	V3f m_gradients[256];
	int  m_permutations[256];

	explicit Noise3D(int seed);
	V3f get_gradient(int x, int y, int z) const;
	void get_gradients(V3f *origins, V3f *grads,
		float x, float y, float z) const;

	float get(float x, float y, float z) const;
};

struct Noise2D {
	V2f m_gradients[256];
	int  m_permutations[256];

	explicit Noise2D(int seed);
	V2f get_gradient(int x, int y) const;
	void get_gradients(V2f *origins, V2f *grads, float x, float y) const;
	float get(float x, float y) const;
};

#endif // ( _MSC_VER >= 1800 )


namespace SDF
{
	struct SimplexTerrain2D : SDF::Isosurface
	{
		V3f	scale;
	public:
		SimplexTerrain2D()
		{
			scale = V3_SetAll(1);
		}
		virtual F32 DistanceTo( const V3f& _position ) const override;
	};

	struct HillTerrain2D : SDF::Isosurface
	{
	public:
		HillTerrain2D()
		{
		}
		virtual F32 DistanceTo( const V3f& _position ) const override;
	};
}//namespace SDF

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
