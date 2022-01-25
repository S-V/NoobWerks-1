#include <Base/Base.h>
#pragma hdrstop

#include <Engine/Private/EngineGlobals.h>


namespace NEngine
{

GlobalSettings	g_settings;

mxDEFINE_CLASS(GlobalSettings);
mxBEGIN_REFLECTION(GlobalSettings)
	mxMEMBER_FIELD( rendering ),
mxEND_REFLECTION;
GlobalSettings::GlobalSettings()
{
	//invert_mouse_x = false;
	//invert_mouse_y = false;
	//mouse_sensitivity = 1.0f;
}

}//namespace NEngine
