// generates impl for "stb_image.h"
#include <Base/Base.h>
#include <Core/Core.h>
#pragma hdrstop

#include <Utility/Image/stb_image_Util.h>

// stb_image declares a variable named 'final'
#undef final
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

namespace stb_image_Util
{

ImageDeleter::ImageDeleter( unsigned char* image_data )
	: _image_data( image_data )
{}

ImageDeleter::~ImageDeleter()
{
	stbi_image_free( _image_data );
}

}//namespace stb_image_Util

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
