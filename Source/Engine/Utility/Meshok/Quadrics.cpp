// Quadric error metrics. Linear Least Squares (QEF) solvers.
#include "stdafx.h"
#pragma hdrstop
#include <algorithm>
#include <Meshok/Meshok.h>
#include <Meshok/Quadrics.h>

/// turn off annoying asserts
#define DBG_QUADRICS	(MX_DEBUG)

/// note: leads to long compile times
#define WITH_EIGEN_LIBRARY	(0)

#if WITH_EIGEN_LIBRARY
#include <Eigen/Dense>
#endif

/*

  Numerical functions for computing minimizers of a least-squares system
  of equations.

  Copyright (C) 2011 Scott Schaefer
*/
namespace Solver2
{


#define TOLERANCE 0.0001f


//#define UCHAR unsigned char
//#define USHORT unsigned short

#define USE_MINIMIZER

// 3d point with integer coordinates
typedef struct
{
	int x, y, z;
} Point3i;

typedef struct
{
	Point3i begin;
	Point3i end;
} BoundingBox;

// triangle that points to three vertices
typedef struct 
{
	float vt[3][3] ;
} Triangle;

struct TriangleList
{
	float vt[3][3] ;
	TriangleList* next ;
};

struct VertexList
{
	float vt[3] ;
	VertexList* next ;
};

struct IndexedTriangleList
{
	int vt[3] ;
	IndexedTriangleList* next ;
};

// 3d point with float coordinates
typedef struct
{
	float x, y, z;
} Point3f;

typedef struct
{
	Point3f begin;
	Point3f end;
} BoundingBoxf;


/**
 * Uses a jacobi method to return the eigenvectors and eigenvalues 
 * of a 3x3 symmetric matrix.  Note: "a" will be destroyed in this
 * process.  "d" will contain the eigenvalues sorted in order of 
 * decreasing modulus and v will contain the corresponding eigenvectors.
 *
 * @param a the 3x3 symmetric matrix to calculate the eigensystem for
 * @param d the variable to hold the eigenvalues
 * @param v the variables to hold the eigenvectors
 */
void jacobi ( float a[][3], float d[], float v[][3] );

/**
 * Inverts a 3x3 symmetric matrix by computing the pseudo-inverse.
 *
 * @param mat the matrix to invert
 * @param midpoint the point to minimize towards
 * @param rvalue the variable to store the pseudo-inverse in
 * @param w the place to store the inverse of the eigenvalues
 * @param u the place to store the eigenvectors
 */
void matInverse ( float mat[][3], float midpoint[], float rvalue[][3], float w[], float u[][3] );

/**
 * Calculates the L2 norm of the residual (the error)
 * (Transpose[A].A).x = Transpose[A].B
 * 
 * @param a the matrix Transpose[A].A
 * @param b the matrix Transpose[A].B
 * @param btb the value Transpose[B].B
 * @param point the minimizer found
 *
 * @return the error of the minimizer
 */
float calcError ( float a[][3], float b[], float btb, float point[] );

/**
 * Calculates the normal.  This function is not called and doesn't do
 * anything right now.  It was originally meant to orient the normals 
 * correctly that came from the principle eigenvector, but we're flat
 * shading now.
 */
float *calcNormal ( float halfA[], float norm[], float expectedNorm[] );

/**
 * Calculates the minimizer of the given system and returns its error.
 *
 * @param halfA the compressed form of the symmetric matrix Transpose[A].A
 * @param b the matrix Transpose[A].B
 * @param btb the value Transpose[B].B
 * @param midpoint the point to minimize towards
 * @param rvalue the place to store the minimizer
 * @param box the volume bounding the voxel this QEF is for
 *
 * @return the error of the minimizer
 */
float calcPoint ( float halfA[], float b[], float btb, float midpoint[], float rvalue[], BoundingBoxf *box, float *mat );


void qr ( float eqs[][4], int num, float *rvalue );
void qr ( float *mat1, float *mat2, float *rvalue );
void qr ( float eqs[][4], int num = 4, float tol = 0.000001f );

int estimateRank ( float *a );

}//namespace Solver2






namespace Solver2
{

#define ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);a[k][l]=h+s*(g-h*tau);

int method = 3;

// for reducing two upper triangular systems of equations into 1
void qr ( float *mat1, float *mat2, float *rvalue )
{
	int i, j;
	float temp1 [ 8 ] [ 4 ];

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < i; j++ )
		{
			temp1 [ i ] [ j ] = 0;
			temp1 [ i + 4 ] [ j ] = 0;
		}
		for ( j = i; j < 4; j++ )
		{
			temp1 [ i ] [ j ] = mat1 [ ( 7 * i - i * i ) / 2 + j ];
			temp1 [ i + 4 ] [ j ] = mat2 [ ( 7 * i - i * i ) / 2 + j ];
		}
	}

	qr ( temp1, 8, rvalue );
}

// WARNING: destroys eqs in the process
void qr ( float eqs[][4], int num, float *rvalue )
{
	int i, j, k;

	qr ( eqs, num, 0.000001f );
	for ( i = 0; i < 10; i++ )
	{
		rvalue [ i ] = 0;
	}

	k = 0;
	for ( i = 0; i < num && i < 4; i++ )
	{
		for ( j = i; j < 4; j++ )
		{
			rvalue [ k++ ] = eqs [ i ] [ j ];
		}
	}
}

void qr ( float eqs[][4], int num, float tol )
{
	int i, j, k;
	float a, b, mag, temp;

	for ( i = 0; i < 4 && i < num; i++ )
	{
		for ( j = i + 1; j < num; j++ )
		{
			a = eqs [ i ] [ i ];
			b = eqs [ j ] [ i ];

			if ( fabs ( a ) > 0.000001f || fabs ( b ) > 0.000001f )
			{
				mag = (float)sqrt ( a * a + b * b );
				a /= mag;
				b /= mag;

				for ( k = 0; k < 4; k++ )
				{
					temp = a * eqs [ i ] [ k ] + b * eqs [ j ] [ k ];
					eqs [ j ] [ k ] = b * eqs [ i ] [ k ] - a * eqs [ j ] [ k ];
					eqs [ i ] [ k ] = temp;
				}
			}
		}
		for ( j = i - 1; j >= 0; j-- )
		{
			if ( eqs [ j ] [ j ] < 0.000001f && eqs [ j ] [ j ] > -0.000001f )
			{
				a = eqs [ i ] [ i ];
				b = eqs [ j ] [ i ];

				if ( fabs ( a ) > 0.000001f || fabs ( b ) > 0.000001f )
				{
					mag = (float)sqrt ( a * a + b * b );
					a /= mag;
					b /= mag;

					for ( k = 0; k < 4; k++ )
					{
						temp = a * eqs [ i ] [ k ] + b * eqs [ j ] [ k ];
						eqs [ j ] [ k ] = b * eqs [ i ] [ k ] - a * eqs [ j ] [ k ];
						eqs [ i ] [ k ] = temp;
					}
				}
			}
		}
	}

}

void jacobi ( float u[][3], float d[], float v[][3] )
{
	int j, iq, ip, i;
	float tresh, theta, tau, t, sm, s, h, g, c, b [ 3 ], z [ 3 ];
	float a [ 3 ] [ 3 ];

	a [ 0 ] [ 0 ] = u [ 0 ] [ 0 ];
	a [ 0 ] [ 1 ] = u [ 0 ] [ 1 ];
	a [ 0 ] [ 2 ] = u [ 0 ] [ 2 ];
	a [ 1 ] [ 0 ] = u [ 1 ] [ 0 ];
	a [ 1 ] [ 1 ] = u [ 1 ] [ 1 ];
	a [ 1 ] [ 2 ] = u [ 1 ] [ 2 ];
	a [ 2 ] [ 0 ] = u [ 2 ] [ 0 ];
	a [ 2 ] [ 1 ] = u [ 2 ] [ 1 ];
	a [ 2 ] [ 2 ] = u [ 2 ] [ 2 ];

	for ( ip = 0; ip < 3; ip++ ) 
	{
		for ( iq = 0; iq < 3; iq++ )
		{
			v [ ip ] [ iq ] = 0.0f;
		}
		v [ ip ] [ ip ] = 1.0f;
	}

	for ( ip = 0; ip < 3; ip++ )
	{
		b [ ip ] = a [ ip ] [ ip ];
		d [ ip ] = b [ ip ];
		z [ ip ] = 0.0f;
	}

	for ( i = 1; i <= 50; i++ )
	{
		sm = 0.0f;
		for ( ip = 0; ip < 2; ip++ )
		{
			for ( iq = ip + 1; iq < 3; iq++ )
			{
				sm += (float)fabs ( a [ ip ] [ iq ] );
			}
		}

		if ( sm == 0.0f )
		{
			// sort the stupid things and transpose
			a [ 0 ] [ 0 ] = v [ 0 ] [ 0 ];
			a [ 0 ] [ 1 ] = v [ 1 ] [ 0 ];
			a [ 0 ] [ 2 ] = v [ 2 ] [ 0 ];
			a [ 1 ] [ 0 ] = v [ 0 ] [ 1 ];
			a [ 1 ] [ 1 ] = v [ 1 ] [ 1 ];
			a [ 1 ] [ 2 ] = v [ 2 ] [ 1 ];
			a [ 2 ] [ 0 ] = v [ 0 ] [ 2 ];
			a [ 2 ] [ 1 ] = v [ 1 ] [ 2 ];
			a [ 2 ] [ 2 ] = v [ 2 ] [ 2 ];

			if ( fabs ( d [ 0 ] ) < fabs ( d [ 1 ] ) )
			{
				sm = d [ 0 ];
				d [ 0 ] = d [ 1 ];
				d [ 1 ] = sm;

				sm = a [ 0 ] [ 0 ];
				a [ 0 ] [ 0 ] = a [ 1 ] [ 0 ];
				a [ 1 ] [ 0 ] = sm;
				sm = a [ 0 ] [ 1 ];
				a [ 0 ] [ 1 ] = a [ 1 ] [ 1 ];
				a [ 1 ] [ 1 ] = sm;
				sm = a [ 0 ] [ 2 ];
				a [ 0 ] [ 2 ] = a [ 1 ] [ 2 ];
				a [ 1 ] [ 2 ] = sm;
			}
			if ( fabs ( d [ 1 ] ) < fabs ( d [ 2 ] ) )
			{
				sm = d [ 1 ];
				d [ 1 ] = d [ 2 ];
				d [ 2 ] = sm;

				sm = a [ 1 ] [ 0 ];
				a [ 1] [ 0 ] = a [ 2 ] [ 0 ];
				a [ 2 ] [ 0 ] = sm;
				sm = a [ 1 ] [ 1 ];
				a [ 1 ] [ 1 ] = a [ 2 ] [ 1 ];
				a [ 2 ] [ 1 ] = sm;
				sm = a [ 1 ] [ 2 ];
				a [ 1 ] [ 2 ] = a [ 2 ] [ 2 ];
				a [ 2 ] [ 2 ] = sm;
			}
			if ( fabs ( d [ 0 ] ) < fabs ( d [ 1 ] ) )
			{
				sm = d [ 0 ];
				d [ 0 ] = d [ 1 ];
				d [ 1 ] = sm;

				sm = a [ 0 ] [ 0 ];
				a [ 0 ] [ 0 ] = a [ 1 ] [ 0 ];
				a [ 1 ] [ 0 ] = sm;
				sm = a [ 0 ] [ 1 ];
				a [ 0 ] [ 1 ] = a [ 1 ] [ 1 ];
				a [ 1 ] [ 1 ] = sm;
				sm = a [ 0 ] [ 2 ];
				a [ 0 ] [ 2 ] = a [ 1 ] [ 2 ];
				a [ 1 ] [ 2 ] = sm;
			}

			v [ 0 ] [ 0 ] = a [ 0 ] [ 0 ];
			v [ 0 ] [ 1 ] = a [ 0 ] [ 1 ];
			v [ 0 ] [ 2 ] = a [ 0 ] [ 2 ];
			v [ 1 ] [ 0 ] = a [ 1 ] [ 0 ];
			v [ 1 ] [ 1 ] = a [ 1 ] [ 1 ];
			v [ 1 ] [ 2 ] = a [ 1 ] [ 2 ];
			v [ 2 ] [ 0 ] = a [ 2 ] [ 0 ];
			v [ 2 ] [ 1 ] = a [ 2 ] [ 1 ];
			v [ 2 ] [ 2 ] = a [ 2 ] [ 2 ];

			return;
		}

		if ( i < 4 )
		{
			tresh = 0.2f * sm / 9;
		}
		else
		{
			tresh = 0.0f;
		}

		for ( ip = 0; ip < 2; ip++ )
		{
			for ( iq = ip + 1; iq < 3; iq++ ) 
			{
				g = 100.0f * (float)fabs ( a [ ip ] [ iq ] );
				if ( i > 4 && (float)( fabs ( d [ ip ] ) + g ) == (float)fabs ( d [ ip ] )
					&& (float)( fabs ( d [ iq ] ) + g ) == (float)fabs ( d [ iq ] ) )
				{
					a [ ip ] [ iq ] = 0.0f;
				}
				else
				{
					if ( fabs ( a [ ip ] [ iq ] ) > tresh )
					{
						h = d [ iq ] - d [ ip ];
						if ( (float)( fabs ( h ) + g ) == (float)fabs ( h ) )
						{
							t = ( a [ ip ] [ iq ] ) / h;
						}
						else
						{
							theta = 0.5f * h / ( a [ ip ] [ iq ] );
							t = 1.0f / ( (float)fabs ( theta ) + (float)sqrt ( 1.0f + theta * theta ) );
							if ( theta < 0.0f ) 
							{
								t = -1.0f * t;
							}
						}

						c = 1.0f / (float)sqrt ( 1 + t * t );
						s = t * c;
						tau = s / ( 1.0f + c );
						h = t * a [ ip ] [ iq ];
						z [ ip ] -= h;
						z [ iq ] += h;
						d [ ip ] -= h;
						d [ iq ] += h;
						a [ ip ] [ iq ] = 0.0f;
						for ( j = 0; j <= ip - 1; j++ )
						{
							ROTATE ( a, j, ip, j, iq )
						}
						for ( j = ip + 1; j <= iq - 1; j++ )
						{
							ROTATE ( a, ip, j, j, iq )
						}
						for ( j = iq + 1; j < 3; j++ )
						{
							ROTATE ( a, ip, j, iq, j )
						}
						for ( j = 0; j < 3; j++ )
						{
							ROTATE ( v, j, ip, j, iq )
						}
					}
				}
			}
		}

		for ( ip = 0; ip < 3; ip++ )
		{
			b [ ip ] += z [ ip ];
			d [ ip ] = b [ ip ];
			z [ ip ] = 0.0f;
		}
	}
	printf ( "too many iterations in jacobi\n" );
	exit ( 1 );
}

int estimateRank ( float *a )
{
	float w [ 3 ];
	float u [ 3 ] [ 3 ];
	float mat [ 3 ] [ 3 ];
	int i;

	mat [ 0 ] [ 0 ] = a [ 0 ];
	mat [ 0 ] [ 1 ] = a [ 1 ];
	mat [ 0 ] [ 2 ] = a [ 2 ];
	mat [ 1 ] [ 1 ] = a [ 3 ];
	mat [ 1 ] [ 2 ] = a [ 4 ];
	mat [ 2 ] [ 2 ] = a [ 5 ];
	mat [ 1 ] [ 0 ] = a [ 1 ];
	mat [ 2 ] [ 0 ] = a [ 2 ];
	mat [ 2 ] [ 1 ] = a [ 4 ];

	jacobi ( mat, w, u );

	if ( w [ 0 ] == 0.0f )
	{
		return 0;
	}
	else
	{
		for ( i = 1; i < 3; i++ )
		{
			if ( w [ i ] < 0.1f )
			{
				return i;
			}
		}

		return 3;
	}

}

void matInverse ( float mat[][3], float midpoint[], float rvalue[][3], float w[], float u[][3] )
{
	// there is an implicit assumption that mat is symmetric and real
	// U and V in the SVD will then be the same matrix whose rows are the eigenvectors of mat
	// W will just be the eigenvalues of mat
//	float w [ 3 ];
//	float u [ 3 ] [ 3 ];
	int i;

	jacobi ( mat, w, u );

	if ( w [ 0 ] == 0.0f )
	{
//		printf ( "error: largest eigenvalue is 0!\n" );
	}
	else
	{
		for ( i = 1; i < 3; i++ )
		{
			if ( w [ i ] < 0.001f ) // / w [ 0 ] < TOLERANCE )
			{
					w [ i ] = 0;
			}
			else
			{
				w [ i ] = 1.0f / w [ i ];
			}
		}
		w [ 0 ] = 1.0f / w [ 0 ];
	}

	rvalue [ 0 ] [ 0 ] = w [ 0 ] * u [ 0 ] [ 0 ] * u [ 0 ] [ 0 ] +
					w [ 1 ] * u [ 1 ] [ 0 ] * u [ 1 ] [ 0 ] +
					w [ 2 ] * u [ 2 ] [ 0 ] * u [ 2 ] [ 0 ];
	rvalue [ 0 ] [ 1 ] = w [ 0 ] * u [ 0 ] [ 0 ] * u [ 0 ] [ 1 ] +
					w [ 1 ] * u [ 1 ] [ 0 ] * u [ 1 ] [ 1 ] +
					w [ 2 ] * u [ 2 ] [ 0 ] * u [ 2 ] [ 1 ];
	rvalue [ 0 ] [ 2 ] = w [ 0 ] * u [ 0 ] [ 0 ] * u [ 0 ] [ 2 ] +
					w [ 1 ] * u [ 1 ] [ 0 ] * u [ 1 ] [ 2 ] +
					w [ 2 ] * u [ 2 ] [ 0 ] * u [ 2 ] [ 2 ];
	rvalue [ 1 ] [ 0 ] = w [ 0 ] * u [ 0 ] [ 1 ] * u [ 0 ] [ 0 ] +
					w [ 1 ] * u [ 1 ] [ 1 ] * u [ 1 ] [ 0 ] +
					w [ 2 ] * u [ 2 ] [ 1 ] * u [ 2 ] [ 0 ];
	rvalue [ 1 ] [ 1 ] = w [ 0 ] * u [ 0 ] [ 1 ] * u [ 0 ] [ 1 ] +
					w [ 1 ] * u [ 1 ] [ 1 ] * u [ 1 ] [ 1 ] +
					w [ 2 ] * u [ 2 ] [ 1 ] * u [ 2 ] [ 1 ];
	rvalue [ 1 ] [ 2 ] = w [ 0 ] * u [ 0 ] [ 1 ] * u [ 0 ] [ 2 ] +
					w [ 1 ] * u [ 1 ] [ 1 ] * u [ 1 ] [ 2 ] +
					w [ 2 ] * u [ 2 ] [ 1 ] * u [ 2 ] [ 2 ];
	rvalue [ 2 ] [ 0 ] = w [ 0 ] * u [ 0 ] [ 2 ] * u [ 0 ] [ 0 ] +
					w [ 1 ] * u [ 1 ] [ 2 ] * u [ 1 ] [ 0 ] +
					w [ 2 ] * u [ 2 ] [ 2 ] * u [ 2 ] [ 0 ];
	rvalue [ 2 ] [ 1 ] = w [ 0 ] * u [ 0 ] [ 2 ] * u [ 0 ] [ 1 ] +
					w [ 1 ] * u [ 1 ] [ 2 ] * u [ 1 ] [ 1 ] +
					w [ 2 ] * u [ 2 ] [ 2 ] * u [ 2 ] [ 1 ];
	rvalue [ 2 ] [ 2 ] = w [ 0 ] * u [ 0 ] [ 2 ] * u [ 0 ] [ 2 ] +
					w [ 1 ] * u [ 1 ] [ 2 ] * u [ 1 ] [ 2 ] +
					w [ 2 ] * u [ 2 ] [ 2 ] * u [ 2 ] [ 2 ];
}

float calcError ( float a[][3], float b[], float btb, float point[] )
{
	float rvalue = btb;
 
	rvalue += -2.0f * ( point [ 0 ] * b [ 0 ] + point [ 1 ] * b [ 1 ] + point [ 2 ] * b [ 2 ] );
	rvalue += point [ 0 ] * ( a [ 0 ] [ 0 ] * point [ 0 ] + a [ 0 ] [ 1 ] * point [ 1 ] + a [ 0 ] [ 2 ] * point [ 2 ] );
	rvalue += point [ 1 ] * ( a [ 1 ] [ 0 ] * point [ 0 ] + a [ 1 ] [ 1 ] * point [ 1 ] + a [ 1 ] [ 2 ] * point [ 2 ] );
	rvalue += point [ 2 ] * ( a [ 2 ] [ 0 ] * point [ 0 ] + a [ 2 ] [ 1 ] * point [ 1 ] + a [ 2 ] [ 2 ] * point [ 2 ] );

	return rvalue;
}

float *calcNormal ( float halfA[], float norm[], float expectedNorm[] )
{
/*
	float a [ 3 ] [ 3 ];
	float w [ 3 ];
	float u [ 3 ] [ 3 ];

	a [ 0 ] [ 0 ] = halfA [ 0 ];
	a [ 0 ] [ 1 ] = halfA [ 1 ];
	a [ 0 ] [ 2 ] = halfA [ 2 ];
	a [ 1 ] [ 1 ] = halfA [ 3 ];
	a [ 1 ] [ 2 ] = halfA [ 4 ];
	a [ 1 ] [ 0 ] = halfA [ 1 ];
	a [ 2 ] [ 0 ] = halfA [ 2 ];
	a [ 2 ] [ 1 ] = halfA [ 4 ];
	a [ 2 ] [ 2 ] = halfA [ 5 ];

	jacobi ( a, w, u );

	if ( u [ 1 ] != 0 )
	{
		if ( w [ 1 ] / w [ 0 ] > 0.2f )
		{
			// two dominant eigen values, just return the expectedNorm
			norm [ 0 ] = expectedNorm [ 0 ];
			norm [ 1 ] = expectedNorm [ 1 ];
			norm [ 2 ] = expectedNorm [ 2 ];
			return;
		}
	}

	norm [ 0 ] = u [ 0 ] [ 0 ];
	norm [ 1 ] = u [ 0 ] [ 1 ];
	norm [ 2 ] = u [ 0 ] [ 2 ];
*/
	float dot = norm [ 0 ] * expectedNorm [ 0 ] + norm [ 1 ] * expectedNorm [ 1 ] +
				norm [ 2 ] * expectedNorm [ 2 ];

	if ( dot < 0 )
	{
		norm [ 0 ] *= -1.0f;
		norm [ 1 ] *= -1.0f;
		norm [ 2 ] *= -1.0f;

		dot *= -1.0f;
	}

	if ( dot < 0.707f )
	{
		return expectedNorm;
	}
	else
	{
		return norm;
	}
}

void descent ( float A[][3], float B[], float guess[], BoundingBoxf *box )
{
	int i;
	float r [ 3 ];
	float delta, delta0;
	int n = 10;
	float alpha, div;
	float newPoint [ 3 ];
	float c;
	float store [ 3 ];

	store [ 0 ] = guess [ 0 ];
	store [ 1 ] = guess [ 1 ];
	store [ 2 ] = guess [ 2 ];

	if ( method == 2 || method == 0 )
	{

	i = 0;
	r [ 0 ] = B [ 0 ] - ( A [ 0 ] [ 0 ] * guess [ 0 ] + A [ 0 ] [ 1 ] * guess [ 1 ] + A [ 0 ] [ 2 ] * guess [ 2 ] );
	r [ 1 ] = B [ 1 ] - ( A [ 1 ] [ 0 ] * guess [ 0 ] + A [ 1 ] [ 1 ] * guess [ 1 ] + A [ 1 ] [ 2 ] * guess [ 2 ] );
	r [ 2 ] = B [ 2 ] - ( A [ 2 ] [ 0 ] * guess [ 0 ] + A [ 2 ] [ 1 ] * guess [ 1 ] + A [ 2 ] [ 2 ] * guess [ 2 ] );

	delta = r [ 0 ] * r [ 0 ] + r [ 1 ] * r [ 1 ] + r [ 2 ] * r [ 2 ];
	delta0 = delta * TOLERANCE * TOLERANCE;

	while ( i < n && delta > delta0 )
	{
		div = r [ 0 ] * ( A [ 0 ] [ 0 ] * r [ 0 ] + A [ 0 ] [ 1 ] * r [ 1 ] + A [ 0 ] [ 2 ] * r [ 2 ] );
		div += r [ 1 ] * ( A [ 1 ] [ 0 ] * r [ 0 ] + A [ 1 ] [ 1 ] * r [ 1 ] + A [ 1 ] [ 2 ] * r [ 2 ] );
		div += r [ 2 ] * ( A [ 2 ] [ 0 ] * r [ 0 ] + A [ 2 ] [ 1 ] * r [ 1 ] + A [ 2 ] [ 2 ] * r [ 2 ] );

		if ( fabs ( div ) < 0.0000001f )
		{
			break;
		}

		alpha = delta / div;

		newPoint [ 0 ] = guess [ 0 ] + alpha * r [ 0 ];
		newPoint [ 1 ] = guess [ 1 ] + alpha * r [ 1 ];
		newPoint [ 2 ] = guess [ 2 ] + alpha * r [ 2 ];

		guess [ 0 ] = newPoint [ 0 ];
		guess [ 1 ] = newPoint [ 1 ];
		guess [ 2 ] = newPoint [ 2 ];

		r [ 0 ] = B [ 0 ] - ( A [ 0 ] [ 0 ] * guess [ 0 ] + A [ 0 ] [ 1 ] * guess [ 1 ] + A [ 0 ] [ 2 ] * guess [ 2 ] );
		r [ 1 ] = B [ 1 ] - ( A [ 1 ] [ 0 ] * guess [ 0 ] + A [ 1 ] [ 1 ] * guess [ 1 ] + A [ 1 ] [ 2 ] * guess [ 2 ] );
		r [ 2 ] = B [ 2 ] - ( A [ 2 ] [ 0 ] * guess [ 0 ] + A [ 2 ] [ 1 ] * guess [ 1 ] + A [ 2 ] [ 2 ] * guess [ 2 ] );

		delta = r [ 0 ] * r [ 0 ] + r [ 1 ] * r [ 1 ] + r [ 2 ] * r [ 2 ];

		i++;
	}

	if ( guess [ 0 ] >= box->begin.x && guess [ 0 ] <= box->end.x && 
		guess [ 1 ] >= box->begin.y && guess [ 1 ] <= box->end.y &&
		guess [ 2 ] >= box->begin.z && guess [ 2 ] <= box->end.z )
	{
		return;
	}
	}

	if ( method == 0 || method == 1 )
	{
		c = A [ 0 ] [ 0 ] + A [ 1 ] [ 1 ] + A [ 2 ] [ 2 ];
		if ( c == 0 )
		{
			return;
		}
		c = ( 0.75f / c );

		guess [ 0 ] = store [ 0 ];
		guess [ 1 ] = store [ 1 ];
		guess [ 2 ] = store [ 2 ];

		r [ 0 ] = B [ 0 ] - ( A [ 0 ] [ 0 ] * guess [ 0 ] + A [ 0 ] [ 1 ] * guess [ 1 ] + A [ 0 ] [ 2 ] * guess [ 2 ] );
		r [ 1 ] = B [ 1 ] - ( A [ 1 ] [ 0 ] * guess [ 0 ] + A [ 1 ] [ 1 ] * guess [ 1 ] + A [ 1 ] [ 2 ] * guess [ 2 ] );
		r [ 2 ] = B [ 2 ] - ( A [ 2 ] [ 0 ] * guess [ 0 ] + A [ 2 ] [ 1 ] * guess [ 1 ] + A [ 2 ] [ 2 ] * guess [ 2 ] );

		for ( i = 0; i < n; i++ )
		{
			guess [ 0 ] = guess [ 0 ] + c * r [ 0 ];
			guess [ 1 ] = guess [ 1 ] + c * r [ 1 ];
			guess [ 2 ] = guess [ 2 ] + c * r [ 2 ];

			r [ 0 ] = B [ 0 ] - ( A [ 0 ] [ 0 ] * guess [ 0 ] + A [ 0 ] [ 1 ] * guess [ 1 ] + A [ 0 ] [ 2 ] * guess [ 2 ] );
			r [ 1 ] = B [ 1 ] - ( A [ 1 ] [ 0 ] * guess [ 0 ] + A [ 1 ] [ 1 ] * guess [ 1 ] + A [ 1 ] [ 2 ] * guess [ 2 ] );
			r [ 2 ] = B [ 2 ] - ( A [ 2 ] [ 0 ] * guess [ 0 ] + A [ 2 ] [ 1 ] * guess [ 1 ] + A [ 2 ] [ 2 ] * guess [ 2 ] );
		}
	}
/*
	if ( guess [ 0 ] > store [ 0 ] + 1 || guess [ 0 ] < store [ 0 ] - 1 ||
		guess [ 1 ] > store [ 1 ] + 1 || guess [ 1 ] < store [ 1 ] - 1 ||
		guess [ 2 ] > store [ 2 ] + 1 || guess [ 2 ] < store [ 2 ] - 1 )
	{
		printf ( "water let point go from %f,%f,%f to %f,%f,%f\n", 
			store [ 0 ], store [ 1 ], store [ 2 ], guess [ 0 ], guess [ 1 ], guess [ 2 ] );
		printf ( "A is %f,%f,%f %f,%f,%f %f,%f,%f\n", A [ 0 ] [ 0 ], A [ 0 ] [ 1 ], A [ 0 ] [ 2 ],
			A [ 1 ] [ 0 ] , A [ 1 ] [ 1 ], A [ 1 ] [ 2 ], A [ 2 ] [ 0 ], A [ 2 ] [ 1 ], A [ 2 ] [ 2 ] );
		printf ( "B is %f,%f,%f\n", B [ 0 ], B [ 1 ], B [ 2 ] );
		printf ( "bounding box is %f,%f,%f to %f,%f,%f\n", 
			box->begin.x, box->begin.y, box->begin.z, box->end.x, box->end.y, box->end.z );
	}
*/
}

float calcPoint ( float halfA[], float b[], float btb, float midpoint[], float rvalue[], BoundingBoxf *box, float *mat )
{
	float newB [ 3 ];
	float a [ 3 ] [ 3 ];
	float inv [ 3 ] [ 3 ];
	float w [ 3 ];
	float u [ 3 ] [ 3 ];

	a [ 0 ] [ 0 ] = halfA [ 0 ];
	a [ 0 ] [ 1 ] = halfA [ 1 ];
	a [ 0 ] [ 2 ] = halfA [ 2 ];
	a [ 1 ] [ 1 ] = halfA [ 3 ];
	a [ 1 ] [ 2 ] = halfA [ 4 ];
	a [ 1 ] [ 0 ] = halfA [ 1 ];
	a [ 2 ] [ 0 ] = halfA [ 2 ];
	a [ 2 ] [ 1 ] = halfA [ 4 ];
	a [ 2 ] [ 2 ] = halfA [ 5 ];

	switch ( method )
	{
	case 0:
	case 1:
	case 2:
		rvalue [ 0 ] = midpoint [ 0 ];
		rvalue [ 1 ] = midpoint [ 1 ];
		rvalue [ 2 ] = midpoint [ 2 ];

		descent ( a, b, rvalue, box );
		return calcError ( a, b, btb, rvalue );
		break;
	case 3:
		matInverse ( a, midpoint, inv, w, u );


		newB [ 0 ] = b [ 0 ] - a [ 0 ] [ 0 ] * midpoint [ 0 ] - a [ 0 ] [ 1 ] * midpoint [ 1 ] - a [ 0 ] [ 2 ] * midpoint [ 2 ];
		newB [ 1 ] = b [ 1 ] - a [ 1 ] [ 0 ] * midpoint [ 0 ] - a [ 1 ] [ 1 ] * midpoint [ 1 ] - a [ 1 ] [ 2 ] * midpoint [ 2 ];
		newB [ 2 ] = b [ 2 ] - a [ 2 ] [ 0 ] * midpoint [ 0 ] - a [ 2 ] [ 1 ] * midpoint [ 1 ] - a [ 2 ] [ 2 ] * midpoint [ 2 ];

		rvalue [ 0 ] = inv [ 0 ] [ 0 ] * newB [ 0 ] + inv [ 1 ] [ 0 ] * newB [ 1 ] + inv [ 2 ] [ 0 ] * newB [ 2 ] + midpoint [ 0 ];
		rvalue [ 1 ] = inv [ 0 ] [ 1 ] * newB [ 0 ] + inv [ 1 ] [ 1 ] * newB [ 1 ] + inv [ 2 ] [ 1 ] * newB [ 2 ] + midpoint [ 1 ];
		rvalue [ 2 ] = inv [ 0 ] [ 2 ] * newB [ 0 ] + inv [ 1 ] [ 2 ] * newB [ 1 ] + inv [ 2 ] [ 2 ] * newB [ 2 ] + midpoint [ 2 ];
		return calcError ( a, b, btb, rvalue );
		break;
	case 4:
		method = 3;
		calcPoint ( halfA, b, btb, midpoint, rvalue, box, mat );
		method = 4;
/*
		int rank;
		float eqs [ 4 ] [ 4 ];

		// form the square matrix
		eqs [ 0 ] [ 0 ] = mat [ 0 ];
		eqs [ 0 ] [ 1 ] = mat [ 1 ];
		eqs [ 0 ] [ 2 ] = mat [ 2 ];
		eqs [ 0 ] [ 3 ] = mat [ 3 ];
		eqs [ 1 ] [ 1 ] = mat [ 4 ];
		eqs [ 1 ] [ 2 ] = mat [ 5 ];
		eqs [ 1 ] [ 3 ] = mat [ 6 ];
		eqs [ 2 ] [ 2 ] = mat [ 7 ];
		eqs [ 2 ] [ 3 ] = mat [ 8 ];
		eqs [ 3 ] [ 3 ] = mat [ 9 ];
		eqs [ 1 ] [ 0 ] = eqs [ 2 ] [ 0 ] = eqs [ 2 ] [ 1 ] = eqs [ 3 ] [ 0 ] = eqs [ 3 ] [ 1 ] = eqs [ 3 ] [ 2 ] = 0;

		// compute the new QR decomposition and rank
		rank = qr ( eqs );

		method = 2;
		calcPoint ( halfA, b, btb, midpoint, rvalue, box, mat );
		method = 4;
/*
		if ( rank == 0 )
		{
			// it's zero, no equations
			rvalue [ 0 ] = midpoint [ 0 ];
			rvalue [ 1 ] = midpoint [ 1 ];
			rvalue [ 2 ] = midpoint [ 2 ];
		}
		else 
		{
			if ( rank == 1 )
			{
				// one equation, it's a plane
				float temp = ( eqs [ 0 ] [ 0 ] * midpoint [ 0 ] + eqs [ 0 ] [ 1 ] * midpoint [ 1 ] + eqs [ 0 ] [ 2 ] * midpoint [ 2 ] - eqs [ 0 ] [ 3 ] ) /
							( eqs [ 0 ] [ 0 ] * eqs [ 0 ] [ 0 ] + eqs [ 0 ] [ 1 ] * eqs [ 0 ] [ 1 ] + eqs [ 0 ] [ 2 ] * eqs [ 0 ] [ 2 ] );

				rvalue [ 0 ] = midpoint [ 0 ] - temp * eqs [ 0 ] [ 0 ];
				rvalue [ 1 ] = midpoint [ 1 ] - temp * eqs [ 0 ] [ 1 ];
				rvalue [ 2 ] = midpoint [ 2 ] - temp * eqs [ 0 ] [ 2 ];
			}
			else
			{
				if ( rank == 2 )
				{
					// two equations, it's a line
					float a, b, c, d, e, f, g;

					// reduce back to upper triangular
					qr ( eqs, 2, 0.000001f );

					a = eqs [ 0 ] [ 0 ];
					b = eqs [ 0 ] [ 1 ];
					c = eqs [ 0 ] [ 2 ];
					d = eqs [ 0 ] [ 3 ];
					e = eqs [ 1 ] [ 1 ];
					f = eqs [ 1 ] [ 2 ];
					g = eqs [ 1 ] [ 3 ];

					// solved using the equations
					// ax + by + cz = d
					//      ey + fz = g
					// minimize (x-px)^2 + (y-py)^2 + (z-pz)^2
					if ( a > 0.000001f || a < -0.000001f )
					{
						if ( e > 0.00000f || e < -0.000001f )
						{
							rvalue [ 2 ] = ( -1 * b * d * e * f + ( a * a + b * b ) * g * f + c * e * ( d * e - b * g ) +
								a * e * ( ( b * f - c * e ) * midpoint [ 0 ] - a * f * midpoint [ 1 ] + a * e * midpoint [ 2 ] ) ) /
								( a * a * ( e * e + f * f ) + ( c * e - b * f ) * ( c * e - b * f ) );
							rvalue [ 1 ] = ( g - f * rvalue [ 2 ] ) / e;
							rvalue [ 0 ] = ( d - b * rvalue [ 1 ] - c * rvalue [ 2 ] ) / a;
						}
						else
						{
							// slightly degenerate case where e==0
							rvalue [ 2 ] = g / f;
							rvalue [ 1 ] = ( b * d * f - b * c * g - a * b * f * midpoint [ 0 ] + a * a * f * midpoint [ 1 ] ) /
								( a * a * f + b * b * f );
							rvalue [ 0 ] = ( d - b * rvalue [ 1 ] - c * rvalue [ 2 ] ) / a;
						}
					}
					else
					{
						// degenerate case where a==0 so e == 0 (upper triangular)

						rvalue [ 2 ] = g / f;
						rvalue [ 1 ] = ( d - c * rvalue [ 2 ] ) / b;
						rvalue [ 0 ] = midpoint [ 0 ];
					}

				}
				else
				{
					// must be three equations or more now... solve using back-substitution
					rvalue [ 2 ] = mat [ 8 ] / mat [ 7 ];
					rvalue [ 1 ] = ( mat [ 6 ] - mat [ 5 ] * rvalue [ 2 ] ) / mat [ 4 ];
					rvalue [ 0 ] = ( mat [ 3 ] - mat [ 2 ] * rvalue [ 2 ] - mat [ 1 ] * rvalue [ 1 ] ) / mat [ 0 ];
				}
			}
		}
*/

		float ret;
		float tmp;

		ret = mat [ 9 ] * mat [ 9 ];

		tmp = mat [ 0 ] * rvalue [ 0 ] + mat [ 1 ] * rvalue [ 1 ] + mat [ 2 ] * rvalue [ 2 ] - mat [ 3 ];
		ret += tmp * tmp;

		tmp = mat [ 4 ] * rvalue [ 1 ] + mat [ 5 ] * rvalue [ 2 ] - mat [ 6 ];
		ret += tmp * tmp;

		tmp = mat [ 7 ] * rvalue [ 2 ] - mat [ 8 ];
		ret += tmp * tmp;

		return ret;

		break;
	case 5:
		rvalue [ 0 ] = midpoint [ 0 ];
		rvalue [ 1 ] = midpoint [ 1 ];
		rvalue [ 2 ] = midpoint [ 2 ];

		return calcError ( a, b, btb, rvalue );
	}

	return 0 ;
}

}//namespace Solver2




	float determinant(
		float a, float b, float c,
		float d, float e, float f,
		float g, float h, float i )
	{
		return a * e * i 
			+ b * f * g
			+ c * d * h
			- a * f * h
			- b * d * i
			- c * e * g;
	}


	/* Solves for x in  A*x = b.
	'A' contains the matrix row-wise.
	'b' and 'x' are column vectors.
	Uses cramers rule.
	*/
	V3f solve3x3(const float* A, const float b[3]) {
		double det = determinant(
			A[0*3+0], A[0*3+1], A[0*3+2],
			A[1*3+0], A[1*3+1], A[1*3+2],
			A[2*3+0], A[2*3+1], A[2*3+2]);

		if (abs(det) <= 1e-12) {
			//std::cerr << "Oh-oh - small determinant: " << det << std::endl;
//Unimplemented;
mxASSERT(false);
			//return CV3f(NAN);
			return V3_SetAll(BIG_NUMBER);
		}

		const V3f result = {
			determinant(
				b[0],    A[0*3+1], A[0*3+2],
				b[1],    A[1*3+1], A[1*3+2],
				b[2],    A[2*3+1], A[2*3+2]
			),
			determinant(
				A[0*3+0], b[0],    A[0*3+2],
				A[1*3+0], b[1],    A[1*3+2],
				A[2*3+0], b[2],    A[2*3+2]
			),
			determinant(
				A[0*3+0], A[0*3+1], b[0],
				A[1*3+0], A[1*3+1], b[1],
				A[2*3+0], A[2*3+1], b[2]
			)
		};

		return result / float(det);
	}

	/*
	Solves A*x = b for over-determined systems.

	Solves using  At*A*x = At*b   trick where At is the transponate of A
	*/
	V3f leastSquares(size_t N, const V3f* A, const float* b)
	{
		if (N == 3) {
			const float A_mat[3*3] = {
				A[0].x, A[0].y, A[0].z,
				A[1].x, A[1].y, A[1].z,
				A[2].x, A[2].y, A[2].z,
			};
			return solve3x3(A_mat, b);
		}

		float At_A[3][3];
		float At_b[3];

		for (int i=0; i<3; ++i) {
			for (int j=0; j<3; ++j) {
				float sum = 0;
				for (size_t k=0; k<N; ++k) {
					sum += A[k][i] * A[k][j];
				}
				At_A[i][j] = sum;
			}
		}

		for (int i=0; i<3; ++i) {
			float sum = 0;

			for (size_t k=0; k<N; ++k) {
				sum += A[k][i] * b[k];
			}

			At_b[i] = sum;
		}


		/*
		// Improve conditioning:
		float offset = 0.0001;
		At_A[0][0] += offset;
		At_A[1][1] += offset;
		At_A[2][2] += offset;
		*/

		mxSTATIC_ASSERT(sizeof(At_A) == 9*sizeof(float));

		return solve3x3(&At_A[0][0], At_b);
	}



mxBEGIN_REFLECT_ENUM( vxDRAW_MODE_T )
	mxREFLECT_ENUM_ITEM( NORMAL, vxDRAW_NORMAL ),
	//mxREFLECT_ENUM_ITEM( BLOBBY, vxDRAW_BLOBBY ),
	mxREFLECT_ENUM_ITEM( SMOOTH, vxDRAW_SMOOTH ),
	mxREFLECT_ENUM_ITEM( BLOXEL, vxDRAW_BLOXEL ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( CLAMP_MODE_E )
	mxREFLECT_ENUM_ITEM( CLAMP_NONE, CLAMP_MODE::CLAMP_NONE ),
	mxREFLECT_ENUM_ITEM( CLAMP_TO_CELL_BOUNDS, CLAMP_MODE::CLAMP_TO_CELL_BOUNDS ),
	mxREFLECT_ENUM_ITEM( CLAMP_TO_MASS_POINT, CLAMP_MODE::CLAMP_TO_MASS_POINT ),
mxEND_REFLECT_ENUM


QEF_Solver::Input::Input()
{
	AABBf_Clear(&bounds);
	threshold = 1e-5f;
	maxIterations = ~0;
}
ERet QEF_Solver::Input::addPoint( const V3f& _position, const V3f& _normal )
{
#if DBG_QUADRICS
	mxASSERT(V3_IsNormalized(_normal));	// annoying when VX_CONFIG_PACK_HERMITE_DATA is enabled
	mxASSERT(num_points < MAX_POINTS);
#endif
	positions[num_points] = _position;
	normals[num_points] = _normal;
	num_points++;
	return ALL_OK;
}
V3f QEF_Solver::Input::CalculateAveragePoint() const
{
	V3f masspoint = V3_Zero();
	for( int i = 0; i < this->num_points; i++ ) {
		masspoint += this->positions[ i ];
	}
	masspoint /= float(this->num_points);
	return masspoint;
}
V3f QEF_Solver::Input::CalculateAverageNormal() const
{
	V3f averageNormal = V3_Zero();
	for( int i = 0; i < this->num_points; i++ ) {
		averageNormal += this->normals[ i ];
	}
	averageNormal /= (float) this->num_points;
	return V3_Normalized(averageNormal);
}
/// Estimates the angle of the normal cone.
float QEF_Solver::Input::EstimateNormalConeAngle() const
{
	const QEF_Solver::Input& solverInput = *this;
	mxBIBREF("Kobbelt L., Botsch M., Schwanecke U., Seidel. P. Feature sensitive surface extraction from volume data, SIGGRAPH 2001, p.7");
	// Estimate the angle of the normal cone.
	// find the maximal angle formed by normals => find the minimum cos(angle)
	float minimumTheta = 1.0f;	// the lower this value, the 'sharper' the feature
	for( int i = 0; i < solverInput.num_points - 1; i++ )
	{
		for( int j = i; j < solverInput.num_points; j++ )
		{
			const V3f& normalA = solverInput.normals[i];
			const V3f& normalB = solverInput.normals[j];
			const float dp = V3_Dot( normalA, normalB );
			if( dp < minimumTheta ) {
				minimumTheta = dp;	// minimum cos(angle) => maximum angle
			}
		}
	}
	return minimumTheta;
}

// Basically, you start the vertex in the centre of the cell.
// Then you average all the vectors taken from the vertex to each plane
// and move the vertex along that resultant, and repeat this step a fixed number of times.
// http://gamedev.stackexchange.com/a/83757
mxBIBREF("Efficient and High Quality Contouring of Isosurfaces on Uniform Grids, 2009, Particle-based minimizer function");
//
void QEF_Solver_ParticleBased::Solve( const Input& input, Output &output ) const
{
	// Center the particle on the masspoint.
	V3f masspoint = V3_Zero();
	for( int i = 0; i < input.num_points; i++ )
	{
		masspoint += input.positions[ i ];
	}
	masspoint /= (float) input.num_points;

	V3f particlePosition = masspoint;

	V4f planes[Input::MAX_POINTS];
	for( int i = 0; i < input.num_points; i++ )
	{
		const V3f& planePoint = input.positions[ i ];
		const V3f& planeNormal = input.normals[ i ];
		planes[ i ] = Plane_FromPointNormal( planePoint, planeNormal );
	}

	// find the force that starts moving the particle from the masspoint towards the iso-surface
	V3f force;

    const float forceThreshhold = squaref(input.threshold);
    //const float forceRatio = 0.75f;
	const float forceRatio = 0.05f;

	int iteration = 0;
	while( iteration++ < MAX_ITERATIONS )
	{
		force = V3_Zero();

		// For each intersection point:
		for (int i = 0; i < input.num_points; i++)
		{
			const V3f& point = input.positions[ i ];
			force += Plane_GetNormal( planes[i] ) * Plane_PointDistance( planes[i], point );
		}

		// Average the force over all the intersection points, and multiply 
		// with a ratio and some damping to avoid instabilities.
		float damping = 1.0f - ((float) iteration) / MAX_ITERATIONS;

		force *= forceRatio * damping / input.num_points;

		// Apply the force.
		particlePosition += force;

		// If the force was almost null, break.
		if( V3_LengthSquared(force) < forceThreshhold )
		{
			break;
		}
	}

	output.position = particlePosition;
	output.error = V3_Length(force);
	output.feature = Feature_None;
}

void QEF_Solver_SVD::Solve( const Input& input, Output &output ) const
{
	mxASSERT( input.num_points );
UNDONE;

#if 0
	svd::QefSolver	qef;

	for( int i = 0; i < input.num_points; i++ )
	{
		const V3f& planePoint = input.positions[ i ];
		const V3f& planeNormal = input.normals[ i ];
		qef.add(
			planePoint.x, planePoint.y, planePoint.z,
			planeNormal.x, planeNormal.y, planeNormal.z
		);
	}

	const int QEF_SWEEPS = 5;

	svd::Vec3 qefPosition;
	int featureType = Feature_None;
	const float error = qef.solve( qefPosition, input.threshold, QEF_SWEEPS, 0.1f /* pseudo-inverse threshold */, featureType );

	output.position = V3_Set(qefPosition.x, qefPosition.y, qefPosition.z);
	output.error = error;
	output.feature = (EFeatureType) featureType;

	mxSTATIC_ASSERT(sizeof(output.qef) > sizeof(qef.getData()));
	const svd::QefData& qefData = qef.getData();
	memcpy( &output.qef, &qefData, sizeof(qefData) );
	//output.clamped = false;

//#if 0
//	//NOTE: this is important!
//	if( !AABBf_ContainsPoint( input.bounds, output.position ) )
//	{
//		const svd::Vec3& mp = qef.getMassPoint();
//		output.position = V3_Set(mp.x, mp.y, mp.z);
//
//output.position.x = Clamp<F32>(output.position.x, input.bounds.mins.x, input.bounds.maxs.x);
//output.position.y = Clamp<F32>(output.position.y, input.bounds.mins.y, input.bounds.maxs.y);
//output.position.z = Clamp<F32>(output.position.z, input.bounds.mins.z, input.bounds.maxs.z);
//
//		output.clamped = true;
////VX_STATS( g_num_clamped_vertices++ );
//	}
//#endif

#endif

}

void QEF_Solver_QR::Solve( const Input& solverInput, Output &output ) const
{
	// QEF
	float ata[6], atb[3], btb ;

	for ( int i = 0 ; i < 6 ; i ++ )
	{
		ata[i] = 0 ;
	}

	// Minimizer
	float mp[3] ;
	for ( int i = 0 ; i < 3 ; i ++ )
	{
		mp[i] = 0 ;
		atb[i] = 0 ;
	}
	btb = 0 ;


	float pt[3] ={0,0,0} ;

	for ( int i = 0 ; i < solverInput.num_points ; i ++ )
	{
		float* norm = (float*) &solverInput.normals[i] ;
		float* p = (float*) &solverInput.positions[i] ;

		// printf("Norm: %f, %f, %f Pts: %f, %f, %f\n", norm[0], norm[1], norm[2], p[0], p[1], p[2] ) ;

		// QEF
		ata[ 0 ] += (float) ( norm[ 0 ] * norm[ 0 ] );
		ata[ 1 ] += (float) ( norm[ 0 ] * norm[ 1 ] );
		ata[ 2 ] += (float) ( norm[ 0 ] * norm[ 2 ] );
		ata[ 3 ] += (float) ( norm[ 1 ] * norm[ 1 ] );
		ata[ 4 ] += (float) ( norm[ 1 ] * norm[ 2 ] );
		ata[ 5 ] += (float) ( norm[ 2 ] * norm[ 2 ] );

		double pn = p[0] * norm[0] + p[1] * norm[1] + p[2] * norm[2] ;

		atb[ 0 ] += (float) ( norm[ 0 ] * pn ) ;
		atb[ 1 ] += (float) ( norm[ 1 ] * pn ) ;
		atb[ 2 ] += (float) ( norm[ 2 ] * pn ) ;

		btb += (float) pn * (float) pn ;

		// Minimizer
		pt[0] += p[0] ;
		pt[1] += p[1] ;
		pt[2] += p[2] ;
	}

	pt[0] /= solverInput.num_points ;
	pt[1] /= solverInput.num_points ;
	pt[2] /= solverInput.num_points ;

	// Solve
	float mat[10] ;
	Solver2::BoundingBoxf box;
	box.begin.x = solverInput.bounds.min_corner.x;
	box.begin.y = solverInput.bounds.min_corner.y;
	box.begin.z = solverInput.bounds.min_corner.z;
	box.end.x = solverInput.bounds.max_corner.x;
	box.end.y = solverInput.bounds.max_corner.y;
	box.end.z = solverInput.bounds.max_corner.z;

	float error = Solver2::calcPoint( ata, atb, btb, pt, mp, &box, mat ) ;

#if 0
	// CLAMP
	if ( mp[0] < box.begin.x || mp[1] < box.begin.y || mp[2] < box.begin.z ||
		mp[0] > box.end.x || mp[1] > box.end.y || mp[2] > box.end.z )
	{
		mp[0] = pt[0] ;
		mp[1] = pt[1] ;
		mp[2] = pt[2] ;
	}
#endif

	output.position = V3_Set(mp[0], mp[1], mp[2]);
	output.error = error;
	output.feature = Feature_None;
	//output.qef = qef.getData();
	//output.clamped = false;
}

#if WITH_EIGEN_LIBRARY

void QEF_Solver_SVD_Eigen::Solve( const Input& input, Output &output ) const
{
	mxSTOLEN("https://github.com/mkeeter/ao/blob/master/kernel/src/render/octree.cpp");
	// Center the particle on the masspoint.
	const V3f masspoint = input.CalculateAveragePoint();

    /*  The A matrix is of the form
     *  [n1x, n1y, n1z]
     *  [n2x, n2y, n2z]
     *  [n3x, n3y, n3z]
     *  ...
     *  (with one row for each Hermite intersection)
     */

	Eigen::MatrixX3f A(input.num_points, 3);
	for (int i=0; i < input.num_points; ++i)
	{
		const V3f& N = input.normals[i];
		A.row(i) << Eigen::Vector3f(N.x, N.y, N.z).transpose();
	}

	/*  The B matrix is of the form
     *  [p1 . n1]
     *  [p2 . n2]
     *  [p3 . n3]
     *  ...
     *  (with one row for each Hermite intersection)
     *
     *  Positions are preemptively shifted so that the center of the contour
     *  is at 0, 0, 0 (since the least-squares fix minimizes distance to the
     *  origin); we'll unshift afterwards.
     */
    Eigen::VectorXf B(input.num_points, 1);
    for (int i=0; i < input.num_points; ++i)
    {
		const V3f& P = input.positions[i];
		const V3f& N = input.normals[i];
		const F32 dot = V3_Dot( N, P - masspoint );
        B.row(i) << dot;
    }

	// Use singular value decomposition to solve the least-squares fit.
	Eigen::JacobiSVD<Eigen::MatrixX3f> svd( A,
		Eigen::ComputeFullU | Eigen::ComputeFullV
	);

	// Truncate singular values below 0.1
	const Eigen::VectorXf singular = svd.singularValues();
	//svd.setThreshold(0.1 / singular.maxCoeff());	// truncation based on relative magnitudes

	mxBIBREF("techreport02408 (Dual Contouring - The secret sauce)[2002], 2.1 Solving QEF's");
	// The dimension of the feature (1=plane,2=edge,3=corner) is computed during the pseudoinverse
	// and is equal to three minus the number of singular values truncated.
	svd.setThreshold(0.1f);	// truncation based on absolute magnitudes
	const int rank = svd.rank();

	// Solve the equation and convert back to cell coordinates
	const Eigen::Vector3f solution = svd.solve(B);
	output.position = V3_Set(solution.x(), solution.y(), solution.z()) + masspoint;

	// find and return QEF residual
	const Eigen::VectorXf m = A * solution - B;
	output.error = m.transpose() * m;

	mxASSERT(rank > 0);
	output.feature = (EFeatureType) std::max( rank - 1, 0 );
}

#endif // WITH_EIGEN_LIBRARY


mxSTOLEN("https://github.com/nickgildea/qef/blob/master/qef.cl");
namespace svd2
{

// minimal SVD implementation for calculating feature points from hermite data
// public domain

typedef V4f float4;
typedef float mat3x3[3][3];
typedef float mat3x3_tri[6];

#define SVD_NUM_SWEEPS 5

// SVD
////////////////////////////////////////////////////////////////////////////////

#define PSUEDO_INVERSE_THRESHOLD (0.1f)

void svd_mul_matrix_vec(float4* result, const mat3x3& a, const float4& b)
{
	*result = V4f::set(
		V4f::dot( V4f::set(a[0][0], a[0][1], a[0][2], 0.f), b ),
		V4f::dot( V4f::set(a[1][0], a[1][1], a[1][2], 0.f), b ),
		V4f::dot( V4f::set(a[2][0], a[2][1], a[2][2], 0.f), b ),
		0.f	// W
	);
}

void givens_coeffs_sym(float a_pp, float a_pq, float a_qq, float* c, float* s) {
	if (a_pq == 0.f) {
		*c = 1.f;
		*s = 0.f;
		return;
	}
	float tau = (a_qq - a_pp) / (2.f * a_pq);
	float stt = sqrt(1.f + tau * tau);
	float tan = 1.f / ((tau >= 0.f) ? (tau + stt) : (tau - stt));
	*c = mmInvSqrt(1.f + tan * tan);
	*s = tan * (*c);
}

void svd_rotate_xy(float* x, float* y, float c, float s) {
	float u = *x; float v = *y;
	*x = c * u - s * v;
	*y = s * u + c * v;
}

void svd_rotateq_xy(float* x, float* y, float* a, float c, float s) {
	float cc = c * c; float ss = s * s;
	float mx = 2.0 * c * s * (*a);
	float u = *x; float v = *y;
	*x = cc * u - mx + ss * v;
	*y = ss * u + mx + cc * v;
}

void svd_rotate(mat3x3 &vtav, mat3x3 &v, int a, int b) {
	if (vtav[a][b] == 0.0) return;
	
	float c, s;
	givens_coeffs_sym(vtav[a][a], vtav[a][b], vtav[b][b], &c, &s);

	float x, y, z;
	x = vtav[a][a]; y = vtav[b][b]; z = vtav[a][b];
	svd_rotateq_xy(&x,&y,&z,c,s);
	vtav[a][a] = x; vtav[b][b] = y; vtav[a][b] = z;

	x = vtav[0][3-b]; y = vtav[1-a][2];
	svd_rotate_xy(&x, &y, c, s);
	vtav[0][3-b] = x; vtav[1-a][2] = y;

	vtav[a][b] = 0.0;
	
	x = v[0][a]; y = v[0][b];
	svd_rotate_xy(&x, &y, c, s);
	v[0][a] = x; v[0][b] = y;

	x = v[1][a]; y = v[1][b];
	svd_rotate_xy(&x, &y, c, s);
	v[1][a] = x; v[1][b] = y;

	x = v[2][a]; y = v[2][b];
	svd_rotate_xy(&x, &y, c, s);
	v[2][a] = x; v[2][b] = y;
}

void svd_solve_sym(const mat3x3_tri& a, float4* sigma, mat3x3 &v) {
	// assuming that A is symmetric: can optimize all operations for 
	// the upper right triagonal
	mat3x3 vtav;
	vtav[0][0] = a[0]; vtav[0][1] = a[1]; vtav[0][2] = a[2]; 
	vtav[1][0] = 0.f;  vtav[1][1] = a[3]; vtav[1][2] = a[4]; 
	vtav[2][0] = 0.f;  vtav[2][1] = 0.f;  vtav[2][2] = a[5]; 

	// assuming V is identity: you can also pass a matrix the rotations
	// should be applied to. (U is not computed)
	for (int i = 0; i < SVD_NUM_SWEEPS; ++i) {
		svd_rotate(vtav, v, 0, 1);
		svd_rotate(vtav, v, 0, 2);
		svd_rotate(vtav, v, 1, 2);
	}

	*sigma = V4f::set( vtav[0][0], vtav[1][1], vtav[2][2], 0.f );
}

float svd_invdet(float x, float tol) {
	return (fabs(x) < tol || fabs(1.0f / x) < tol) ? 0 : (1.0f / x);
}

EFeatureType svd_pseudoinverse(mat3x3 &o, const float4& sigma, const mat3x3& v) {
	float d0 = svd_invdet(sigma.x, PSUEDO_INVERSE_THRESHOLD);
	float d1 = svd_invdet(sigma.y, PSUEDO_INVERSE_THRESHOLD);
	float d2 = svd_invdet(sigma.z, PSUEDO_INVERSE_THRESHOLD);

	o[0][0] = v[0][0] * d0 * v[0][0] + v[0][1] * d1 * v[0][1] + v[0][2] * d2 * v[0][2];
	o[0][1] = v[0][0] * d0 * v[1][0] + v[0][1] * d1 * v[1][1] + v[0][2] * d2 * v[1][2];
	o[0][2] = v[0][0] * d0 * v[2][0] + v[0][1] * d1 * v[2][1] + v[0][2] * d2 * v[2][2];
	o[1][0] = v[1][0] * d0 * v[0][0] + v[1][1] * d1 * v[0][1] + v[1][2] * d2 * v[0][2];
	o[1][1] = v[1][0] * d0 * v[1][0] + v[1][1] * d1 * v[1][1] + v[1][2] * d2 * v[1][2];
	o[1][2] = v[1][0] * d0 * v[2][0] + v[1][1] * d1 * v[2][1] + v[1][2] * d2 * v[2][2];
	o[2][0] = v[2][0] * d0 * v[0][0] + v[2][1] * d1 * v[0][1] + v[2][2] * d2 * v[0][2];
	o[2][1] = v[2][0] * d0 * v[1][0] + v[2][1] * d1 * v[1][1] + v[2][2] * d2 * v[1][2];
	o[2][2] = v[2][0] * d0 * v[2][0] + v[2][1] * d1 * v[2][1] + v[2][2] * d2 * v[2][2];

	int rank = (d0 != 0) + (d1 != 0) + (d2 != 0);
	int featureType = std::max( rank - 1, 0 );
	//Feature_None	= 0,	//!< ("plane") the vertex doesn't lie on a sharp feature
	//Feature_Edge	= 1,	//!< ("edge") the vertex lies on a crease
	//Feature_Corner	= 2,	//!< ("corner") the vertex is a sharp corner
	return (EFeatureType) featureType;
}

EFeatureType svd_solve_ATA_ATb(
	const mat3x3_tri& ATA, 
	const float4& ATb, 
	float4* x)
{
	mat3x3 V;
	V[0][0] = 1.f; V[0][1] = 0.f; V[0][2] = 0.f;
	V[1][0] = 0.f; V[1][1] = 1.f; V[1][2] = 0.f;
	V[2][0] = 0.f; V[2][1] = 0.f; V[2][2] = 1.f;

	float4 sigma = { 0.f, 0.f, 0.f, 0.f };
	svd_solve_sym(ATA, &sigma, V);
	
	// A = UEV^T; U = A / (E*V^T)
	mat3x3 Vinv;
	EFeatureType ft = svd_pseudoinverse(Vinv, sigma, V);
	svd_mul_matrix_vec(x, Vinv, ATb);
	return ft;
}

void svd_vmul_sym(float4* result, const mat3x3_tri& A, const float4& v) {
	float4 A_row_x = { A[0], A[1], A[2], 0.f };

	(*result).x = V4f::dot(A_row_x, v);
	(*result).y = A[1] * v.x + A[3] * v.y + A[4] * v.z;
	(*result).z = A[2] * v.x + A[4] * v.y + A[5] * v.z;
}

// QEF
////////////////////////////////////////////////////////////////////////////////

void qef_add(
	const V3f& n, const V3f& p,
	mat3x3_tri &ATA, 
	float4* ATb,
	float4* pointaccum)
{
	ATA[0] += n.x * n.x;
	ATA[1] += n.x * n.y;
	ATA[2] += n.x * n.z;
	ATA[3] += n.y * n.y;
	ATA[4] += n.y * n.z;
	ATA[5] += n.z * n.z;

	float b = V3_Dot(p, n);
	(*ATb).x += n.x * b;
	(*ATb).y += n.y * b;
	(*ATb).z += n.z * b;

	(*pointaccum).x += p.x;
	(*pointaccum).y += p.y;
	(*pointaccum).z += p.z;
	(*pointaccum).w += 1.f;
}

float qef_calc_error(const mat3x3_tri& A, const float4& x, const float4& b) {
	float4 tmp;

	svd_vmul_sym(&tmp, A, x);
	tmp = b - tmp;

	return V4f::dot(tmp, tmp);
}

float qef_solve(
	const mat3x3_tri& ATA, 
	const float4& ATb,
	const float4& pointaccum,
	float4* x,
	EFeatureType &_feature
	)
{
	float4 masspoint = {
		pointaccum.x / pointaccum.w,
		pointaccum.y / pointaccum.w,
		pointaccum.z / pointaccum.w,
		1.0f
	};

	float4 A_mp = { 0.f, 0.f, 0.f, 0.f };
	svd_vmul_sym(&A_mp, ATA, masspoint);
	A_mp = ATb - A_mp;

	_feature = svd_solve_ATA_ATb(ATA, A_mp, x);

	float error = qef_calc_error(ATA, *x, ATb);
	(*x) += masspoint;
		
	return error;
}

float4 qef_solve_from_points(
	const V3f* positions,
	const V3f* normals,
	const size_t count,
	float* error,
	EFeatureType &_feature
	)
{
	float4 pointaccum = {0.f, 0.f, 0.f, 0.f};
	float4 ATb = {0.f, 0.f, 0.f, 0.f};
	mat3x3_tri ATA = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
	
	for (int i= 0; i < count; ++i) {
		qef_add(normals[i],positions[i],ATA,&ATb,&pointaccum);
	}
	
	float4 solved_position = { 0.f, 0.f, 0.f, 0.f };
	*error = qef_solve( ATA,ATb,pointaccum,&solved_position, _feature );
	return solved_position;
}

}//namespace svd2

void QEF_Solver_SVD2::Solve( const Input& input, Output &output ) const
{
	svd2::float4 pos = svd2::qef_solve_from_points( input.positions, input.normals, input.num_points, &output.error, output.feature );
	output.position = V3f::fromXYZ(pos);
}

/*
A C Library for Computing Singular Value Decompositions
http://tedlab.mit.edu/~dr/SVDLIBC/
*/
