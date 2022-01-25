// parses *.def files which contain declarations
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Text/Lexer.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_Def_FileParser.h>


namespace Doom3
{

// in DarkMod, names can contain a colon, e.g.:
// entityDef atdm:weapon_shortsword

ERet ParseExtendedName(
	String &name_
	, Lexer& lexer
	)
{
	mxDO(expectName( lexer, name_ ));

	TbToken	next_token;
	if(lexer.PeekToken( next_token ))
	{
		if( next_token == ":" )
		{
			SkipToken( lexer );

			String32	second_name_part;
			mxDO(expectName( lexer, second_name_part ));

			Str::Append(name_, ':');
			Str::Append(name_, second_name_part);

			return ALL_OK;
		}
	}

	return ALL_OK;
}

ERet ParseEntityDef(
				   EntityDef &entity_def_
				   , Lexer& lexer
				   )
{
	//
	mxDO(expectTokenString( lexer, "entityDef" ));

	//
	mxDO(ParseExtendedName( entity_def_.name, lexer ));

	//
	mxDO(expectChar( lexer, '{' ));

	//
	TbToken	next_token;
	while( lexer.PeekToken( next_token ) )
	{
		if( next_token == "}" )
		{
			break;
		}

		//
		if( next_token == "model" )
		{
			SkipToken( lexer );
			mxDO(expectString( lexer, entity_def_.model_name ));
			continue;
		}
		//
		if( next_token == "model_view" )
		{
			SkipToken( lexer );
			mxDO(expectString( lexer, entity_def_.viewmodel_name ));
			continue;
		}
		//
		if( next_token == "model_world" )
		{
			SkipToken( lexer );
			mxDO(expectString( lexer, entity_def_.worldmodel_name ));
			continue;
		}

		//
		if( Str::StartsWith(next_token.text, "snd_") )
		{
			EntityDef::SoundRef	new_sound_ref;
			mxDO(expectString( lexer, new_sound_ref.sound_local_name ));
			mxDO(expectString( lexer, new_sound_ref.sound_resource ));

			mxDO(entity_def_.sounds.add(new_sound_ref));
			continue;
		}

		SkipToken( lexer );

	}//while peek token

	//
	mxDO(expectChar( lexer, '}' ));

	DBGOUT("Parsed entity def: '%s', %d sounds"
		, entity_def_.name.c_str(), entity_def_.sounds.num()
		);
	return ALL_OK;
}

ERet ParseDefFile(
	DefFileContents &def_file_contents_
	, const char* def_filename
	)
{
	DynamicArray< char >	source_file_data( MemoryHeaps::temporary() );

	mxDO(Str::loadFileToString(
		source_file_data
		, def_filename
		));

	//
	Lexer	lexer(
		source_file_data.raw()
		, source_file_data.rawSize()
		, def_filename
		);

	//
	LexerOptions	lexerOptions;
	lexerOptions.m_flags.raw_value
		= LexerOptions::SkipComments
		| LexerOptions::IgnoreUnknownCharacterErrors
		| LexerOptions::AllowPathNames
		;

	lexer.SetOptions( lexerOptions );

	//
	TbToken	next_token;
	while( lexer.PeekToken( next_token ) )
	{
		String64	section_name;

		if( next_token == "export" )
		{
			SkipToken( lexer );
			mxDO(expectName( lexer, section_name ));
			mxDO(SkipBracedSection( lexer ));
			continue;
		}

		if( next_token == "model" )
		{
			ModelDef	new_model_def;
			mxDO(ParseModelDef( new_model_def, lexer ));

			mxDO(def_file_contents_.models.add(new_model_def));
			continue;
		}

		if( next_token == "entityDef" )
		{
			EntityDef	new_entity_def;
			mxDO(ParseEntityDef( new_entity_def, lexer ));

			mxDO(def_file_contents_.entities.add(new_entity_def));
			continue;
		}

		if( next_token == "particle" )
		{
			SkipToken( lexer );
			mxDO(expectName( lexer, section_name ));
			mxDO(SkipBracedSection( lexer ));
			continue;
		}

		SkipToken(lexer);

	}//while peek token

	//
	DBGOUT("Parsed .def file: '%s', %d models, %d entities"
		, def_filename
		, def_file_contents_.models.num()
		, def_file_contents_.entities.num()
		);

	return ALL_OK;
}

}//namespace Doom3
