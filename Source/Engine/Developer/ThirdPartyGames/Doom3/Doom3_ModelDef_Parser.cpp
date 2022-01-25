// parses *.def files which contain declarations
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Text/Lexer.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_ModelDef_Parser.h>


namespace Doom3
{

using namespace Rendering;

static
ERet parseAnimEvent(
	Rendering::NwAnimEvent &new_anim_event_
	, bool *is_known_event_type_
	, Lexer & lexer
	)
{
	String32	command_name;
	mxDO(expectName( lexer, command_name ));

	String64	command_parameter;

	//
	*is_known_event_type_ = true;


#pragma region Sounds

	if( Str::EqualS( command_name, "sound_voice" ) )
	{
		new_anim_event_.type = FC_SOUND_VOICE;
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "sound_voice2" ) )
	{
		new_anim_event_.type = FC_SOUND_VOICE2;
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "sound_body" ) )
	{
		new_anim_event_.type = FC_SOUND_BODY;
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "sound_body2" ) )
	{
		new_anim_event_.type = FC_SOUND_BODY2;
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "sound_weapon" ) )
	{
		new_anim_event_.type = FC_SOUND_WEAPON;
		mxDO(expectName( lexer, command_parameter ));
	}

#pragma endregion


#pragma region Attacking

	else if( Str::EqualS( command_name, "melee" ) )
	{
		new_anim_event_.type = FC_MELEE;
		// entityDef melee_impRightClaw
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "direct_damage" ) )
	{
		new_anim_event_.type = FC_DIRECT_DAMAGE;
		// entityDef
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "attack_begin" ) )
	{
		new_anim_event_.type = FC_BEGIN_ATTACK;
		// entityDef melee_impLeapAttack
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "attack_end" ) )
	{
		new_anim_event_.type = FC_END_ATTACK;
		// no params
	}


	else if( Str::EqualS( command_name, "muzzle_flash" ) )
	{
		new_anim_event_.type = FC_MUZZLE_FLASH;
		// joint name
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "create_missile" ) )
	{
		new_anim_event_.type = FC_CREATE_MISSILE;
		mxDO(expectName( lexer, command_parameter ));
	}
	else if( Str::EqualS( command_name, "launch_missile" ) )
	{
		new_anim_event_.type = FC_LAUNCH_MISSILE;
		mxDO(expectName( lexer, command_parameter ));
	}

#pragma endregion


	else if( Str::EqualS( command_name, "disableGravity" ) )
	{
		lexer.Warning("unknown anim event: %s", command_name.c_str());
		*is_known_event_type_ = false;
	}
	else if( Str::EqualS( command_name, "enableWalkIK" ) )
	{
		lexer.Warning("unknown anim event: %s", command_name.c_str());
		*is_known_event_type_ = false;
	}
	else if( Str::EqualS( command_name, "disableWalkIK" ) )
	{
		lexer.Warning("unknown anim event: %s", command_name.c_str());
		*is_known_event_type_ = false;
	}
	else if( Str::EqualS( command_name, "leftfoot" ) )
	{
		lexer.Warning("unknown anim event: %s", command_name.c_str());
		*is_known_event_type_ = false;
	}
	else if( Str::EqualS( command_name, "rightfoot" ) )
	{
		lexer.Warning("unknown anim event: %s", command_name.c_str());
		*is_known_event_type_ = false;
	}

	else
	{
		lexer.Warning("unknown anim event: %s, skipping rest of line...", command_name.c_str());
		mxENSURE(lexer.SkipRestOfLine(), ERR_FAILED_TO_PARSE_DATA, "");
		*is_known_event_type_ = false;
	}

	if( *is_known_event_type_ )
	{
		if( !command_parameter.IsEmpty() )
		{
			new_anim_event_.parameter.name = AssetID_from_FilePath(command_parameter.c_str());
			new_anim_event_.parameter.hash = GetDynamicStringHash(command_parameter.c_str());
		}
	}

	return ALL_OK;
}

static
ERet parseAnimEvents(
	ModelDef::Anim &new_anim_
	, Lexer & lexer
	)
{
	TbToken	next_token;

	while( lexer.PeekToken( next_token ) )
	{
		if( next_token == '}' )
		{
			return ALL_OK;
		}

		if( next_token == "random_cycle_start" )
		{
			lexer.ReadToken( next_token );
			// start the animation at a random time so that characters don't walk in sync
		}
		else if( next_token == "ai_no_turn" )
		{
			lexer.ReadToken( next_token );
		}
		else
		{
			mxDO(expectTokenString( lexer, "frame" ));

			//
			int		frame_number;
			mxDO(expectInt32( lexer, next_token, frame_number, 0 ));

			//
			Rendering::NwAnimEventList::FrameLookup	frame_lookup;
			// NOTE: 'time' will be changed from 'frame_number' to quantized time
			frame_lookup.time = frame_number;

			Rendering::NwAnimEvent	animation_event;
			bool			is_known_event_type = false;
			mxDO(parseAnimEvent(
				animation_event
				, &is_known_event_type
				, lexer
				));

			if( is_known_event_type )
			{
				mxDO(new_anim_.events._frame_lookup.add( frame_lookup ));
				mxDO(new_anim_.events._frame_events.add( animation_event ));
			}
		}
	}

	return ALL_OK;
}

// e.g. run, pain_left_arm, "196drop",
// i.e. anim names may start with a leading quote.
static
ERet ParseAnimName( ATokenReader& lexer, String &tokenString )
{
	//TbToken	t;
	//lexer.PeekToken(t);
	if(PeekTokenChar(lexer, '\"'))
	{
		return expectString( lexer, tokenString );
	}
	else
	{
		return expectName( lexer, tokenString );
	}
}

static
ERet parseAnimations(
	ModelDef &model_def_
	, Lexer & lexer
	)
{
	TbToken	next_token;

	while( lexer.PeekToken( next_token ) )
	{
		if( next_token == '}' )
		{
			return ALL_OK;
		}

		//
		if( PeekTokenString( lexer, "channel" ) )
		{
			lexer.Warning("Channels are not supported!");

			mxDO(lexer.SkipRestOfLine()
				? ALL_OK : ERR_FAILED_TO_PARSE_DATA);
		}
		else if( PeekTokenString( lexer, "skin" ) )
		{
			lexer.Warning("Skins are not supported!");

			mxDO(lexer.SkipRestOfLine()
				? ALL_OK : ERR_FAILED_TO_PARSE_DATA);
		}
		else if( PeekTokenString( lexer, "offset" ) )
		{
			lexer.Warning("Offsets are not supported!");

			mxDO(lexer.SkipRestOfLine()
				? ALL_OK : ERR_FAILED_TO_PARSE_DATA);
		}
		else
		{
			mxDO(expectTokenString( lexer, "anim" ));

			//
			ModelDef::Anim	new_anim;

			mxDO(ParseAnimName( lexer, new_anim.name ));

			mxDO(expectName( lexer, new_anim.md5_anim_id ));

			lexer.PeekToken( next_token );

			if( next_token == '{' )
			{
				mxDO(expectTokenChar( lexer, '{' ));
				mxDO(parseAnimEvents( new_anim, lexer ));
				mxDO(expectTokenChar( lexer, '}' ));
			}

			mxDO(model_def_.anims.add( new_anim ));
		}
	}

	return ALL_OK;
}

namespace
{
	ERet skipUnknownSections(
		Lexer & lexer
		)
	{
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

			if( next_token == "entityDef" )
			{
				SkipToken( lexer );
				mxDO(expectName( lexer, section_name ));
				mxDO(SkipBracedSection( lexer ));
				continue;
			}

			if( next_token == "particle" )
			{
				SkipToken( lexer );
				mxDO(expectName( lexer, section_name ));
				mxDO(SkipBracedSection( lexer ));
				continue;
			}

			break;
		}//

		return ALL_OK;
	}
}//namespace

ERet ParseModelDef(
	const char* model_def_filename
	, ModelDef &model_def_
	)
{
	DynamicArray< char >	source_file_data( MemoryHeaps::temporary() );

	mxDO(Str::loadFileToString(
		source_file_data
		, model_def_filename
		));

	//
	Lexer	lexer(
		source_file_data.raw()
		, source_file_data.rawSize()
		, model_def_filename
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
	mxDO(skipUnknownSections( lexer ));

	//
	mxDO(ParseModelDef( model_def_, lexer ));

	return ALL_OK;
}

ERet ParseModelDef(
				   ModelDef &model_def_
				   , Lexer& lexer
				   )
{
	//
	mxDO(skipUnknownSections( lexer ));

	//
	mxDO(expectTokenString( lexer, "model" ));

	//
	mxDO(expectName( lexer, model_def_.name ));

	//
	mxDO(expectChar( lexer, '{' ));

	//
	if( PeekTokenString( lexer, "inherit" ) )
	{
		SkipToken(lexer);
		//
		mxDO(expectName( lexer, model_def_.inherited_model_name ));
		return ALL_OK;
	}

	//
	mxDO(expectTokenString( lexer, "mesh" ));
	mxDO(expectName( lexer, model_def_.mesh_name ));

	if( PeekTokenString( lexer, "offset" ) )
	{
		mxDO(expectTokenString( lexer, "offset" ));
		mxDO(parseVec3D( lexer, model_def_.view_offset.a ));
	}

	//
	mxDO(parseAnimations(
		model_def_
		, lexer
		));

	mxDO(expectChar( lexer, '}' ));

	DBGOUT("Parsed model def: '%s', %d anims"
		, model_def_.name.c_str(), model_def_.anims.num()
		);
	return ALL_OK;
}

}//namespace Doom3
