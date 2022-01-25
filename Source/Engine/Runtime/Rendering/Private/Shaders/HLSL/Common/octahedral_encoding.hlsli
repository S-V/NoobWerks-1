/**
  \file data-files/shader/octahedral.glsl

  Efficient GPU implementation of the octahedral unit vector encoding from 
  
  \cite Cigolle, Donow, Evangelakos, Mara, McGuire, Meyer, 
  \cite A Survey of Efficient Representations for Independent Unit Vectors, Journal of Computer Graphics Techniques (JCGT), vol. 3, no. 2, 1-30, 2014 
  \cite Available online http://jcgt.org/published/0003/02/01/

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef OCTAHEDRAL_ENCODING_HLSLI
#define OCTAHEDRAL_ENCODING_HLSLI

// We have 4 inner triangles segments that encode normal vectors in the +Z hemisphere and the other 4 in the outer corners encode for the -Z hemisphere.
// The OctWrap function is responsible for mirroring the XY components around these diamond diagonals in cases where the Z is negative.
float2 OctWrap( float2 v )
{
    return ( 1.0 - abs( v.yx ) ) * ( v.xy >= 0.0 ? 1.0 : -1.0 );
}
 
// Assume normalized input. Output is on [-1, 1] for each component.
float2 OctEncodeNormal( float3 n )
{
	// Project the sphere onto the octahedron, and then onto the xy plane
    n /= ( abs( n.x ) + abs( n.y ) + abs( n.z ) );
    n.xy = n.z >= 0.0
		? n.xy
		: OctWrap( n.xy )	// Reflect the folds of the lower hemisphere over the diagonals
		;
    return n.xy;
}

// input is [-1, 1]
float3 OctDecodeNormal( float2 f )
{
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = saturate( -n.z );
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize( n );
}


#if 0

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
vec2 octEncode(in vec3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    }
    return result;
}


/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
vec3 octDecode(vec2 o) {
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    }
    return normalize(v);
}

#endif

#endif // OCTAHEDRAL_ENCODING_HLSLI
