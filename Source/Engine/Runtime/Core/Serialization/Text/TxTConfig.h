/*
=============================================================================
	SON (Simple Object Notation)
=============================================================================
*/
#pragma once

#include <Core/Core.h>
#include <Core/Memory.h>
#include <Core/Serialization/Text/TxTCommon.h>

namespace SON
{

class TextConfig : public AConfigFile
{
	SON::Node *		m_root;
	ByteBuffer32	m_text;
	SON::NodeAllocator	m_nodes;
public:
	TextConfig( AllocatorI & allocator );
	~TextConfig();
	ERet LoadFromFile( const char* path );
	virtual const char* FindString( const char* _key ) const override;
	virtual bool FindInteger( const char* _key, int &_value ) const override;
	virtual bool FindSingle( const char* _key, float &_value ) const override;
	virtual bool FindBoolean( const char* _key, bool &_value ) const override;
};

// first searches in app directory, then in "../../Config/"
ERet GetPathToConfigFile( const char* _filename, String &_filepath );
mxDEPRECATED mxREMOVE_THIS
ERet TryLoadConfig( SON::TextConfig &_config, const char* _filename );

}//namespace SON

mxREMOVE_THIS

#if 0
class Scoped_LoadConfigs
{
	SON::TextConfig	m_engineINI;
	SON::TextConfig	m_clientINI;
public:
	Scoped_LoadConfigs( const EngineSettings& _settings );
	~Scoped_LoadConfigs();
};
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
