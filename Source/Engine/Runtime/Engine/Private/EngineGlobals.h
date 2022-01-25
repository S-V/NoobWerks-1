//
#pragma once

#include <Rendering/Public/Settings.h>

namespace NEngine
{

struct GlobalSettings: CStruct
{
	Rendering::RrGlobalSettings	rendering;

public:
	mxDECLARE_CLASS(GlobalSettings, CStruct);
	mxDECLARE_REFLECTION;
	GlobalSettings();

	void FixBadValues()
	{
		rendering.FixBadValues();
	}
};

extern GlobalSettings	g_settings;

}//namespace NEngine
