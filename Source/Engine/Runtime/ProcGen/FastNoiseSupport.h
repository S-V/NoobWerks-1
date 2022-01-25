//
#pragma once

//
#include <FastNoise/FastNoise.h>
#pragma comment( lib, "FastNoise.lib" )


enum FastNoise_Type
{
	FN_Value			= FastNoise::NoiseType::Value,
	FN_ValueFractal		= FastNoise::NoiseType::ValueFractal,
	FN_Perlin			= FastNoise::NoiseType::Perlin,
	FN_PerlinFractal	= FastNoise::NoiseType::PerlinFractal,
	FN_Simplex			= FastNoise::NoiseType::Simplex,
	FN_SimplexFractal	= FastNoise::NoiseType::SimplexFractal,
	FN_Cellular			= FastNoise::NoiseType::Cellular,
	FN_WhiteNoise		= FastNoise::NoiseType::WhiteNoise,
	FN_Cubic			= FastNoise::NoiseType::Cubic,
	FN_CubicFractal		= FastNoise::NoiseType::CubicFractal,
};
mxDECLARE_ENUM( FastNoise_Type, U8, FastNoise_TypeT );

enum FastNoise_Interp
{
	FN_Linear		= FastNoise::Interp::Linear,
	FN_Hermite		= FastNoise::Interp::Hermite,
	FN_Quintic		= FastNoise::Interp::Quintic,
};
mxDECLARE_ENUM( FastNoise_Interp, U8, FastNoise_InterpT );

enum FastNoise_FractalType
{
	FN_FBM			= FastNoise::FractalType::FBM,
	FN_Billow		= FastNoise::FractalType::Billow,
	FN_RidgedMulti	= FastNoise::FractalType::RigidMulti,
};
mxDECLARE_ENUM( FastNoise_FractalType, U8, FastNoise_FractalTypeT );

enum FastNoise_CellularDistanceFunction
{
	FN_Euclidean	= FastNoise::CellularDistanceFunction::Euclidean,
	FN_Manhattan	= FastNoise::CellularDistanceFunction::Manhattan,
	FN_Natural		= FastNoise::CellularDistanceFunction::Natural,
};
mxDECLARE_ENUM( FastNoise_CellularDistanceFunction, U8, FastNoise_CellularDistanceFunctionT );

enum FastNoise_CellularReturnType
{
	FN_CellValue	= FastNoise::CellularReturnType::CellValue,
	FN_NoiseLookup	= FastNoise::CellularReturnType::NoiseLookup,
	FN_Distance		= FastNoise::CellularReturnType::Distance,
	FN_Distance2	= FastNoise::CellularReturnType::Distance2,
	FN_Distance2Add	= FastNoise::CellularReturnType::Distance2Add,
	FN_Distance2Sub	= FastNoise::CellularReturnType::Distance2Sub,
	FN_Distance2Mul	= FastNoise::CellularReturnType::Distance2Mul,
	FN_Distance2Div	= FastNoise::CellularReturnType::Distance2Div,
};
mxDECLARE_ENUM( FastNoise_CellularReturnType, U8, FastNoise_CellularReturnTypeT );


struct FastNoiseSettings: CStruct
{
	int						seed;

	//
	FastNoise_TypeT			type;
	FN_DECIMAL				frequency;
	FastNoise_InterpT		interp;
	int						octaves;
	FN_DECIMAL				lacunarity;
	FN_DECIMAL				gain;
	FastNoise_FractalTypeT	fractal_type;

	//
	FastNoise_CellularDistanceFunctionT	cellularDistanceFunction;
	FastNoise_CellularReturnTypeT		cellular_return_type;
	int									cellular_distance_index0;
	int									cellular_distance_index1;
	FN_DECIMAL							cellular_jitter;

	//
	FN_DECIMAL							gradient_perturb_amp;

public:
	mxDECLARE_CLASS(FastNoiseSettings, CStruct);
	mxDECLARE_REFLECTION;

	FastNoiseSettings();

	void setDefaults();

	void applyTo( FastNoise &fast_noise ) const;
};
