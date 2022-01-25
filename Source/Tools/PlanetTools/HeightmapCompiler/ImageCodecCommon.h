// 
#pragma once


namespace ImageCodec
{
	/// Subbands are four spatially filtered lower-resolution versions of the image.
	/// We identify subbands by LL, LH, HL, orHH, where L and H indicate low-pass and high-pass filtering, respectively;
	/// the first letter refers to the horizontal direction, and the second letter refers to the vertical direction.
	/// LL is a low-frequency subband (an approximation of the input image),
	/// HL (high-low - horizontal high-pass filter and the vertical low-pass filter)
	/// subband extracts vertical features of the original image,
	/// LH (low-high) subband gives horizontal features,
	/// HH (high-high) subband gives diagonal features.
	enum SubbandType
	{
		LL = 0, HL = 1,
		LH = 2, HH = 3,
		NumSubbands
	};
}
