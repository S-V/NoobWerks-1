
#include "stdafx.h"
#pragma hdrstop

#include <AssetCompiler/AssetCompilers/ScriptCompiler.h>

#if WITH_SCRIPT_COMPILER
#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include <Scripting/Scripting.h>
#include <Scripting/Lua/Lua_Helpers.h>
#include <Scripting/ScriptResources.h>


// Include the Lua API header files.
extern "C"{
	#include <Lua/lundump.h>
}//extern "C"


namespace AssetBaking
{

mxDEFINE_CLASS(ScriptCompiler);

// The writer returns an error code: 0 means no errors;
// any other value means an error and stops lua_dump from calling the writer again.
//
static int My_Lua_Writer( lua_State* L, const void* data, size_t size, void* _writer )
{
	UNUSED(L);
	NwBlobWriter* writer = static_cast< NwBlobWriter* >( _writer );
	writer->Write( data, size );
	return 0;
}

const TbMetaClass* ScriptCompiler::getOutputAssetClass() const
{
	return &NwScript::metaClass();
}

ERet ScriptCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	NwBlob	source_code_blob(MemoryHeaps::temporary());
	mxDO(NwBlob_::loadBlobFromFile(
		source_code_blob
		, inputs.path.c_str()
		));

	//
	NwBlob	compiled_bytecode_blob(MemoryHeaps::temporary());
	NwBlobWriter	writer( compiled_bytecode_blob );

	const char* sourceText = static_cast< char* >( source_code_blob.raw() );
	const int sourceLength = source_code_blob.rawSize();
	// the name of the chunk, which is used for error messages and in debug information
	const char* sourcefilename = inputs.path.c_str();

	//
	lua_State* L = AssetProcessor::Get().GetScriptEngine()->GetLuaState();

	lua_lock(L);

	// If there are no errors, lua_load() pushes the compiled chunk as a Lua function on top of the stack.
	LUA_TRY(
		luaL_loadbuffer( L, sourceText, sourceLength, sourcefilename )
		, L
		);

	// dump Lua bytecode as precompiled chunk
	LUA_TRY(
		lua_dump(
		L
		, &My_Lua_Writer
		, &writer
		, inputs.cfg.script_compiler.strip_debug_info
		)
		, L
		);

	lua_unlock(L);


	if( !compiled_bytecode_blob.num() ) {
		return ERR_UNKNOWN_ERROR;
	}


	// save compiled data

	NwScript	script_resource;
	mxDO(Arrays::Copy(script_resource.code, compiled_bytecode_blob));
	mxDO(make_AssetID_from_filepath(
		script_resource.sourcefilename,
		sourcefilename
		));

	//
	mxDO(Serialization::SaveMemoryImage(
		script_resource
		, NwBlobWriter( outputs.object_data )
		));

	return ALL_OK;
}

}//namespace AssetBaking

#endif
