#ifndef DISTANCE_FUNCTIONS_HLSLI
#define DISTANCE_FUNCTIONS_HLSLI

struct shdSphere
{
	float3	center;
	float	radius;
};

float SDF_Sphere(
			   in float3 sample_position,
			   in shdSphere sphere
			   )
{
    return length( sample_position - sphere.center ) - sphere.radius;
}

#endif // DISTANCE_FUNCTIONS_HLSLI
