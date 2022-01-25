#include "stdafx.h"
#pragma hdrstop


extern ERet gameEntryPoint();

int main(int /*argc*/, char** /*argv*/)
{
	//runUnitTests_M44f();

	//
	NwSetupMemorySystem	setupMemory;

	//
	ERet ret = gameEntryPoint();

	return (ret == ALL_OK) ? 0 : -1;
}
