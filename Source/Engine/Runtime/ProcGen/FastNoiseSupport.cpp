#include "Base/Base.h"
#pragma hdrstop
#include <ProcGen/FastNoiseSupport.h>

mxBEGIN_REFLECT_ENUM( FastNoise_TypeT )
	mxREFLECT_ENUM_ITEM( Value			, FastNoise::NoiseType::Value			),
	mxREFLECT_ENUM_ITEM( ValueFractal	, FastNoise::NoiseType::ValueFractal	),
	mxREFLECT_ENUM_ITEM( Perlin			, FastNoise::NoiseType::Perlin			),
	mxREFLECT_ENUM_ITEM( PerlinFractal	, FastNoise::NoiseType::PerlinFractal	),
	mxREFLECT_ENUM_ITEM( Simplex		, FastNoise::NoiseType::Simplex			),
	mxREFLECT_ENUM_ITEM( SimplexFractal	, FastNoise::NoiseType::SimplexFractal	),
	mxREFLECT_ENUM_ITEM( Cellular		, FastNoise::NoiseType::Cellular		),
	mxREFLECT_ENUM_ITEM( WhiteNoise		, FastNoise::NoiseType::WhiteNoise		),
	mxREFLECT_ENUM_ITEM( Cubic			, FastNoise::NoiseType::Cubic			),
	mxREFLECT_ENUM_ITEM( CubicFractal	, FastNoise::NoiseType::CubicFractal	),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( FastNoise_InterpT )
	mxREFLECT_ENUM_ITEM( Linear			, FastNoise::Interp::Linear			),
	mxREFLECT_ENUM_ITEM( Hermite		, FastNoise::Interp::Hermite		),
	mxREFLECT_ENUM_ITEM( Quintic		, FastNoise::Interp::Quintic		),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( FastNoise_FractalTypeT )
	mxREFLECT_ENUM_ITEM( FBM			, FastNoise::FractalType::FBM			),
	mxREFLECT_ENUM_ITEM( Billow			, FastNoise::FractalType::Billow		),
	mxREFLECT_ENUM_ITEM( RidgedMulti	, FastNoise::FractalType::RigidMulti	),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( FastNoise_CellularDistanceFunctionT )
	mxREFLECT_ENUM_ITEM( Euclidean	, FastNoise::CellularDistanceFunction::Euclidean	),
	mxREFLECT_ENUM_ITEM( Manhattan	, FastNoise::CellularDistanceFunction::Manhattan	),
	mxREFLECT_ENUM_ITEM( Natural	, FastNoise::CellularDistanceFunction::Natural		),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( FastNoise_CellularReturnTypeT )
	mxREFLECT_ENUM_ITEM( CellValue		, FastNoise::CellularReturnType::CellValue		),
	mxREFLECT_ENUM_ITEM( NoiseLookup	, FastNoise::CellularReturnType::NoiseLookup	),
	mxREFLECT_ENUM_ITEM( Distance		, FastNoise::CellularReturnType::Distance		),
	mxREFLECT_ENUM_ITEM( Distance2		, FastNoise::CellularReturnType::Distance2		),
	mxREFLECT_ENUM_ITEM( Distance2Add	, FastNoise::CellularReturnType::Distance2Add	),
	mxREFLECT_ENUM_ITEM( Distance2Sub	, FastNoise::CellularReturnType::Distance2Sub	),
	mxREFLECT_ENUM_ITEM( Distance2Mul	, FastNoise::CellularReturnType::Distance2Mul	),
	mxREFLECT_ENUM_ITEM( Distance2Div	, FastNoise::CellularReturnType::Distance2Div	),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(FastNoiseSettings);
mxBEGIN_REFLECTION(FastNoiseSettings)
	mxMEMBER_FIELD( seed ),
	mxMEMBER_FIELD( type ),
	mxMEMBER_FIELD( frequency ),
	mxMEMBER_FIELD( interp ),
	mxMEMBER_FIELD( octaves ),
	mxMEMBER_FIELD( lacunarity ),
	mxMEMBER_FIELD( gain ),
	mxMEMBER_FIELD( fractal_type ),

	mxMEMBER_FIELD( cellularDistanceFunction ),
	mxMEMBER_FIELD( cellular_return_type ),
	mxMEMBER_FIELD( cellular_distance_index0 ),
	mxMEMBER_FIELD( cellular_distance_index1 ),
	mxMEMBER_FIELD( cellular_jitter ),
	mxMEMBER_FIELD( gradient_perturb_amp ),
mxEND_REFLECTION;

FastNoiseSettings::FastNoiseSettings()
{
	this->setDefaults();
}

void FastNoiseSettings::setDefaults()
{
	seed = 1337;
	//
	type			= FN_Simplex;
	frequency		= FN_DECIMAL(0.01);
	interp			= FN_Quintic;
	octaves			= 3;
	lacunarity		= FN_DECIMAL(2);
	gain			= FN_DECIMAL(0.5);
	fractal_type	= FN_FBM;
	//
	cellularDistanceFunction	= FN_Euclidean;
	cellular_return_type		= FN_CellValue;
	cellular_distance_index0	= 0;
	cellular_distance_index1	= 1;
	cellular_jitter				= FN_DECIMAL(0.45);
	//
	gradient_perturb_amp = FN_DECIMAL(1);
}

void FastNoiseSettings::applyTo( FastNoise &fast_noise ) const
{
	fast_noise.SetSeed( seed );
	//
	fast_noise.SetNoiseType( (FastNoise::NoiseType)type.raw_value );
	fast_noise.SetFrequency( frequency );
	fast_noise.SetInterp( (FastNoise::Interp)interp.raw_value );
	fast_noise.SetFractalOctaves( octaves );
	fast_noise.SetFractalLacunarity( lacunarity );
	fast_noise.SetFractalGain( gain );
	fast_noise.SetFractalType( (FastNoise::FractalType)fractal_type.raw_value );
	//
	fast_noise.SetCellularDistanceFunction( (FastNoise::CellularDistanceFunction)cellularDistanceFunction.raw_value );
	fast_noise.SetCellularReturnType( (FastNoise::CellularReturnType)cellular_return_type.raw_value );
	fast_noise.SetCellularDistance2Indices( cellular_distance_index0, cellular_distance_index1 );
	fast_noise.SetCellularJitter( cellular_jitter );
	fast_noise.SetGradientPerturbAmp( gradient_perturb_amp );
}
