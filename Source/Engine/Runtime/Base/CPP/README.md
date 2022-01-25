Utilities to simplify C++ programming.

TODO:
- FOR_EACH for linked lists.

- FOR_EACH for static arrays.
e.g. this doesn't work:
	TScopedPtr< ozz::math::SoaTransform >	locals_per_each_anim[ MAX_BLENDED_ANIMS ];
	nwFOR_EACH(TScopedPtr< ozz::math::SoaTransform > &o, locals_per_each_anim) {
		new(&o) TScopedPtr< ozz::math::SoaTransform >(local_scratchpad);
	}