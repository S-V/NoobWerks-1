// parses *.def files which contain declarations
#pragma once

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>	// Rendering::NwAnimEventList
#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>
#include <Utility/Animation/AnimationUtil.h>

namespace Doom3
{
	struct ModelDef
	{
		/// what goes right after "model" in .def file
		/// e.g.: "viewmodel_plasmagun"
		String32	name;


		// "inherit" keys will cause all values from another entityDef to be copied into this one
		String32	inherited_model_name;


		/// e.g.: models/md5/monsters/zcc/zcc.md5mesh
		String64	mesh_name;

		/// used by weapon models for first-person view
		V3f			view_offset;

		struct Anim
		{
			/// The name by which the animation is referred to in .def file:
			/// e.g.: "idle", "walk", "melee_attack4", "range_attack_loop1"
			String32		name;

			/// The id of the MD5 animation:
			/// e.g.: models/md5/monsters/zcc/chaingun_wallrotleft_D.md5anim
			String64		md5_anim_id;

			///
			Rendering::NwAnimEventList	events;
		};
		TArray< Anim >	anims;

	public:
		ModelDef()
		{
			view_offset = CV3f(0);
		}
	};

	ERet ParseModelDef(
		const char* model_def_filename
		, ModelDef &model_def_
		);

	ERet ParseModelDef(
		ModelDef &model_def_
		, Lexer& lexer
		);

}//namespace Doom3
