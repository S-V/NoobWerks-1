#pragma once


#include <Base/Math/Matrix/TMatrix3x3.h>
#include <Base/Math/Matrix/TMatrix4x4.h>
#include <Base/Math/Matrix/M44f.h>



///
///------------------------------------------------------------------
///	Packed matrix: the fourth row is (0,0,0,1),
///	the translation is computed from r[0].w, r[1].w, r[2].w,
///	the first three elements of each row form a rotation matrix
///	and the last element of every row is the translation over one axis.
///
///	Mat3x3	R;	// rotation component
///	Vec3	T;	// translation component
///
///	R[0][0], R[0][1], R[0][2], Tx[0]
///	R[1][0], R[1][1], R[1][2], Ty[1]
///	R[2][0], R[2][1], R[2][2], Tz[2]
///------------------------------------------------------------------
///
mxPREALIGN(16) struct M34f
{
	union {
		Vector4	r[3];	// laid out as 3 rows in memory
		V4f		v[3];
	};
	// r[0].xyz contains translation,
	// (r[0].x, r[1].y, r[2].z, 0f) are the third row of the full 4x4 matrix.
	//mmDECLARE_ALIGNED_ALLOCATOR16(M34f);
};

///*
//------------------------------------------------------------------
//	Packed matrix
//------------------------------------------------------------------
//*/
//struct M34f
//{
//	Vector4	T;
//	Vector4	axisX;
//	Vector4	axisY;
//	// Z-axis is computed as cross product X-axis X Y-axis
//};


const M34f		M34_Identity();
const M34f		M34_Pack( const M44f& m );
const M44f		M34_Unpack( const M34f& m );
const V3f		M34_GetTranslation( const M34f& m );
void			M34_SetTranslation( M34f & m, const V3f& T );

const M44f		M44_From_Float3x4( const M34f& m );

/*
=======================================================================
	REFLECTION
=======================================================================
*/
#if MM_ENABLE_REFLECTION

#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/StructDescriptor.h>

mxDECLARE_STRUCT(M34f);
mxDECLARE_POD_TYPE(M34f);

#endif // MM_ENABLE_REFLECTION
