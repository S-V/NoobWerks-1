/*
==========================================================
	Compile-time string hashes
	with optional static strings in debug mode
==========================================================
*/
#pragma once

#define nwCFG_DEBUG_NAMEHASHES	(MX_DEBUG || MX_DEVELOPER)


#if nwCFG_DEBUG_NAMEHASHES

	struct NwNameHash
	{
		const char* const	str;
		const NameHash32	hash;

		mxFORCEINLINE NwNameHash()
			: str(nil), hash(0)
		{}
		mxFORCEINLINE NwNameHash(const char* str, const NameHash32 hash)
			: str(str), hash(hash)
		{}
	};

#else
	
	typedef U32 NwNameHash;

#endif

#if nwCFG_DEBUG_NAMEHASHES
	#define nwNAMEHASH( STATIC_STRING )			NwNameHash(STATIC_STRING, GetStaticStringHash(STATIC_STRING))
	#define nwNAMEHASH_STR( NAME_HASH_OBJ )		((NAME_HASH_OBJ).str)
	#define nwNAMEHASH_VAL( NAME_HASH_OBJ )		((NAME_HASH_OBJ).hash)
	#define nwMAKE_NAMEHASH2( str, str_hash )	NwNameHash((str), (str_hash))
#else
	#define nwNAMEHASH( STATIC_STRING )			GetStaticStringHash(STATIC_STRING)
	#define nwNAMEHASH_STR( NAME_HASH_OBJ )		("?")
	#define nwNAMEHASH_VAL( NAME_HASH_OBJ )		(NAME_HASH_OBJ)
	#define nwMAKE_NAMEHASH2( str, str_hash )	(str_hash)
#endif
