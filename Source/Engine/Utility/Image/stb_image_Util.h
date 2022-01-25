//
#pragma once


namespace stb_image_Util
{

class ImageDeleter: NonCopyable
{
	unsigned char* _image_data;

public:
	ImageDeleter( unsigned char* image_data );
	~ImageDeleter();
};

}//namespace stb_image_Util
