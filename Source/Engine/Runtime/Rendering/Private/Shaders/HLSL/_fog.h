// fog rendering

#include <base.hlsli>

static const float3 DEFAULT_FOG_COLOR = float3(0.5,0.6,0.7);

// http://iquilezles.org/www/articles/fog/fog.htm
float3 applyFog(
	in float3 rgb,		// original color of the pixel
	in float _distance	// camera to point distance
	)
{
    float  fogAmount = 1.0 - exp( -_distance );
    float3 fogColor  = float3(0.5,0.6,0.7);
    return lerp( rgb, fogColor, fogAmount );
}





#define FOGMODE_NONE    0
#define FOGMODE_LINEAR  1
#define FOGMODE_EXP     2
#define FOGMODE_EXP2    3

// fog params
struct FogParams
{
	int      fogMode;
	float    fogStart;
	float    fogEnd;
	float    fogDensity;
	float4   fogColor;
};

//
// Calculates fog factor based upon distance
//
float CalcFogFactor( in FogParams _params, in float _distance )
{
    float fogCoeff = 1.0;
    
    if( FOGMODE_LINEAR == _params.fogMode )
    {
        fogCoeff = (_params.fogEnd - _distance)/(_params.fogMode - _params.fogStart);
    }
    else if( FOGMODE_EXP == _params.fogMode )
    {
        fogCoeff = 1.0 / pow( M_E, _distance*_params.fogDensity );
    }
    else if( FOGMODE_EXP2 == _params.fogMode )
    {
        fogCoeff = 1.0 / pow( M_E, _distance*_distance*_params.fogDensity*_params.fogDensity );
    }
    
    return clamp( fogCoeff, 0, 1 );
}


mxSTOLEN("Urho3D")
float3 GetFog( float3 color, float fogFactor, float3 fogColor )
{
    return lerp( fogColor, color, fogFactor );
}

float3 GetLitFog(float3 color, float fogFactor)
{
    return color * fogFactor;
}

#if 0
float GetFogFactor(float depth)
{
    return saturate((cFogParams.x - depth) * cFogParams.y);
}

float GetHeightFogFactor(float depth, float height)
{
    float fogFactor = GetFogFactor(depth);
    float heightFogFactor = (height - cFogParams.z) * cFogParams.w;
    heightFogFactor = 1.0 - saturate(exp(-(heightFogFactor * heightFogFactor)));
    return min(heightFogFactor, fogFactor);
}
#endif
