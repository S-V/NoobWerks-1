/*
=============================================================================
	File:	Quaternion.cpp
	Desc:	Quaternion.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Math/AxisAngle.h>

ATextStream & operator << ( ATextStream & log, const NwAxisAngle& axis_angle )
{
	if( axis_angle.angle != 0 )
	{
		log.PrintF("{Axis: (%.3f, %.3f, %.3f), Angle: %.3f}",
			axis_angle.axis.x, axis_angle.axis.y, axis_angle.axis.z,
			RAD2DEG( axis_angle.angle )
			);
	}
	else
	{
		log.PrintF("{Angle: 0}");
	}
	return log;
}

void NwAxisAngle::toString( String *str_ ) const
{
	StringStream	writer( *str_ );
	writer << *this;
}
