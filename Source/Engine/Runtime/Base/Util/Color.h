/*
=============================================================================
	File:	Color.h
	Desc:	Color representation.
=============================================================================
*/

#ifndef __MX_COLOR_H__
#define __MX_COLOR_H__

#include <Base/Object/Reflection/Reflection.h>

// The per-color weighting to be used for luminance calculations in RGB order.
extern const float LUMINANCE_VECTOR[ 3 ];

mxSTOLEN("idLib, Doom3/Prey SDK");
/*
================
ColorFloatToByte
================
*/
mxFORCEINLINE BYTE ColorFloatToByte( float c )
{
	DWORD colorMask = FLOATSIGNBITSET(c) ? 0 : 255;
	return (BYTE) ( ( (DWORD) ( c * 255.0f ) ) & colorMask );
}

#pragma pack (push,1)

struct R8G8B8 { U8 R,G,B; };

union R8G8B8A8
{
	struct { U8 R,G,B,A; };
	U32 asU32;
};

/*
==========================================================================

		Color representation.

==========================================================================
*/

///
///	RGBAi - a 32 bit RGBA color.
///
struct RGBAi {
	union {
		struct {
			U8	R, G, B, A;	//!< The color is stored in R8G8B8A8 format.
		};
		U32 u;
	};
public:
	mxFORCEINLINE RGBAi()
	{}
	mxFORCEINLINE RGBAi( U32 rgba )
	{
		u = rgba;
	}
	mxFORCEINLINE RGBAi( U8 red, U8 green, U8 blue, U8 alpha = ~0 )
		: R( red ), G( green ), B( blue ), A( alpha )
	{}

public:
	///
	///	Some common colors.
	///
	/// Colors taken from http://www.keller.com/rgb.html
	/// NOTE: some of them are incorrect and must be reversed.
	///
	enum
	{
		//MAROON					= 0x800000FF,
		//DARKRED					= 0x8B0000FF,
		RED						= 0xFF0000FF,	// +
		//LIGHTPINK				= 0xFFB6C1FF,
		//CRIMSON					= 0xDC143CFF,
		//PALEVIOLETRED			= 0xDB7093FF,
		//HOTPINK					= 0xFF69B4FF,
		//DEEPPINK				= 0xFF1493FF,
		//MEDIUMVIOLETRED 		= 0xC71585FF,
		//PURPLE					= 0x800080FF,
		//DARKMAGENTA				= 0x8B008BFF,
		//ORCHID					= 0xDA70D6FF,
		//THISTLE					= 0xD8BFD8FF,
		//PLUM					= 0xDDA0DDFF,
		VIOLET					= 0xFFEE82EE,
		MAGENTA 				= 0xFF00FFFF,
		//MEDIUMORCHID			= 0xBA55D3FF,
		//DARKVIOLET 				= 0x9400D3FF,
		//DARKORCHID 				= 0x9932CCFF,
		//BLUEVIOLET 				= 0x8A2BE2FF,
		//INDIGO					= 0x4B0082FF,
		//MEDIUMPURPLE			= 0x9370DBFF,
		//SLATEBLUE				= 0x6A5ACDFF,
		//MEDIUMSLATEBLUE 		= 0x7B68EEFF,
		//DARKBLUE				= 0x00008BFF,
		//MEDIUMBLUE				= 0x0000CDFF,
		BLUE 					= 0xFFFF0000,	// +, was 0x0000FFFF,

		NAVY 					= 0xFF800000,
		MIDNIGHTBLUE			= 0xFF701919,
		DARKSLATEBLUE			= 0xFF8B3D48,
		ROYALBLUE				= 0xFFE16941,
		CORNFLOWERBLUE			= 0xFFED9564,
		LIGHTSTEELBLUE			= 0xFFDEC4B0,
		ALICEBLUE				= 0xFFFFF8F0,
		GHOSTWHITE				= 0xFFFFF8F8,
		LAVENDER				= 0xFFFAE6E6,
		DODGERBLUE				= 0xFFFF901E,
		STEELBLUE				= 0xFFB48246,
		DEEPSKYBLUE				= 0xFFFFBF00,
		SLATEGRAY				= 0xFF908070,
		LIGHTSLATEGRAY			= 0xFF998877,
		LIGHTSKYBLUE			= 0xFFFACE87,
		SKYBLUE					= 0xFFEBCE87,
		LIGHTBLUE				= 0xFFE6D8AD,
		TEAL					= 0xFF808000,
		DARKCYAN				= 0xFF8B8B00,
		DARKTURQUOISE			= 0xFFD1CE00,
		CYAN					= 0xFFFFFF00,
		MEDIUMTURQUOISE 		= 0xFFCCD148,
		CADETBLUE				= 0xFFA09E5F,
		PALETURQUOISE			= 0xFFEEEEAF,
		LIGHTCYAN				= 0xFFFFFFE0,
		AZURE					= 0xFFFFFFF0,
		LIGHTSEAGREEN			= 0xFFAAB220,
		TURQUOISE				= 0xFFD0E040,
		POWDERBLUE				= 0xFFE6E0B0,
		DARKSLATEGRAY			= 0xFF4F4F2F,
		AQUAMARINE				= 0xFFD4FF7F,
		MEDIUMSPRINGGREEN		= 0xFF9AFA00,
		MEDIUMAQUAMARINE		= 0xFFAACD66,
		SPRINGGREEN				= 0xFF7FFF00,
		MEDIUMSEAGREEN			= 0xFF71B33C,
		SEAGREEN				= 0xFF578B2E,
		LIMEGREEN 				= 0xFF32CD32,
		DARKGREEN 				= 0xFF006400,
		GREEN					= 0xFF00FF00,	//+
		LIME					= 0xFF00FF00,
		FORESTGREEN				= 0xFF228B22,
		DARKSEAGREEN			= 0xFF8FBC8F,
		LIGHTGREEN				= 0xFF90EE90,
		PALEGREEN				= 0xFF98FB98,
		MINTCREAM				= 0xFFFAFFF5,
		HONEYDEW				= 0xFFF0FFF0,
		CHARTREUSE				= 0xFF00FF7F,
		LAWNGREEN				= 0xFF00FC7C,
		OLIVEDRAB				= 0xFF238E6B,
		DARKOLIVEGREEN			= 0xFF2F6B55,
		YELLOWGREEN				= 0xFF32CD9A,
		GREENYELLOW				= 0xFF2FFFAD,
		BEIGE					= 0xFFDCF5F5,
		LINEN					= 0xFFE6F0FA,
		LIGHTGOLDENRODYELLOW	= 0xFFD2FAFA,
		OLIVE					= 0xFF008080,
		YELLOW					= 0xFF00FFFF,
		LIGHTYELLOW				= 0xFFE0FFFF,
		IVORY					= 0xFFF0FFFF,
		DARKKHAKI				= 0xFF6BB7BD,
		KHAKI					= 0xFF8CE6F0,
		PALEGOLDENROD			= 0xFFAAE8EE,
		WHEAT					= 0xFFB3DEF5,
		GOLD					= 0xFF00D7FF,
		LEMONCHIFFON			= 0xFFCDFAFF,
		PAPAYAWHIP				= 0xFFD5EFFF,
		DARKGOLDENROD			= 0xFF0B86B8,
		GOLDENROD				= 0xFF20A5DA,
		ANTIQUEWHITE			= 0xFFD7EBFA,
		CORNSILK				= 0xFFDCF8FF,
		OLDLACE					= 0xFFE6F5FD,
		MOCCASIN				= 0xFFB5E4FF,
		NAVAJOWHITE				= 0xFFADDEFF,
		ORANGE 					= 0xFF00A5FF,
		BISQUE 					= 0xFFC4E4FF,
		TAN						= 0xFF8CB4D2,
		DARKORANGE				= 0xFF008CFF,
		BURLYWOOD				= 0xFF87B8DE,
		SADDLEBROWN				= 0xFF13458B,
		SANDYBROWN				= 0xFF60A4F4,
		BLANCHEDALMOND			= 0xFFCDEBFF,
		LAVENDERBLUSH			= 0xFFF5F0FF,
		SEASHELL				= 0xFFEEF5FF,
		FLORALWHITE 			= 0xFFF0FAFF,
		SNOW					= 0xFFFAFAFF,
		PERU					= 0xFF3F85CD,
		PEACHPUFF				= 0xFFB9DAFF,
		CHOCOLATE				= 0xFF1E69D2,
		SIENNA					= 0xFF2D52A0,
		LIGHTSALMON 			= 0xFF7AA0FF,
		CORAL					= 0xFF507FFF,
		DARKSALMON				= 0xFF7A96E9,
		MISTYROSE 				= 0xFFE1E4FF,
		ORANGERED 				= 0xFF0045FF,
		SALMON 					= 0xFF7280FA,
		TOMATO 					= 0xFF4763FF,
		ROSYBROWN				= 0xFF8F8FBC,
		PINK					= 0xFFCBC0FF,
		INDIANRED				= 0xFF5C5CCD,
		LIGHTCORAL				= 0xFF8080F0,
		BROWN					= 0xFF2A2AA5,
		FIREBRICK				= 0xFF2222B2,
		BLACK					= 0xFF000000,
		DIMGRAY					= 0xFF696969,
		GRAY					= 0xFF808080,
		DARKGRAY				= 0xFFA9A9A9,
		SILVER					= 0xFFC0C0C0,
		LIGHTGREY				= 0xFFD3D3D3,
		GAINSBORO				= 0xFFDCDCDC,
		WHITESMOKE				= 0xFFF5F5F5,
		WHITE					= 0xFFFFFFFF,
		GREY					= 0xFF888888,
		GREY25 					= 0xFF404040,
		GREY50 					= 0xFF808080,
		GREY75 					= 0xFFC0C0C0,
	};

	/// This code is taken from GDI+.
	/// Common color constants:
	enum
	{
		AliceBlue            = 0xFFFFF8F0,
		AntiqueWhite         = 0xFFD7EBFA,
		Aqua                 = 0xFFFFFF00,
		Aquamarine           = 0xFFD4FF7F,
		Azure                = 0xFFFFFFF0,
		Beige                = 0xFFDCF5F5,
		Bisque               = 0xFFC4E4FF,
		Black                = 0xFF000000,
		BlanchedAlmond       = 0xFFCDEBFF,
		Blue                 = 0xFFFF0000,
		BlueViolet           = 0xFFE22B8A,
		Brown                = 0xFF2A2AA5,
		BurlyWood            = 0xFF87B8DE,
		CadetBlue            = 0xFFA09E5F,
		Chartreuse           = 0xFF00FF7F,
		Chocolate            = 0xFF1E69D2,
		Coral                = 0xFF507FFF,
		CornflowerBlue       = 0xFFED9564,
		Cornsilk             = 0xFFDCF8FF,
		Crimson              = 0xFF3C14DC,
		Cyan                 = 0xFFFFFF00,
		DarkBlue             = 0xFF8B0000,
		DarkCyan             = 0xFF8B8B00,
		DarkGoldenrod        = 0xFF0B86B8,
		DarkGray             = 0xFFA9A9A9,
		DarkGreen            = 0xFF006400,
		DarkKhaki            = 0xFF6BB7BD,
		DarkMagenta          = 0xFF8B008B,
		DarkOliveGreen       = 0xFF2F6B55,
		DarkOrange           = 0xFF008CFF,
		DarkOrchid           = 0xFFCC3299,
		DarkRed              = 0xFF00008B,
		DarkSalmon           = 0xFF7A96E9,
		DarkSeaGreen         = 0xFF8BBC8F,
		DarkSlateBlue        = 0xFF8B3D48,
		DarkSlateGray        = 0xFF4F4F2F,
		DarkTurquoise        = 0xFFD1CE00,
		DarkViolet           = 0xFFD30094,
		DeepPink             = 0xFF9314FF,
		DeepSkyBlue          = 0xFFFFBF00,
		DimGray              = 0xFF696969,
		DodgerBlue           = 0xFFFF901E,
		Firebrick            = 0xFF2222B2,
		FloralWhite          = 0xFFF0FAFF,
		ForestGreen          = 0xFF228B22,
		Fuchsia              = 0xFFFF00FF,
		Gainsboro            = 0xFFDCDCDC,
		GhostWhite           = 0xFFFFF8F8,
		Gold                 = 0xFF00D7FF,
		Goldenrod            = 0xFF20A5DA,
		Gray                 = 0xFF808080,
		Green                = 0xFF008000,
		GreenYellow          = 0xFF2FFFAD,
		Honeydew             = 0xFFF0FFF0,
		HotPink              = 0xFFB469FF,
		IndianRed            = 0xFF5C5CCD,
		Indigo               = 0xFF82004B,
		Ivory                = 0xFFF0FFFF,
		Khaki                = 0xFF8CE6F0,
		Lavender             = 0xFFFAE6E6,
		LavenderBlush        = 0xFFF5F0FF,
		LawnGreen            = 0xFF00FC7C,
		LemonChiffon         = 0xFFCDFAFF,
		LightBlue            = 0xFFE6D8AD,
		LightCoral           = 0xFF8080F0,
		LightCyan            = 0xFFFFFFE0,
		LightGoldenrodYellow = 0xFFD2FAFA,
		LightGray            = 0xFFD3D3D3,
		LightGreen           = 0xFF90EE90,
		LightPink            = 0xFFC1B6FF,
		LightSalmon          = 0xFF7AA0FF,
		LightSeaGreen        = 0xFFAAB220,
		LightSkyBlue         = 0xFFFACE87,
		LightSlateGray       = 0xFF998877,
		LightSteelBlue       = 0xFFDEC4B0,
		LightYellow          = 0xFFE0FFFF,
		Lime                 = 0xFF00FF00,
		LimeGreen            = 0xFF32CD32,
		Linen                = 0xFFE6F0FA,
		Magenta              = 0xFFFF00FF,
		Maroon               = 0xFF000080,
		MediumAquamarine     = 0xFFAACD66,
		MediumBlue           = 0xFFCD0000,
		MediumOrchid         = 0xFFD355BA,
		MediumPurple         = 0xFFDB7093,
		MediumSeaGreen       = 0xFF71B33C,
		MediumSlateBlue      = 0xFFEE687B,
		MediumSpringGreen    = 0xFF9AFA00,
		MediumTurquoise      = 0xFFCCD148,
		MediumVioletRed      = 0xFF8515C7,
		MidnightBlue         = 0xFF701919,
		MintCream            = 0xFFFAFFF5,
		MistyRose            = 0xFFE1E4FF,
		Moccasin             = 0xFFB5E4FF,
		NavajoWhite          = 0xFFADDEFF,
		Navy                 = 0xFF800000,
		OldLace              = 0xFFE6F5FD,
		Olive                = 0xFF008080,
		OliveDrab            = 0xFF238E6B,
		Orange               = 0xFF00A5FF,
		OrangeRed            = 0xFF0045FF,
		Orchid               = 0xFFD670DA,
		PaleGoldenrod        = 0xFFAAE8EE,
		PaleGreen            = 0xFF98FB98,
		PaleTurquoise        = 0xFFEEEEAF,
		PaleVioletRed        = 0xFF9370DB,
		PapayaWhip           = 0xFFD5EFFF,
		PeachPuff            = 0xFFB9DAFF,
		Peru                 = 0xFF3F85CD,
		Pink                 = 0xFFCBC0FF,
		Plum                 = 0xFFDDA0DD,
		PowderBlue           = 0xFFE6E0B0,
		Purple               = 0xFF800080,
		Red                  = 0xFF0000FF,
		RosyBrown            = 0xFF8F8FBC,
		RoyalBlue            = 0xFFE16941,
		SaddleBrown          = 0xFF13458B,
		Salmon               = 0xFF7280FA,
		SandyBrown           = 0xFF60A4F4,
		SeaGreen             = 0xFF578B2E,
		SeaShell             = 0xFFEEF5FF,
		Sienna               = 0xFF2D52A0,
		Silver               = 0xFFC0C0C0,
		SkyBlue              = 0xFFEBCE87,
		SlateBlue            = 0xFFCD5A6A,
		SlateGray            = 0xFF908070,
		Snow                 = 0xFFFAFAFF,
		SpringGreen          = 0xFF7FFF00,
		SteelBlue            = 0xFFB48246,
		Tan                  = 0xFF8CB4D2,
		Teal                 = 0xFF808000,
		Thistle              = 0xFFD8BFD8,
		Tomato               = 0xFF4763FF,
		Transparent          = 0x00FFFFFF,
		Turquoise            = 0xFFD0E040,
		Violet               = 0xFFEE82EE,
		Wheat                = 0xFFB3DEF5,
		White                = 0xFFFFFFFF,
		WhiteSmoke           = 0xFFF5F5F5,
		Yellow               = 0xFF00FFFF,
		YellowGreen          = 0xFF32CD9A
	};

private:
	// This function is never called, it just contains compile-time assertions.
	void StaticChecks()
	{
		mxSTATIC_ASSERT( sizeof( RGBAi ) == sizeof( INT32 ) );
	}
};

//
//	RGBf - represents a color in RGB space; consists of 3 floating-point values.
//
struct RGBf {
	union {
		struct {
			float	R, G, B;	// The red, the green and the blue components.
		};
		struct {
			float	x, y, z;
		};
	};

public:
	mxFORCEINLINE RGBf( float red, float green, float blue )
		: R( red ), G( green ), B( blue )
	{}

	mxFORCEINLINE RGBf( const RGBf& other )
		: R( other.R ), G( other.G ), B( other.B )
	{}

	// Sets all three components of a color.

	mxFORCEINLINE void Set( float red, float green, float blue )
	{
		R = clampf( red, 0.0f, 1.0f );
		G = clampf( green, 0.0f, 1.0f );
		B = clampf( blue, 0.0f, 1.0f );
	}

	mxFORCEINLINE float * Raw()
	{
		return &R;
	}
	mxFORCEINLINE const float * Raw() const
	{
		return &R;
	}

	/// Clamps all components to the specified range.
	mxFORCEINLINE void ClampTo( float min, float max )
	{
		R = clampf( R, min, max );
		G = clampf( G, min, max );
		B = clampf( B, min, max );
	}

	// Clamps all components to [0..1].
	mxFORCEINLINE void Saturate()
	{
		ClampTo( 0.0f, 1.0f );
	}

	// Returns 'true' if all components are within the specified range.
	mxFORCEINLINE
	bool InsideRange( float min = 0.0f, float max = 1.0f ) const
	{
		return IsInRangeInc( R, min, max )
			&& IsInRangeInc( G, min, max )
			&& IsInRangeInc( B, min, max );
	}

	// extracts luminance
	mxFORCEINLINE float Brightness() const
	{
		return R * LUMINANCE_VECTOR[0] + G * LUMINANCE_VECTOR[1] + B * LUMINANCE_VECTOR[2];
	}

	// Returns a reference to the i-th component of a color. The value of i must be 0, 1, or 2. 
	float &			operator [] ( UINT i );

	//	Returns a constant reference to the i-th component of a color. The value of i must be 0, 1, or 2. 
	const float &	operator [] ( UINT i ) const;

	//	Sets all three components to the value f. 
	RGBf &		operator = ( float f );

	//	Adds the color c. 
	RGBf &		operator += ( const RGBf& c );

	//	Subtracts the color c. 
	RGBf &		operator -= ( const RGBf& c );

	//	Multiplies by the color c. 
	RGBf &		operator *= ( const RGBf& c );

	//	Multiplies all three components by t. 
	RGBf &		operator *= ( float t );

	//	Divides all three components by t.
	RGBf &		operator /= ( float t );

	RGBf & operator = ( const RGBf& other );

	// Returns the negation of the color c.
	friend RGBf operator - ( const RGBf& c );	

	// Returns the sum of the colors color1 and color2.
	friend RGBf operator + ( const RGBf& color1, const RGBf& color2 );

	//	Returns the difference of the colors color1 and color2. 
	friend RGBf operator - ( const RGBf& color1, const RGBf& color2 );

	//	Returns the product of the colors color1 and color2. 
	friend RGBf operator * ( const RGBf& color1, const RGBf& color2 );

	//	Returns the product of the color c and the scalar t. 
	friend RGBf operator * ( const RGBf& c, float t );

	//	Returns the product of the color c and the scalar t. 
	friend RGBf operator * ( float t, const RGBf& c );

	//	Returns the product of the color c and the inverse of the scalar t. 
	friend RGBf operator / ( const RGBf& c, float t );

	//	Returns a boolean value indicating the equality of the colors color1 and color2. 
	friend bool operator == ( const RGBf& color1, const RGBf& color2 );

	//	Returns a boolean value indicating the inequality of the colors color1 and color2.
	friend bool operator != ( const RGBf& color1, const RGBf& c2 );
};

mxDECLARE_POD_TYPE(RGBAi);


///
///	RGBAf - a 128-bit color in RGBA space; consists of 4 floating-point values.
///
struct RGBAf {
	union {
		struct {
			float	R, G, B, A;
		};
		struct {
			float	x, y, z, w;
		};
		V4f			v;
		Vector4		q;
	};
public:
	// warning C4316 : 'RGBAf' : object allocated on the heap may not be aligned 16
//	mmDECLARE_ALIGNED_ALLOCATOR16(RGBAf);

	static mxFORCEINLINE RGBAf make( float red, float green, float blue, float alpha = 1.0f ) {
		const RGBAf result = { red, green, blue, alpha };
		return result;
	}

	static mxFORCEINLINE RGBAf fromVector3D( const V3f& v, float alpha = 1.0f )
	{
		return RGBAf::make( v.x, v.y, v.z, alpha );
	}

	static mxFORCEINLINE RGBAf fromVec( Vec4Arg0 q ) {
		RGBAf result;
		result.q = q;
		return result;
	}

	static mxFORCEINLINE RGBAf FromRGBAi( const RGBAi& rgba )
	{
		return fromRgba32(rgba.u);
	}

	static mxFORCEINLINE RGBAf fromRgba32( const U32 _rgba ) {
		const float f = 1.0f / 255.0f;
		const RGBAf result = {
			f * (float) U8( _rgba >> 0 ),
			f * (float) U8( _rgba >> 8 ),
			f * (float) U8( _rgba >> 16 ),
			f * (float) U8( _rgba >> 24 ),
		};
		return result;
	}
	static mxFORCEINLINE RGBAf fromFloatPtr( const float* _rgba ) {
		const RGBAf result = { _rgba[0], _rgba[1], _rgba[2], _rgba[3] };
		return result;
	}

	mxFORCEINLINE const RGBAf withAlpha( float alpha )
	{
		return RGBAf::make( R, G, B, alpha );
	}

public:	// Conversions.

	mxFORCEINLINE RGBAi ToRGBAi() const {
		return RGBAi( R * 255.0f, G * 255.0f, B * 255.0f, A * 255.0f );
	}
	mxFORCEINLINE RGBAi ToRGBAi_Safe() const {
		BYTE r = mmFloatToByte( R * 255.0f );
		BYTE g = mmFloatToByte( G * 255.0f );
		BYTE b = mmFloatToByte( B * 255.0f );
		BYTE a = mmFloatToByte( A * 255.0f );
		return RGBAi( r, g, b, a );
	}

	mxFORCEINLINE RGBf &		AsRGB()			{ return *(RGBf*)this; }
	mxFORCEINLINE const RGBf &	AsRGB() const	{ return *(RGBf*)this; }

	mxFORCEINLINE float* raw() {
		return (float*) this;
	}
	mxFORCEINLINE const float* raw() const {
		return (const float*) this;
	}

public:	// Overloaded operators.

	mxFORCEINLINE RGBAf operator + ( float f ) const
	{
		return RGBAf::make( R + f, G + f, B + f, A + f );
	}
	mxFORCEINLINE RGBAf	operator += ( float f )
	{
		this->R += f;
		this->G += f;
		this->B += f;
		this->A += f;
		return *this;
	}
	mxFORCEINLINE RGBAf operator * ( float f ) const
	{
		return RGBAf::make( R * f, G * f, B * f, A * f );
	}
	mxFORCEINLINE RGBAf& operator *= ( float f )
	{
		this->R *= f;
		this->G *= f;
		this->B *= f;
		this->A *= f;
		return *this;
	}
	mxFORCEINLINE RGBAf operator * ( const RGBAf& other ) const
	{
		return RGBAf::make( R * other.R, G * other.G, B * other.B, A * other.A );
	}
	mxFORCEINLINE RGBAf operator / ( float f ) const
	{
		mxASSERT(f!=0.0f);
		float inv = 1.0f / f;
		//Vector4_Scale()
		return RGBAf::make( R * inv, G * inv, B * inv, A * inv );
	}
	RGBAf & operator /= ( float f )
	{
		mxASSERT(f!=0.0f);
		float inv = 1.0f / f;
		this->R *= inv;
		this->G *= inv;
		this->B *= inv;
		this->A *= inv;
		return *this;
	}

public:
	static const RGBAf		BLACK;
	static const RGBAf		DARK_GREY;
	static const RGBAf		GRAY;	//MID_GREY
	static const RGBAf		LIGHT_GREY;
	static const RGBAf		WHITE;

	static const RGBAf		RED;
	static const RGBAf		GREEN;
	static const RGBAf		BLUE;
	static const RGBAf		YELLOW;
	static const RGBAf		MAGENTA;
	static const RGBAf		CYAN;
	static const RGBAf		ORANGE;
	static const RGBAf		PURPLE;
	static const RGBAf		PINK;
	static const RGBAf		BROWN;

	static const RGBAf		BEACH_SAND;
	static const RGBAf		LIGHT_YELLOW_GREEN;
	static const RGBAf		DARK_YELLOW_GREEN;
	static const RGBAf		DARKBROWN;

	//
	// colors taken from http://prideout.net/archive/colors.php
	//
	static const RGBAf		aliceblue			;
	static const RGBAf		antiquewhite		;
	static const RGBAf		aqua				;
	static const RGBAf		aquamarine			;
	static const RGBAf		azure				;
	static const RGBAf		beige				;
	static const RGBAf		bisque				;
	static const RGBAf		black				;
	static const RGBAf		blanchedalmond		;
	static const RGBAf		blue				;
	static const RGBAf		blueviolet			;
	static const RGBAf		brown				;
	static const RGBAf		burlywood			;
	static const RGBAf		cadetblue			;
	static const RGBAf		chartreuse			;
	static const RGBAf		chocolate			;
	static const RGBAf		coral				;
	static const RGBAf		cornflowerblue		;
	static const RGBAf		cornsilk			;
	static const RGBAf		crimson				;
	static const RGBAf		cyan				;
	static const RGBAf		darkblue			;
	static const RGBAf		darkcyan			;
	static const RGBAf		darkgoldenrod		;
	static const RGBAf		darkgray			;
	static const RGBAf		darkgreen			;
	static const RGBAf		darkgrey			;
	static const RGBAf		darkkhaki			;
	static const RGBAf		darkmagenta			;
	static const RGBAf		darkolivegreen		;
	static const RGBAf		darkorange			;
	static const RGBAf		darkorchid			;
	static const RGBAf		darkred				;
	static const RGBAf		darksalmon			;
	static const RGBAf		darkseagreen		;
	static const RGBAf		darkslateblue		;
	static const RGBAf		darkslategray		;
	static const RGBAf		darkslategrey		;
	static const RGBAf		darkturquoise		;
	static const RGBAf		darkviolet			;
	static const RGBAf		deeppink			;
	static const RGBAf		deepskyblue			;
	static const RGBAf		dimgray				;
	static const RGBAf		dimgrey				;
	static const RGBAf		dodgerblue			;
	static const RGBAf		firebrick			;
	static const RGBAf		floralwhite			;
	static const RGBAf		forestgreen			;
	static const RGBAf		fuchsia				;
	static const RGBAf		gainsboro			;
	static const RGBAf		ghostwhite			;
	static const RGBAf		gold				;
	static const RGBAf		goldenrod			;
	static const RGBAf		gray				;
	static const RGBAf		green				;
	static const RGBAf		greenyellow			;
	static const RGBAf		grey				;
	static const RGBAf		honeydew			;
	static const RGBAf		hotpink				;
	static const RGBAf		indianred			;
	static const RGBAf		indigo				;
	static const RGBAf		ivory				;
	static const RGBAf		khaki				;
	static const RGBAf		lavender			;
	static const RGBAf		lavenderblush		;
	static const RGBAf		lawngreen			;
	static const RGBAf		lemonchiffon		;
	static const RGBAf		lightblue			;
	static const RGBAf		lightcoral			;
	static const RGBAf		lightcyan			;
	static const RGBAf		lightgoldenrodyellow;
	static const RGBAf		lightgray			;
	static const RGBAf		lightgreen			;
	static const RGBAf		lightgrey			;
	static const RGBAf		lightpink			;
	static const RGBAf		lightsalmon			;
	static const RGBAf		lightseagreen		;
	static const RGBAf		lightskyblue		;
	static const RGBAf		lightslategray		;
	static const RGBAf		lightslategrey		;
	static const RGBAf		lightsteelblue		;
	static const RGBAf		lightyellow			;
	static const RGBAf		lime				;
	static const RGBAf		limegreen			;
	static const RGBAf		linen				;
	static const RGBAf		magenta				;
	static const RGBAf		maroon				;
	static const RGBAf		mediumaquamarine	;
	static const RGBAf		mediumblue			;
	static const RGBAf		mediumorchid		;
	static const RGBAf		mediumpurple		;
	static const RGBAf		mediumseagreen		;
	static const RGBAf		mediumslateblue		;
	static const RGBAf		mediumspringgreen	;
	static const RGBAf		mediumturquoise		;
	static const RGBAf		mediumvioletred		;
	static const RGBAf		midnightblue		;
	static const RGBAf		mintcream			;
	static const RGBAf		mistyrose			;
	static const RGBAf		moccasin			;
	static const RGBAf		navajowhite			;
	static const RGBAf		navy				;
	static const RGBAf		oldlace				;
	static const RGBAf		olive				;
	static const RGBAf		olivedrab			;
	static const RGBAf		orange				;
	static const RGBAf		orangered			;
	static const RGBAf		orchid				;
	static const RGBAf		palegoldenrod		;
	static const RGBAf		palegreen			;
	static const RGBAf		paleturquoise		;
	static const RGBAf		palevioletred		;
	static const RGBAf		papayawhip			;
	static const RGBAf		peachpuff			;
	static const RGBAf		peru				;
	static const RGBAf		pink				;
	static const RGBAf		plum				;
	static const RGBAf		powderblue			;
	static const RGBAf		purple				;
	static const RGBAf		red					;
	static const RGBAf		rosybrown			;
	static const RGBAf		royalblue			;
	static const RGBAf		saddlebrown			;
	static const RGBAf		salmon				;
	static const RGBAf		sandybrown			;
	static const RGBAf		seagreen			;
	static const RGBAf		seashell			;
	static const RGBAf		sienna				;
	static const RGBAf		silver				;
	static const RGBAf		skyblue				;
	static const RGBAf		slateblue			;
	static const RGBAf		slategray			;
	static const RGBAf		slategrey			;
	static const RGBAf		snow				;
	static const RGBAf		springgreen			;
	static const RGBAf		steelblue			;
	static const RGBAf		tan					;
	static const RGBAf		teal				;
	static const RGBAf		thistle				;
	static const RGBAf		tomato				;
	static const RGBAf		turquoise			;
	static const RGBAf		violet				;
	static const RGBAf		wheat				;
	static const RGBAf		white				;
	static const RGBAf		whitesmoke			;
	static const RGBAf		yellow				;
	static const RGBAf		yellowgreen			;
};

#pragma pack (pop)

mxDECLARE_POD_TYPE(RGBAf);
mxDECLARE_STRUCT(RGBAf);

#endif // !__MX_COLOR_H__
