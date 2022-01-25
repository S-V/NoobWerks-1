/*
=============================================================================
	File:	Color.cpp
	Desc:	Color representation.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include "Color.h"

// The per-color weighting to be used for luminance calculations in RGB order.
const float LUMINANCE_VECTOR[ 3 ] =
{
//	0.2126f, 0.7152f, 0.0722f
//	0.27f, 0.67f, 0.06f
	0.3f, 0.59f, 0.11f
};

/*================================
			RGBAi
================================*/

//mxBEGIN_REFLECTION(RGBAi)
//	FIELD(RGBAi,A)
//	FIELD(RGBAi,R)
//	FIELD(RGBAi,G)
//	FIELD(RGBAi,B)
//mxEND_REFLECTION

/*================================
			RGBAf
================================*/

mxBEGIN_STRUCT(RGBAf)
	mxMEMBER_FIELD(R),
	mxMEMBER_FIELD(G),
	mxMEMBER_FIELD(B),
	mxMEMBER_FIELD(A),
mxEND_REFLECTION

const RGBAf	RGBAf::BLACK				={ 0.00f, 0.00f, 0.00f, 1.00f };
const RGBAf	RGBAf::DARK_GREY			={ 0.25f, 0.25f, 0.25f, 1.00f };
const RGBAf	RGBAf::GRAY					={ 0.50f, 0.50f, 0.50f, 1.00f };
const RGBAf	RGBAf::LIGHT_GREY			={ 0.75f, 0.75f, 0.75f, 1.00f };
const RGBAf	RGBAf::WHITE				={ 1.00f, 1.00f, 1.00f, 1.00f };

const RGBAf	RGBAf::RED					={ 1.00f, 0.00f, 0.00f, 1.00f };
const RGBAf	RGBAf::GREEN				={ 0.00f, 1.00f, 0.00f, 1.00f };
const RGBAf	RGBAf::BLUE					={ 0.00f, 0.00f, 1.00f, 1.00f };
const RGBAf	RGBAf::YELLOW				={ 1.00f, 1.00f, 0.00f, 1.00f };
const RGBAf	RGBAf::MAGENTA				={ 1.00f, 0.00f, 1.00f, 1.00f };
const RGBAf	RGBAf::CYAN					={ 0.00f, 1.00f, 1.00f, 1.00f };
const RGBAf	RGBAf::ORANGE				={ 1.00f, 0.50f, 0.00f, 1.00f };	
const RGBAf	RGBAf::PURPLE				={ 0.60f, 0.00f, 0.60f, 1.00f };
const RGBAf	RGBAf::PINK					={ 0.73f, 0.40f, 0.48f, 1.00f };
const RGBAf	RGBAf::BROWN				={ 0.40f, 0.35f, 0.08f, 1.00f };

const RGBAf	RGBAf::BEACH_SAND			={ 1.00f, 0.96f, 0.62f, 1.00f };
const RGBAf	RGBAf::LIGHT_YELLOW_GREEN	={ 0.48f, 0.77f, 0.46f, 1.00f };
const RGBAf	RGBAf::DARK_YELLOW_GREEN	={ 0.10f, 0.48f, 0.19f, 1.00f };
const RGBAf	RGBAf::DARKBROWN			={ 0.45f, 0.39f, 0.34f, 1.00f };

//
// colors taken from http://prideout.net/archive/colors.php
//
const RGBAf	RGBAf::aliceblue			={ 0.941f, 0.973f, 1.000f };
const RGBAf	RGBAf::antiquewhite			={ 0.980f, 0.922f, 0.843f };
const RGBAf	RGBAf::aqua					={ 0.000f, 1.000f, 1.000f };
const RGBAf	RGBAf::aquamarine			={ 0.498f, 1.000f, 0.831f };
const RGBAf	RGBAf::azure				={ 0.941f, 1.000f, 1.000f };
const RGBAf	RGBAf::beige				={ 0.961f, 0.961f, 0.863f };
const RGBAf	RGBAf::bisque				={ 1.000f, 0.894f, 0.769f };
const RGBAf	RGBAf::black				={ 0.000f, 0.000f, 0.000f };
const RGBAf	RGBAf::blanchedalmond		={ 1.000f, 0.922f, 0.804f };
const RGBAf	RGBAf::blue					={ 0.000f, 0.000f, 1.000f };
const RGBAf	RGBAf::blueviolet			={ 0.541f, 0.169f, 0.886f };
const RGBAf	RGBAf::brown				={ 0.647f, 0.165f, 0.165f };
const RGBAf	RGBAf::burlywood			={ 0.871f, 0.722f, 0.529f };
const RGBAf	RGBAf::cadetblue			={ 0.373f, 0.620f, 0.627f };
const RGBAf	RGBAf::chartreuse			={ 0.498f, 1.000f, 0.000f };
const RGBAf	RGBAf::chocolate			={ 0.824f, 0.412f, 0.118f };
const RGBAf	RGBAf::coral				={ 1.000f, 0.498f, 0.314f };
const RGBAf	RGBAf::cornflowerblue		={ 0.392f, 0.584f, 0.929f };
const RGBAf	RGBAf::cornsilk				={ 1.000f, 0.973f, 0.863f };
const RGBAf	RGBAf::crimson				={ 0.863f, 0.078f, 0.235f };
const RGBAf	RGBAf::cyan					={ 0.000f, 1.000f, 1.000f };
const RGBAf	RGBAf::darkblue				={ 0.000f, 0.000f, 0.545f };
const RGBAf	RGBAf::darkcyan				={ 0.000f, 0.545f, 0.545f };
const RGBAf	RGBAf::darkgoldenrod		={ 0.722f, 0.525f, 0.043f };
const RGBAf	RGBAf::darkgray				={ 0.663f, 0.663f, 0.663f };
const RGBAf	RGBAf::darkgreen			={ 0.000f, 0.392f, 0.000f };
const RGBAf	RGBAf::darkgrey				={ 0.663f, 0.663f, 0.663f };
const RGBAf	RGBAf::darkkhaki			={ 0.741f, 0.718f, 0.420f };
const RGBAf	RGBAf::darkmagenta			={ 0.545f, 0.000f, 0.545f };
const RGBAf	RGBAf::darkolivegreen		={ 0.333f, 0.420f, 0.184f };
const RGBAf	RGBAf::darkorange			={ 1.000f, 0.549f, 0.000f };
const RGBAf	RGBAf::darkorchid			={ 0.600f, 0.196f, 0.800f };
const RGBAf	RGBAf::darkred				={ 0.545f, 0.000f, 0.000f };
const RGBAf	RGBAf::darksalmon			={ 0.914f, 0.588f, 0.478f };
const RGBAf	RGBAf::darkseagreen			={ 0.561f, 0.737f, 0.561f };
const RGBAf	RGBAf::darkslateblue		={ 0.282f, 0.239f, 0.545f };
const RGBAf	RGBAf::darkslategray		={ 0.184f, 0.310f, 0.310f };
const RGBAf	RGBAf::darkslategrey		={ 0.184f, 0.310f, 0.310f };
const RGBAf	RGBAf::darkturquoise		={ 0.000f, 0.808f, 0.820f };
const RGBAf	RGBAf::darkviolet			={ 0.580f, 0.000f, 0.827f };
const RGBAf	RGBAf::deeppink				={ 1.000f, 0.078f, 0.576f };
const RGBAf	RGBAf::deepskyblue			={ 0.000f, 0.749f, 1.000f };
const RGBAf	RGBAf::dimgray				={ 0.412f, 0.412f, 0.412f };
const RGBAf	RGBAf::dimgrey				={ 0.412f, 0.412f, 0.412f };
const RGBAf	RGBAf::dodgerblue			={ 0.118f, 0.565f, 1.000f };
const RGBAf	RGBAf::firebrick			={ 0.698f, 0.133f, 0.133f };
const RGBAf	RGBAf::floralwhite			={ 1.000f, 0.980f, 0.941f };
const RGBAf	RGBAf::forestgreen			={ 0.133f, 0.545f, 0.133f };
const RGBAf	RGBAf::fuchsia				={ 1.000f, 0.000f, 1.000f };
const RGBAf	RGBAf::gainsboro			={ 0.863f, 0.863f, 0.863f };
const RGBAf	RGBAf::ghostwhite			={ 0.973f, 0.973f, 1.000f };
const RGBAf	RGBAf::gold					={ 1.000f, 0.843f, 0.000f };
const RGBAf	RGBAf::goldenrod			={ 0.855f, 0.647f, 0.125f };
const RGBAf	RGBAf::gray					={ 0.502f, 0.502f, 0.502f };
const RGBAf	RGBAf::green				={ 0.000f, 0.502f, 0.000f };
const RGBAf	RGBAf::greenyellow			={ 0.678f, 1.000f, 0.184f };
const RGBAf	RGBAf::grey					={ 0.502f, 0.502f, 0.502f };
const RGBAf	RGBAf::honeydew				={ 0.941f, 1.000f, 0.941f };
const RGBAf	RGBAf::hotpink				={ 1.000f, 0.412f, 0.706f };
const RGBAf	RGBAf::indianred			={ 0.804f, 0.361f, 0.361f };
const RGBAf	RGBAf::indigo				={ 0.294f, 0.000f, 0.510f };
const RGBAf	RGBAf::ivory				={ 1.000f, 1.000f, 0.941f };
const RGBAf	RGBAf::khaki				={ 0.941f, 0.902f, 0.549f };
const RGBAf	RGBAf::lavender				={ 0.902f, 0.902f, 0.980f };
const RGBAf	RGBAf::lavenderblush		={ 1.000f, 0.941f, 0.961f };
const RGBAf	RGBAf::lawngreen			={ 0.486f, 0.988f, 0.000f };
const RGBAf	RGBAf::lemonchiffon			={ 1.000f, 0.980f, 0.804f };
const RGBAf	RGBAf::lightblue			={ 0.678f, 0.847f, 0.902f };
const RGBAf	RGBAf::lightcoral			={ 0.941f, 0.502f, 0.502f };
const RGBAf	RGBAf::lightcyan			={ 0.878f, 1.000f, 1.000f };
const RGBAf	RGBAf::lightgoldenrodyellow	={ 0.980f, 0.980f, 0.824f };
const RGBAf	RGBAf::lightgray			={ 0.827f, 0.827f, 0.827f };
const RGBAf	RGBAf::lightgreen			={ 0.565f, 0.933f, 0.565f };
const RGBAf	RGBAf::lightgrey			={ 0.827f, 0.827f, 0.827f };
const RGBAf	RGBAf::lightpink			={ 1.000f, 0.714f, 0.757f };
const RGBAf	RGBAf::lightsalmon			={ 1.000f, 0.627f, 0.478f };
const RGBAf	RGBAf::lightseagreen		={ 0.125f, 0.698f, 0.667f };
const RGBAf	RGBAf::lightskyblue			={ 0.529f, 0.808f, 0.980f };
const RGBAf	RGBAf::lightslategray		={ 0.467f, 0.533f, 0.600f };
const RGBAf	RGBAf::lightslategrey		={ 0.467f, 0.533f, 0.600f };
const RGBAf	RGBAf::lightsteelblue		={ 0.690f, 0.769f, 0.871f };
const RGBAf	RGBAf::lightyellow			={ 1.000f, 1.000f, 0.878f };
const RGBAf	RGBAf::lime					={ 0.000f, 1.000f, 0.000f };
const RGBAf	RGBAf::limegreen			={ 0.196f, 0.804f, 0.196f };
const RGBAf	RGBAf::linen				={ 0.980f, 0.941f, 0.902f };
const RGBAf	RGBAf::magenta				={ 1.000f, 0.000f, 1.000f };
const RGBAf	RGBAf::maroon				={ 0.502f, 0.000f, 0.000f };
const RGBAf	RGBAf::mediumaquamarine		={ 0.400f, 0.804f, 0.667f };
const RGBAf	RGBAf::mediumblue			={ 0.000f, 0.000f, 0.804f };
const RGBAf	RGBAf::mediumorchid			={ 0.729f, 0.333f, 0.827f };
const RGBAf	RGBAf::mediumpurple			={ 0.576f, 0.439f, 0.859f };
const RGBAf	RGBAf::mediumseagreen		={ 0.235f, 0.702f, 0.443f };
const RGBAf	RGBAf::mediumslateblue		={ 0.482f, 0.408f, 0.933f };
const RGBAf	RGBAf::mediumspringgreen	={ 0.000f, 0.980f, 0.604f };
const RGBAf	RGBAf::mediumturquoise		={ 0.282f, 0.820f, 0.800f };
const RGBAf	RGBAf::mediumvioletred		={ 0.780f, 0.082f, 0.522f };
const RGBAf	RGBAf::midnightblue			={ 0.098f, 0.098f, 0.439f };
const RGBAf	RGBAf::mintcream			={ 0.961f, 1.000f, 0.980f };
const RGBAf	RGBAf::mistyrose			={ 1.000f, 0.894f, 0.882f };
const RGBAf	RGBAf::moccasin				={ 1.000f, 0.894f, 0.710f };
const RGBAf	RGBAf::navajowhite			={ 1.000f, 0.871f, 0.678f };
const RGBAf	RGBAf::navy					={ 0.000f, 0.000f, 0.502f };
const RGBAf	RGBAf::oldlace				={ 0.992f, 0.961f, 0.902f };
const RGBAf	RGBAf::olive				={ 0.502f, 0.502f, 0.000f };
const RGBAf	RGBAf::olivedrab			={ 0.420f, 0.557f, 0.137f };
const RGBAf	RGBAf::orange				={ 1.000f, 0.647f, 0.000f };
const RGBAf	RGBAf::orangered			={ 1.000f, 0.271f, 0.000f };
const RGBAf	RGBAf::orchid				={ 0.855f, 0.439f, 0.839f };
const RGBAf	RGBAf::palegoldenrod		={ 0.933f, 0.910f, 0.667f };
const RGBAf	RGBAf::palegreen			={ 0.596f, 0.984f, 0.596f };
const RGBAf	RGBAf::paleturquoise		={ 0.686f, 0.933f, 0.933f };
const RGBAf	RGBAf::palevioletred		={ 0.859f, 0.439f, 0.576f };
const RGBAf	RGBAf::papayawhip			={ 1.000f, 0.937f, 0.835f };
const RGBAf	RGBAf::peachpuff			={ 1.000f, 0.855f, 0.725f };
const RGBAf	RGBAf::peru					={ 0.804f, 0.522f, 0.247f };
const RGBAf	RGBAf::pink					={ 1.000f, 0.753f, 0.796f };
const RGBAf	RGBAf::plum					={ 0.867f, 0.627f, 0.867f };
const RGBAf	RGBAf::powderblue			={ 0.690f, 0.878f, 0.902f };
const RGBAf	RGBAf::purple				={ 0.502f, 0.000f, 0.502f };
const RGBAf	RGBAf::red					={ 1.000f, 0.000f, 0.000f };
const RGBAf	RGBAf::rosybrown			={ 0.737f, 0.561f, 0.561f };
const RGBAf	RGBAf::royalblue			={ 0.255f, 0.412f, 0.882f };
const RGBAf	RGBAf::saddlebrown			={ 0.545f, 0.271f, 0.075f };
const RGBAf	RGBAf::salmon				={ 0.980f, 0.502f, 0.447f };
const RGBAf	RGBAf::sandybrown			={ 0.957f, 0.643f, 0.376f };
const RGBAf	RGBAf::seagreen				={ 0.180f, 0.545f, 0.341f };
const RGBAf	RGBAf::seashell				={ 1.000f, 0.961f, 0.933f };
const RGBAf	RGBAf::sienna				={ 0.627f, 0.322f, 0.176f };
const RGBAf	RGBAf::silver				={ 0.753f, 0.753f, 0.753f };
const RGBAf	RGBAf::skyblue				={ 0.529f, 0.808f, 0.922f };
const RGBAf	RGBAf::slateblue			={ 0.416f, 0.353f, 0.804f };
const RGBAf	RGBAf::slategray			={ 0.439f, 0.502f, 0.565f };
const RGBAf	RGBAf::slategrey			={ 0.439f, 0.502f, 0.565f };
const RGBAf	RGBAf::snow					={ 1.000f, 0.980f, 0.980f };
const RGBAf	RGBAf::springgreen			={ 0.000f, 1.000f, 0.498f };
const RGBAf	RGBAf::steelblue			={ 0.275f, 0.510f, 0.706f };
const RGBAf	RGBAf::tan					={ 0.824f, 0.706f, 0.549f };
const RGBAf	RGBAf::teal					={ 0.000f, 0.502f, 0.502f };
const RGBAf	RGBAf::thistle				={ 0.847f, 0.749f, 0.847f };
const RGBAf	RGBAf::tomato				={ 1.000f, 0.388f, 0.278f };
const RGBAf	RGBAf::turquoise			={ 0.251f, 0.878f, 0.816f };
const RGBAf	RGBAf::violet				={ 0.933f, 0.510f, 0.933f };
const RGBAf	RGBAf::wheat				={ 0.961f, 0.871f, 0.702f };
const RGBAf	RGBAf::white				={ 1.000f, 1.000f, 1.000f };
const RGBAf	RGBAf::whitesmoke			={ 0.961f, 0.961f, 0.961f };
const RGBAf	RGBAf::yellow				={ 1.000f, 1.000f, 0.000f };
const RGBAf	RGBAf::yellowgreen			={ 0.604f, 0.804f, 0.196f };

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
