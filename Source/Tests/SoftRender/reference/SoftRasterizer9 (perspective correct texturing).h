#pragma once

#include <Base/Util/Rectangle.h>



#if SOFT_RENDER_DEBUG
	inline
	void DbgDrawRect( const SoftRenderContext& context, SoftPixel color, int topLeftX, int topLeftY, int width, int height )
	{
		const int W = context.W;	// viewport width
		const int H = context.H;	// viewport height

		TRectArea<int>	screen(0,0,W-1,H-1);
		TRectArea<int>	rect(topLeftX,topLeftY,width,height);
		rect.Clip(screen);

		context.renderer->DrawLine2D( rect.TopLeftX, rect.TopLeftY, rect.TopLeftX + rect.Width, rect.TopLeftY, color );
		context.renderer->DrawLine2D( rect.TopLeftX + rect.Width, rect.TopLeftY, rect.TopLeftX + rect.Width, rect.TopLeftY + rect.Height, color );
		context.renderer->DrawLine2D( rect.TopLeftX + rect.Width, rect.TopLeftY + rect.Height, rect.TopLeftX, rect.TopLeftY + rect.Height, color );
		context.renderer->DrawLine2D( rect.TopLeftX, rect.TopLeftY + rect.Height, rect.TopLeftX, rect.TopLeftY, color );
	}
#endif // SOFT_RENDER_DEBUG


static inline
void ComputeInterpolants(
	FLOAT fX, FLOAT fY,
	FLOAT fX3, FLOAT fY3,
	FLOAT fDeltaY23, FLOAT fDeltaX23,
	FLOAT fDeltaY31, FLOAT fDeltaX31,
	FLOAT invDenominator,
	FLOAT * __restrict t1, FLOAT * __restrict t2, FLOAT * __restrict t3
)
{
	const float fDX = fX - fX3;
	const float fDY = fY - fY3;

	// calculate barycentric coordinates
	*t1 = ( fDeltaY23 * fDX - fDeltaX23 * fDY ) * invDenominator;
	*t2 = ( fDeltaY31 * fDX - fDeltaX31 * fDY ) * invDenominator;
	*t3 = 1.0 - *t1 - *t2;
}

// calculates gradient for interpolation via the plane equation.
// See: http://devmaster.net/forums/topic/1145-advanced-rasterization/page__st__40
// page 3.
// 
static inline
void ComputeGradient(float C, 
					 float di21, float di31,	// deltas of interpolated parameter
					 float dx21, float dx31,
					 float dy21, float dy31,
					 float & dx, float & dy) 
{
	// A * x + B * y + C * z + D = 0
	// z = -A / C * x - B / C * y - D
	// z = z0 + vZ/dx * (x - x0) + vZ/dy * (y - y0)
	// 
	// A = (z3 - z1) * (y2 - y1) - (z2 - z1) * (y3 - y1)
	// B = (x3 - x1) * (z2 - z1) - (x2 - x1) * (z3 - z1)
	// C = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)
	//
	const float A = di31 * dy21 - di21 * dy31;
	const float B = dx31 * di21 - dx21 * di31;

	//Let's say we want to interpolate some component z linearly across the polygon (note that z stands for any interpolant). We can visualize this as a plane going through the x, y and z positions of the triangle, in 3D space. Now, the equation of a 3D plane is generally:
	//A * x + B * y + C * z + D = 0

	//From this we can derive that:
	//z = -A / C * x - B / C * y - D

	//Note that for every step in the x-direction, z increments by -A / C, and likewise it increments by -B / C for every step in the y-direction. So these are the gradients we're looking for to perform linear interpolation. In the plane equation (A, B, C) is the normal vector of the plane. It can easily be computed with a cross product.
	//Now that we have the gradients, let's call them vZ/dx (which is -A / C) and vZ/dy (which is -B / C), we can easily compute z everywhere on the triangle. We know the z value in all three vertex positions. Let's call the one of the first vertex z0, and it's position coordinates (x0, y0). Then a generic z value of a point (x, y) can be computed as:
	//z = z0 + vZ/dx * (x - x0) + vZ/dy * (y - y0)

	//Once you've computed the z value for the center of the starting pixel this way, you can easily add vZ/dx to get the z value for the next pixel, or vZ/dy for the pixel below (with the y-axis going down).
	dx = -A/C;
	dy = -B/C;
}

//static inline
//void InterpolateVaryings( const V2f varsOverW[NUM_VARYINGS] )
//{
//	for( UINT i = 0; i < NUM_VARYINGS; i++ )
//	{
//		for( UINT k = 0; k < 4; k++ )
//		{
//			const float v1v = v1.v[i].f[k] * fInvW1;
//			const float v2v = v2.v[i].f[k] * fInvW2;
//			const float v3v = v3.v[i].f[k] * fInvW3;
//			ComputeGradient(
//				INTERP_C,
//				v2v - v1v, v3v - v1v,
//				fDeltaX21, fDeltaX31,
//				fDeltaY21, fDeltaY31,
//				varsOverW[i].x, varsOverW[i].y
//				);
//		}
//	}
//}

static inline
RGBA32 SINGLE_FLOAT_TO_RGBA32( FLOAT x ) {
	int i = iround( x * 255.f );
	return (i<<24)|(i<<16)|(i<<8)|i;
}

// 'half spaces' rasterization
// See: http://devmaster.net/forums/topic/1145-advanced-rasterization/
// and http://www.cs.unc.edu/~olano/papers/2dh-tri/
//
template< ECullMode CULL_MODE >
static inline
void TemplateRasterizeTriangle( const XVertex& v1, const XVertex& v2, const XVertex& v3, const SoftRenderContext& context )
{
	mxPROFILE_SCOPE("Rasterize Triangle");

	const int W = context.W;	// viewport width
	const int H = context.H;	// viewport height


#if SOFT_RENDER_USE_HALFSPACE_RASTERIZER

	const float fX1 = v1.P.x;
	const float fX2 = v2.P.x;
	const float fX3 = v3.P.x;

	const float fY1 = v1.P.y;
	const float fY2 = v2.P.y;
	const float fY3 = v3.P.y;

	const float fZ1 = v1.P.z;
	const float fZ2 = v2.P.z;
	const float fZ3 = v3.P.z;

	// NOTE: these are actually inverses of W (because of perspective division 1/w in ProjectVertex(), after vertex shader).
	const float fInvW1 = v1.P.w;
	const float fInvW2 = v2.P.w;
	const float fInvW3 = v3.P.w;





	/* excerpt from "A Parallel Algorithm for Polygon Rasterization" by Juan Pineda
	(Computer Graphics, Volume 22, Number 4, August 1988).

	Typically in  3D graphics rendering, polygon vertices are in floating
	point  format  after  3D  transformation  and  projection.  Some
	implementations  round  the  X  and  Y floating  point  ordinates to
	integer  values, so that  simple  integer  line  algorithms can be used
	compute the  triangle  edges.  This rounding can leave  gaps on the
	order of 1/2 pixel wide between adjacent polygons that do not share
	common vertices.  Gaps also occur as a result of the  finite precision
	used in specifying the endpoints, but these gaps are much narrower. 
	Some implementations attempt to eUminate these gaps by growing the 
	edges of triangles to insure  overlap, but these  solutions cause other
	artifacts. 
	In order to minimize these artifacts, it is desirable to render triangle 
	edges as close as possible to the real line between two vertices.  This
	is  conveniently  done  with  this  algorithm  by  performing  the 
	interpolator  setup computations in floating point, and converting to 
	fixed point at the end: 
	*/
	// 28.4 fixed-point coordinates
	// (12 bit resolution, enough for a 4096x4096 color buffer)
	// (4 bits of sub-pixel precision, enough for a 2048x2048 color buffer)

	const INT32 X1 = iround( 16.0f * fX1 );
	const INT32 X2 = iround( 16.0f * fX2 );
	const INT32 X3 = iround( 16.0f * fX3 );

	const INT32 Y1 = iround( 16.0f * fY1 );
	const INT32 Y2 = iround( 16.0f * fY2 );
	const INT32 Y3 = iround( 16.0f * fY3 );

	//const INT32 Z1 = iround( 16.0f * fZ1 );
	//const INT32 Z2 = iround( 16.0f * fZ2 );
	//const INT32 Z3 = iround( 16.0f * fZ3 );

	//const INT32 W1 = iround( 16.0f * fInvW1 );
	//const INT32 W2 = iround( 16.0f * fInvW2 );
	//const INT32 W3 = iround( 16.0f * fInvW3 );

	// deltas
	const INT32 DeltaX12 = X1 - X2;
	const INT32 DeltaX23 = X2 - X3;
	const INT32 DeltaX31 = X3 - X1;

	const INT32 DeltaY12 = Y1 - Y2;
	const INT32 DeltaY23 = Y2 - Y3;
	const INT32 DeltaY31 = Y3 - Y1;




	// back-face culling

	// calculate signed area of triangle in screen space
	// this is actually twice the area
	const int nArea = DeltaX12 * DeltaY31 - DeltaY12 * DeltaX31;
	(void)nArea;

	if( CULL_MODE == Cull_CW && nArea <= 0 )
	{
		return;
	}
	else if( CULL_MODE == Cull_CCW && nArea >= 0 )
	{
		return;
	}



	// 24.8 Fixed-point deltas
	const INT32 FDX12 = DeltaX12 << 4;
	const INT32 FDX23 = DeltaX23 << 4;
	const INT32 FDX31 = DeltaX31 << 4;

	const INT32 FDY12 = DeltaY12 << 4;
	const INT32 FDY23 = DeltaY23 << 4;
	const INT32 FDY31 = DeltaY31 << 4;





	// Compute interpolation data
	const float fDeltaX12 = fX1 - fX2;
	const float fDeltaX23 = fX2 - fX3;
	const float fDeltaX31 = fX3 - fX1;

	const float fDeltaY12 = fY1 - fY2;
	const float fDeltaY23 = fY2 - fY3;
	const float fDeltaY31 = fY3 - fY1;

	const float fDeltaZ21 = fZ2 - fZ1;
	const float fDeltaZ31 = fZ3 - fZ1;

	// NOTE: these are actually inverses of W (because of perspective division 1/w in ProjectVertex(), after vertex shader).
	const float fDeltaInvW21 = fInvW2 - fInvW1;
	const float fDeltaInvW31 = fInvW3 - fInvW1;

	const float fDeltaX21 = fX2 - fX1;
	const float fDeltaY21 = fY2 - fY1;

	const float INTERP_C = fDeltaX21 * fDeltaY31 - fDeltaX31 * fDeltaY21;


	// compute gradient for interpolating depth (Z)

	mxPREALIGN(16) V2f	vZ;
	ComputeGradient(
		INTERP_C,
		fDeltaZ21, fDeltaZ31,
		fDeltaX21, fDeltaX31,
		fDeltaY21, fDeltaY31,
		vZ.x, vZ.y
		);

	// Setup inverse W (for interpolating W)
	mxPREALIGN(16) V2f	vInvW;
	ComputeGradient(
		INTERP_C,
		fDeltaInvW21, fDeltaInvW31,
		fDeltaX21, fDeltaX31,
		fDeltaY21, fDeltaY31,
		vInvW.x, vInvW.y
		);

/*
	// to calculate perspective correct u:
	// 1. For every vertex: calculate u' = u / w and w' = 1 / w (which can be linearly interpolated in screen space)
	// 2. For every pixel: linearly interpolate u' _and_ w' and calculate perspective correct u ( interpolated u' / interpolated w' ).

	mxPREALIGN(16) V2f	dInvZ;	
	ComputeGradient(
		INTERP_C,
		fDeltaInvW21, fDeltaInvW31,
		fDeltaX21, fDeltaX31,
		fDeltaY21, fDeltaY31,
		dInvZ.x, dInvZ.y
		);
*/




#if 0
	// Varyings
	// When dividing a value through the w component it can be properly interpolated in image space.
	// We can then recover the w component at any position we want through inversion.
	// This is also why the pseudodepth for depth-testing is not affected by the aforementioned problem. It has already been divided by w during homogenization.

	// varyings multiplied by inverse Z (for interpolation)
	mxPREALIGN(16) V2f	dV[ NUM_VARYINGS ];

	for( UINT i = 0; i < NUM_VARYINGS; i++ )
	{
		for( UINT k = 0; k < 4; k++ )
		{
			const float v1v = v1.v[i].f[k] * fInvZ1;
			const float v2v = v2.v[i].f[k] * fInvZ2;
			const float v3v = v3.v[i].f[k] * fInvZ3;
			ComputeGradient(
				INTERP_C,
				v2v - v1v, v3v - v1v,
				fDeltaX21, fDeltaX31,
				fDeltaY21, fDeltaY31,
				dV[i].x, dV[i].y
				);
		}
	}
#endif

	// Cache perspective correct varyings for the first vertex (for interpolation)
	FLOAT	vars1[ NUM_VARYINGS ];
	for( UINT i = 0; i < NUM_VARYINGS; i++ )
	{
		vars1[i] = v1.vars[i] * fInvW1;
	}

	// varyings multiplied by inverse W (they can be linearly interpolated in screen space)
	mxPREALIGN(16) V2f	varsOverW[ NUM_VARYINGS ];

	// setup varyings for interpolation
	for( UINT i = 0; i < NUM_VARYINGS; i++ )
	{
		const float v1v = v1.vars[i] * fInvW1;
		const float v2v = v2.vars[i] * fInvW2;
		const float v3v = v3.vars[i] * fInvW3;
		ComputeGradient(
			INTERP_C,
			v2v - v1v, v3v - v1v,
			fDeltaX21, fDeltaX31,
			fDeltaY21, fDeltaY31,
			varsOverW[i].x, varsOverW[i].y
			);
	}


	// Block size, standard 8x8 (must be power of two)
	// NOTE: should be 8, must be small enough so that accumulative errors don't grow too much
	enum { BLOCK_SIZE = 8 };	//enum { BLOCK_SIZE_LOG2 = 3 };
	//enum { BLOCK_SIZE = 16 };
	//enum { BLOCK_SIZE = 32 };
	//enum { BLOCK_SIZE = 4 };
	//enum { BLOCK_SIZE = 2 };

	const FLOAT fInvBlockSize = 1.0f / (FLOAT)BLOCK_SIZE;


	// "A Parallel Algorithm for Polygon Rasterization":
	// Since the edge function is linear,  it is possible to compute the value 
	// of the  edge function for a pixel an arbitrary distance L away from a 
	// given point (x,y) :

	// Half-edge constants in 28.4
	INT32 C1 = DeltaY12 * X1 - DeltaX12 * Y1;
	INT32 C2 = DeltaY23 * X2 - DeltaX23 * Y2;
	INT32 C3 = DeltaY31 * X3 - DeltaX31 * Y3;

	// correct for top-left fill convention
	if( DeltaY12 < 0 || (DeltaY12 == 0 && DeltaX12 > 0) ) {
		++C1;
	}
	if( DeltaY23 < 0 || (DeltaY23 == 0 && DeltaX23 > 0) ) {
		++C2;
	}
	if( DeltaY31 < 0 || (DeltaY31 == 0 && DeltaX31 > 0) ) {
		++C3;
	}


	// Bounding rectangle of this triangle in screen space

	// Adding 0xF and shifting right 4 bits is the equivalent of a ceil() function for the 28.4 fixed-point format.
	// The first pixel to be filled is the one strictly to the right of the first edge,
	// so that's why a ceil() operation is used (the fill convention for pixels spot on the edge is taken care of a few lines lower).

#if 0
	INT32 nMinX = Clamp( (Min3(X1, X2, X3) + 0xF) >> 4, 0, W );
	INT32 nMaxX = Clamp( (Max3(X1, X2, X3) + 0xF) >> 4, 0, W );
	INT32 nMinY = Clamp( (Min3(Y1, Y2, Y3) + 0xF) >> 4, 0, H );
	INT32 nMaxY = Clamp( (Max3(Y1, Y2, Y3) + 0xF) >> 4, 0, H );
#else
	INT32 nMinX = (Min3(X1, X2, X3) + 0xF) >> 4;
	INT32 nMaxX = (Max3(X1, X2, X3) + 0xF) >> 4;
	INT32 nMinY = (Min3(Y1, Y2, Y3) + 0xF) >> 4;
	INT32 nMaxY = (Max3(Y1, Y2, Y3) + 0xF) >> 4;
#endif

	//const INT32 iArea = (imaxx - iminx) * (imaxy - iminy);
	//if( iArea == 0 ) {
	//	return;
	//}


	// Start in corner of 8x8 block
	nMinX &= ~(BLOCK_SIZE - 1);
	nMinY &= ~(BLOCK_SIZE - 1);



	//const UINT stride = W * sizeof SoftPixel;	// aka 'pitch'
	SoftPixel* startPixels = context.pixels + nMinY * W;
	ZBufElem* startZBuffer = context.zbuffer + nMinY * W;



	PS_INPUT	pixelShaderInput;
	pixelShaderInput.globals = context.globals;

	PS_OUTPUT	pixelShaderOutput;


	for( UINT iBlockY = nMinY; iBlockY < nMaxY; iBlockY += BLOCK_SIZE )
	{
		for( UINT iBlockX = nMinX; iBlockX < nMaxX; iBlockX += BLOCK_SIZE )
		{
			// Corners of block in 28.4 fixed-point (4 bits of sub-pixel accuracy)
			const UINT FBlockX0 = (iBlockX << 4);
			const UINT FBlockX1 = (iBlockX + (BLOCK_SIZE - 1)) << 4;
			const UINT FBlockY0 = (iBlockY << 4);
			const UINT FBlockY1 = (iBlockY + (BLOCK_SIZE - 1)) << 4;

			// Evaluate half-space functions in the 4 corners of the block

			//OPTIMIZED: extract sign bit instead of converting to bool
		#if 0
			const bool a00 = (C1 + DeltaX12 * FBlockY0 - DeltaY12 * FBlockX0) < 0;
			const bool a10 = (C1 + DeltaX12 * FBlockY0 - DeltaY12 * FBlockX1) < 0;
			const bool a01 = (C1 + DeltaX12 * FBlockY1 - DeltaY12 * FBlockX0) < 0;
			const bool a11 = (C1 + DeltaX12 * FBlockY1 - DeltaY12 * FBlockX1) < 0;
			const UINT a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);	// compose mask for all four corners

			const bool b00 = (C2 + DeltaX23 * FBlockY0 - DeltaY23 * FBlockX0) < 0;
			const bool b10 = (C2 + DeltaX23 * FBlockY0 - DeltaY23 * FBlockX1) < 0;
			const bool b01 = (C2 + DeltaX23 * FBlockY1 - DeltaY23 * FBlockX0) < 0;
			const bool b11 = (C2 + DeltaX23 * FBlockY1 - DeltaY23 * FBlockX1) < 0;
			const UINT b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);	// compose mask for all four corners

			const bool c00 = (C3 + DeltaX31 * FBlockY0 - DeltaY31 * FBlockX0) < 0;
			const bool c10 = (C3 + DeltaX31 * FBlockY0 - DeltaY31 * FBlockX1) < 0;
			const bool c01 = (C3 + DeltaX31 * FBlockY1 - DeltaY31 * FBlockX0) < 0;
			const bool c11 = (C3 + DeltaX31 * FBlockY1 - DeltaY31 * FBlockX1) < 0;
			const UINT c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);	// compose mask for all four corners
		#else
			const UINT a00 = INT32_SIGN_BIT_SET( C1 + DeltaX12 * FBlockY0 - DeltaY12 * FBlockX0 );
			const UINT a10 = INT32_SIGN_BIT_SET( C1 + DeltaX12 * FBlockY0 - DeltaY12 * FBlockX1 );
			const UINT a01 = INT32_SIGN_BIT_SET( C1 + DeltaX12 * FBlockY1 - DeltaY12 * FBlockX0 );
			const UINT a11 = INT32_SIGN_BIT_SET( C1 + DeltaX12 * FBlockY1 - DeltaY12 * FBlockX1 );
			const UINT a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);	// compose 4-bit mask for all four corners

			const UINT b00 = INT32_SIGN_BIT_SET( C2 + DeltaX23 * FBlockY0 - DeltaY23 * FBlockX0 );
			const UINT b10 = INT32_SIGN_BIT_SET( C2 + DeltaX23 * FBlockY0 - DeltaY23 * FBlockX1 );
			const UINT b01 = INT32_SIGN_BIT_SET( C2 + DeltaX23 * FBlockY1 - DeltaY23 * FBlockX0 );
			const UINT b11 = INT32_SIGN_BIT_SET( C2 + DeltaX23 * FBlockY1 - DeltaY23 * FBlockX1 );
			const UINT b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);	// compose 4-bit mask for all four corners

			const UINT c00 = INT32_SIGN_BIT_SET( C3 + DeltaX31 * FBlockY0 - DeltaY31 * FBlockX0 );
			const UINT c10 = INT32_SIGN_BIT_SET( C3 + DeltaX31 * FBlockY0 - DeltaY31 * FBlockX1 );
			const UINT c01 = INT32_SIGN_BIT_SET( C3 + DeltaX31 * FBlockY1 - DeltaY31 * FBlockX0 );
			const UINT c11 = INT32_SIGN_BIT_SET( C3 + DeltaX31 * FBlockY1 - DeltaY31 * FBlockX1 );
			const UINT c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);	// compose 4-bit mask for all four corners
		#endif


			// Skip block when outside an edge => outside the triangle

			//OPTIMIZED: replaced logical 'or' (3 jmps) with bitwise ops and one comparison
		#if 0
			if( a == 0 || b == 0 || c == 0 ) {
				continue;
			}
		#else
			const UINT mask = ((~0U << 24) | (a << 16) | (b << 8) | (c));

			#define has_nullbyte_(x) ((x - 0x01010101) & ~x & 0x80808080)
			if( has_nullbyte_( mask ) )
			{
				continue;
			}
		#endif


			SoftPixel* pixels = startPixels;
			ZBufElem* zbuffer = startZBuffer;





			// Accept whole block when totally covered (all four corners are inside triangle and their coverage masks are (1|2|4|8) = 15)

#if 0
			//OPTIMIZED: replaced 'and' with sum
		#if 0
			if( a == 0xF && b == 0xF && c == 0xF )
		#elif 0
			if( a + b + c == (0xF + 0xF + 0xF) )
		#else
			if( mask == 0xFF0F0F0F )
		#endif
			{
#if SOFT_RENDER_USE_SSE
				for( UINT iY = iBlockY; iY < iBlockY + BLOCK_SIZE; iY++ )
				{
					for( UINT iX = iBlockX; iX < iBlockX + BLOCK_SIZE; iX++ )
					{
						//pixels[ iX ] = RGBA8_WHITE;

						// Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
						//__m128i a;
						//_mm_cvtepi32_ps();

						const F32 fCurrX = (F32)iX - fX1;
						const F32 fCurrY = (F32)iY - fY1;
						const float Z = fZ1 + vZ.x * fCurrX + vZ.y * fCurrY;

						const ZBufElem thisDepth = FloatDepthToZBufferValue( Z );
						const ZBufElem prevDepth = zbuffer[ iX ];
						if( thisDepth < prevDepth )
						{
							// depth test passed
							zbuffer[ iX ] = thisDepth;

							pixels[ iX ] = SINGLE_FLOAT_TO_RGBA32(Z);
						}//if depth test passed

					}//for x

					pixels += W;
					zbuffer += W;
				}//for y
#else
				for( UINT iY = iBlockY; iY < iBlockY + BLOCK_SIZE; iY++ )
				{
					for( UINT iX = iBlockX; iX < iBlockX + BLOCK_SIZE; iX++ )
					{
						//pixels[ iX ] = RGBA8_WHITE;

						const F32 fCurrX = (F32)iX - fX1;
						const F32 fCurrY = (F32)iY - fY1;
						const float Z = fZ1 + vZ.x * fCurrX + vZ.y * fCurrY;

						const ZBufElem thisDepth = FloatDepthToZBufferValue( Z );
						const ZBufElem prevDepth = zbuffer[ iX ];
						if( thisDepth < prevDepth )
						{
							// depth test passed
							zbuffer[ iX ] = thisDepth;

							pixels[ iX ] = SINGLE_FLOAT_TO_RGBA32(Z);
						}//if depth test passed

					}//for x

					pixels += W;
					zbuffer += W;
				}//for y
#endif


#if SOFT_RENDER_DEBUG
				if( SoftRenderer::bDrawBlockBounds ) {
					DbgDrawRect( context, RGBA8_GREEN, FBlockX0 >> 4, FBlockY0 >> 4, BLOCK_SIZE, BLOCK_SIZE );
				}
#endif // SOFT_RENDER_DEBUG

				// (end of fully covered block)
			}
			else // Partially covered block
#endif
			{
				//Assert(!( a == 0xF && b == 0xF && c == 0xF ));

				// in 28.4
				INT32 CY1 = C1 + DeltaX12 * FBlockY0 - DeltaY12 * FBlockX0;
				INT32 CY2 = C2 + DeltaX23 * FBlockY0 - DeltaY23 * FBlockX0;
				INT32 CY3 = C3 + DeltaX31 * FBlockY0 - DeltaY31 * FBlockX0;

				for( UINT iY = iBlockY; iY < iBlockY + BLOCK_SIZE; iY++ )
				{
					INT32 CX1 = CY1;
					INT32 CX2 = CY2;
					INT32 CX3 = CY3;

					for( UINT iX = iBlockX; iX < iBlockX + BLOCK_SIZE; iX++ )
					{
						// no tiny dots and holes between polygons if we use less or equal
						if( CX1 <= 0 && CX2 <= 0 && CX3 <= 0 )
						//if( CX1 < 0 && CX2 < 0 && CX3 < 0 )
						{
							// inside triangle

							const F32 fStartDX = (F32)iX - fX1;
							const F32 fStartDY = (F32)iY - fY1;
							const float Z = fZ1 + vZ.x * fStartDX + vZ.y * fStartDY;

							const ZBufElem thisDepth = FloatDepthToZBufferValue( Z );
							const ZBufElem prevDepth = zbuffer[ iX ];
							if( thisDepth < prevDepth )
							{
								// depth test passed
								zbuffer[ iX ] = thisDepth;




								// compute inverse W
								const float invW = fInvW1 + vInvW.x * fStartDX + vInvW.y * fStartDY;

								for( UINT i = 0; i < NUM_VARYINGS; i++ )
								{
									//const float W = 1.0f / invW;

									const float varOverW = vars1[i] + varsOverW[i].x * fStartDX + varsOverW[i].y * fStartDY;
									pixelShaderInput.vars[i] = varOverW / invW; // <= perspective correction via division
								}
								pixelShaderInput.depth = Z;



								//(*context.pixelShader)( pixelShaderInput, pixelShaderOutput );

								//pixels[ iX ] = pixelShaderOutput.color.ToARGB().AsUInt32();



								//pixels[ iX ] = SINGLE_FLOAT_TO_RGBA32(Z);


								pixels[ iX ] = context.globals->texture->Sample_Point_Wrap( pixelShaderInput.vars[0],pixelShaderInput.vars[1] );
							}

						}//if depth test passed

						CX1 -= FDY12;
						CX2 -= FDY23;
						CX3 -= FDY31;

					}//for x

					CY1 += FDX12;
					CY2 += FDX23;
					CY3 += FDX31;

					pixels += W;
					zbuffer += W;
				}//for y

#if SOFT_RENDER_DEBUG
				if( SoftRenderer::bDrawBlockBounds ) {
					DbgDrawRect( context, RGBA8_RED, FBlockX0 >> 4, FBlockY0 >> 4, BLOCK_SIZE, BLOCK_SIZE );
				}
#endif // SOFT_RENDER_DEBUG

			}//Partially covered block

		}//for each block on X axis

		startPixels += W * BLOCK_SIZE;
		startZBuffer += W * BLOCK_SIZE;

	}//for each block on Y axis

#else // use old-school scan line rasterizer


	StaticAssert(false);

#endif

	//if(1)	// DBG: draw wire frame triangle
	//{
	//	context.renderer->DrawWireframeTriangle( v1.P.ToVec2(), v2.P.ToVec2(), v3.P.ToVec2(), RGBA8_RED );
	//}

	context.renderer->m_stats.numTrianglesRendered++;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
