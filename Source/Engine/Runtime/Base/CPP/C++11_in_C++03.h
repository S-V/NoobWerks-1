/*

defines library functions from C++11 so that we can use them in MVC+2008

e.g.
std::stoi, std::stol, std::stoll

*/
#pragma once

#include <Base/Base.h>



/*
===============================================================================
	MATH
===============================================================================
*/

// cos()
#include <math.h>

//inline
//float cos(float x) {
//	return cosf(x);
//}
//inline
//double cos(double x) {
//	return cos(x);
//}

/*
===============================================================================
	STRING TO INT CONVERSION
===============================================================================
*/
//
#include <string>

// https://en.cppreference.com/w/cpp/string/basic_string/stol
inline
int stoi( const std::string& str, std::size_t* pos = 0, int base = 10 )
{
	return strtol(str.c_str(), NULL, base);
}

inline
long stol( const std::string& str, std::size_t* pos = 0, int base = 10 )
{
	return strtol(str.c_str(), NULL, base);
}

inline
long long stoll( const std::string& str, std::size_t* pos = 0, int base = 10 )
{
	return strtol(str.c_str(), NULL, base);
}


/*
===============================================================================
	NUMBER-TO-STRING CONVERSION/FORMATTING
===============================================================================
*/
#include <sstream>

namespace std
{
	template<typename T>
	std::string to_string( const T &t, int digits = 20 )
	{
		std::stringstream ss;
		ss.precision( digits );
		ss << std::fixed << t;
		return ss.str();
	}
}//namespace std
