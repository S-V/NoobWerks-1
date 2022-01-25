This folder contains the source code for the shaders.

.Shared/ - contains shared header files included by shader code (HLSL) and host application code (C++).



G-Buffer format:

// RT0 =       Normal.x     | Normal.y         | Normal.z        | -
// RT1 =       Diffuse.r    | Diffuse.g        | Diffuse.b       | Ambient Occlusion
// RT2 =       Specular.x   | Specular.y       | Specular.b      | Specular Power
// RT3 =                                                         | 

//
// We are using a "right-handed" coordinate system where the positive X-Axis points 
// to the right, the positive Y-Axis points away from the viewer and the positive
// Z-Axis points up. The following illustration shows our coordinate system.
//
// <PRE>
//  Z-axis                                  
//    ^                               
//    |                               
//    |   Y-axis                   
//    |  /                         
//    | /                           
//    |/                             
//    +----------------> X-axis     
// </PRE>
//
// This same system is also used in Blender/3D-Studio-MAX and CryENGINE.

NOTE: Y is depth!

/*
===============================================================================
	PROJECTION MATRIX
===============================================================================
// Constructs a symmetric view frustum using Direct3D's clip-space conventions (NDC depth in range [0..+1]).
// FoV_Y_radians - (full) vertical field of view, in radians (e.g. PI/2 radians or 90 degrees)

	float tan_half_FoV = tan( FoV_Y_radians * 0.5f );
	float V = 1.0f / tan_half_FoV;	// vertical scaling
	float H = V / aspect_ratio;		// horizontal scaling
	float A = far_clip / (far_clip - near_clip);
	float B = -near_clip * projA;

	(Memory layout: row-major, DirectX-style):
	
	| H   0   0   0 |
	| 0   0   A   1 |
	| 0   V   0   0 |
	| 0   0   B   0 |
*/

/*
Multiplication by our projection matrix (z corresponds to height, y - to depth):

            | H   0   0   0 |
[x y z w] * | 0   0   A   1 | = | (x*H)  (z*V)  (y*A + w*B) (y) |, where w = 1.
            | 0   V   0   0 |
            | 0   0   B   0 |
			
Device Z-buffer stores (y*A+w*B)/y.
*/

/*
Multiplication by inverse projection matrix (P^-1):

            | 1/H   0   0    0  |
[x y z w] * |  0    0  1/V   0  | = | (x/H)   (w)   (y/V)  (z/B - w*A/B) |.
            |  0    0   0   1/B |
            |  0    1   0  -A/B |
*/

Restoring view-space position from hardware (normalized, post-projection) depth:

	// texel coordinates of this pixel in the [0, textureWidth - 1] x [0, textureHeight - 1] range
	const uint2 texCoords = dispatchThreadId.xy;
	float hardware_z_depth = DepthTexture.Load( int3(texCoords.x, texCoords.y, 0) );
	float4 clipSpacePosition = float4( clipSpaceXY, hardware_z_depth, 1 );

	float H = projection_matrix[0][0];
	float V = projection_matrix[2][1];
	float A = projection_matrix[1][2];
	float B = projection_matrix[3][2];

	float3 view_space_position;
	view_space_position.x = clipSpacePosition.x / H;
	view_space_position.y = 1;
	view_space_position.z = clipSpacePosition.y / V;

	view_space_position /= (clipSpacePosition.z / B) - (A / B);

or instead of division by W we can multiply by the inverse: 1 / (z/B - A/B) => B / (z - A)

	float invW = B / (clipSpacePosition.z - A);
	view_space_position *= invW;

or better yet, transform it to multiply-add and reciprocal:

	invW := B / (z - A)
				=> 1 / (z/B - A/B).
					=> 1 / (P*z + Q),
					where P = 1/B, Q = -A/B.
					Note that P and Q can be taken from the original projection matrix.

Now:

	float3 view_space_position;
	view_space_position.x = clipSpacePosition.x / H;
	view_space_position.y = 1;
	view_space_position.z = clipSpacePosition.y / V;
	float invW = 1 / (clipSpacePosition.z * P + Q);
	view_space_position *= invW;
