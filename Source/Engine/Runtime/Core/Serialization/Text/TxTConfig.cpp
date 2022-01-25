/*
=============================================================================
=============================================================================
*/
#include <Base/Base.h>
#include <Core/Serialization/Text/TxTConfig.h>
#include <Core/Serialization/Text/TxTReader.h>

namespace SON
{

TextConfig::TextConfig( AllocatorI & allocator )
	: m_nodes( allocator )
{
	m_root = nil;
}
TextConfig::~TextConfig()
{
}
ERet TextConfig::LoadFromFile( const char* path )
{
	FileReader	stream;
	mxTRY(stream.Open( path, FileRead_NoErrors ));

	const size_t fileSize = stream.Length();
	mxDO(m_text.setNum( fileSize ));

	char* buffer = (char*) m_text.raw();
	mxDO(stream.Read( buffer, fileSize ));

	SON::Parser		parser;
	parser.buffer = buffer;
	parser.length = fileSize;
	parser.file = path;

	m_root = SON::ParseBuffer(parser, m_nodes);
	chkRET_X_IF_NIL(m_root, ERR_FAILED_TO_PARSE_DATA);
	chkRET_X_IF_NOT(parser.errorCode == 0, ERR_FAILED_TO_PARSE_DATA);

	ptPRINT("Loaded text config: '%s'", path);

	return ALL_OK;
}
const char* TextConfig::FindString( const char* _key ) const
{
	const SON::Node* valueNode = SON::FindValue( m_root, _key );
	return valueNode ? SON::AsString( valueNode ) : nil;
}
bool TextConfig::FindInteger( const char* _key, int &_value ) const
{
	const SON::Node* valueNode = SON::FindValue( m_root, _key );
	if( valueNode ) {
		_value = SON::AsDouble(valueNode);
		return true;
	}
	return false;
}
bool TextConfig::FindSingle( const char* _key, float &_value ) const
{
	const SON::Node* valueNode = SON::FindValue( m_root, _key );
	if( valueNode ) {
		_value = SON::AsDouble(valueNode);
		return true;
	}
	return false;
}
bool TextConfig::FindBoolean( const char* _key, bool &_value ) const
{
	const SON::Node* valueNode = SON::FindValue( m_root, _key );
	if( valueNode ) {
		_value = SON::AsBoolean(valueNode);
		return true;
	}
	return false;
}

// first searches in app directory, then in "../../Config/"
ERet GetPathToConfigFile( const char* _filename, String &_filepath )
{
	//const char* exe_folder = mxGetLauchDirectory();
	HINSTANCE	hAppInstance = ::GetModuleHandle( NULL );
	char	exe_folder[MAX_PATH];
	::GetModuleFileNameA( hAppInstance, exe_folder, mxCOUNT_OF(exe_folder) );


	String512	current_search_path;
	mxDO(Str::CopyS( current_search_path, exe_folder ));
	Str::StripFileName( current_search_path );

	mxDO(Str::ComposeFilePath( _filepath, current_search_path.c_str(), _filename ));
	if(OS::IO::FileExists( _filepath.c_str() ))
	{
		return ALL_OK;
	}

	// look in "../../Config"
	Str::StripLastFolders( current_search_path, 2 );	// e.g. "../Bin/x64/EXE.exe"
	Str::NormalizePath( current_search_path );
	mxDO(Str::AppendS( current_search_path, "Config/" ));

	mxDO(Str::ComposeFilePath( _filepath, current_search_path.c_str(), _filename ));
	if(OS::IO::FileExists( _filepath.c_str() ))
	{
		return ALL_OK;
	}

	return ERR_FILE_OR_PATH_NOT_FOUND;
}

}//namespace SON

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
