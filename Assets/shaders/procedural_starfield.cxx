// Worth reading:
// https://www.cs.unc.edu/~blloyd/comp238/shader.html

#include <nw_shared_globals.h>
#include <_screen_shader.h>
#include <_noise.h>

//Texture2D		t_sourceTexture;
//SamplerState	s_sourceTexture;

cbuffer cbStarfield
{
	// Higher values (i.e., closer to one) yield a sparser starfield.
	float	g_Threshold = 0.8f;
};

// Based on:
// https://www.shadertoy.com/view/Md2SR3
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

/*
// The input, n, should have a magnitude in the approximate range [0, 100].
// The output is pseudo-random, in the range [0,1].
float HashX( float n )
{
	// NOTE: fract() is omitted, because we take fract() in Noise2d():
	return (1.0 + cos(n)) * 415.92653;
}
float Noise2d( in vec2 x )
{
	float xhash = HashX( x.x * 37.0 );
	float yhash = HashX( x.y * 57.0 );
	return fract( xhash + yhash );
}*/

void DrawStar( in vec2 pos, in float size )
{

}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	// Add a camera offset in "FragCoord-space".
//	vec2 vCameraOffset = g_Mouse.xy;
//	vec2 vSamplePos = ( fragCoord.xy + floor( vCameraOffset ) ) / g_FramebufferDimensions.xy;
	vec2 vSamplePos = fragCoord;

	vec3 vColor  = vec3(0.0, 0.0, 0.0);

	// Sky Background Color
	vColor += vec3( 0.1, 0.2, 0.4 ) * vSamplePos.y;

	// Stars
	// Note: Choose fThreshhold in the range [0.99, 0.9999].
	// Higher values (i.e., closer to one) yield a sparser starfield.
	//float fThreshhold = 0.97;
	float fThreshhold = g_Threshold;

	//float StarVal = Noise2d( vSamplePos );
	float StarVal = randomf( vSamplePos );

	vec2 rand2 = random2f(vSamplePos);
	StarVal = rand2.x;

	if ( StarVal >= fThreshhold )
	{
		StarVal = pow( abs((StarVal - fThreshhold)/(1.0 - fThreshhold)), 9.0 );

		vColor += (vec3) StarVal;

		//if( rand2.y > 0.99 ) {
		//	vColor.r = 1;
		//}
	}

	fragColor = vec4(vColor, 1.0);
}


float4 PixelShaderMain( in VS_ScreenOutput inputs ) : SV_Target
{
	float4 fragColor = (float4) 0;

	vec2 uv = inputs.position.xy / g_FramebufferDimensions.xx;


	mainImage(fragColor, uv);



#if 0
	vec2 p = 0.5 - 0.5*sin( g_Time.x*vec2(1.01,1.71) );

//	if( g_Mouse.z != 0 ) {
//		p = vec2(0.0,1.0) + vec2(1.0,-1.0)*(float2)g_Mouse.xy/g_FramebufferDimensions.xy;
//	}
	p = p*p*(3.0-2.0*p);
	p = p*p*(3.0-2.0*p);
	p = p*p*(3.0-2.0*p);

	float f = iqnoise( 24.0*uv, p.x, p.y );
	//float f = iqnoise( 24.0*uv, 1, 0.8 );

	fragColor = vec4( f, f, f, 1.0 );



	//const uint maxStars = 13;
	//const float starProbability = f;
	//if( starProbability > 0.5 )
	//{
	//	fragColor = vec4( 1, 1, 1, 1.0 );
	//}
#endif

	return fragColor;
	//return float4(0,0,0,1);
	//return t_sourceTexture.Sample( s_sourceTexture, inputs.texCoord.xy );
}

#if 0

// https://github.com/cacheflowe/haxademic/blob/master/data/shaders/textures/star-field.glsl

// -*- c++ -*-
// \file starfield.pix
// \author Morgan McGuire
//
// \cite Based on Star Nest by Kali 
// https://www.shadertoy.com/view/4dfGDM
// That shader and this one are open source under the MIT license
//
// Assumes an sRGB target (i.e., the output is already encoded to gamma 2.1)

// viewport resolution (in pixels)
uniform float2    resolution;
uniform float2    invResolution;

// In the noise-function space. xy corresponds to screen-space XY
uniform float3    origin;

uniform mat2      rotate;

uniform sampler2D oldImage;

#define iterations 17

#define volsteps 8

#define sparsity 0.5  // .4 to .5 (sparse)
#define stepsize 0.2

#expect zoom 
#define frequencyVariation   1.3 // 0.5 to 2.0

#define brightness 0.0018
#define distfading 0.6800

void main(void) {    
	float2 uv = gl_FragCoord.xy * invResolution - 0.5;
	uv.y *= resolution.y * invResolution.x;

	float3 dir = float3(uv * zoom, 1.0);
	dir.xz *= rotate;

	float s = 0.1, fade = 0.01;
	gl_FragColor.rgb = float3(0.0);

	for (int r = 0; r < volsteps; ++r) {
		float3 p = origin + dir * (s * 0.5);
		p = abs(float3(frequencyVariation) - mod(p, float3(frequencyVariation * 2.0)));

		float prevlen = 0.0, a = 0.0;
		for (int i = 0; i < iterations; ++i) {
			p = abs(p);
			p = p * (1.0 / dot(p, p)) + (-sparsity); // the magic formula            
			float len = length(p);
			a += abs(len - prevlen); // absolute sum of average change
			prevlen = len;
		}

		a *= a * a; // add contrast

		// coloring based on distance        
		gl_FragColor.rgb += (float3(s, s*s, s*s*s) * a * brightness + 1.0) * fade;
		fade *= distfading; // distance fading
		s += stepsize;
	}

	gl_FragColor.rgb = min(gl_FragColor.rgb, float3(1.2));

	// Detect and suppress flickering single pixels (ignoring the huge gradients that we encounter inside bright areas)
	float intensity = min(gl_FragColor.r + gl_FragColor.g + gl_FragColor.b, 0.7);

	int2 sgn = (int2(gl_FragCoord.xy) & 1) * 2 - 1;
	float2 gradient = float2(dFdx(intensity) * sgn.x, dFdy(intensity) * sgn.y);
	float cutoff = max(max(gradient.x, gradient.y) - 0.1, 0.0);
	gl_FragColor.rgb *= max(1.0 - cutoff * 6.0, 0.3);

	// Motion blur; increases temporal coherence of undersampled flickering stars
	// and provides temporal filtering under true motion.  
	float3 oldValue = texelFetch(oldImage, int2(gl_FragCoord.xy), 0).rgb;
	gl_FragColor.rgb = lerp(oldValue - vec3(0.004), gl_FragColor.rgb, 0.5);
	gl_FragColor.a = 1.0;
}
#endif
