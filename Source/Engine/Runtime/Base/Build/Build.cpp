/*
=============================================================================
	File:	Build.cpp
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

const char* mxGetBaseBuildTimestamp()
{
	// the string containing timestamp information
	local_ const char* g_timeStamp =
		__DATE__ " - " __TIME__

	// MVC++ has the __TIMESTAMP__ macro
	// e.g. "Fri Oct  1 17:06:21 2010"
	;

	return g_timeStamp;
}
