//
#pragma once


namespace PCG
{

class INoise
{
public:
	virtual float getRidgedMultiFractalSingle(
		const V3f& position
		, const int num_octaves
		) = 0;

	virtual void getRidgedMultiFractal(
		const float *values
		, const int cube_dimension
		, const int num_octaves
		, const float scale = 1
		) = 0;
};


}//namespace PCG
