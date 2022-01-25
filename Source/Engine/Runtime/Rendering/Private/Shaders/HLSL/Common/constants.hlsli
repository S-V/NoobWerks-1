//	Misc constants and helpers

#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

#define	M_PI		3.1415926535897932384626433832795
#define M_TWO_PI	6.28318530717958647692528676655901
#define	M_INV_PI	0.31830988618379067154f
#define	M_E			2.71828182845904523536f

#define	nwSQRT2		(1.414213562)
#define	nwSQRT3		(1.732050807568877293527446341505)		// sqrt(3)

/// 1 / (pi * 2)
#define	tbINV_TWOPI		(0.15915494309189533576888376337254)


///
/// Golden ratio: PHI = sqrt(5) * 0.5 + 0.5
///
#define M_PHI		(1.61803398874989484820f)
#define M_PHI_f64	(1.6180339887498948482045868343656)


#define	M_DEG2RAD		(M_PI / 180.0f)
#define	M_RAD2DEG		(180.0f / M_PI)
#define DEG2RAD(x)		((x) * M_DEG2RAD)
#define RAD2DEG(x)		((x) * M_RAD2DEG)



#define BIG_NUMBER			(9999999.0f)
#define SMALL_NUMBER		(1.e-6f)

/// 'ooeps' - often used to fix ray directions that are too close to 0.
#define VERY_SMALL_NUMBER	(exp2(-80.0f))


#define AIR_REFRACTION_INDEX          1.0f
#define GLASS_REFRACTION_INDEX        1.61f
#define WATER_REFRACTION_INDEX        1.33f
#define DIAMOND_REFRACTION_INDEX      2.42f
#define ZIRCONIUM_REFRACTION_INDEX    2.17f

//LUM_WEIGHTS is used to calculate the intensity of a color value
// (note green contributes more than red contributes more than blue)
//static const float3 LUM_WEIGHTS = float3(0.27f, 0.67f, 0.06f);
static const float3 LUM_WEIGHTS = float3(0.299f, 0.587f, 0.114f);//LUMINANCE_VECTOR

#endif // CONSTANTS_HLSLI
