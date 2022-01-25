// 
#pragma once

#include <Core/Assets/Asset.h>
#include <GPU/Public/graphics_types.h>


#define SG_USE_SDF_VOLUMES	(0)




#if SG_USE_SDF_VOLUMES



#define SG_DISTANCE_FIELD_FORMAT_FLOAT	(0)
#define SG_DISTANCE_FIELD_FORMAT_UINT16	(1)
#define SG_DISTANCE_FIELD_FORMAT_UINT8	(2)

#define SG_DISTANCE_FIELD_FORMAT_USED	(SG_DISTANCE_FIELD_FORMAT_UINT16)



#if SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_FLOAT
	typedef float SDFValue;
#elif SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_UINT16
	typedef U16 SDFValue;
#endif



enum {
	SDF_VOL_RES = 32
};





// 16^3 * sizeof(U16) = 8192
const U32 SDF_VOL_TEX_SIZE = (SDF_VOL_RES*SDF_VOL_RES*SDF_VOL_RES) * sizeof(U16);




#endif
