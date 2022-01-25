// parses *.def files which contain declarations
#pragma once

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>	// Rendering::NwAnimEventList
#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_ModelDef_Parser.h>
#include <Utility/Animation/AnimationUtil.h>


namespace Doom3
{
	struct EntityDef
	{
		/// what goes right after "entityDef" in .def file, e.g. "weapon_plasmagun"
		String64	name;

		/// "model", e.g. "models/weapons/plasmagun/plasmagun_world.lwo"
		String64	model_name;

		/// "model_view", e.g. "viewmodel_plasmagun"
		String64	viewmodel_name;

		/// "model_world", e.g. "worldmodel_plasmagun"
		String64	worldmodel_name;

		//
		struct SoundRef
		{
			/// "snd_acquire"
			String32	sound_local_name;

			/// "player_plasma_raise"
			String64	sound_resource;
		};
		TArray< SoundRef >	sounds;

	public:
		const char* FindUsedModelName() const
		{
			const char* used_model_name = nil;

			if( !viewmodel_name.IsEmpty() ) {
				used_model_name = viewmodel_name.c_str();
			} else if( !model_name.IsEmpty() ) {
				used_model_name = model_name.c_str();
			}

			return used_model_name;
		}
	};

	struct DefFileContents
	{
		DynamicArray< ModelDef >	models;
		DynamicArray< EntityDef >	entities;

	public:
		DefFileContents( AllocatorI & allocator )
			: models( allocator )
			, entities( allocator )
		{
		}

		const EntityDef* FindFirstEntityDefWithModel()
		{
			nwFOR_EACH(const EntityDef& ent_def, entities) {
				if(
					!ent_def.model_name.IsEmpty()
					||
					!ent_def.viewmodel_name.IsEmpty()
					||
					!ent_def.worldmodel_name.IsEmpty()
					)
				{
					return &ent_def;
				}
			}
			return nil;
		}

		const ModelDef* FindModelDefByName(const char* model_name)
		{
			nwFOR_EACH(const ModelDef& model_def, models) {
				if(Str::EqualS(model_def.name, model_name))
				{
					return &model_def;
				}
			}
			return nil;
		}
	};

	ERet ParseDefFile(
		DefFileContents &def_file_contents_
		, const char* def_filename
		);

}//namespace Doom3
