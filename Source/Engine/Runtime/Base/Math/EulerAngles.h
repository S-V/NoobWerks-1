#pragma once


/// in degrees
struct NwEulerAngles
{
	union
	{
		struct
		{
			float	pitch;	// up / down	(X)
			float	roll;	// fall over	(Y)
			float	yaw;	// left / right	(Z)
		};

		float	a[3];
	};

public:
	NwEulerAngles();
};
