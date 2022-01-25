#include "stdafx.h"
#pragma hdrstop

#include <Base/Util/PathUtils.h>

#include <AssetCompiler/AssetCompiler.h>


namespace AssetBaking
{

mxDEFINE_ABSTRACT_CLASS(AssetCompilerI);
mxBEGIN_REFLECTION(AssetCompilerI)
mxEND_REFLECTION

mxDEFINE_CLASS( AssetBuildRules );
mxBEGIN_REFLECTION( AssetBuildRules )
	mxMEMBER_FIELD( compiler ),
mxEND_REFLECTION

}//namespace AssetBaking
