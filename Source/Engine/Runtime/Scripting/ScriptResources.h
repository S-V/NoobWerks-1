// The scripting module interface.
#pragma once

#include <Core/Assets/AssetManagement.h>


// represents compiled bytecode ready for execution
struct NwScript: NwResource
{
	/// either compiled bytecode or source text
	TArray< char >	code;

	///// entry points
	//TArray< String >	function_names;

	/// for debug information and error messages
	AssetID		sourcefilename;

public:
	mxDECLARE_CLASS(NwScript,CStruct);
	mxDECLARE_REFLECTION;
};
