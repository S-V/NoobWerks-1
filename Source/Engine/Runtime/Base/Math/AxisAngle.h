#pragma once


struct NwAxisAngle
{
	V3f		axis;
	float	angle;

public:
	friend ATextStream & operator << ( ATextStream & log, const NwAxisAngle& axis_angle );

	void toString( String *str_ ) const;
};
