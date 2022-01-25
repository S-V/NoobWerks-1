#include "Scripting_PCH.h"
#pragma hdrstop

#include <Scripting/ScriptResources.h>

mxDEFINE_CLASS( NwScript );
mxBEGIN_REFLECTION( NwScript )
	mxMEMBER_FIELD( code ),
	//mxMEMBER_FIELD( function_names ),
	mxMEMBER_FIELD( sourcefilename ),
mxEND_REFLECTION
//ERet NwScript::Load( NwScript *_script, const LoadContext& _ctx )
//{
//	UNDONE;
//	NwScript::Header_d	header;
//	mxDO(_ctx.stream.Get(header));
//
//	mxDO(_script->code.setNum(header.size));
//	mxDO(_ctx.stream.Read(_script->code.raw(), header.size));
//	return ALL_OK;
//}
//void NwScript::Unload( NwScript * _script )
//{
//	UNDONE;
//}
