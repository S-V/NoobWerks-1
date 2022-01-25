#include "stdafx.h"
#pragma hdrstop
#include <Meshok/Meshok.h>
#include <Meshok/Noise.h>

#include <time.h>
#include <random>	// std::default_random_engine

///////////////////////////////////////////////////////////////////////////////
//	Copyright (C) 2011 Robert Engeln (engeln@gmail.com) All rights reserved.
//	See accompanying LICENSE file for full license information.
///////////////////////////////////////////////////////////////////////////////

const V3f Noise::GradientVectors[] = 
{
    CV3f(1,1,0), CV3f(-1,1,0), CV3f(1,-1,0), CV3f(-1,-1,0),
    CV3f(1,0,1), CV3f(-1,0,1), CV3f(1,0,-1), CV3f(-1,0,-1),
    CV3f(0,1,1), CV3f(0,-1,1), CV3f(0,1,-1), CV3f(0,-1,-1)
};

const int Noise::Permutations[] = 
{
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

Noise::Noise()
{
    srand(time(0));
    int shuffledPerms[256];
    memcpy(shuffledPerms, Permutations, sizeof(Permutations));
    for (size_t i = 255; i > 0; --i)
    {
        size_t j = rand() % (i + 1);
        std::swap(shuffledPerms[j], shuffledPerms[i]);
    }
    for (UINT i = 0; i < 512; i++) {
        m_permutationTable[i] = shuffledPerms[i & 255];
    }
}

Noise::~Noise()
{
}

float Noise::SampleRange(float x, float y, float z, float a, float b) const
{
    float n = (1.0f + Sample(x, y, z)) * 0.5f;
    return a + (n * (b - a));
}

float Noise::Sample(float x, float y, float z) const
{
    const float F3 = 1.0f / 3.0f, G3 = 1.0f / 6.0f;
    float n0, n1, n2, n3;
    float s = (x + y + z) * F3;
    int i = (int)floor(x + s),
        j = (int)floor(y + s),
        k = (int)floor(z + s);
    float t = (float)(i + j + k) * G3;
    float X0 = (float)i - t,
          Y0 = (float)j - t,
          Z0 = (float)k - t,
          x0 = x - X0,
          y0 = y - Y0,
          z0 = z - Z0;

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0) {
            i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; // X Y Z order
        }
        else if (x0 >= z0) {
            i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; // X Z Y order
        }
        else { 
            i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; // Z X Y order
        } 
    } else { // x0<y0
        if (y0 < z0) { 
            i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; // Z Y X order
        }
        else if (x0 < z0) { 
            i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; // Y Z X order
        }
        else { 
            i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; // Y X Z order
        }
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.    
    float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int gi0 = m_permutationTable[ii+m_permutationTable[jj+m_permutationTable[kk]]] % 12;
    int gi1 = m_permutationTable[ii+i1+m_permutationTable[jj+j1+m_permutationTable[kk+k1]]] % 12;
    int gi2 = m_permutationTable[ii+i2+m_permutationTable[jj+j2+m_permutationTable[kk+k2]]] % 12;
    int gi3 = m_permutationTable[ii+1+m_permutationTable[jj+1+m_permutationTable[kk+1]]] % 12;

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0) {
        n0 = 0.0;
    }
    else {
      t0 *= t0;
      n0 = t0 * t0 * (GradientVectors[gi0].x * x0 + 
                      GradientVectors[gi0].y * y0 +
                      GradientVectors[gi0].z * z0);
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0) {
        n1 = 0.0;
    }
    else {
        t1 *= t1;
        n1 = t1 * t1 * (GradientVectors[gi1].x * x1 + 
                        GradientVectors[gi1].y * y1 +
                        GradientVectors[gi1].z * z1);
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0) {
        n2 = 0.0;
    }
    else {
        t2 *= t2;
        n2 = t2 * t2 * (GradientVectors[gi2].x * x2 + 
                        GradientVectors[gi2].y * y2 +
                        GradientVectors[gi2].z * z2);
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0) {
        n3 = 0.0;
    }
    else {
        t3 *= t3;
        n3 = t3 * t3 * (GradientVectors[gi3].x * x3 + 
                        GradientVectors[gi3].y * y3 +
                        GradientVectors[gi3].z * z3);
    }

    float r = 32.0f * (n0 + n1 + n2 + n3);
    return r;
}

void Noise::Fill(float* ptr,
                 size_t x,
                 size_t y,
                 size_t z,
                 float xf,
                 float yf,
                 float zf) const
{
    for (size_t i = 0; i < x; ++i)
    {
        for (size_t j = 0; j < y; ++j)
        {
            for (size_t k = 0; k < z; ++k)
            {
                *ptr++ = SampleRange(static_cast<float>(i) * xf,
                                     static_cast<float>(j) * yf,
                                     static_cast<float>(k) * zf,
                                     0.0f,
                                     1.0f);
            }
        }
    }
}

// The gradients are the midpoints of the vertices of a cube.
static const int grad3[12][3] = {
	{ 1, 1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { -1, -1, 0 },
	{ 1, 0, 1 }, { -1, 0, 1 }, { 1, 0, -1 }, { -1, 0, -1 },
	{ 0, 1, 1 }, { 0, -1, 1 }, { 0, 1, -1 }, { 0, -1, -1 }
};


// The gradients are the midpoints of the vertices of a hypercube.
static const int grad4[32][4] = {
	{ 0, 1, 1, 1 }, { 0, 1, 1, -1 }, { 0, 1, -1, 1 }, { 0, 1, -1, -1 },
	{ 0, -1, 1, 1 }, { 0, -1, 1, -1 }, { 0, -1, -1, 1 }, { 0, -1, -1, -1 },
	{ 1, 0, 1, 1 }, { 1, 0, 1, -1 }, { 1, 0, -1, 1 }, { 1, 0, -1, -1 },
	{ -1, 0, 1, 1 }, { -1, 0, 1, -1 }, { -1, 0, -1, 1 }, { -1, 0, -1, -1 },
	{ 1, 1, 0, 1 }, { 1, 1, 0, -1 }, { 1, -1, 0, 1 }, { 1, -1, 0, -1 },
	{ -1, 1, 0, 1 }, { -1, 1, 0, -1 }, { -1, -1, 0, 1 }, { -1, -1, 0, -1 },
	{ 1, 1, 1, 0 }, { 1, 1, -1, 0 }, { 1, -1, 1, 0 }, { 1, -1, -1, 0 },
	{ -1, 1, 1, 0 }, { -1, 1, -1, 0 }, { -1, -1, 1, 0 }, { -1, -1, -1, 0 }
};


// Permutation table.  The same list is repeated twice.
int perm[512] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
	8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
	35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
	134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
	55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
	18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
	250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
	189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
	172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
	228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
	107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,

	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
	8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
	35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
	134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
	55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
	18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
	250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
	189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
	172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
	228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
	107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};


// A lookup table to traverse the simplex around a given point in 4D.
static const int simplex[64][4] = {
	{ 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 0, 0, 0 }, { 0, 2, 3, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 1, 2, 3, 0 },
	{ 0, 2, 1, 3 }, { 0, 0, 0, 0 }, { 0, 3, 1, 2 }, { 0, 3, 2, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 1, 3, 2, 0 },
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
	{ 1, 2, 0, 3 }, { 0, 0, 0, 0 }, { 1, 3, 0, 2 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 2, 3, 0, 1 }, { 2, 3, 1, 0 },
	{ 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 2, 0, 3, 1 }, { 0, 0, 0, 0 }, { 2, 1, 3, 0 },
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
	{ 2, 0, 1, 3 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 3, 0, 1, 2 }, { 3, 0, 2, 1 }, { 0, 0, 0, 0 }, { 3, 1, 2, 0 },
	{ 2, 1, 0, 3 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 3, 1, 0, 2 }, { 0, 0, 0, 0 }, { 3, 2, 0, 1 }, { 3, 2, 1, 0 }
};

namespace SimplexNoise
{


	void CreatePermutationTable(int seed)
	{
		srand(seed);
		for (int i = 0; i < 512; ++i)
		{
			perm[i] = rand()%500;
		}

	}

	float OctaveNoise2D(const float octaves, const float persistence, const float scale, const float x, const float y)
	{
		float total = 0;
		float frequency = scale;
		float amplitude = 1;

		// We have to keep track of the largest possible amplitude,
		// because each octave adds more, and we need a value in [-1, 1].
		float maxAmplitude = 0;

		for (int i = 0; i < octaves; i++) {
			total += RawNoise2D(x * frequency, y * frequency) * amplitude;

			frequency *= 2;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;
	}


	// 3D Multi-octave Simplex noise.
	//
	// For each octave, a higher frequency/lower amplitude function will be added to the original.
	// The higher the persistence [0-1], the more of each succeeding octave will be added.
	float OctaveNoise3D(const float octaves, const float persistence, const float scale, const float x, const float y, const float z)
	{
		float total = 0;
		float frequency = scale;
		float amplitude = 1;

		// We have to keep track of the largest possible amplitude,
		// because each octave adds more, and we need a value in [-1, 1].
		float maxAmplitude = 0;

		for (int i = 0; i < octaves; i++) {
			total += RawNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;

			frequency *= 2;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;
	}


	// 4D Multi-octave Simplex noise.
	//
	// For each octave, a higher frequency/lower amplitude function will be added to the original.
	// The higher the persistence [0-1], the more of each succeeding octave will be added.
	float OctaveNoise4D(const float octaves, const float persistence, const float scale, const float x, const float y, const float z, const float w)
	{
		float total = 0;
		float frequency = scale;
		float amplitude = 1;

		// We have to keep track of the largest possible amplitude,
		// because each octave adds more, and we need a value in [-1, 1].
		float maxAmplitude = 0;

		for (int i = 0; i < octaves; i++) {
			total += RawNoise4D(x * frequency, y * frequency, z * frequency, w * frequency) * amplitude;

			frequency *= 2;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;
	}



	// 2D Scaled Multi-octave Simplex noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledNoise2D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y)
	{
		return OctaveNoise2D(octaves, persistence, scale, x, y) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}


	// 3D Scaled Multi-octave Simplex noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledNoise3D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y, const float z)
	{
		return OctaveNoise3D(octaves, persistence, scale, x, y, z) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}

	// 4D Scaled Multi-octave Simplex noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledNoise4D(const float octaves, const float persistence, const float scale, const float loBound, const float hiBound, const float x, const float y, const float z, const float w)
	{
		return OctaveNoise4D(octaves, persistence, scale, x, y, z, w) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}



	// 2D Scaled Simplex raw noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledRawNoise2D(const float loBound, const float hiBound, const float x, const float y)
	{
		return RawNoise2D(x, y) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}


	// 3D Scaled Simplex raw noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledRawNoise3D(const float loBound, const float hiBound, const float x, const float y, const float z)
	{
		return RawNoise3D(x, y, z) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}

	// 4D Scaled Simplex raw noise.
	//
	// Returned value will be between loBound and hiBound.
	float ScaledRawNoise4D(const float loBound, const float hiBound, const float x, const float y, const float z, const float w)
	{
		return RawNoise4D(x, y, z, w) * (hiBound - loBound) / 2 + (hiBound + loBound) / 2;
	}



	// 2D raw Simplex noise
	float RawNoise2D(const float x, const float y)
	{
		// Noise contributions from the three corners
		float n0, n1, n2;

		// Skew the input space to determine which simplex cell we're in
		float F2 = 0.5 * (sqrtf(3.0) - 1.0);
		// Hairy factor for 2D
		float s = (x + y) * F2;
		int i = fastfloor(x + s);
		int j = fastfloor(y + s);

		float G2 = (3.0 - sqrtf(3.0)) / 6.0;
		float t = (i + j) * G2;
		// Unskew the cell origin back to (x,y) space
		float X0 = i - t;
		float Y0 = j - t;
		// The x,y distances from the cell origin
		float x0 = x - X0;
		float y0 = y - Y0;

		// For the 2D case, the simplex shape is an equilateral triangle.
		// Determine which simplex we are in.
		int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
		if (x0 > y0) { i1 = 1; j1 = 0; } // lower triangle, XY order: (0,0)->(1,0)->(1,1)
		else { i1 = 0; j1 = 1; } // upper triangle, YX order: (0,0)->(0,1)->(1,1)

		// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
		// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
		// c = (3-sqrt(3))/6
		float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
		float y1 = y0 - j1 + G2;
		float x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
		float y2 = y0 - 1.0 + 2.0 * G2;

		// Work out the hashed gradient indices of the three simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int gi0 = perm[ii + perm[jj]] % 12;
		int gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
		int gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

		// Calculate the contribution from the three corners
		float t0 = 0.5 - x0*x0 - y0*y0;
		if (t0 < 0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad3[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
		}

		float t1 = 0.5 - x1*x1 - y1*y1;
		if (t1 < 0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
		}

		float t2 = 0.5 - x2*x2 - y2*y2;
		if (t2 < 0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
		}

		// add contributions from each corner to get the final noise value.
		// The result is scaled to return values in the interval [-1,1].
		return 70.0 * (n0 + n1 + n2);
	}


	// 3D raw Simplex noise
	float RawNoise3D(const float x, const float y, const float z)
	{
		float n0, n1, n2, n3; // Noise contributions from the four corners

		// Skew the input space to determine which simplex cell we're in
		float F3 = 1.0 / 3.0;
		float s = (x + y + z)*F3; // Very nice and simple skew factor for 3D
		int i = fastfloor(x + s);
		int j = fastfloor(y + s);
		int k = fastfloor(z + s);

		float G3 = 1.0 / 6.0; // Very nice and simple unskew factor, too
		float t = (i + j + k)*G3;
		float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
		float Y0 = j - t;
		float Z0 = k - t;
		float x0 = x - X0; // The x,y,z distances from the cell origin
		float y0 = y - Y0;
		float z0 = z - Z0;

		// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
		// Determine which simplex we are in.
		int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
		int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

		if (x0 >= y0) {
			if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // X Y Z order
			else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; } // X Z Y order
			else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; } // Z X Y order
		}
		else { // x0<y0
			if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; } // Z Y X order
			else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; } // Y Z X order
			else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // Y X Z order
		}

		// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
		// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
		// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
		// c = 1/6.
		float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
		float y1 = y0 - j1 + G3;
		float z1 = z0 - k1 + G3;
		float x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
		float y2 = y0 - j2 + 2.0*G3;
		float z2 = z0 - k2 + 2.0*G3;
		float x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
		float y3 = y0 - 1.0 + 3.0*G3;
		float z3 = z0 - 1.0 + 3.0*G3;

		// Work out the hashed gradient indices of the four simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int kk = k & 255;
		int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
		int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
		int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
		int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

		// Calculate the contribution from the four corners
		float t0 = 0.6 - x0*x0 - y0*y0 - z0*z0;
		if (t0 < 0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
		}

		float t1 = 0.6 - x1*x1 - y1*y1 - z1*z1;
		if (t1 < 0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
		}

		float t2 = 0.6 - x2*x2 - y2*y2 - z2*z2;
		if (t2 < 0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
		}

		float t3 = 0.6 - x3*x3 - y3*y3 - z3*z3;
		if (t3 < 0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
		}

		// add contributions from each corner to get the final noise value.
		// The result is scaled to stay just inside [-1,1]
		return 32.0*(n0 + n1 + n2 + n3);
	}


	// 4D raw Simplex noise
	float RawNoise4D(const float x, const float y, const float z, const float w)
	{
		// The skewing and unskewing factors are hairy again for the 4D case
		float F4 = (sqrtf(5.0) - 1.0) / 4.0;
		float G4 = (5.0 - sqrtf(5.0)) / 20.0;
		float n0, n1, n2, n3, n4; // Noise contributions from the five corners

		// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
		float s = (x + y + z + w) * F4; // Factor for 4D skewing
		int i = fastfloor(x + s);
		int j = fastfloor(y + s);
		int k = fastfloor(z + s);
		int l = fastfloor(w + s);
		float t = (i + j + k + l) * G4; // Factor for 4D unskewing
		float X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
		float Y0 = j - t;
		float Z0 = k - t;
		float W0 = l - t;

		float x0 = x - X0; // The x,y,z,w distances from the cell origin
		float y0 = y - Y0;
		float z0 = z - Z0;
		float w0 = w - W0;

		// For the 4D case, the simplex is a 4D shape I won't even try to describe.
		// To find out which of the 24 possible simplices we're in, we need to
		// determine the magnitude ordering of x0, y0, z0 and w0.
		// The method below is a good way of finding the ordering of x,y,z,w and
		// then find the correct traversal order for the simplex we're in.
		// First, six pair-wise comparisons are performed between each possible pair
		// of the four coordinates, and the results are used to add up binary bits
		// for an integer index.
		int c1 = (x0 > y0) ? 32 : 0;
		int c2 = (x0 > z0) ? 16 : 0;
		int c3 = (y0 > z0) ? 8 : 0;
		int c4 = (x0 > w0) ? 4 : 0;
		int c5 = (y0 > w0) ? 2 : 0;
		int c6 = (z0 > w0) ? 1 : 0;
		int c = c1 + c2 + c3 + c4 + c5 + c6;

		int i1, j1, k1, l1; // The integer offsets for the second simplex corner
		int i2, j2, k2, l2; // The integer offsets for the third simplex corner
		int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner

		// simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
		// Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
		// impossible. Only the 24 indices which have non-zero entries make any sense.
		// We use a thresholding to set the coordinates in turn from the largest magnitude.
		// The number 3 in the "simplex" array is at the position of the largest coordinate.
		i1 = simplex[c][0] >= 3 ? 1 : 0;
		j1 = simplex[c][1] >= 3 ? 1 : 0;
		k1 = simplex[c][2] >= 3 ? 1 : 0;
		l1 = simplex[c][3] >= 3 ? 1 : 0;
		// The number 2 in the "simplex" array is at the second largest coordinate.
		i2 = simplex[c][0] >= 2 ? 1 : 0;
		j2 = simplex[c][1] >= 2 ? 1 : 0;
		k2 = simplex[c][2] >= 2 ? 1 : 0;
		l2 = simplex[c][3] >= 2 ? 1 : 0;
		// The number 1 in the "simplex" array is at the second smallest coordinate.
		i3 = simplex[c][0] >= 1 ? 1 : 0;
		j3 = simplex[c][1] >= 1 ? 1 : 0;
		k3 = simplex[c][2] >= 1 ? 1 : 0;
		l3 = simplex[c][3] >= 1 ? 1 : 0;
		// The fifth corner has all coordinate offsets = 1, so no need to look that up.

		float x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
		float y1 = y0 - j1 + G4;
		float z1 = z0 - k1 + G4;
		float w1 = w0 - l1 + G4;
		float x2 = x0 - i2 + 2.0*G4; // Offsets for third corner in (x,y,z,w) coords
		float y2 = y0 - j2 + 2.0*G4;
		float z2 = z0 - k2 + 2.0*G4;
		float w2 = w0 - l2 + 2.0*G4;
		float x3 = x0 - i3 + 3.0*G4; // Offsets for fourth corner in (x,y,z,w) coords
		float y3 = y0 - j3 + 3.0*G4;
		float z3 = z0 - k3 + 3.0*G4;
		float w3 = w0 - l3 + 3.0*G4;
		float x4 = x0 - 1.0 + 4.0*G4; // Offsets for last corner in (x,y,z,w) coords
		float y4 = y0 - 1.0 + 4.0*G4;
		float z4 = z0 - 1.0 + 4.0*G4;
		float w4 = w0 - 1.0 + 4.0*G4;

		// Work out the hashed gradient indices of the five simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int kk = k & 255;
		int ll = l & 255;
		int gi0 = perm[ii + perm[jj + perm[kk + perm[ll]]]] % 32;
		int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]] % 32;
		int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]] % 32;
		int gi3 = perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]] % 32;
		int gi4 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]] % 32;

		// Calculate the contribution from the five corners
		float t0 = 0.6 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
		if (t0 < 0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad4[gi0], x0, y0, z0, w0);
		}

		float t1 = 0.6 - x1*x1 - y1*y1 - z1*z1 - w1*w1;
		if (t1 < 0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad4[gi1], x1, y1, z1, w1);
		}

		float t2 = 0.6 - x2*x2 - y2*y2 - z2*z2 - w2*w2;
		if (t2 < 0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad4[gi2], x2, y2, z2, w2);
		}

		float t3 = 0.6 - x3*x3 - y3*y3 - z3*z3 - w3*w3;
		if (t3 < 0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * dot(grad4[gi3], x3, y3, z3, w3);
		}

		float t4 = 0.6 - x4*x4 - y4*y4 - z4*z4 - w4*w4;
		if (t4 < 0) n4 = 0.0;
		else {
			t4 *= t4;
			n4 = t4 * t4 * dot(grad4[gi4], x4, y4, z4, w4);
		}

		// Sum up and scale the result to cover the range [-1,1]
		return 27.0 * (n0 + n1 + n2 + n3 + n4);
	}


	int fastfloor(const float x) { return x > 0 ? (int)x : (int)x - 1; }

	float dot(const int* g, const float x, const float y) { return g[0] * x + g[1] * y; }
	float dot(const int* g, const float x, const float y, const float z) { return g[0] * x + g[1] * y + g[2] * z; }
	float dot(const int* g, const float x, const float y, const float z, const float w) { return g[0] * x + g[1] * y + g[2] * z + g[3] * w; }
}

// "Microsoft Visual C++ 12.0" (2013)
#if ( _MSC_VER >= 1800 )

static inline float lerp10(float a, float b, float v)
{
	return a * (1 - v) + b * v;
}

static inline float Gradient(const V3f &orig, const V3f &grad, const V3f &p)
{
	return V3_Dot(grad, p - orig);
}

static inline float Gradient(const V2f &orig, const V2f &grad, const V2f &p)
{
	return V2_Dot(grad, p - orig);
}

template <typename Rnd>
static inline V3f RandomGradient3D(Rnd &g)
{
	std::uniform_real_distribution<float> _2pi(0.0f, mxPI * 2.0f);
	const float x = _2pi(g);
	const float angle = _2pi(g);
	return V3_Set( cosf(angle) * cosf(x), sinf(angle) * cosf(x), sinf(x) );
}

template <typename Rnd>
static inline V2f RandomGradient2D(Rnd &g)
{
	std::uniform_real_distribution<float> _2pi(0.0f, mxPI * 2.0f);
	const float angle = _2pi(g);
	return V2_Set( cosf(angle), sinf(angle) );
}

Noise3D::Noise3D(int seed)
{
	std::default_random_engine rnd(seed);
	for( int i = 0; i < mxCOUNT_OF(m_gradients); i++ ) {
		m_gradients[i] = RandomGradient3D(rnd);
	}

	for (int i = 0; i < 256; i++) {
		int j = std::uniform_int_distribution<int>(0, i)(rnd);
		m_permutations[i] = m_permutations[j];
		m_permutations[j] = i;
	}
}

V3f Noise3D::get_gradient(int x, int y, int z) const
{
	int idx =
		m_permutations[x & 255] +
		m_permutations[y & 255] +
		m_permutations[z & 255];
	return m_gradients[idx & 255];
}

void Noise3D::get_gradients(V3f *origins, V3f *grads,
	float x, float y, float z) const
{
	float x0f = std::floor(x);
	float y0f = std::floor(y);
	float z0f = std::floor(z);
	int x0 = x0f;
	int y0 = y0f;
	int z0 = z0f;
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;

	grads[0] = get_gradient(x0, y0, z0);
	grads[1] = get_gradient(x0, y0, z1);
	grads[2] = get_gradient(x0, y1, z0);
	grads[3] = get_gradient(x0, y1, z1);
	grads[4] = get_gradient(x1, y0, z0);
	grads[5] = get_gradient(x1, y0, z1);
	grads[6] = get_gradient(x1, y1, z0);
	grads[7] = get_gradient(x1, y1, z1);

	origins[0] = V3_Set( x0f + 0.0f, y0f + 0.0f, z0f + 0.0f );
	origins[1] = V3_Set( x0f + 0.0f, y0f + 0.0f, z0f + 1.0f );
	origins[2] = V3_Set( x0f + 0.0f, y0f + 1.0f, z0f + 0.0f );
	origins[3] = V3_Set( x0f + 0.0f, y0f + 1.0f, z0f + 1.0f );
	origins[4] = V3_Set( x0f + 1.0f, y0f + 0.0f, z0f + 0.0f );
	origins[5] = V3_Set( x0f + 1.0f, y0f + 0.0f, z0f + 1.0f );
	origins[6] = V3_Set( x0f + 1.0f, y0f + 1.0f, z0f + 0.0f );
	origins[7] = V3_Set( x0f + 1.0f, y0f + 1.0f, z0f + 1.0f );
}

float Noise3D::get(float x, float y, float z) const
{
	V3f origins[8];
	V3f grads[8];

	get_gradients(origins, grads, x, y, z);
	float vals[] = {
		Gradient(origins[0], grads[0], V3_Set( x, y, z )),
		Gradient(origins[1], grads[1], V3_Set( x, y, z )),
		Gradient(origins[2], grads[2], V3_Set( x, y, z )),
		Gradient(origins[3], grads[3], V3_Set( x, y, z )),
		Gradient(origins[4], grads[4], V3_Set( x, y, z )),
		Gradient(origins[5], grads[5], V3_Set( x, y, z )),
		Gradient(origins[6], grads[6], V3_Set( x, y, z )),
		Gradient(origins[7], grads[7], V3_Set( x, y, z )),
	};

	float fz = SmoothStep(z - origins[0].z);
	float vz0 = lerp10(vals[0], vals[1], fz);
	float vz1 = lerp10(vals[2], vals[3], fz);
	float vz2 = lerp10(vals[4], vals[5], fz);
	float vz3 = lerp10(vals[6], vals[7], fz);

	float fy = SmoothStep(y - origins[0].y);
	float vy0 = lerp10(vz0, vz1, fy);
	float vy1 = lerp10(vz2, vz3, fy);

	float fx = SmoothStep(x - origins[0].x);
	return lerp10(vy0, vy1, fx);
}

Noise2D::Noise2D(int seed)
{
	std::default_random_engine rnd(seed);
	for( int i = 0; i < mxCOUNT_OF(m_gradients); i++ ) {
		m_gradients[i] = RandomGradient2D(rnd);
	}

	for (int i = 0; i < 256; i++) {
		int j = std::uniform_int_distribution<int>(0, i)(rnd);
		m_permutations[i] = m_permutations[j];
		m_permutations[j] = i;
	}
}

V2f Noise2D::get_gradient(int x, int y) const
{
	int idx =
		m_permutations[x & 255] +
		m_permutations[y & 255];
	return m_gradients[idx & 255];
}

void Noise2D::get_gradients(V2f *origins, V2f *grads, float x, float y) const
{
	float x0f = floorf(x);
	float y0f = floorf(y);
	int x0 = x0f;
	int y0 = y0f;
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	grads[0] = get_gradient(x0, y0);
	grads[1] = get_gradient(x1, y0);
	grads[2] = get_gradient(x0, y1);
	grads[3] = get_gradient(x1, y1);

	origins[0] = V2_Set( x0f + 0.0f, y0f + 0.0f );
	origins[1] = V2_Set( x0f + 1.0f, y0f + 0.0f );
	origins[2] = V2_Set( x0f + 0.0f, y0f + 1.0f );
	origins[3] = V2_Set( x0f + 1.0f, y0f + 1.0f );
}

float Noise2D::get(float x, float y) const
{
	V2f origins[4];
	V2f grads[4];

	get_gradients(origins, grads, x, y);
	float vals[] = {
		Gradient(origins[0], grads[0], V2_Set( x, y )),
		Gradient(origins[1], grads[1], V2_Set( x, y )),
		Gradient(origins[2], grads[2], V2_Set( x, y )),
		Gradient(origins[3], grads[3], V2_Set( x, y )),
	};

	float fx = SmoothStep(x - origins[0].x);
	float vx0 = lerp10(vals[0], vals[1], fx);
	float vx1 = lerp10(vals[2], vals[3], fx);
	float fy = SmoothStep(y - origins[0].y);
	return lerp10(vx0, vx1, fy);
}

#endif // ( _MSC_VER >= 1800 )



namespace SDF
{

	F32 SimplexTerrain2D::DistanceTo( const V3f& _position ) const
	{
		const V3f scaled = V3_Multiply( _position, this->scale );
		// hills
		//		return scaled.z - SimplexNoise::RawNoise2D( scaled.x, scaled.y ) * 0.4;
		// better:
		float val1 = scaled.z - SimplexNoise::OctaveNoise2D( 4.0f, 0.3f, 0.5f, scaled.x, scaled.y ) * 0.4f;
//		val1 += SimplexNoise::OctaveNoise2D( 1.0f, 1.0f, 4.5f, scaled.x, scaled.y );
		return val1 - SimplexNoise::OctaveNoise2D( 2.0f, 0.5f, 0.2f, scaled.x, scaled.y );
	}

	F32 HillTerrain2D::DistanceTo( const V3f& _position ) const
	{
		const V3f scaled =
			V3_Multiply( _position, V3_SetAll( 0.06f ) )
			;
		// hills
		//		return scaled.z - SimplexNoise::RawNoise2D( scaled.x, scaled.y ) * 0.4;
		// better:
		float val1 = scaled.z
			-
			SimplexNoise::OctaveNoise2D( 4.0f, 0.3f, 0.9f, scaled.x, scaled.y )// * 0.4f
			*
			SimplexNoise::OctaveNoise2D( 2.0f, 0.3f, 0.5f, scaled.x, scaled.y )// * 0.4f
			*
			SimplexNoise::OctaveNoise2D( 1.0f, 0.6f, 0.6f, scaled.x, scaled.y );
		return val1;
	}

}//namespace SDF

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
