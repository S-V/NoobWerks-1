// GUI rendering
#include <Base/Base.h>
#pragma hdrstop

#include <Utility/GUI/GUI_Common.h>


void build_orthographic_projection_matrix_D3D11(
								   float *result
								   , float width, float height
								   )
{
	const float L = 0.0f;
	const float R = width;
	const float T = 0.0f;
	const float B = height;
	float matrix[4][4] =
	{
		{    2.0f / (R - L),              0.0f, 0.0f, 0.0f },
		{              0.0f,    2.0f / (T - B), 0.0f, 0.0f },
		{              0.0f,              0.0f, 0.5f, 0.0f },
		{ (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
	};
	memcpy(result, matrix, sizeof(matrix));
}
