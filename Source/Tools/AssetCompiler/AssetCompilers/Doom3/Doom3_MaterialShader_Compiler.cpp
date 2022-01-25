// Doom 3's *.mtr file reader, e.g. "weapons.mtr".
#include "stdafx.h"
#pragma hdrstop

#include <assert.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <Base/Base.h>
#include <Base/Template/Algorithm/Sorting/InsertionSort.h>

#include <Core/Text/Lexer.h>
#include <Core/Text/TextWriter.h>
#include <Core/Assets/AssetBundle.h>

#include <Graphics/Private/shader_effect_impl.h>
#include <Rendering/Private/Modules/idTech4/idMaterial.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <AssetCompiler/AssetCompilers/Graphics/TextureCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/MaterialCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/_MaterialShaderEffectsCompilation.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_MaterialShaders.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_MaterialShader_Compiler.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_GLState.h>
#include <AssetCompiler/AssetBundleBuilder.h>



// Debugging
static const bool DBG_PRINT_MATERIAL_VERBOSE = true;
static const bool DBG_PRINT_MATERIAL_PASS_MERGING = true;
static const bool DBG_DUMP_COMPILED_MATERIAL_TO_TEXT_FILE = 0;

#define DBG_PRINT_EXPR_REGS	(0)
#define DBG_PRINT_EXPR_OPS	(0)

// Config
#define SKIP_FAILED_MATERIALS_SILENTLY					(1)
#define SKIP_STAGES_FOR_MONSTER_SKIN_BURNAWAY_EFFECT	(1)
#define FOLD_CONSTANT_DIVISIONS	(1)


using namespace Rendering;


namespace Doom3
{
	const char* stageLightingToString(stageLighting_t sl)
	{
		switch( sl )
		{
		case SL_AMBIENT:// execute after lighting
			return "AMBIENT";

		case SL_BUMP:
			return "BUMP";

		case SL_DIFFUSE:
			return "DIFFUSE";

		case SL_SPECULAR:
			return "SPECULAR";

		case SL_COVERAGE:
			return "COVERAGE";

			mxNO_SWITCH_DEFAULT;
		}
		return nil;
	}


	namespace Heuristics
	{
		bool IsFirstPersonWeaponModel(const char* material_name)
		{
			return strstr(material_name, "models/weapons/")
				|| strstr(material_name, "models/items/flashlight/")
				|| strstr(material_name, "models/items/lantern/")
				;
		}
	}//namespace Heuristics

}

using namespace Doom3;





namespace
{


#define UNDONE_PARTS_MARKER
//#define UNDONE_PARTS_MARKER	mxASSERT(0)


//static const char* MISSING_TEXTURE_NAME = "missing_texture";


AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }

template< class STRING >
mxFORCEINLINE bool streqi( const STRING& token, const Chars& s )
{
	return 0 == stricmp( token.c_str(), s.c_str() );
}

bool tableExists( const AssetID& table_name )
{
	UNDONE_PARTS_MARKER;
	return true;
	//return Doom3_AssetBundles::Get().material_tables.containsAsset(
	//	table_name,
	//	idDeclTable::metaClass()
	//	);
}



// info parms
typedef struct {
	const Chars	name;
	int			clearSolid, surfaceFlags, contents;
} infoParm_t;

static const infoParm_t	infoParms[] = {
	// game relevant attributes
	{"solid",		0,	0,	CONTENTS_SOLID },		// may need to override a clearSolid
	{"water",		1,	0,	CONTENTS_WATER },		// used for water
	{"playerclip",	0,	0,	CONTENTS_PLAYERCLIP },	// solid to players
	{"monsterclip",	0,	0,	CONTENTS_MONSTERCLIP },	// solid to monsters
	{"moveableclip",0,	0,	CONTENTS_MOVEABLECLIP },// solid to moveable entities
	{"ikclip",		0,	0,	CONTENTS_IKCLIP },		// solid to IK
	{"blood",		0,	0,	CONTENTS_BLOOD },		// used to detect blood decals
	{"trigger",		0,	0,	CONTENTS_TRIGGER },		// used for triggers
	{"aassolid",	0,	0,	CONTENTS_AAS_SOLID },	// solid for AAS
	{"aasobstacle",	0,	0,	CONTENTS_AAS_OBSTACLE },// used to compile an obstacle into AAS that can be enabled/disabled
	{"flashlight_trigger",	0,	0,	CONTENTS_FLASHLIGHT_TRIGGER }, // used for triggers that are activated by the flashlight
	{"nonsolid",	1,	0,	0 },					// clears the solid flag
	{"nullNormal",	0,	SURF_NULLNORMAL,0 },		// renderbump will draw as 0x80 0x80 0x80

	// utility relevant attributes
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"qer_nocarve",	1,	0,	CONTENTS_NOCSG},		// don't cut brushes in editor

	{"discrete",	1,	SURF_DISCRETE,	0 },		// surfaces should not be automatically merged together or
	// clipped to the world,
	// because they represent discrete objects like gui shaders
	// mirrors, or autosprites
	{"noFragment",	0,	SURF_NOFRAGMENT,	0 },

	{"slick",		0,	SURF_SLICK,		0 },
	{"collision",	0,	SURF_COLLISION,	0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nodamage",	0,	SURF_NODAMAGE,	0 },		// no falling damage when hitting
	{"ladder",		0,	SURF_LADDER,	0 },		// climbable
	{"nosteps",		0,	SURF_NOSTEPS,	0 },		// no footsteps

	// material types for particle, sound, footstep feedback
	{"metal",		0,  SURFTYPE_METAL,		0 },	// metal
	{"stone",		0,  SURFTYPE_STONE,		0 },	// stone
	{"flesh",		0,  SURFTYPE_FLESH,		0 },	// flesh
	{"wood",		0,  SURFTYPE_WOOD,		0 },	// wood
	{"cardboard",	0,	SURFTYPE_CARDBOARD,	0 },	// cardboard
	{"liquid",		0,	SURFTYPE_LIQUID,	0 },	// liquid
	{"glass",		0,	SURFTYPE_GLASS,		0 },	// glass
	{"plastic",		0,	SURFTYPE_PLASTIC,	0 },	// plastic
	{"ricochet",	0,	SURFTYPE_RICOCHET,	0 },	// behaves like metal but causes a ricochet sound

	// unassigned surface types
	{"surftype10",	0,	SURFTYPE_10,	0 },
	{"surftype11",	0,	SURFTYPE_11,	0 },
	{"surftype12",	0,	SURFTYPE_12,	0 },
	{"surftype13",	0,	SURFTYPE_13,	0 },
	{"surftype14",	0,	SURFTYPE_14,	0 },
	{"surftype15",	0,	SURFTYPE_15,	0 },
};
static const int numInfoParms = sizeof(infoParms) / sizeof (infoParms[0]);

}//namespace



//////////////////////////////////////////////////////////////////////////

namespace Doom3
{

/*

Any errors during parsing just set MF_DEFAULTED and return, rather than throwing
a hard error. This will cause the material to fall back to default material,
but otherwise let things continue.

Each material may have a set of calculations that must be evaluated before
drawing with it.

Every expression that a material uses can be evaluated at one time, which
will allow for perfect common subexpression removal when I get around to
writing it.

Without this, scrolling an entire surface could result in evaluating the
same texture texture_matrix calculations a half dozen times.

  Open question: should I allow arbitrary per-vertex color, texCoord, and vertex
  calculations to be specified in the material code?

  Every stage will definitely have a valid image pointer.

  We might want the ability to change the sort value based on conditionals,
  but it could be a hassle to implement,

*/

// keep all of these on the stack, when they are static it makes material parsing non-reentrant
struct mtrParsingData_t
{
	int				numOps;
	int				numRegisters;

	// true if all registers are filled with constants
	bool			registersAreConstant;

	int				numStages;
	int				numAmbientStages;

	//
	DynamicArray< idMtrTableRef >	table_refs;

	int			contentFlags;		// content flags
	int			surfaceFlags;		// surface flags
	int			materialFlags;		// material flags (materialFlags_t)

	materialCoverage_t	coverage;

	cullType_t			cullType;		// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED

	bool			forceOverlays;

	//
	char			material_name[128];

	bool			registerIsTemporary[MAX_EXPRESSION_REGISTERS];
	float			shaderRegisters[MAX_EXPRESSION_REGISTERS];
	expOp_t			shaderOps[MAX_EXPRESSION_OPS];
	shaderStage_t	parseStages[MAX_SHADER_STAGES];

public:
	mtrParsingData_t( const char* mat_shader_name, AllocatorI & allocator )
		: table_refs( allocator )
	{
		numOps = 0;
		numRegisters = EXP_REG_NUM_PREDEFINED;	// leave space for the parms to be copied in
		for ( int i = 0; i < numRegisters; i++ ) {
			registerIsTemporary[i] = true;		// they aren't constants that can be folded
		}

		registersAreConstant = true;			// until shown otherwise

		numStages = 0;
		numAmbientStages = 0;

		contentFlags = 0;
		surfaceFlags = 0;
		materialFlags = 0;

		coverage = MC_BAD;

		cullType = CT_FRONT_SIDED;

		forceOverlays = false;

		strcpy(material_name, mat_shader_name);

		for ( int i = EXP_REG_NUM_PREDEFINED; i < mxCOUNT_OF(registerIsTemporary); i++ ) {
			registerIsTemporary[i] = false;
		}

		mxZERO_OUT(shaderRegisters);
		mxZERO_OUT(shaderOps);
	}
};

}//namespace Doom3

/*
=============
idMaterial::allocConstantRegister
=============
*/
ERet allocConstantRegister(
						   Rendering::RegisterIndex_t &result_
						   , float f
						   , mtrParsingData_t * pd
						   , Lexer & lexer
						   )
{
	int		i;
	for ( i = EXP_REG_NUM_PREDEFINED; i < pd->numRegisters ; i++ ) {
		if ( !pd->registerIsTemporary[i] && pd->shaderRegisters[i] == f ) {
			result_ = i;
			return ALL_OK;
		}
	}

	if ( pd->numRegisters == MAX_EXPRESSION_REGISTERS ) {
		lexer.Warning(
			"allocConstantRegister: material '%s' hit MAX_EXPRESSION_REGISTERS",
			pd->material_name
			);
		//SetMaterialFlag( MF_DEFAULTED );
		return ERR_BUFFER_TOO_SMALL;
	}

#if DBG_PRINT_EXPR_REGS
	DEVOUT("AllocConst[%d]:= %f", pd->numRegisters, f);
#endif

	pd->registerIsTemporary[i] = false;
	pd->shaderRegisters[i] = f;
	pd->numRegisters++;

	result_ = i;

	return ALL_OK;
}

Rendering::RegisterIndex_t GetExpressionConstant(
									  float f
									  , mtrParsingData_t * pd
									  , Lexer & lexer
									  )
{
	Rendering::RegisterIndex_t	result = 0;
	const ERet ret = allocConstantRegister( result, f, pd, lexer );
	mxASSERT(mxSUCCEDED(ret));
	return result;
}

/*
=============
idMaterial::GetExpressionTemporary
=============
*/
ERet allocExpressionTemporary(
							Rendering::RegisterIndex_t &result_
							, mtrParsingData_t * pd
							, Lexer & lexer
							)
{
	if ( pd->numRegisters >= MAX_EXPRESSION_REGISTERS ) {
		lexer.Warning(
			"GetExpressionTemporary: material '%s' hit MAX_EXPRESSION_REGISTERS",
			pd->material_name
			);
		//SetMaterialFlag( MF_DEFAULTED );
		return ERR_BUFFER_TOO_SMALL;
	}
	pd->registerIsTemporary[pd->numRegisters] = true;
	pd->numRegisters++;
	result_ = pd->numRegisters - 1;
	return ALL_OK;
}

/*
=============
idMaterial::GetExpressionOp
=============
*/
ERet allocExpressionOp(
					 expOp_t *&result_
					 , mtrParsingData_t * pd
					 , Lexer & lexer
					 )
{
	if ( pd->numOps == MAX_EXPRESSION_OPS ) {
		lexer.Warning(
			"GetExpressionOp: material '%s' hit MAX_EXPRESSION_OPS",
			pd->material_name
			);
		//SetMaterialFlag( MF_DEFAULTED );
		return ERR_BUFFER_TOO_SMALL;
	}

	result_ = &pd->shaderOps[pd->numOps++];
	return ALL_OK;
}

ERet emitOp(
			Rendering::RegisterIndex_t &result_
			, const Rendering::RegisterIndex_t lhs
			, const Rendering::RegisterIndex_t rhs
			, const expOpType_t op
			, mtrParsingData_t *pd
			, Lexer & lexer
			);

ERet emitTableLookupOp(
					   Rendering::RegisterIndex_t &result_
					   , const AssetID& table_id
					   , const Rendering::RegisterIndex_t index_expr
					   , const expOpType_t op
					   , mtrParsingData_t *pd
					   , Lexer & lexer
					   );

ERet parseAtom(
			   Rendering::RegisterIndex_t &result_
			   , mtrParsingData_t *pd	// only used during parsing
			   , Lexer & lexer
			   );

// *, /, % .
ERet parseFactors(
				  Rendering::RegisterIndex_t &result_
				  , mtrParsingData_t *pd
				  , Lexer & lexer
				  )
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseAtom( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));

		// E -> E * E
		if ( token == '*' ) {
			SkipToken( lexer );
			mxDO(parseAtom( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_MULTIPLY, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E / E
		if ( token == '/' ) {
			SkipToken( lexer );
			mxDO(parseAtom( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_DIVIDE, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E % E
		if ( token == '%' ) {
			SkipToken( lexer );
			mxDO(parseAtom( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_MOD, pd, lexer ));
			L = E;
			continue;
		}
		break;		 
	}

	result_ = L;
	return ALL_OK;
}

// +, - .
ERet parseTerms(
				Rendering::RegisterIndex_t &result_
				, mtrParsingData_t *pd
				, Lexer & lexer
				)
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseFactors( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));
		
		// E -> E + E
		if ( token == '+' ) {
			SkipToken( lexer );
			mxDO(parseFactors( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_ADD, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E - E
		if ( token == '-' ) {
			SkipToken( lexer );
			mxDO(parseFactors( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_SUBTRACT, pd, lexer ));
			L = E;
			continue;
		}
		break;		 
	}
		
	result_ = L;
	return ALL_OK;
}

// >, >=, <, <= .
ERet parseRelational(
					 Rendering::RegisterIndex_t &result_
					 , mtrParsingData_t *pd
					 , Lexer & lexer
					 )
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseTerms( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));
		
		// E -> E < E
		if ( token == '<' ) {
			SkipToken( lexer );
			mxDO(parseTerms( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_LT, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E <= E
		if ( token == "<=" ) {
			SkipToken( lexer );
			mxDO(parseTerms( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_LE, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E > E
		if ( token == '>' ) {
			SkipToken( lexer );
			mxDO(parseTerms( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_GT, pd, lexer ));
			L = E;
			continue;
		}
		// E -> E >= E
		if ( token == ">=" ) {
			SkipToken( lexer );
			mxDO(parseTerms( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_GE, pd, lexer ));
			L = E;
			continue;
		}
		break;
	}

	result_ = L;
	return ALL_OK;
}

// ==, != .
ERet parseEquality(
				   Rendering::RegisterIndex_t &result_
				   , mtrParsingData_t *pd
				   , Lexer & lexer
				   )
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseRelational( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));

		// E -> E == E
		if ( token == "==" ) {
			SkipToken( lexer );
			mxDO(parseRelational( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_EQ, pd, lexer ));
			L = E;
			continue;
		}

		// E -> E != E
		if ( token == "!=" ) {
			SkipToken( lexer );
			mxDO(parseRelational( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_NE, pd, lexer ));
			L = E;
			continue;
		}

		break;
	}

	result_ = L;
	return ALL_OK;
}

// && .
ERet parseLogicalAnd(
				   Rendering::RegisterIndex_t &result_
				   , mtrParsingData_t *pd
				   , Lexer & lexer
				   )
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseEquality( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));

		if ( token == "&&" ) {
			SkipToken( lexer );
			mxDO(parseEquality( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_AND, pd, lexer ));
			L = E;
			continue;
		}

		break;
	}

	result_ = L;
	return ALL_OK;
}

// || .
ERet parseLogicalOr(
					Rendering::RegisterIndex_t &result_
					, mtrParsingData_t *pd
					, Lexer & lexer
					)
{
	Rendering::RegisterIndex_t	L, R, E;

	mxDO(parseLogicalAnd( L, pd, lexer ));

	while ( true )
	{
		TbToken	token;
		mxDO(peekNextToken( token, lexer ));

		if ( token == "||" ) {
			SkipToken( lexer );
			mxDO(parseLogicalAnd( R, pd, lexer ));
			mxDO(emitOp( E, L, R, OP_TYPE_OR, pd, lexer ));
			L = E;
			continue;
		}

		break;
	}

	result_ = L;
	return ALL_OK;
}

ERet parseExpression_R(
	Rendering::RegisterIndex_t &result_
	, mtrParsingData_t *pd	// only used during parsing
	, Lexer & lexer
	)
{
	mxDO(parseLogicalOr(result_, pd, lexer));
	return ALL_OK;
}

/*
=================
idMaterial::ParseExpression

Returns a register index
=================
*/
ERet ParseExpression(
	Rendering::RegisterIndex_t &result_
	, mtrParsingData_t * pd	// only used during parsing
	, Lexer & lexer
	)
{
	mxDO(parseExpression_R( result_, pd, lexer ));
	return ALL_OK;
}

ERet parseAtom(
	Rendering::RegisterIndex_t &result_
	, mtrParsingData_t *pd	// only used during parsing
	, Lexer & lexer
	)
{
	TbToken token;
	mxDO(expectAnyToken( token, lexer ));

	//
	Rendering::RegisterIndex_t	lhs;

	if ( token == "(" ) {
		mxDO(ParseExpression( lhs, pd, lexer ));
		mxDO(expectTokenString( lexer, ")" ));
		result_ = lhs;
		return ALL_OK;
	}

	if ( streqi( token, "time" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_TIME;
		return ALL_OK;
	}
	if ( streqi( token, "parm0" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM0;
		return ALL_OK;
	}
	if ( streqi( token, "parm1" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM1;
		return ALL_OK;
	}
	if ( streqi( token, "parm2" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM2;
		return ALL_OK;
	}
	if ( streqi( token, "parm3" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM3;
		return ALL_OK;
	}
	if ( streqi( token, "parm4" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM4;
		return ALL_OK;
	}
	if ( streqi( token, "parm5" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM5;
		return ALL_OK;
	}
	if ( streqi( token, "parm6" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM6;
		return ALL_OK;
	}
	if ( streqi( token, "parm7" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM7;
		return ALL_OK;
	}
	if ( streqi( token, "parm8" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM8;
		return ALL_OK;
	}
	if ( streqi( token, "parm9" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM9;
		return ALL_OK;
	}
	if ( streqi( token, "parm10" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM10;
		return ALL_OK;
	}
	if ( streqi( token, "parm11" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_PARM11;
		return ALL_OK;
	}

	//
	if ( streqi( token, "global0" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL0;
		return ALL_OK;
	}
	if ( streqi( token, "global1" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL1;
		return ALL_OK;
	}
	if ( streqi( token, "global2" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL2;
		return ALL_OK;
	}
	if ( streqi( token, "global3" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL3;
		return ALL_OK;
	}
	if ( streqi( token, "global4" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL4;
		return ALL_OK;
	}
	if ( streqi( token, "global5" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL5;
		return ALL_OK;
	}
	if ( streqi( token, "global6" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL6;
		return ALL_OK;
	}
	if ( streqi( token, "global7" ) ) {
		pd->registersAreConstant = false;
		result_ = EXP_REG_GLOBAL7;
		return ALL_OK;
	}
	if ( streqi( token, "fragmentPrograms" ) ) {
		UNDONE;
		//return 1.0f;
	}

	if ( streqi( token, "sound" ) ) {
		pd->registersAreConstant = false;
		return emitOp( result_, 0, 0, OP_TYPE_SOUND, pd, lexer );
	}

	// parse negative numbers
	if ( token == "-" ) {
		mxDO(expectAnyToken( token, lexer ));
		if ( token.type == TT_Number || token == "." ) {
			return allocConstantRegister( result_, -(float) token.GetFloatValue(), pd, lexer );
		}
		lexer.Error( "Bad negative number '%s'", token.c_str() );
		//SetMaterialFlag( MF_DEFAULTED );
		return ERR_FAILED_TO_PARSE_DATA;
	}

	if ( token.type == TT_Number || token == "." || token == "-" ) {
		return allocConstantRegister( result_, (float) token.GetFloatValue(), pd, lexer );
	}

	//
	const AssetID table_id = Doom3::PathToAssetID( token.c_str() );

	// see if it is a table name
	if( !tableExists( table_id ) ) {
		lexer.Warning("Couldn't find table '%s'!", AssetId_ToChars(table_id));
	}

	// parse rhs table expression
	mxDO(expectTokenString( lexer, "[" ));

	Rendering::RegisterIndex_t	rhs;
	mxDO(ParseExpression( rhs, pd, lexer ));

	mxDO(expectTokenString( lexer, "]" ));

	//
	mxDO(emitTableLookupOp(
		result_
		, table_id, rhs
		, OP_TYPE_TABLE
		, pd
		, lexer
		));

	return ALL_OK;
}

ERet emitTableLookupOp(
					   Rendering::RegisterIndex_t &result_
					   , const AssetID& table_id
					   , const Rendering::RegisterIndex_t index_expr
					   , const expOpType_t op
					   , mtrParsingData_t *pd
					   , Lexer & lexer
					   )
{
	const U32 table_op_index = pd->numOps;

	Rendering::RegisterIndex_t	table_index = ~0;	// will be patched later

	mxDO(emitOp(
		result_
		, table_index, index_expr
		, OP_TYPE_TABLE
		, pd
		, lexer
		));

	//
	idMtrTableRef	table_ref;
	{
		table_ref.table_id = table_id;
		table_ref.op_index = table_op_index;
	}
	mxDO(pd->table_refs.add( table_ref ));

	return ALL_OK;
}

ERet emitOp(
	Rendering::RegisterIndex_t &result_
	, const Rendering::RegisterIndex_t lhs
	, const Rendering::RegisterIndex_t rhs
	, const expOpType_t op
	, mtrParsingData_t *pd
	, Lexer & lexer
	)
{
	// optimize away identity operations
	if ( op == OP_TYPE_ADD ) {
		if ( !pd->registerIsTemporary[lhs] && pd->shaderRegisters[lhs] == 0 ) {
			result_ = rhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[rhs] && pd->shaderRegisters[rhs] == 0 ) {
			result_ = lhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[lhs] && !pd->registerIsTemporary[rhs] ) {
			return allocConstantRegister( result_, pd->shaderRegisters[lhs] + pd->shaderRegisters[rhs], pd, lexer );
		}
	}
	if ( op == OP_TYPE_MULTIPLY ) {
		if ( !pd->registerIsTemporary[lhs] && pd->shaderRegisters[lhs] == 1 ) {
			result_ = rhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[lhs] && pd->shaderRegisters[lhs] == 0 ) {
			result_ = lhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[rhs] && pd->shaderRegisters[rhs] == 1 ) {
			result_ = lhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[rhs] && pd->shaderRegisters[rhs] == 0 ) {
			result_ = rhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[lhs] && !pd->registerIsTemporary[rhs] ) {
			return allocConstantRegister( result_, pd->shaderRegisters[lhs] * pd->shaderRegisters[rhs], pd, lexer );
		}
	}

#if FOLD_CONSTANT_DIVISIONS
	if ( op == OP_TYPE_DIVIDE ) {
		if ( !pd->registerIsTemporary[lhs] && pd->shaderRegisters[lhs] == 0 ) {
			result_ = lhs;
			return ALL_OK;
		}

		if ( !pd->registerIsTemporary[rhs] && pd->shaderRegisters[rhs] == 1 ) {
			result_ = lhs;
			return ALL_OK;
		}
		if ( !pd->registerIsTemporary[rhs] && pd->shaderRegisters[rhs] == 0 ) {
			lexer.Warning("Division by zero!");
			result_ = rhs;
			return ERR_DIVISION_BY_ZERO;
		}

		if ( !pd->registerIsTemporary[lhs] && !pd->registerIsTemporary[rhs] ) {
			return allocConstantRegister( result_, pd->shaderRegisters[lhs] / pd->shaderRegisters[rhs], pd, lexer );
		}
	}
#endif // FOLD_CONSTANT_DIVISIONS


//
#if DBG_PRINT_EXPR_OPS
const U32 newOpIdx = pd->numOps;
#endif

	//
	expOp_t	*expOp;
	mxDO(allocExpressionOp( expOp, pd, lexer ));

	expOp->opType = op;
	expOp->lhs = lhs;
	expOp->rhs = rhs;
	mxDO(allocExpressionTemporary( expOp->res, pd, lexer ));

//
#if DBG_PRINT_EXPR_OPS
expOp->dbgprint( newOpIdx );
#endif

	result_ = expOp->res;

	return ALL_OK;
}





static
ERet ExpressionToString_R(
						String &dst_str_
						, const OpIndex_t op_index
						, const Doom3::mtrParsingData_t& mtr_parsing_data
						);

static
ERet RegisterToString_R(
						String &dst_str_
						, const Rendering::RegisterIndex_t reg_index
						, const Doom3::mtrParsingData_t& mtr_parsing_data
						)
{
	if(mtr_parsing_data.registerIsTemporary[reg_index])
	{
		// This reg is stores the result of evaluating an expression.

		// Find that expression.


		//
		if(reg_index < EXP_REG_NUM_PREDEFINED)
		{
			switch(reg_index)
			{
			case EXP_REG_TIME:		dst_str_ = "TIME";	break;

			case EXP_REG_PARM0:		dst_str_ = "PARM0";	break;
			case EXP_REG_PARM1:		dst_str_ = "PARM1";	break;
			case EXP_REG_PARM2:		dst_str_ = "PARM2";	break;
			case EXP_REG_PARM3:		dst_str_ = "PARM3";	break;
			case EXP_REG_PARM4:		dst_str_ = "PARM4";	break;
			case EXP_REG_PARM5:		dst_str_ = "PARM5";	break;
			case EXP_REG_PARM6:		dst_str_ = "PARM6";	break;
			case EXP_REG_PARM7:		dst_str_ = "PARM7";	break;
			case EXP_REG_PARM8:		dst_str_ = "PARM8";	break;
			case EXP_REG_PARM9:		dst_str_ = "PARM9";	break;
			case EXP_REG_PARM10:	dst_str_ = "PARM10";	break;
			case EXP_REG_PARM11:	dst_str_ = "PARM11";	break;

			case EXP_REG_GLOBAL0:		dst_str_ = "GLOBAL0";	break;
			case EXP_REG_GLOBAL1:		dst_str_ = "GLOBAL1";	break;
			case EXP_REG_GLOBAL2:		dst_str_ = "GLOBAL2";	break;
			case EXP_REG_GLOBAL3:		dst_str_ = "GLOBAL3";	break;
			case EXP_REG_GLOBAL4:		dst_str_ = "GLOBAL4";	break;
			case EXP_REG_GLOBAL5:		dst_str_ = "GLOBAL5";	break;
			case EXP_REG_GLOBAL6:		dst_str_ = "GLOBAL6";	break;
			case EXP_REG_GLOBAL7:		dst_str_ = "GLOBAL7";	break;

				mxNO_SWITCH_DEFAULT;
			}
			return ALL_OK;
		}

		//
		for( OpIndex_t iOp = 0; iOp < mtr_parsing_data.numOps; iOp++ )
		{
			const expOp_t op = mtr_parsing_data.shaderOps[ iOp ];
			//
			if(op.res == reg_index)
			{
				return ExpressionToString_R(
					dst_str_
					, iOp
					, mtr_parsing_data
					);
			}
		}

		UNDONE;
		return ERR_OBJECT_NOT_FOUND;
	}
	else
	{
		// this reg stores a constant
		Str::Format(dst_str_, "%.2f", mtr_parsing_data.shaderRegisters[reg_index]);
		return ALL_OK;
	}
}

static
ERet ExpressionToString_R(
						String &dst_str_
						, const OpIndex_t op_index
						, const Doom3::mtrParsingData_t& mtr_parsing_data
						)
{
	expOp_t op = mtr_parsing_data.shaderOps[ op_index ];

	String64	lhs_str, rhs_str;

	switch(op.opType)
	{
	case OP_TYPE_ADD:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s + %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_SUBTRACT:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s - %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_MULTIPLY:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s * %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_DIVIDE:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s / %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_MOD:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s %% %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_GT:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s > %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_GE:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s >= %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_LT:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s < %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_LE:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s <= %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_EQ:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s == %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_NE:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s != %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_AND:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s && %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	case OP_TYPE_OR:
		mxTRY(RegisterToString_R( lhs_str, op.lhs, mtr_parsing_data ));
		mxTRY(RegisterToString_R( rhs_str, op.rhs, mtr_parsing_data ));
		Str::Format(dst_str_, "(%s || %s)", lhs_str.c_str(), rhs_str.c_str());
		break;

	default:
		UNDONE;
	}

	return ALL_OK;
}





/*
=================
idMaterial::ParseDecalInfo
=================
*/
ERet ParseDecalInfo(
	decalInfo_t * decalInfo
	, Lexer & lexer
	)
{
	float	stayTime, fadeTime;
	mxDO(expectFloat2( &stayTime, lexer ));
	mxDO(expectFloat2( &fadeTime, lexer ));

	decalInfo->stayTime = stayTime * 1000;
	decalInfo->fadeTime = fadeTime * 1000;

	float	start[4], end[4];
	Parse1DMatrix( lexer, 4, start );
	Parse1DMatrix( lexer, 4, end );

	for ( int i = 0 ; i < 4 ; i++ ) {
		decalInfo->start[i] = start[i];
		decalInfo->end[i] = end[i];
	}

	return ALL_OK;
}

mxSTOLEN("Doom 3 BFG Edition GPL Source Code, Image_program.cpp");


//
struct Image_t
{
	DynamicArray< BYTE >	data;
	U32						width;
	U32						height;
	AssetID					asset_id;
public:
	Image_t( AllocatorI& heap ) : data( heap )
	{
		width = height = 0;
	}

	bool isOk() const
	{
		return !data.IsEmpty()
			&& width > 0 && height > 0
			&& AssetId_IsValid(asset_id);
	}
};

/*

Anywhere that an image name is used (diffusemaps, bumpmaps, specularmaps, lights, etc),
an imageProgram can be specified.

This allows load time operations, like heightmap-to-normalmap conversion and image
composition, to be automatically handled in a way that supports timestamped reloads.

*/

/*
=================
R_HeightmapToNormalMap

it is not possible to convert a heightmap into a normal map
properly without knowing the texture coordinate stretching.
We can assume constant and equal ST vectors for walls, but not for characters.
=================
*/

//static void R_HeightmapToNormalMap( byte *src_data, int width, int height, float scale ) {
static ERet R_HeightmapToNormalMap(
								   const Image_t& image,
								   float scale,
								   Image_t &image_
								   )
{
	int		i, j;
	BYTE	*depth;

	scale = scale / 256;

	const int width = image.width;
	const int height = image.height;

	// copy and convert to grey scale
	j = image.width * image.height;
	depth = (BYTE *)tempMemHeap().Allocate( j, EFFICIENT_ALIGNMENT );

	const BYTE* src_data = image.data.raw();

	for ( i = 0 ; i < j ; i++ ) {
		depth[i] = ( src_data[i*4] + src_data[i*4+1] + src_data[i*4+2] ) / 3;
	}

	const U32 dst_data_size = image.width * image.height * sizeof(R8G8B8A8);
	mxDO(image_.data.setNum( dst_data_size ));
	image_.width = width;
	image_.height = height;

	//
	R8G8B8A8 *dst_data = (R8G8B8A8*) image_.data.raw();

	//
	V3f	dir, dir2;
	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width ; j++ ) {
			int		d1, d2, d3, d4;
			int		a1, a2, a3, a4;

			// FIXME: look at five points?

			// look at three points to estimate the gradient
			a1 = d1 = depth[ ( i * width + j ) ];
			a2 = d2 = depth[ ( i * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];
			a3 = d3 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + j ) ];
			a4 = d4 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];

			d2 -= d1;
			d3 -= d1;

			dir[0] = -d2 * scale;
			dir[1] = -d3 * scale;
			dir[2] = 1;
			dir.normalize();

			a1 -= a3;
			a4 -= a3;

			dir2[0] = -a4 * scale;
			dir2[1] = a1 * scale;
			dir2[2] = 1;
			dir2.normalize();
	
			dir += dir2;
			dir.normalize();

			R8G8B8A8 &dst_texel = dst_data[ i * width + j ];
			dst_texel.R = (BYTE)(dir[0] * 127 + 128);
			dst_texel.G = (BYTE)(dir[1] * 127 + 128);
			dst_texel.B = (BYTE)(dir[2] * 127 + 128);
			dst_texel.A = 255;
		}
	}

	tempMemHeap().Deallocate( depth );

	return ALL_OK;
}

ERet loadImage(
	Image_t &image_
	, const char* image_name
	, const Doom3AssetsConfig& doom3_cfg
	)
{
	String256	image_filepath;
	mxDO(Doom3::composeFilePath(
		image_filepath
		, image_name
		, doom3_cfg
		));

	if( Str::FindExtensionS( image_filepath.c_str() ) == NULL )
	{
		mxDO(Str::setFileExtension( image_filepath, "tga" ));
	}

	//
	NwBlob	imageDataBlob( tempMemHeap() );
	const ERet res = NwBlob_::loadBlobFromFile( imageDataBlob, image_filepath.c_str() );
	if( mxFAILED( res ) )
	{
		ptWARN("Failed to load image from '%s'!", image_filepath.c_str());
		return res;
	}

	if( AssetBaking::LogLevelEnabled( LL_Debug ) )
	{
		DBGOUT("Loaded image '%s'", image_filepath.c_str());
	}

	//
	const int num_required_color_components = STBI_rgb_alpha;

	int image_width, image_height;
	int bytes_per_pixel;
	unsigned char* raw_image_data = stbi_load_from_memory(
		(stbi_uc*) imageDataBlob.raw(), imageDataBlob.rawSize(),
		&image_width, &image_height, &bytes_per_pixel, num_required_color_components
		);
	mxENSURE(raw_image_data, ERR_FAILED_TO_PARSE_DATA, "Failed to load image from '%s'", image_filepath.c_str());

	//
	const int size_of_raw_image_data = (image_width * image_height) * num_required_color_components;

	mxDO(image_.data.setNum(size_of_raw_image_data));
	memcpy( image_.data.raw(), raw_image_data, size_of_raw_image_data );

	image_.width = image_width;
	image_.height = image_height;
	image_.asset_id = Doom3::PathToAssetID(image_name);

	stbi_image_free( raw_image_data );

	return ALL_OK;
}

// Image Program Functions:
// https://www.iddevnet.com/doom3/materials.html
// 
ERet parseImageProgram_R(
	Image_t &image_
	, Lexer & lexer
	, const Doom3AssetsConfig& doom3_cfg
	)
{
	String64 token;
	mxDO(expectName( lexer, token ));
//lexer.Message("read token: %s", token.c_str());

	// heightmap(<map>, <float>)	Turns a grayscale height map into a normal map. <float> varies the bumpiness
	// E.g.:
	// bumpmap	   heightmap(models/monsters/zombie/commando/teeth_b.tga , 6 )
	// 
	if( streqi( token, "heightmap" ) )
	{
		mxDO(expectChar( lexer, '(' ));

		Image_t	heightmap_image( tempMemHeap() );
		/*mxTRY*/(parseImageProgram_R( heightmap_image, lexer, doom3_cfg ));

		mxDO(expectChar( lexer, ',' ));

		// generate a normal map from the heightmap
		float	scale;
		mxDO(expectFloat2( &scale, lexer ));

		if( heightmap_image.isOk() )
		{
			mxDO(R_HeightmapToNormalMap( heightmap_image, scale, image_ ));
		}

		mxDO(expectChar( lexer, ')' ));

		// generate an id for the normal map
		if( heightmap_image.isOk() )
		{
			String64	normal_map_name;
			Str::Format(normal_map_name, "%s_%1.0f",
				AssetId_ToChars(heightmap_image.asset_id), scale
				);
			image_.asset_id = Doom3::PathToAssetID(normal_map_name.c_str());
		}

		return ALL_OK;
	}

	// addnormals(<map>, <map>)	Adds two normal maps together. Result is normalized.
	// E.g.:
	// bumpmap		addnormals( models/monsters/tentacle/dp_tentacle_local.tga, heightmap( models/monsters/tentacle/dp_tentacle_h.tga, 7 ) )
	//
	if( streqi( token, "addnormals" ) )
	{
		mxDO(expectChar( lexer, '(' ));

		UNDONE_PARTS_MARKER;
		// we simply take the first map and ignore the second one

		//
		/*mxTRY*/(parseImageProgram_R( image_, lexer, doom3_cfg ));

		//
		mxDO(expectChar( lexer, ',' ));

		//
		Image_t	imageB( tempMemHeap() );
		/*mxTRY*/(parseImageProgram_R( imageB, lexer, doom3_cfg ));

		//
		mxDO(expectChar( lexer, ')' ));

		return ALL_OK;
	}

	// load it as an image
	mxTRY(loadImage( image_, token.c_str(), doom3_cfg ));

	return ALL_OK;
}

// analogue of R_ParsePastImageProgram()
ERet parseImageProgram(
	AssetID &texture_id_
	, Lexer & lexer
	, const Doom3AssetsConfig& doom3_cfg
	, AssetDatabase & asset_database
	)
{
	Image_t	image( tempMemHeap() );

	const ERet res = parseImageProgram_R( image, lexer, doom3_cfg );
	if( mxFAILED(res) ) {
		lexer.Error("Failed to parse image program!");
		lexer.SkipRestOfLine();
		return res;
	}

//mxDO(dbg_saveAsTGA( image ));

	if( image.isOk() )
	{
		//
		texture_id_ = image.asset_id;

		//
		if( !asset_database.containsAssetWithId(
			image.asset_id
			, NwTexture::metaClass()
			) )
		{
			//
			CompiledAssetData	compiled_image( tempMemHeap() );

			{
				TextureImage_m	texture_image;
				mxZERO_OUT(texture_image);

				texture_image.data = image.data.raw();
				texture_image.size = image.data.rawSize();
				texture_image.width = image.width;
				texture_image.height = image.height;
				texture_image.depth = 1;
				texture_image.format = NwDataFormat::RGBA8;
				texture_image.numMips = 1;
				texture_image.arraySize = 1;
				texture_image.type = TEXTURE_2D;

				mxDO(::AssetBaking::TextureCompiler::compileTexture_RawR8G8B8A8(
					texture_image
					, NwBlobWriter(compiled_image.object_data)
					));
			}


			mxDO(asset_database.addOrUpdateGeneratedAsset(
				image.asset_id
				, NwTexture::metaClass()
				, compiled_image
				));
		}

		return ALL_OK;
	}

	// we failed to parse the image, but don't throw an error to continue parsing - we'll show the 'missing texture' image at runtime
	lexer.Warning("Failed to parse image program!");
	return ALL_OK;
}


/*
================
idMaterial::ParseBlend

blend <type>
blend <src>, <dst>
================
*/
ERet ParseBlend(
	shaderStage_t * stage
	, Lexer & lexer
	)
{
	String64 token;
	mxDO(expectName( lexer, token ));

	// blending combinations
	if ( streqi( token, "blend" ) )
	{
		// Alpha Blending:
		// vec4 result = vec4(gl_FragColor.a) * gl_FragColor + vec4(1.0 - gl_FragColor.a) * pixel_color;
		stage->drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		return ALL_OK;
	}

	if ( streqi( token, "add" ) )
	{
		// Additive Blending:
		// vec4 result = vec4(1.0) * gl_FragColor + vec4(1.0) * pixel_color;
		stage->drawStateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		return ALL_OK;
	}

	if ( streqi( token, "filter" ) || streqi( token, "modulate" ) )
	{
		stage->drawStateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		return ALL_OK;
	}

	if (  streqi( token, "none" ) )
	{
		// none is used when defining an alpha mask that doesn't draw
		stage->drawStateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
		return ALL_OK;
	}

	if ( streqi( token, "bumpmap" ) ) {
		stage->lighting = SL_BUMP;
		return ALL_OK;
	}
	if ( streqi( token, "diffusemap" ) ) {
		stage->lighting = SL_DIFFUSE;
		return ALL_OK;
	}
	if ( streqi( token, "specularmap" ) ) {
		stage->lighting = SL_SPECULAR;
		return ALL_OK;
	}

	// 
	mxDO(expectTokenChar( lexer, ',' ));

	//
	String64 dstBlend;
	mxDO(expectName( lexer, dstBlend ));

	UNDONE_PARTS_MARKER;

	return ALL_OK;
}

Rendering::RegisterIndex_t EmitOp(
					   Rendering::RegisterIndex_t a, Rendering::RegisterIndex_t b, expOpType_t opType
					   , mtrParsingData_t *pd
					   , Lexer & lexer
					   )
{
	Rendering::RegisterIndex_t	result = 0;

	ERet ret = emitOp(
		result
		, a
		, b
		, opType
		, pd
		, lexer
		);
	mxASSERT(mxSUCCEDED(ret));

	return result;
}

/*
===============
idMaterial::MultiplyTextureMatrix
===============
*/
static
void MultiplyTextureMatrix( textureStage_t *ts
						   , TextureMatrix registers
						   , mtrParsingData_t *pd
						   , Lexer & lexer )
{
	TextureMatrix		old;

	if ( !ts->hasMatrix ) {
		ts->hasMatrix = true;
		memcpy( ts->texture_matrix, registers, sizeof( ts->texture_matrix ) );
		return;
	}

	memcpy( old, ts->texture_matrix, sizeof( old ) );

	// multiply the two matrices
	ts->texture_matrix[0][0] = EmitOp(
		EmitOp( old[0][0], registers[0][0], OP_TYPE_MULTIPLY, pd, lexer ),
		EmitOp( old[0][1], registers[1][0], OP_TYPE_MULTIPLY, pd, lexer ),
		OP_TYPE_ADD, pd, lexer
		);
	ts->texture_matrix[0][1] = EmitOp(
		EmitOp( old[0][0], registers[0][1], OP_TYPE_MULTIPLY, pd, lexer ),
		EmitOp( old[0][1], registers[1][1], OP_TYPE_MULTIPLY, pd, lexer ),
		OP_TYPE_ADD, pd, lexer
		);
	ts->texture_matrix[0][2] = EmitOp( 
		EmitOp(
			EmitOp( old[0][0], registers[0][2], OP_TYPE_MULTIPLY, pd, lexer ),
			EmitOp( old[0][1], registers[1][2], OP_TYPE_MULTIPLY, pd, lexer ),
			OP_TYPE_ADD, pd, lexer
		),
		old[0][2],
		OP_TYPE_ADD, pd, lexer
		);

	ts->texture_matrix[1][0] = EmitOp(
		EmitOp( old[1][0], registers[0][0], OP_TYPE_MULTIPLY, pd, lexer ),
		EmitOp( old[1][1], registers[1][0], OP_TYPE_MULTIPLY, pd, lexer ),
		OP_TYPE_ADD, pd, lexer
		);
	ts->texture_matrix[1][1] = EmitOp(
		EmitOp( old[1][0], registers[0][1], OP_TYPE_MULTIPLY, pd, lexer ),
		EmitOp( old[1][1], registers[1][1], OP_TYPE_MULTIPLY, pd, lexer ),
		OP_TYPE_ADD, pd, lexer
		);
	ts->texture_matrix[1][2] = EmitOp( 
		EmitOp(
			EmitOp( old[1][0], registers[0][2], OP_TYPE_MULTIPLY, pd, lexer ),
			EmitOp( old[1][1], registers[1][2], OP_TYPE_MULTIPLY, pd, lexer ),
			OP_TYPE_ADD, pd, lexer
		),
		old[1][2],
		OP_TYPE_ADD, pd, lexer
		);
}

/*
===============
idMaterial::ClearStage
===============
*/
static
void ClearStage( shaderStage_t *ss
				, mtrParsingData_t * pd
				, Lexer & lexer )
{
	//ss->drawStateBits = 0;

	ss->conditionRegister = GetExpressionConstant( 1, pd, lexer );
	ss->condition_op_index = -1;

	ss->color.registers[0] =
	ss->color.registers[1] =
	ss->color.registers[2] =
	ss->color.registers[3] = GetExpressionConstant( 1, pd, lexer );
}

static
ERet CreateStage(
				 shaderStage_t **ss_
				 , Lexer & lexer
				 , mtrParsingData_t * pd	// only used during parsing
				 )
{
	if ( pd->numStages >= mxCOUNT_OF(pd->parseStages) ) {
		UNDONE;
		//SetMaterialFlag( MF_DEFAULTED );
		//lexer.Warning( "material '%s' exceeded %i stages", GetName(), MAX_SHADER_STAGES );
		return ERR_BUFFER_TOO_SMALL;
	}

	*ss_ = &pd->parseStages[ pd->numStages++ ];
	ClearStage(*ss_, pd, lexer);

	return ALL_OK;
}

/*
=================
idMaterial::ParseStage

An open brace has been parsed


{
	if <expression>
	map <imageprogram>
	"nearest" "linear" "clamp" "zeroclamp" "uncompressed" "highquality" "nopicmip"
	scroll, scale, rotate
}

=================
*/
ERet ParseStage(
	Lexer & lexer

	, const textureRepeat_t* trpDefault

	, mtrParsingData_t * pd	// only used during parsing

	, const Doom3AssetsConfig& doom3_cfg
	, AssetDatabase & asset_database
	)
{
	shaderStage_t		*ss;
	mxDO(CreateStage(&ss, lexer, pd));

	textureStage_t		*ts = &ss->texture;

	//
	TextureMatrix		texture_matrix;

	newShaderStage_t	newStage;

	//
	ClearStage( ss, pd, lexer );

	//
	mxLOOP_FOREVER
	{
		TbToken	token;
		if( !lexer.ReadToken( token ) ) {
			return ERR_EOF_REACHED;
		}

		// the close brace for the entire material ends the draw block
		if( token == '}' ) {
			break;
		}

		// image options
		if ( streqi( token, "blend" ) )
		{
			mxDO(ParseBlend(
				ss
				, lexer
				));
			continue;
		}

		// map <map>	The image program to use for this stage
		if ( streqi( token, "map" ) )
		{
			const ERet res = parseImageProgram(
				ss->texture.texture_id
				, lexer
				, doom3_cfg
				, asset_database
				);
			if(mxFAILED(res)) {
				ss->should_be_removed = true;
			}
			continue;
		}

		if ( streqi( token, "cubeMap" ) ) {
			UNDONE_PARTS_MARKER;
			lexer.SkipRestOfLine();
			//str = R_ParsePastImageProgram( src );
			//idStr::Copynz( imageName, str, sizeof( imageName ) );
			//cubeMap = CF_NATIVE;
			continue;
		}

		if ( streqi( token, "cameraCubeMap" ) ) {
			UNDONE_PARTS_MARKER;
			lexer.SkipRestOfLine();
			//str = R_ParsePastImageProgram( src );
			//idStr::Copynz( imageName, str, sizeof( imageName ) );
			//cubeMap = CF_CAMERA;
			continue;
		}

		if ( streqi( token, "ignoreAlphaTest" ) ) {
			ss->ignoreAlphaTest = true;
			continue;
		}
		if ( streqi( token, "nearest" ) ) {
			UNDONE_PARTS_MARKER;
			//tf = TF_NEAREST;
			continue;
		}
		if ( streqi( token, "linear" ) ) {
			UNDONE_PARTS_MARKER;
			//tf = TF_LINEAR;
			continue;
		}
		if ( streqi( token, "clamp" ) ) {
			UNDONE_PARTS_MARKER;
			//trp = TR_CLAMP;
			continue;
		}
		if ( streqi( token, "noclamp" ) ) {
			UNDONE_PARTS_MARKER;
			//trp = TR_REPEAT;
			continue;
		}
		if ( streqi( token, "zeroclamp" ) ) {
			UNDONE_PARTS_MARKER;
			//trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		if ( streqi( token, "alphazeroclamp" ) ) {
			UNDONE_PARTS_MARKER;
			//trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		if ( streqi( token, "forceHighQuality" ) ) {
			continue;
		}
		if ( streqi( token, "highquality" ) ) {
			continue;
		}
		if ( streqi( token, "uncompressed" ) ) {
			continue;
		}
		if ( streqi( token, "nopicmip" ) ) {
			continue;
		}
		if ( streqi( token, "vertexColor" ) ) {
			ss->vertexColor = SVC_MODULATE;
			continue;
		}
		if ( streqi( token, "inverseVertexColor" ) ) {
			ss->vertexColor = SVC_INVERSE_MODULATE;
			continue;
		}

		// privatePolygonOffset
		else if ( streqi( token, "privatePolygonOffset" ) ) {
			UNDONE_PARTS_MARKER;
			lexer.SkipRestOfLine();
			//if ( !src.ReadTokenOnLine( &token ) ) {
			//	ss->privatePolygonOffset = 1;
			//	continue;
			//}
			//// explicitly larger (or negative) offset
			//src.UnreadToken( &token );
			//ss->privatePolygonOffset = src.ParseFloat();
			continue;
		}



#pragma region Texture Coordinates Transform

		// texture coordinate generation
		// Type is one of: normal, reflect, skybox, wobbleSky <exp> <exp> <exp>
		if ( streqi( token, "texGen" ) )
		{
			mxDO(expectAnyToken( token, lexer ));

			UNDONE_PARTS_MARKER;
			/*
			if ( streqi( token, "normal" ) ) {
				ts->texgen = TG_DIFFUSE_CUBE;
			} else if ( streqi( token, "reflect" ) ) {
				ts->texgen = TG_REFLECT_CUBE;
			} else if ( streqi( token, "skybox" ) ) {
				ts->texgen = TG_SKYBOX_CUBE;
			} else if ( streqi( token, "wobbleSky" ) ) {
				ts->texgen = TG_WOBBLESKY_CUBE;
				texGenRegisters[0] = ParseExpression( src );
				texGenRegisters[1] = ParseExpression( src );
				texGenRegisters[2] = ParseExpression( src );
			} else {
				lexer.Warning( "bad texGen '%s' in material %s", token.c_str(), GetName() );
				SetMaterialFlag( MF_DEFAULTED );
			}
			*/
			continue;
		}

		// Scroll the texture coordinates
		// scroll <exp>, <exp>
		// translate <exp>, <exp>
		if ( streqi( token, "scroll" ) || streqi( token, "translate" ) )
		{
			Rendering::RegisterIndex_t	dU, dV;
			mxDO(ParseExpression( dU, pd, lexer ));
			mxDO(expectTokenChar( lexer, ',' ));
			mxDO(ParseExpression( dV, pd, lexer ));

			// row 0
			mxDO(allocConstantRegister( texture_matrix[0][0], 1, pd, lexer ));
			mxDO(allocConstantRegister( texture_matrix[0][1], 0, pd, lexer ));
			texture_matrix[0][2] = dU;

			// row 1
			mxDO(allocConstantRegister( texture_matrix[1][0], 0, pd, lexer ));
			mxDO(allocConstantRegister( texture_matrix[1][1], 1, pd, lexer ));
			texture_matrix[1][2] = dV;

			MultiplyTextureMatrix( ts, texture_matrix, pd, lexer );
			continue;
		}

		// Just scales without a centering
		// scale <exp>, <exp>
		if ( streqi( token, "scale" ) )
		{
			Rendering::RegisterIndex_t	a, b;
			mxDO(ParseExpression( a, pd, lexer ));
			mxDO(expectTokenChar( lexer, ',' ));
			mxDO(ParseExpression( b, pd, lexer ));

			// this just scales without a centering
			texture_matrix[0][0] = a;
			texture_matrix[0][1] = GetExpressionConstant( 0, pd, lexer );
			texture_matrix[0][2] = GetExpressionConstant( 0, pd, lexer );

			texture_matrix[1][0] = GetExpressionConstant( 0, pd, lexer );
			texture_matrix[1][1] = b;
			texture_matrix[1][2] = GetExpressionConstant( 0, pd, lexer );

			MultiplyTextureMatrix( ts, texture_matrix, pd, lexer );
			continue;
		}
		if ( streqi( token, "centerScale" ) ) {
			Rendering::RegisterIndex_t	a, b;
			mxDO(ParseExpression( a, pd, lexer ));
			mxDO(expectTokenChar( lexer, ',' ));
			mxDO(ParseExpression( b, pd, lexer ));
			// this subtracts 0.5, then scales, then adds 0.5
			texture_matrix[0][0] = a;
			texture_matrix[0][1] = GetExpressionConstant( 0, pd, lexer );
			texture_matrix[0][2] = EmitOp( GetExpressionConstant( 0.5, pd, lexer ), EmitOp( GetExpressionConstant( 0.5, pd, lexer ), a, OP_TYPE_MULTIPLY, pd, lexer ), OP_TYPE_SUBTRACT, pd, lexer );
			texture_matrix[1][0] = GetExpressionConstant( 0, pd, lexer );
			texture_matrix[1][1] = b;
			texture_matrix[1][2] = EmitOp( GetExpressionConstant( 0.5, pd, lexer ), EmitOp( GetExpressionConstant( 0.5, pd, lexer ), b, OP_TYPE_MULTIPLY, pd, lexer ), OP_TYPE_SUBTRACT, pd, lexer );

			MultiplyTextureMatrix( ts, texture_matrix, pd, lexer );
			continue;
		}
		if ( streqi( token, "shear" ) ) {
			Rendering::RegisterIndex_t	a, b;
			mxDO(ParseExpression( a, pd, lexer ));
			mxDO(expectTokenChar( lexer, ',' ));
			mxDO(ParseExpression( b, pd, lexer ));
			// this subtracts 0.5, then shears, then adds 0.5
			texture_matrix[0][0] = GetExpressionConstant( 1, pd, lexer );
			texture_matrix[0][1] = a;
			texture_matrix[0][2] = EmitOp( GetExpressionConstant( -0.5, pd, lexer ), a, OP_TYPE_MULTIPLY, pd, lexer );
			texture_matrix[1][0] = b;
			texture_matrix[1][1] = GetExpressionConstant( 1, pd, lexer );
			texture_matrix[1][2] = EmitOp( GetExpressionConstant( -0.5, pd, lexer ), b, OP_TYPE_MULTIPLY, pd, lexer );

			MultiplyTextureMatrix( ts, texture_matrix, pd, lexer );
			continue;
		}
		if ( streqi( token, "rotate" ) )
		{
			// in cycles
			Rendering::RegisterIndex_t	a;
			mxDO(ParseExpression( a, pd, lexer ));

			//
			const AssetID sin_table_id = Doom3::PathToAssetID( "sinTable" );
			const AssetID cos_table_id = Doom3::PathToAssetID( "cosTable" );

			Rendering::RegisterIndex_t		sinReg, cosReg;

			//const idDeclTable *table;
			//table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, "sinTable", false ) );
			//if ( !table ) {
			//	lexer.Warning( "no sinTable for rotate defined" );
			//	SetMaterialFlag( MF_DEFAULTED );
			//	return;
			//}
			if( !tableExists( sin_table_id ) ) {
				lexer.Warning( "no %s for rotate defined", AssetId_ToChars(sin_table_id) );
			}
			//sinReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );
			mxDO(emitTableLookupOp(
				sinReg
				, sin_table_id, a
				, OP_TYPE_TABLE
				, pd
				, lexer
				));

			//table = static_cast<const idDeclTable *>( declManager->FindType( DECL_TABLE, "cosTable", false ) );
			//if ( !table ) {
			//	lexer.Warning( "no cosTable for rotate defined" );
			//	SetMaterialFlag( MF_DEFAULTED );
			//	return;
			//}
			if( !tableExists( cos_table_id ) ) {
				lexer.Warning( "no %s for rotate defined", AssetId_ToChars(cos_table_id ) );
			}
			//cosReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );
			mxDO(emitTableLookupOp(
				cosReg
				, cos_table_id, a
				, OP_TYPE_TABLE
				, pd
				, lexer
				));

			// this subtracts 0.5, then rotates, then adds 0.5
			texture_matrix[0][0] = cosReg;
			texture_matrix[0][1] = EmitOp( GetExpressionConstant( 0, pd, lexer ), sinReg, OP_TYPE_SUBTRACT, pd, lexer );
			texture_matrix[0][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5, pd, lexer ), cosReg, OP_TYPE_MULTIPLY, pd, lexer ), 
				EmitOp( GetExpressionConstant( 0.5, pd, lexer ), sinReg, OP_TYPE_MULTIPLY, pd, lexer ), OP_TYPE_ADD, pd, lexer ),
				GetExpressionConstant( 0.5, pd, lexer ), OP_TYPE_ADD, pd, lexer );

			texture_matrix[1][0] = sinReg;
			texture_matrix[1][1] = cosReg;
			texture_matrix[1][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5, pd, lexer ), sinReg, OP_TYPE_MULTIPLY, pd, lexer ), 
				EmitOp( GetExpressionConstant( -0.5, pd, lexer ), cosReg, OP_TYPE_MULTIPLY, pd, lexer ), OP_TYPE_ADD, pd, lexer ),
				GetExpressionConstant( 0.5, pd, lexer ), OP_TYPE_ADD, pd, lexer );

			MultiplyTextureMatrix( ts, texture_matrix, pd, lexer );
			continue;
		}

#pragma endregion


#pragma region Color mask options

		if ( streqi( token, "maskRed" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_REDMASK;
			continue;
		}		
		if ( streqi( token, "maskGreen" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_GREENMASK;
			continue;
		}		
		if ( streqi( token, "maskBlue" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_BLUEMASK;
			continue;
		}		
		if ( streqi( token, "maskAlpha" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_ALPHAMASK;
			continue;
		}		
		if ( streqi( token, "maskColor" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_COLORMASK;
			continue;
		}		
		if ( streqi( token, "maskDepth" ) ) {
			UNDONE_PARTS_MARKER;
			//ss->drawStateBits |= GLS_DEPTHMASK;
			continue;
		}		
		if ( streqi( token, "alphaTest" ) ) {
			ss->hasAlphaTest = true;
			mxDO(ParseExpression(
				ss->alphaTestRegister
				, pd
				, lexer
				));
			UNDONE_PARTS_MARKER;
			//coverage = MC_PERFORATED;
			continue;
		}

#pragma endregion


#pragma region Color channels
		// shorthand for 2D modulated
		if ( streqi( token, "colored" ) ) {
			ss->color.registers[0] = EXP_REG_PARM0;
			ss->color.registers[1] = EXP_REG_PARM1;
			ss->color.registers[2] = EXP_REG_PARM2;
			ss->color.registers[3] = EXP_REG_PARM3;
			pd->registersAreConstant = false;
			continue;
		}

		if ( streqi( token, "color" ) )
		{
			mxDO(ParseExpression(
				ss->color.registers[0]
				, pd
				, lexer
				));

			mxDO(expectTokenChar( lexer, ',' ));

			mxDO(ParseExpression(
				ss->color.registers[1]
				, pd
				, lexer
				));

			mxDO(expectTokenChar( lexer, ',' ));

			mxDO(ParseExpression(
				ss->color.registers[2]
				, pd
				, lexer
				));

			mxDO(expectTokenChar( lexer, ',' ));

			mxDO(ParseExpression(
				ss->color.registers[3]
				, pd
				, lexer
				));
			continue;
		}
		if ( streqi( token, "red" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));
			ss->color.registers[0] = regIdx;
			continue;
		}
		if ( streqi( token, "green" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));
			ss->color.registers[1] = regIdx;
			continue;
		}
		if ( streqi( token, "blue" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));
			ss->color.registers[2] = regIdx;
			continue;
		}
		if ( streqi( token, "alpha" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));
			ss->color.registers[3] = regIdx;
			continue;
		}
		if ( streqi( token, "rgb" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));

			ss->color.registers[0] = ss->color.registers[1] = 
				ss->color.registers[2] = regIdx;
			continue;
		}
		if ( streqi( token, "rgba" ) ) {
			Rendering::RegisterIndex_t	regIdx;
			mxDO(ParseExpression(
				regIdx
				, pd
				, lexer
				));
			ss->color.registers[0] = ss->color.registers[1] = 
				ss->color.registers[2] = ss->color.registers[3] = regIdx;
			continue;
		}
#pragma endregion



		// Conditionally disable stages
		// if <exp>
		if ( streqi( token, "if" ) )
		{
			const OpIndex_t cond_expr_op_index = pd->numOps;

			mxDO(ParseExpression(
				ss->conditionRegister
				, pd
				, lexer
				));
			mxASSERT(pd->numOps >= cond_expr_op_index);

			mxASSERT(ss->condition_op_index == -1);
			ss->condition_op_index = cond_expr_op_index;

			//DBGOUT("Cond expr op = %d", cond_expr_op_index);

			mxDO(RegisterToString_R(ss->condition_expr_string, ss->conditionRegister, *pd));
			//DBGOUT("Cond expr: '%s'", ss->condition_expr_string.c_str());

			continue;
		}



#pragma region Shader Programs

		// program <prog>
		// Shortcut for
		// fragmentProgram <prog>
		// vertexProgram <prog>

		// vertexProgram <prog>	Use an ARB vertex program with this stage
		if ( streqi( token, "program" ) ) {
			String64	program_name;
			mxDO(expectName( lexer, program_name ));
			newStage.vertexProgram = Doom3::PathToAssetID( token.c_str() );
			newStage.fragmentProgram = Doom3::PathToAssetID( token.c_str() );
			continue;
		}
		if ( streqi( token, "fragmentProgram" ) ) {
			String64	program_name;
			mxDO(expectName( lexer, program_name ));
			newStage.fragmentProgram = Doom3::PathToAssetID( token.c_str() );
			continue;
		}
		if ( streqi( token, "vertexProgram" ) ) {
			String64	program_name;
			mxDO(expectName( lexer, program_name ));
			newStage.vertexProgram = Doom3::PathToAssetID( token.c_str() );
			continue;
		}

		if ( streqi( token, "vertexParm2" ) ) {
			mxUNDONE;
			//ParseVertexParm2( src, &newStage );
			lexer.SkipRestOfLine();
			continue;
		}

		if ( streqi( token, "vertexParm" ) ) {
			mxUNDONE;
			//ParseVertexParm( src, &newStage );
			lexer.SkipRestOfLine();
			continue;
		}

		if (  streqi( token, "fragmentMap" ) ) {	
			mxUNDONE;
			//ParseFragmentMap( src, &newStage );
			lexer.SkipRestOfLine();
			continue;
		}

#pragma endregion

		//
		lexer.Warning( "unknown general material parameter '%s' in '%s'",
			token.c_str(), lexer.GetFileName()
			);

		//
		mxASSERT(false);

//		materialFlags |= ( MF_DEFAULTED );
		return ERR_UNEXPECTED_TOKEN;
	}//LOOP

	// successfully parsed a stage
	return ALL_OK;
}


//////////////////////////////////////////////////////////////////////////

namespace AssetBaking
{

///
static
ERet dbg_saveAsTGA( const Image_t& image )
{
	mxASSERT(image.isOk());

	String256	tmp;
	Str::CopyS( tmp, AssetId_ToChars( image.asset_id ) );
	Str::ReplaceChar( tmp, '/', '_' );

	//
	String256	dumpfilename;
	Str::Format(
		dumpfilename,
		"R:/temp/%s",
		tmp.c_str()
		);
	if( Str::FindExtensionS(dumpfilename.c_str()) == nil ) {
		Str::setFileExtension(dumpfilename, "tga");
	}

	int res = stbi_write_tga(
		dumpfilename.c_str()
		, image.width
		, image.height
		, 4	// RGBA
		, image.data.raw()
		);

	mxENSURE( res != 0, ERR_FAILED_TO_WRITE_FILE,
		"Failed to save TGA to file '%s'",
		dumpfilename.c_str()
		);

	return ALL_OK;
}

/*
===============
idMaterial::ParseDeform
===============
*/
ERet ParseDeform(
	deform_t &	deform
	, Rendering::RegisterIndex_t	deformRegisters[4]		// numeric parameter for deforms
	, mtrParsingData_t * pd
	, Lexer & lexer
	, Doom3_MaterialShader_Compiler& material_compiler
	)
{
	String64 deform_name;
	mxDO(expectName( lexer, deform_name ));

	if ( deform_name == "sprite" ) {
		deform = DFRM_SPRITE;
		pd->cullType = CT_TWO_SIDED;
		pd->materialFlags |= ( MF_NOSHADOWS );
		return ALL_OK;
	}
	if ( deform_name == ( "tube" ) ) {
		deform = DFRM_TUBE;
		pd->cullType = CT_TWO_SIDED;
		pd->materialFlags |= ( MF_NOSHADOWS );
		return ALL_OK;
	}
	if ( deform_name == ( "flare" ) ) {
		deform = DFRM_FLARE;
		pd->cullType = CT_TWO_SIDED;
		mxDO(ParseExpression( deformRegisters[0], pd, lexer ));
		pd->materialFlags |= ( MF_NOSHADOWS );
		return ALL_OK;
	}
	if ( deform_name == ( "expand" ) ) {
		deform = DFRM_EXPAND;
		mxDO(ParseExpression( deformRegisters[0], pd, lexer ));
		return ALL_OK;
	}
	if ( deform_name == ( "move" ) ) {
		deform = DFRM_MOVE;
		mxDO(ParseExpression( deformRegisters[0], pd, lexer ));
		return ALL_OK;
	}
	if ( deform_name == ( "turbulent" ) ) {
		deform = DFRM_TURB;
UNDONE;
		//if ( !lexer.ExpectAnyToken( &deform_name ) ) {
		//	lexer.Warning( "deform particle missing particle name" );
		//	materialFlags |= ( MF_DEFAULTED );
		//	return ERR_MISSING_NAME;
		//}
		//deformDecl = declManager->FindType( DECL_TABLE, deform_name.c_str(), true );

		//deformRegisters[0] = material_compiler.ParseExpression( lexer );
		//deformRegisters[1] = material_compiler.ParseExpression( lexer );
		//deformRegisters[2] = material_compiler.ParseExpression( lexer );
		//return ALL_OK;
	}
	if ( deform_name == ( "eyeBall" ) ) {
		deform = DFRM_EYEBALL;
		return ALL_OK;
	}
	if ( deform_name == ( "particle" ) ) {
		deform = DFRM_PARTICLE;

		String64 deform_name2;
		mxDO(expectName( lexer, deform_name2 ));

		//UNDONE;
		mxDO(SkipBracedSection( lexer ));
		//if ( !lexer.ExpectAnyToken( &deform_name ) ) {
		//	lexer.Warning( "deform particle missing particle name" );
		//	materialFlags |= ( MF_DEFAULTED );
		//	return ERR_MISSING_NAME;
		//}
		//deformDecl = declManager->FindType( DECL_PARTICLE, deform_name.c_str(), true );
		return ALL_OK;
	}
	if ( deform_name == ( "particle2" ) ) {
		deform = DFRM_PARTICLE2;

		String64 deform_name2;
		mxDO(expectName( lexer, deform_name2 ));

		//UNDONE;
		mxDO(SkipBracedSection( lexer ));

		//if ( !lexer.ExpectAnyToken( &deform_name ) ) {
		//	lexer.Warning( "deform particle missing particle name" );
		//	materialFlags |= ( MF_DEFAULTED );
		//	return ERR_MISSING_NAME;
		//}
		//deformDecl = declManager->FindType( DECL_PARTICLE, deform_name.c_str(), true );
		return ALL_OK;
	}

	lexer.Warning( "Bad deform type '%s'", deform_name.c_str() );

	pd->materialFlags |= ( MF_DEFAULTED );

	return ERR_UNEXPECTED_TOKEN;
}

/*
-----------------------------------------------------------------------------
	Doom3_MaterialShader_Compiler
-----------------------------------------------------------------------------
*/

/// parses *.mtr file
ERet Doom3_MaterialShader_Compiler::parseAndCompileMaterialDecls(
	const char* filename
	, const ShaderCompilerConfig& shader_compiler_settings
	, const Doom3AssetsConfig& doom3_cfg
	, AssetDatabase & asset_database
	, IAssetBundler * asset_bundler
	, AllocatorI &	scratchpad
	)
{
	DynamicArray< char >	source_file_data( scratchpad );

	mxDO(Str::loadFileToString(
		source_file_data
		, filename
		));

	//
	Lexer	lexer(
		source_file_data.raw()
		, source_file_data.rawSize()
		, filename
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
	int	num_parsed_tables = 0;
	int	num_parsed_shaders = 0;

	mxLOOP_FOREVER
	{
		TbToken	nextToken;

		//
		if( !lexer.PeekToken( nextToken ) ) {
			break;	// EOF
		}

		if( nextToken == '}' ) {
			break;
		}

		//
		if( nextToken == "table" ) {
			mxDO(parseTableDecl( lexer ));
			++num_parsed_tables;
			continue;
		}

		//
		const ERet res = this->parseMaterialDecl(
			lexer
			, asset_database
			, asset_bundler
			, shader_compiler_settings
			, doom3_cfg
			, scratchpad
			);
		if(mxFAILED(res))
		{
			if( res == ERR_FAILED_TO_OPEN_FILE ) {
				ptWARN("Failed to parse material!");
				continue;
			} else {
				//mxASSERT(0);
				return res;
			}
		}

		++num_parsed_shaders;
	}

	DBGOUT("Parsed shader material file: '%s', %d tables, %d material shaders"
		, filename, num_parsed_tables, num_parsed_shaders
		);

	return ALL_OK;
}

/* E.g.:
table muzzleflashtable { clamp { 0.45, 1, 0.65, 0.88, 0.55, 0.88, 0.55, 1.0, 1.0, 1, 0.88, 0.75, 0.55, 0.30, 0 } }
table muzzleflashrandom { { 1.1, 0.9, 0.85, 1.2, 0.7, 1.02, 0.94 } }
*/
ERet Doom3_MaterialShader_Compiler::parseTableDecl( Lexer & lexer )
{
	mxDO(expectTokenString( lexer, "table" ));

	String64	table_name;
	mxDO(expectName( lexer, table_name ));

	//
	bool	clamp = false;
	bool	snap = false;

	DynamicArray< float >	values(MemoryHeaps::temporary());
	float					temp[64];
	values.initializeWithExternalStorageAndCount( temp, mxCOUNT_OF(temp) );

	//
	mxDO(expectTokenChar( lexer, '{' ));
	{
		if( PeekTokenString( lexer, "clamp" ) ) {
			SkipToken( lexer );
			clamp = true;
		}
		else if( PeekTokenString( lexer, "snap" ) ) {
			SkipToken( lexer );
			snap = true;
		}

		mxDO(expectTokenChar( lexer, '{' ));
		{
			mxLOOP_FOREVER
			{
				float	x;
				mxDO(expectFloat2( &x, lexer ));

				mxDO(values.add(x));

				if( PeekTokenChar( lexer, ',' ) ) {
					SkipToken( lexer );
					continue;
				}

				if( PeekTokenChar( lexer, '}' ) ) {
					break;
				}
			}
		}
		mxDO(expectTokenChar( lexer, '}' ));
	}
	mxDO(expectTokenChar( lexer, '}' ));

	//
	idDeclTable	decl_table;
	decl_table._name = Doom3::PathToAssetID( table_name.c_str() );
	decl_table._index = 0;
	decl_table._clamp = clamp;
	decl_table._snap = snap;
	mxDO(Arrays::Copy( decl_table._values, values ));

	//
	mxDO(Doom3_AssetBundles::Get().material_tables.tables.add( decl_table ));

	lexer.Message("Parsed table '%s'.", table_name.c_str());

	return ALL_OK;
}

/*
https://www.iddevnet.com/doom3/
E.g.:
models/items/armor/armor_shard
{
noselfShadow
renderbump  -size 512 512 -colorMap -aa 2 -trace .1 models/items/armor/armor_shard_local.tga models/items/armor/armor_shard_hi.lwo
diffusemap	models/items/armor/armor_shard.tga
bumpmap     models/items/armor/armor_shard_local.tga     	
specularmap	models/items/armor/armor_shard_s.tga
}
*/
ERet Doom3_MaterialShader_Compiler::parseMaterialDecl(
	Lexer & lexer
	, AssetDatabase & asset_database
	, IAssetBundler * asset_bundler
	, const ShaderCompilerConfig& shader_compiler_settings
	, const Doom3AssetsConfig& doom3_cfg
	, AllocatorI &	scratchpad
	)
{
	String64	mat_shader_name;
	mxDO(expectName( lexer, mat_shader_name ));

	lexer.Debug("Parsing material '%s'...", mat_shader_name.c_str());

	//
	DebugParam	dbgparam;

if(
  // strstr(mat_shader_name.c_str(),
		//"models/weapons/rocketlauncher/rocketlauncher_mflash"
	 //  //"models/weapons/rocketlauncher/rocketlauncher_fx"
	 //  )
   0 == strcmp(mat_shader_name.c_str(),
		//"models/weapons/rocketlauncher/rocketlauncher"
		"models/monsters/zombie/commando/com1_eye"
	   )	   
	   )
{
	//dbgparam.flag = true;
}

	//
	const AssetID material_asset_id = Doom3::PathToAssetID(
		mat_shader_name.c_str()
		);

	//

	bool		noFog = false;			// surface does not create fog interactions

	int			spectrum = 0;			// for invisible writing, used for both lights and surfaces

	float		polygonOffset = 0;

	decalInfo_t		decalInfo;
	mxZERO_OUT(decalInfo);

	float				sort = 0;				// lower numbered shaders draw before higher numbered

	int					stereoEye = 0;

	deform_t			deform;
	Rendering::RegisterIndex_t		deformRegisters[4];		// numeric parameter for deforms
	mxZERO_OUT(deform);
	mxZERO_OUT(deformRegisters);

	bool				fogLight = false;
	bool				blendLight = false;
	bool				ambientLight = false;
	bool				unsmoothedTangents = false;
//	bool				hasSubview;			// mirror, remote render, etc
	bool				allowOverlays = false;


	bool		suppressInSubview = false;
	bool		portalSky = false;

	// allow a global setting for repeat
	textureRepeat_t trpDefault = TR_REPEAT;

	//
	ERet	last_error = ALL_OK;

	//
	mtrParsingData_t *	pd = mxNEW( scratchpad, mtrParsingData_t, mat_shader_name.c_str(), scratchpad );
	AutoFree			pd_scoped_releaser( scratchpad, pd );

	//
	mxDO(expectTokenChar( lexer, '{' ));

	mxLOOP_FOREVER
	{
		TbToken	token;
		if( !lexer.ReadToken( token ) ) {
			return ERR_EOF_REACHED;
		}

		if( token == '}' ) {
			break;	// end of material definition
		}

		//
		if( token == "qer_editorimage" ) {
			lexer.SkipRestOfLine();
			continue;
		}

		// check for the surface / content bit flags
		if ( CheckSurfaceParm(
			token
			, pd->surfaceFlags
			, pd->contentFlags
			) )
		{
			continue;
		}


#pragma region Material Flags
		//
		// polygonOffset
		//if ( streqi( token, "polygonOffset" ) ) {
		//	materialFlags |= ( MF_POLYGONOFFSET );
		//	if ( !lexer.ReadTokenOnLine( &token ) ) {
		//		polygonOffset = 1;
		//		continue;
		//	}
		//	// explict larger (or negative) offset
		//	polygonOffset = token.GetFloatValue();
		//	continue;
		//}

		// noshadow
		if ( streqi( token, "noShadows" ) ) {
			pd->materialFlags |= ( MF_NOSHADOWS );
			continue;
		}

		// noSelfShadow
		if ( streqi( token, "noSelfShadow" ) ) {
			pd->materialFlags |= ( MF_NOSELFSHADOW );
			continue;
		}

		if ( streqi( token, "suppressInSubview" ) ) {
			suppressInSubview = true;
			continue;
		}
		if ( streqi( token, "portalSky" ) ) {
			portalSky = true;
			continue;
		}

		// noPortalFog
		if ( streqi( token, "noPortalFog" ) ) {
			pd->materialFlags |= ( MF_NOPORTALFOG );
			continue;
		}
		// forceShadows allows nodraw surfaces to cast shadows
		if ( streqi( token, "forceShadows" ) ) {
			pd->materialFlags |= ( MF_FORCESHADOWS );
			continue;
		}
		// overlay / decal suppression
		if ( streqi( token, "noOverlays" ) ) {
			allowOverlays = false;
			continue;
		}
		// monster blood overlay forcing for alpha tested or translucent surfaces
		if ( streqi( token, "forceOverlays" ) ) {
			pd->forceOverlays = true;
			continue;
		}
		// translucent
		if ( streqi( token, "translucent" ) ) {
			// Draw with an alpha blend
			pd->coverage = MC_TRANSLUCENT;
			continue;
		}
		// global zero clamp
		if ( streqi( token, "zeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		}
		// global clamp
		if ( streqi( token, "clamp" ) ) {
			trpDefault = TR_CLAMP;
			continue;
		}
		// global clamp
		if ( streqi( token, "alphazeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		}
		// forceOpaque is used for skies-behind-windows
		if ( streqi( token, "forceOpaque" ) ) {
			pd->coverage = MC_OPAQUE;
			continue;
		}
		// twoSided
		if ( streqi( token, "twoSided" ) ) {
			pd->cullType = CT_TWO_SIDED;
			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			// we could make this no-self-shadows, but it may be more important
			// to receive shadows from no-self-shadow monsters
			pd->materialFlags |= ( MF_NOSHADOWS );
			continue;
		}
		// backSided
		if ( streqi( token, "backSided" ) ) {
			// Draw only the back. This also implies no-shadows
			pd->cullType = CT_BACK_SIDED;
			// the shadow code doesn't handle this, so just disable shadows.
			// We could fix this in the future if there was a need.
			pd->materialFlags |= ( MF_NOSHADOWS );
			continue;
		}
		// foglight
		if ( streqi( token, "fogLight" ) ) {
			fogLight = true;
			continue;
		}
		// blendlight
		if ( streqi( token, "blendLight" ) ) {
			blendLight = true;
			continue;
		}
		// ambientLight
		if ( streqi( token, "ambientLight" ) ) {
			ambientLight = true;
			continue;
		}
		// mirror
		if ( streqi( token, "mirror" ) ) {
			sort = SS_SUBVIEW;
			pd->coverage = MC_OPAQUE;
			continue;
		}
		// noFog
		if ( streqi( token, "noFog" ) ) {
			noFog = true;
			continue;
		}
		// unsmoothedTangents
		if ( streqi( token, "unsmoothedTangents" ) ) {
			// Uses the single largest area triangle for each vertex, instead of smoothing all.
			unsmoothedTangents = true;
			continue;
		}

		//// lightFallofImage <imageprogram>
		//// specifies the image to use for the third axis of projected
		//// light volumes
		//if ( streqi( token, "lightFalloffImage" ) ) {
		//	str = R_ParsePastImageProgram( lexer );
		//	idStr	copy;

		//	copy = str;	// so other things don't step on it
		//	lightFalloffImage = globalImages->ImageFromFile( copy, TF_DEFAULT, TR_CLAMP /* TR_CLAMP_TO_ZERO */, TD_DEFAULT );
		//	continue;
		//}

		// guisurf <guifile> | guisurf entity
		// an entity guisurf must have an idUserInterface
		// specified in the renderEntity
		if ( streqi( token, "guisurf" ) )
		{
			lexer.SkipRestOfLine();

			UNDONE_PARTS_MARKER;
			//lexer.ReadTokenOnLine( &token );
			//if ( streqi( token, "entity" ) ) {
			//	entityGui = 1;
			//} else if ( streqi( token, "entity2" ) ) {
			//	entityGui = 2;
			//} else if ( streqi( token, "entity3" ) ) {
			//	entityGui = 3;
			//} else {
			//	gui = uiManager->FindGui( token.c_str(), true );
			//}
			continue;
		}

		// sort <type>
		if ( streqi( token, "sort" ) ) {
			// Type is one of: subview, opaque, decal, far, medium, close, almostNearest, nearest, postProcess.
			mxDO(ParseSort( sort, pd->materialFlags, lexer ));
			continue;
		}
		if ( streqi( token, "stereoeye") ) {
			mxDO(ParseStereoEye( stereoEye, pd->materialFlags, lexer ));
			continue;
		}

		// spectrum <integer>
		// Spectrums are used for "invisible writing" that can only be illuminated by a light of matching spectrum.
		if ( streqi( token, "spectrum" ) )
		{
			lexer.SkipRestOfLine();
			//spectrum = atoi( token.c_str() );
			continue;
		}

#pragma endregion


		// deform < sprite | tube | flare >
		if ( streqi( token, "deform" ) ) {
			mxDO(ParseDeform(
				deform
				, deformRegisters
				, pd
				, lexer
				, *this
				));
			continue;
		}

		// decalInfo <staySeconds> <fadeSeconds> ( <start rgb> ) ( <end rgb> )
		// Used in decal materials to set how long the decal stays, and how it fades out.
		if ( streqi( token, "decalInfo" ) ) {
			ParseDecalInfo( &decalInfo, lexer );
			continue;
		}


#pragma region Texture Maps

		// renderbump <args...>
		if ( streqi( token, "renderbump") ) {
			lexer.SkipRestOfLine();
			continue;
		}

		// diffusemap for stage shortcut
		if ( streqi( token, "diffusemap" ) )
		{
			AssetID	diffusemap_id;
			const ERet res = parseImageProgram(
				diffusemap_id
				, lexer
				, doom3_cfg
				, asset_database
				);

			if( mxFAILED(res) ) {
				last_error = res;
				if( res != ERR_FAILED_TO_OPEN_FILE ) {
					return res;
				}
			}

			//
			shaderStage_t*	new_stage;
			mxDO(CreateStage(&new_stage, lexer, pd));
			new_stage->lighting = SL_DIFFUSE;
			new_stage->texture.texture_id = diffusemap_id;
			continue;
		}
		// specularmap for stage shortcut
		if ( streqi( token, "specularmap" ) )
		{
			AssetID	specularmap_id;
			const ERet res = parseImageProgram(
				specularmap_id
				, lexer
				, doom3_cfg
				, asset_database
				);

			if( mxFAILED(res) ) {
				last_error = res;
				if( res != ERR_FAILED_TO_OPEN_FILE ) {
					return res;
				}
			}

			//
			shaderStage_t*	new_stage;
			mxDO(CreateStage(&new_stage, lexer, pd));
			new_stage->lighting = SL_SPECULAR;
			new_stage->texture.texture_id = specularmap_id;
			continue;
		}
		// normalmap for stage shortcut
		if ( streqi( token, "bumpmap" ) )
		{
			// don't stop on image load errors, because it breaks parsing of following materials

			AssetID	bumpmap_id;
			const ERet res = parseImageProgram(
				bumpmap_id
				, lexer
				, doom3_cfg
				, asset_database
				);

			if( mxFAILED(res) ) {
				last_error = res;
				if( res != ERR_FAILED_TO_OPEN_FILE ) {
					return res;
				}
			}

			//
			shaderStage_t*	new_stage;
			mxDO(CreateStage(&new_stage, lexer, pd));
			new_stage->lighting = SL_BUMP;
			new_stage->texture.texture_id = bumpmap_id;
			continue;
		}

#pragma endregion


		// DECAL_MACRO for backwards compatibility with the preprocessor macros
		if ( streqi( token, "DECAL_MACRO" ) ) {
			// polygonOffset
			pd->materialFlags |= ( MF_POLYGONOFFSET );
			polygonOffset = 1;

			// discrete
			pd->surfaceFlags |= SURF_DISCRETE;
			pd->contentFlags &= ~CONTENTS_SOLID;

			// sort decal
			sort = SS_DECAL;

			// noShadows
			pd->materialFlags |= ( MF_NOSHADOWS );
			continue;
		}

		if ( token == "{" )
		{
			// create the new stage
			mxDO(ParseStage(
				lexer

				, &trpDefault
				, pd

				, doom3_cfg
				, asset_database
				));
			continue;
		}

		//
		lexer.Warning( "unknown general material parameter '%s' in '%s'",
			token.c_str(), lexer.GetFileName()
			);

		//
		mxASSERT(false);

		pd->materialFlags |= ( MF_DEFAULTED );
		return ERR_UNEXPECTED_TOKEN;
	}//LOOP

	if( mxFAILED( last_error ) ) {
		return last_error;
	}



	mxSTOLEN("Doom 3 BFG Edition GPL Source Code, Material.cpp: idMaterial::Parse()");

	//
	// count non-lit stages
	int numAmbientStages = 0;

	for ( int i = 0 ; i < pd->numStages ; i++ ) {
		if ( pd->parseStages[i].lighting == SL_AMBIENT ) {
			numAmbientStages++;
		}
	}


	// see if there is a subview stage

	bool hasSubview = false;

	if ( sort == SS_SUBVIEW ) {
		hasSubview = true;
	} else {
		hasSubview = false;
		for ( int i = 0 ; i < pd->numStages ; i++ ) {
			if ( pd->parseStages[i].texture.dynamic ) {
				hasSubview = true;
			}
		}
	}


	// automatically determine coverage if not explicitly set
	if ( pd->coverage == MC_BAD ) {
		// automatically set MC_TRANSLUCENT if we don't have any interaction stages and 
		// the first stage is blended and not an alpha test mask or a subview
		if ( !pd->numStages ) {
			// non-visible
			pd->coverage = MC_TRANSLUCENT;
		} else if ( pd->numStages != numAmbientStages ) {
			// we have an interaction draw
			pd->coverage = MC_OPAQUE;
		} else if ( 
			( pd->parseStages[0].drawStateBits & GLS_DSTBLEND_BITS ) != GLS_DSTBLEND_ZERO ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_ALPHA ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
			)
		{
			// blended with the destination
			pd->coverage = MC_TRANSLUCENT;
		} else {
			pd->coverage = MC_OPAQUE;
		}
	}





	//
	CompiledAssetData	compiled_material_data( scratchpad );

	//
	const ERet compilation_result = this->Compile_idMaterial(
		compiled_material_data
		, *pd
		, material_asset_id
		, asset_database
		, shader_compiler_settings
		, scratchpad
		, lexer
		, dbgparam
		);

#if SKIP_FAILED_MATERIALS_SILENTLY
	if(mxFAILED(compilation_result)) {
		return ALL_OK;
	}
#else
	mxDO(compilation_result);
#endif

	mxDO(this->Save_Compiled_idMaterial_Data(
		compiled_material_data
		, material_asset_id
		, asset_database
		, scratchpad
		));

	return ALL_OK;
}

/*
===============
idMaterial::CheckSurfaceParm

See if the current token matches one of the surface parm bit flags
===============
*/
bool Doom3_MaterialShader_Compiler::CheckSurfaceParm(
	const TbToken& token
	, int &surfaceFlags
	, int &contentFlags
	)
{
	for ( int i = 0 ; i < numInfoParms ; i++ ) {
		if ( 0 == stricmp( token.text.c_str(), infoParms[i].name.c_str() ) ) {
			if ( infoParms[i].surfaceFlags & SURF_TYPE_MASK ) {
				// ensure we only have one surface type set
				surfaceFlags &= ~SURF_TYPE_MASK;
			}
			surfaceFlags |= infoParms[i].surfaceFlags;
			contentFlags |= infoParms[i].contents;
			if ( infoParms[i].clearSolid ) {
				contentFlags &= ~CONTENTS_SOLID;
			}
			return true;
		}
	}
	return false;
}

/*
=================
idMaterial::ParseSort
=================
*/
ERet Doom3_MaterialShader_Compiler::ParseSort(
	float &sort
	, int &materialFlags
	, Lexer & lexer
	)
{
	TbToken sort_type;
	mxDO(expectAnyToken( sort_type, lexer ));

	if ( streqi( sort_type, "subview" ) ) {
		sort = SS_SUBVIEW;
	} else if ( streqi( sort_type, "opaque" ) ) {
		sort = SS_OPAQUE;
	}else if ( streqi( sort_type, "decal" ) ) {
		sort = SS_DECAL;
	} else if ( streqi( sort_type, "far" ) ) {
		sort = SS_FAR;
	} else if ( streqi( sort_type, "medium" ) ) {
		sort = SS_MEDIUM;
	} else if ( streqi( sort_type, "close" ) ) {
		sort = SS_CLOSE;
	} else if ( streqi( sort_type, "almostNearest" ) ) {
		sort = SS_ALMOST_NEAREST;
	} else if ( streqi( sort_type, "nearest" ) ) {
		sort = SS_NEAREST;
	} else if ( streqi( sort_type, "postProcess" ) ) {
		sort = SS_POST_PROCESS;
	} else if ( streqi( sort_type, "portalSky" ) ) {
		sort = SS_PORTAL_SKY;
	} else {
		sort = atof( sort_type.c_str() );
	}

	return ALL_OK;
}

/*
=================
idMaterial::ParseStereoEye
=================
*/
ERet Doom3_MaterialShader_Compiler::ParseStereoEye(
	int &stereoEye
	, int &materialFlags
	, Lexer & lexer
	)
{
	String32 token;
	mxDO(expectName( lexer, token ));

UNDONE;
	//if ( !lexer.ReadTokenOnLine( &token ) ) {
	//	lexer.Warning( "missing eye parameter" );
	//	materialFlags |= ( MF_DEFAULTED );
	//	return;
	//}

	if ( streqi( token, "left" ) ) {
		stereoEye = -1;
	} else if ( streqi( token, "right" ) ) {
		stereoEye = 1;
	} else {
		stereoEye = 0;
	}

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	Doom3_MaterialShader_Compiler
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(Doom3_MaterialShader_Compiler);

ERet Doom3_MaterialShader_Compiler::Initialize()
{
	//SON::LoadFromFile("_mesh_compiler.son", m_settings, MemoryHeaps::temporary());
	return ALL_OK;
}

void Doom3_MaterialShader_Compiler::Shutdown()
{
}

const TbMetaClass* Doom3_MaterialShader_Compiler::getOutputAssetClass() const
{
	// *.mtr files are converted into asset bundles with shader assets
	return &MetaFile::metaClass();
}

ERet Doom3_MaterialShader_Compiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	//
	const TbMetaClass& material_class = idMaterial::metaClass();

	DevAssetBundler		asset_bundler_dev( Doom3_AssetBundles::Get().materials, &material_class );
	//AssetBundler_None	asset_bundler_none( &material_class );
	//IAssetBundler *		asset_bundler = Doom3::shouldBeCompiledSeparately( inputs.path.c_str() )
	//	? static_cast<IAssetBundler*>(&asset_bundler_none)
	//	: static_cast<IAssetBundler*>(&asset_bundler_dev)
	//	;

	//
	mxDO(this->parseAndCompileMaterialDecls(
		inputs.path.c_str()
		, inputs.cfg.shader_compiler
		, inputs.cfg.doom3
		, inputs.asset_database
		, &asset_bundler_dev//asset_bundler
		, scratchpad
		));

	//
	//if( !Doom3::shouldBeCompiledSeparately( inputs.path.c_str() ) )
	//{
		outputs.asset_type = &MetaFile::metaClass();
		NwBlobWriter(outputs.object_data).Put(MetaFile());
	//}

	return ALL_OK;
}

static
ERet _RemoveStagesForMonsterSkinBurnAwayEffect(
	Doom3::mtrParsingData_t & mtr_parsing_data
	, const Lexer& lexer
	)
{
	if(mtr_parsing_data.registersAreConstant) {
		return ALL_OK;
	}

	//
	const bool is_monster_skin_material = strstr(
		mtr_parsing_data.material_name,
		"models/monsters"
		);
	if(!is_monster_skin_material) {
		return ALL_OK;
	}

	//
	for( int iStage = mtr_parsing_data.numStages - 1;
		iStage >= 0;
		--iStage )
	{
		shaderStage_t& ss = mtr_parsing_data.parseStages[ iStage ];

		if(ss.condition_expr_string == "PARM7")
		{
			UNDONE;//REMOVE MANUALLY to avoid unused ops and regs!
			if(DBG_PRINT_MATERIAL_VERBOSE)
			{
				lexer.Message("Removing stage '%s': tex='%s', idx=[%d] for monster skin burn away effect.",
					Doom3::stageLightingToString(ss.lighting),
					ss.texture.texture_id.c_str(),
					iStage
					);
			}

			//
			--mtr_parsing_data.numStages;
			//
			for(int j = iStage; j < mtr_parsing_data.numStages; j++)
			{
				mtr_parsing_data.parseStages[ j ] = mtr_parsing_data.parseStages[ j+1 ];
			}
		}
	}

	//
	if(!mtr_parsing_data.numStages) {
		return ERR_OBJECT_NOT_FOUND;
	}




	////
	//bool	remaining_stages_constant_only = true;

	////
	//nwFOR_LOOP( int, iStage, mtr_parsing_data.numStages )
	//{
	//	const shaderStage_t& ss = mtr_parsing_data.parseStages[ iStage] ;
	//	if(ss.conditionRegister)
	//	{
	//		//
	//	}
	//}//for

	return ALL_OK;
}

ERet Doom3_MaterialShader_Compiler::Compile_idMaterial(
	CompiledAssetData &compiled_material_data_
	, Doom3::mtrParsingData_t & mtr_parsing_data
	, const AssetID& material_asset_id
	, AssetDatabase & asset_database
	, const ShaderCompilerConfig& shader_compiler_settings
	, AllocatorI &	scratchpad
	, const Lexer& lexer
	, const DebugParam& dbgparam
	)
{
	if(DBG_PRINT_MATERIAL_VERBOSE)
	{
		lexer.Message("Compling material '%s' (%d stages):",
			mtr_parsing_data.material_name, mtr_parsing_data.numStages
			);
		for( int iStage = 0; iStage < mtr_parsing_data.numStages; iStage++ )
		{
			const shaderStage_t& ss = mtr_parsing_data.parseStages[ iStage] ;
			DEVOUT("Stage[%d]: type: '%s', cond expr: '%s'",
				iStage,
				Doom3::stageLightingToString(ss.lighting),
				ss.condition_expr_string.c_str()
				);
		}//for
	}


	//
#if SKIP_STAGES_FOR_MONSTER_SKIN_BURNAWAY_EFFECT
	mxDO(_RemoveStagesForMonsterSkinBurnAwayEffect(mtr_parsing_data, lexer));
#endif

	//
	if(!mtr_parsing_data.numStages) {
		lexer.Warning("Skipping material '%s', because it doesn't have valid stages!",
			mtr_parsing_data.material_name);
		return ERR_FAILED_TO_PARSE_DATA;
	}

	// Try to merge diffuse, normal and specular stages into a single pass.

	//
	mxTRY(_Compile_idMaterial_MultiplePasses(
		compiled_material_data_
		, mtr_parsing_data
		, material_asset_id
		, asset_database
		, shader_compiler_settings
		, scratchpad
		, lexer
		, dbgparam
		));

	return ALL_OK;
}

ERet Doom3_MaterialShader_Compiler::Save_Compiled_idMaterial_Data(
	const CompiledAssetData& compiled_material_data
	, const AssetID& material_asset_id
	, AssetDatabase & asset_database
	, AllocatorI &	scratchpad
	)
{
	mxDO(asset_database.addOrUpdateGeneratedAsset(
		material_asset_id
		, idMaterial::metaClass()
		, compiled_material_data
		));

	return ALL_OK;
}












struct idMaterialPassDesc
{
	AssetID	diffusemap;
	AssetID	bumpmap;		// may be empty
	AssetID	specularmap;	// may be empty

	//
	NwRenderStateDesc	render_state_desc;

	// G-Buffer or Forward Translucent?
	bool				use_gbuffer_pass;

	//
	materialCoverage_t	coverage;

	TPtr< const shaderStage_t >	stage;

	Rendering::RegisterIndex_t		condition_register;

	Rendering::RegisterIndex_t		alpha_test_register;

	idMaterialPassFlagsT	flags;

	Rendering::RegisterIndex_t			texture_matrix_row0[3];
	Rendering::RegisterIndex_t			texture_matrix_row1[3];

	//NwShaderFeatureBitMask	default_program_permutation;

	//
	//NameHash32	pass_name_hash_in_shader_effect;
	// 
public:
	idMaterialPassDesc()
	{
		use_gbuffer_pass = true;
		coverage = MC_OPAQUE;
		condition_register = ~0;
		alpha_test_register = ~0;
		flags.raw_value = 0;
		TSetStaticArray(texture_matrix_row0, (Rendering::RegisterIndex_t)~0);
		TSetStaticArray(texture_matrix_row1, (Rendering::RegisterIndex_t)~0);
	}
};

typedef TInplaceArray< idMaterialPassDesc, 8 >	idMaterialPassDescArray;


void _SetDefaultRenderStateDesc(
	NwRenderStateDesc &render_state_desc_
	)
{
	const NwComparisonFunc::Enum default_depth_compare_function
		= nwRENDERER_USE_REVERSED_DEPTH
		? NwComparisonFunc::Greater
		: NwComparisonFunc::Less
		;

	render_state_desc_.depth_stencil.comparison_function = default_depth_compare_function;
}



ERet _FillPassRenderState(
	idMaterialPassDesc &new_material_pass_desc_
	, const U64 drawStateBits
	, const Doom3::mtrParsingData_t& mtr_parsing_data
	)
{
	NwRenderStateDesc &render_state_desc_ = new_material_pass_desc_.render_state_desc;

	_SetDefaultRenderStateDesc(render_state_desc_);

	//
	mxASSERT(mtr_parsing_data.coverage != MC_BAD);
	new_material_pass_desc_.coverage = mtr_parsing_data.coverage;

	// Blend state.

	const bool uses_blending = (drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS )) != 0;

	if(uses_blending)
	{
		mxSTOLEN("Doom 3 BFG Edition GPL Source Code, gl_GraphicsAPIWrapper.cpp: GL_State()");

		//
		NwBlendFactor::Enum	source_factor;
		NwBlendFactor::Enum	destination_factor;

		switch ( drawStateBits & GLS_SRCBLEND_BITS )
		{
		case GLS_SRCBLEND_ZERO:					source_factor = NwBlendFactor::ZERO; break;
		case GLS_SRCBLEND_ONE:					source_factor = NwBlendFactor::ONE; break;
		case GLS_SRCBLEND_DST_COLOR:			source_factor = NwBlendFactor::DST_COLOR; break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	source_factor = NwBlendFactor::INV_DST_COLOR; break;
		case GLS_SRCBLEND_SRC_ALPHA:			source_factor = NwBlendFactor::SRC_ALPHA; break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	source_factor = NwBlendFactor::INV_SRC_ALPHA; break;
		case GLS_SRCBLEND_DST_ALPHA:			source_factor = NwBlendFactor::DST_ALPHA; break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	source_factor = NwBlendFactor::INV_DST_ALPHA; break;
		default:
			assert( !"GL_State: invalid src blend state bits\n" );
			return ERR_INVALID_PARAMETER;
		}

		switch ( drawStateBits & GLS_DSTBLEND_BITS )
		{
		case GLS_DSTBLEND_ZERO:					destination_factor = NwBlendFactor::ZERO; break;
		case GLS_DSTBLEND_ONE:					destination_factor = NwBlendFactor::ONE; break;
		case GLS_DSTBLEND_SRC_COLOR:			destination_factor = NwBlendFactor::SRC_COLOR; break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	destination_factor = NwBlendFactor::INV_SRC_COLOR; break;
		case GLS_DSTBLEND_SRC_ALPHA:			destination_factor = NwBlendFactor::SRC_ALPHA; break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	destination_factor = NwBlendFactor::INV_SRC_ALPHA; break;
		case GLS_DSTBLEND_DST_ALPHA:			destination_factor = NwBlendFactor::DST_ALPHA; break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:  destination_factor = NwBlendFactor::INV_DST_ALPHA; break;
		default:
			assert( !"GL_State: invalid dst blend state bits\n" );
			return ERR_INVALID_PARAMETER;
		}

		//
		if ( source_factor == NwBlendFactor::ONE && destination_factor == NwBlendFactor::ZERO ) {
			// don't enable blend
		} else {
			render_state_desc_.blend.flags |= NwBlendFlags::Enable_Blending;

			render_state_desc_.blend.color_channel.operation = NwBlendOp::ADD;
			render_state_desc_.blend.color_channel.source_factor = source_factor;
			render_state_desc_.blend.color_channel.destination_factor = destination_factor;

			render_state_desc_.blend.alpha_channel = render_state_desc_.blend.color_channel;
		}
	}



	// Depth-Stencil state.
	{
		switch(mtr_parsing_data.cullType)
		{
		case CT_FRONT_SIDED:
			// cull back-facing triangles
			render_state_desc_.rasterizer.cull_mode = NwCullMode::Back;
			break;

		case CT_BACK_SIDED:
			// cull front-facing triangles
			render_state_desc_.rasterizer.cull_mode = NwCullMode::Front;
			break;

		case CT_TWO_SIDED:
			render_state_desc_.rasterizer.cull_mode = NwCullMode::None;
			break;
		}

		//
		if(mtr_parsing_data.materialFlags & MF_NOSELFSHADOW)
		{
			if(Heuristics::IsFirstPersonWeaponModel(mtr_parsing_data.material_name))
			{
				DBGOUT("Disabling depth clipping on material '%s' - it must be used for drawing first person weapons!",
					mtr_parsing_data.material_name
					);
				// This flag is usually set for character models (e.g. player's arms),
				// we don't want depth-clip them.
				CLEAR_BITS(
					render_state_desc_.rasterizer.flags.raw_value,
					NwRasterizerFlags::Enable_DepthClip
					);
			}
		}
	}//



	//
	const bool is_blending_enabled = ( render_state_desc_.blend.flags & NwBlendFlags::Enable_Blending );


	//
	switch( mtr_parsing_data.coverage )
	{
		// completely fills the triangle, will have black drawn on fillDepthBuffer		
	case MC_OPAQUE:
		new_material_pass_desc_.use_gbuffer_pass = !is_blending_enabled;
		break;

		// may have alpha tested holes
	case MC_PERFORATED:
		new_material_pass_desc_.use_gbuffer_pass = true;
		break;

		// blended with background
	case MC_TRANSLUCENT:
		new_material_pass_desc_.use_gbuffer_pass = false;
		// disable depth writes
		CLEAR_BITS(
			render_state_desc_.depth_stencil.flags.raw_value,
			NwDepthStencilFlags::Enable_DepthWrite
			);
		break;

	default:
		new_material_pass_desc_.use_gbuffer_pass = !is_blending_enabled;
	}


	//

	return ALL_OK;
}

















//struct 

struct StageSort
{
	// for sorting by increasing condition regs
	Rendering::RegisterIndex_t			condition_reg;

	// only stages having the same condition expr can be merged
	// (this is quick-n-dirty 'solution' to avoid comparing expr trees)
	// e.g. "EXP_REG_PARM7 == 0.000000"
	String32				condition_expr_string;

	const shaderStage_t *	stage;
};
struct StageSortComparator
{
	bool AreInOrder( const StageSort& a, const StageSort& b ) const
	{
		return a.condition_reg < b.condition_reg;
	}
};






static
ERet _MergeStages_into_idMaterialPassDesc(
	idMaterialPassDescArray &material_pass_descs_
	, const shaderStage_t** stages
	, const int num_stages
	, const Doom3::mtrParsingData_t& mtr_parsing_data
	, const Lexer& lexer
	)
{
	mxASSERT(num_stages > 0);


	if(DBG_PRINT_MATERIAL_VERBOSE)
	{
		lexer.Message("Trying to merge %d stages into one material pass...:", num_stages);
		for( int iStage = 0; iStage < num_stages; iStage++ )
		{
			const shaderStage_t* ss = stages[ iStage ];
			DEVOUT("Stage[%d]: type: '%s'",
				iStage,
				Doom3::stageLightingToString(ss->lighting)
				);
		}//for
	}
	


	// Check if we have one stage for bump, diffuse and specular.


	int	number_of_stages_of_each_type[ NUM_STAGE_LIGHTING_TYPES ] = { 0 };

	for( int iStage = 0; iStage < num_stages; iStage++ )
	{
		const shaderStage_t* ss = stages[ iStage ];
		if( ++number_of_stages_of_each_type[ ss->lighting ] > 1 )
		{
			lexer.Warning("Material '%s' has several stages of the same type '%s'",
				mtr_parsing_data.material_name,
				stageLightingToString(ss->lighting)
				);
			return ERR_FAILED_TO_PARSE_DATA;
		}

	}//for


	//
	U64	merged_draw_state_masks = 0;

	//
	idMaterialPassDesc	new_material_pass_desc;

	//
	AssetID	last_used_texture_id;

	//
	for( int iStage = 0; iStage < num_stages; iStage++ )
	{
		const shaderStage_t& ss = *stages[ iStage ] ;

		if( AssetId_IsValid( ss.texture.texture_id ) )
		{
			last_used_texture_id = ss.texture.texture_id;

			switch( ss.lighting )
			{
			case SL_AMBIENT:// execute after lighting
				break;

			case SL_BUMP:
				new_material_pass_desc.bumpmap = ss.texture.texture_id;
				break;

			case SL_DIFFUSE:
				new_material_pass_desc.diffusemap = ss.texture.texture_id;
				break;

			case SL_SPECULAR:
				new_material_pass_desc.specularmap = ss.texture.texture_id;
				break;

			case SL_COVERAGE:
				UNDONE;
				break;
			}
		}

		// I know that this is wrong, but it's mostly working.
		merged_draw_state_masks |= ss.drawStateBits;


		//
		if( ss.hasAlphaTest )
		{
			new_material_pass_desc.flags.raw_value |= idMaterialPassFlags::HasAlphaTest;

			// this is wrong, but works 90% of the time
			new_material_pass_desc.alpha_test_register = ss.alphaTestRegister;
		}

		//
		if( ss.texture.hasMatrix )
		{
			new_material_pass_desc.flags.raw_value |= idMaterialPassFlags::HasTextureTransform;

			// again, this is wrong, but works most of the time
			TCopyStaticArray( new_material_pass_desc.texture_matrix_row0, ss.texture.texture_matrix[0] );
			TCopyStaticArray( new_material_pass_desc.texture_matrix_row1, ss.texture.texture_matrix[1] );
		}

	}//for

	
	//
	if(last_used_texture_id.d.IsEmpty()) {
		lexer.Warning("Skipping material '%s', because it doesn't have a texture map!",
			mtr_parsing_data.material_name);
		return ERR_OBJECT_NOT_FOUND;
	}

	if( new_material_pass_desc.diffusemap.d.IsEmpty() ) {
		new_material_pass_desc.diffusemap = last_used_texture_id;
	}



	// we proved earlier that all stages have the same condition
	new_material_pass_desc.condition_register = stages[0]->conditionRegister;


	//
	mxDO(_FillPassRenderState(
		new_material_pass_desc
		, merged_draw_state_masks
		, mtr_parsing_data
		));


	//
	mxDO(material_pass_descs_.add(new_material_pass_desc));

	return ALL_OK;
}



ERet Fill_idMaterialPass(
						 idMaterialPass &dst_pass_
						 , const idMaterialPassDesc& material_pass_desc
						 , const NwShaderEffectData& shader_effect_data
						 )
{
	dst_pass_.condition_register = material_pass_desc.condition_register;

	dst_pass_.flags = material_pass_desc.flags;

	dst_pass_.alpha_test_register = material_pass_desc.alpha_test_register;

	TCopyStaticArray( dst_pass_.texture_matrix_row0, material_pass_desc.texture_matrix_row0 );
	TCopyStaticArray( dst_pass_.texture_matrix_row1, material_pass_desc.texture_matrix_row1 );


	dst_pass_.diffuse_map	._setId(material_pass_desc.diffusemap);
	dst_pass_.normal_map	._setId(material_pass_desc.bumpmap);
	dst_pass_.specular_map	._setId(material_pass_desc.specularmap);





	// Select shader permutation.


	/// run-time shader permutation switches (e.g. "ENABLE_SPECULAR_MAP" = "1")
	TInplaceArray< FeatureSwitch, 8 >		feature_switches;

	//
	if( AssetId_IsValid( material_pass_desc.bumpmap ) )
	{
		mxDO(feature_switches.add(
			FeatureSwitch( String64("ENABLE_NORMAL_MAP"), 1 )
			));
	}

	//
	if( AssetId_IsValid( material_pass_desc.specularmap ) )
	{
		mxDO(feature_switches.add(
			FeatureSwitch( String64("ENABLE_SPECULAR_MAP"), 1 )
			));
	}

	//
	if( material_pass_desc.flags & idMaterialPassFlags::HasTextureTransform )
	{
		mxDO(feature_switches.add(
			FeatureSwitch( String64("HAS_TEXTURE_TRANSFORM"), 1 )
			));
	}

	//
	if( material_pass_desc.flags & idMaterialPassFlags::HasAlphaTest )
	{
		mxDO(feature_switches.add(
			FeatureSwitch( String64("IS_ALPHA_TESTED"), 1 )
			));
	}


	//
	NwShaderFeatureBitMask	default_program_permutation;
	mxDO(GetDefaultShaderFeatureMask(
		default_program_permutation
		, feature_switches
		, shader_effect_data
		));

	//
	mxENSURE(default_program_permutation < dst_pass_.program_handle.max_valid_handle_value,
		ERR_BUFFER_TOO_SMALL,
		"program permutation index is too big: %d!", default_program_permutation
		);
	// will be patched later
	dst_pass_.program_handle.id = default_program_permutation;




	// Figure out effect pass.
	//
	int selected_pass_index = INDEX_NONE;

	if( material_pass_desc.use_gbuffer_pass )
	{
		selected_pass_index = shader_effect_data.findPassIndex(ScenePasses::FillGBuffer);
	}
	else
	{
		selected_pass_index = shader_effect_data.findPassIndex(ScenePasses::Translucent);
	}

	mxENSURE(selected_pass_index != INDEX_NONE,
		ERR_OBJECT_NOT_FOUND,
		"no such passes in effect!");

	// will be patched later
	dst_pass_.view_id = selected_pass_index;

	return ALL_OK;
}








ERet Doom3_MaterialShader_Compiler::_Compile_idMaterial_MultiplePasses(
	CompiledAssetData &compiled_material_data_
	, const Doom3::mtrParsingData_t& mtr_parsing_data
	, const AssetID& material_asset_id
	, AssetDatabase & asset_database
	, const ShaderCompilerConfig& shader_compiler_settings
	, AllocatorI &	scratchpad
	, const Lexer& lexer
	, const DebugParam& dbgparam
	)
{
	// Try to merge each tuple of lighting stages (bump, diffuse and specular) into one render pass.



	TInplaceArray< StageSort, 16 >	sorted_stages;

	//
	nwFOR_LOOP(int, iStage, mtr_parsing_data.numStages)
	{
		const shaderStage_t* ss = &mtr_parsing_data.parseStages[ iStage ];

		StageSort	new_item;
		new_item.condition_reg = ss->conditionRegister;
		RegisterToString_R(new_item.condition_expr_string, ss->conditionRegister, mtr_parsing_data);
		new_item.stage = ss;

		if(DBG_PRINT_MATERIAL_VERBOSE)
		{
			DEVOUT("[DBG] Stage[%d]: type: '%s', cond reg=%d, expr: %s, should remove?: %d",
				iStage, stageLightingToString(ss->lighting),
				ss->conditionRegister, new_item.condition_expr_string.c_str(),
				ss->should_be_removed
				);
		}

		if(!ss->should_be_removed)
		{
			mxDO(sorted_stages.add(new_item));
		}
		else
		{
			if(DBG_PRINT_MATERIAL_VERBOSE) {
				DEVOUT("[DBG] Removing stage[%d], because it's flagged...", iStage);
			}
		}
	}

	if(sorted_stages.IsEmpty()) {
		return ERR_FAILED_TO_PARSE_DATA;
	}


#if 0	// disabled - rely on the original order
	// Stable-Sort by condition register (must preserve the original order!).
	Sorting::InsertionSort(
		sorted_stages.raw()
		, 0
		, sorted_stages.num() - 1
		, StageSortComparator()
		);
#endif


	//
	idMaterialPassDescArray	material_pass_descs;


	//
	{
		const char*	prev_condition_expr_string = nil;

		TInplaceArray< const shaderStage_t*, 8 >	stages_with_same_cond_reg;

		U32	already_present_stages_mask = 0;

		//
		nwFOR_LOOP(int, iSortedStage, sorted_stages.num())
		{
			const StageSort& sorted_stage = sorted_stages[ iSortedStage ];
			const shaderStage_t& ss = *sorted_stage.stage;

			//
			const char* prev_condition_expr_string_safe
				= prev_condition_expr_string != nil
				? prev_condition_expr_string
				: ""
				;

			//
			const bool condition_expression_is_same_as_prev
				= Str::EqualS(sorted_stage.condition_expr_string, prev_condition_expr_string_safe)
				;

			//
			if(
				// if the condition expression is the same
				condition_expression_is_same_as_prev
				&&
				// if this stage was not yet added
				(0 == TEST_BIT(already_present_stages_mask, BIT(ss.lighting)))
				)
			{
				mxDO(stages_with_same_cond_reg.add(&ss));
				already_present_stages_mask |= BIT(ss.lighting);
			}
			else
			{
				if( !stages_with_same_cond_reg.IsEmpty() )
				{
					mxTRY(_MergeStages_into_idMaterialPassDesc(
						material_pass_descs
						, stages_with_same_cond_reg.raw()
						, stages_with_same_cond_reg.num()
						, mtr_parsing_data
						, lexer
						));
				}
				stages_with_same_cond_reg.RemoveAll();

				//
				prev_condition_expr_string = sorted_stage.condition_expr_string.c_str();

				mxDO(stages_with_same_cond_reg.add(&ss));

				already_present_stages_mask = 0;
				already_present_stages_mask |= BIT(ss.lighting);
			}
		}//for each stage

		//
		if( !stages_with_same_cond_reg.IsEmpty() )
		{
			mxTRY(_MergeStages_into_idMaterialPassDesc(
				material_pass_descs
				, stages_with_same_cond_reg.raw()
				, stages_with_same_cond_reg.num()
				, mtr_parsing_data
				, lexer
				));
		}
	}

	//
	if(DBG_PRINT_MATERIAL_PASS_MERGING)
	{
		DBGOUT("Material '%s' -> %d passes.",
			mtr_parsing_data.material_name, material_pass_descs.num()
			);
	}




	//
	ShaderMetadata			shader_metadata;
	NwShaderEffectData		shader_effect_data;

	mxDO(CompileMaterialEffectIfNeeded(
		idMaterial::EFFECT_FILENAME
		, MakeAssetID( idMaterial::EFFECT_FILENAME )
		, TSpan< const FxDefine >()
		, shader_compiler_settings
		, asset_database

		, shader_metadata
		, shader_effect_data
		, dbgparam
		));

	DBGOUT("Command buffer size: %d bytes", shader_effect_data.command_buffer.d.rawSize());
	// Command buffer size: 112 bytes


	//
	idMaterial::LoadedData	runtime_data;

	// copy registers
	{
		mxDO(runtime_data.ops.setNum( mtr_parsing_data.numOps ));
		memcpy(
			runtime_data.ops.raw(),
			mtr_parsing_data.shaderOps,
			mtr_parsing_data.numOps * sizeof(mtr_parsing_data.shaderOps[0])
			);

		mxDO(runtime_data.expression_registers.setNum( mtr_parsing_data.numRegisters ));
		memcpy(
			runtime_data.expression_registers.raw(),
			mtr_parsing_data.shaderRegisters,
			mtr_parsing_data.numRegisters * sizeof(mtr_parsing_data.shaderRegisters[0])
			);
	}


	// setup passes
	mxDO(runtime_data.passes.setNum(material_pass_descs.num()));
	nwFOR_EACH_INDEXED(const idMaterialPassDesc& material_pass_desc, material_pass_descs, i)
	{
		idMaterialPass &dst_pass = runtime_data.passes[i];
		mxDO(Fill_idMaterialPass(
			dst_pass
			, material_pass_desc
			, shader_effect_data
			));
	}



	//
	//
	NwBlobWriter	material_object_data_writer( compiled_material_data_.object_data );
	{
		idMaterialLoader::Header_d	header;
		mxZERO_OUT(header);
		{
			header.fourCC = idMaterialLoader::SIGNATURE;
			header.flags.raw_value = 0;
		}
		mxDO(material_object_data_writer.Put( header ));

		//
		mxDO(material_object_data_writer.alignBy( idMaterial::LoadedData::ALIGNMENT ));
		mxDO(Serialization::SaveMemoryImage(
			runtime_data
			, material_object_data_writer
			));
	}

	//
	mxDO(material_object_data_writer.alignBy( idMaterial::LoadedData::ALIGNMENT ));

	// Write pass render states.
	nwFOR_EACH(const idMaterialPassDesc& material_pass_desc, material_pass_descs)
	{
		mxDO(material_object_data_writer.Put(material_pass_desc.render_state_desc));
	}

	// Write referenced material tables.
	{
		const U32	num_refs = mtr_parsing_data.table_refs.num();
		mxDO(material_object_data_writer.Put( num_refs ));

		nwFOR_EACH( const idMtrTableRef& table_ref, mtr_parsing_data.table_refs )
		{
			mxDO(writeAssetID( table_ref.table_id, material_object_data_writer ));
			mxDO(material_object_data_writer.Put( table_ref.op_index ));
		}
	}

	//
	if(DBG_DUMP_COMPILED_MATERIAL_TO_TEXT_FILE)
	{
		FileWriter	stream;

		//
		mxTRY(stream.Open( "R:/test.son", FileWrite_Append ));

		//
		TextWriter	text_writer(stream);

		//
		text_writer.Emitf("\n; %s\n", mtr_parsing_data.material_name);
		SON::Save(runtime_data, stream);

		//
		text_writer.Emitf("; %d passes\n", runtime_data.passes.num());
		text_writer.Emit("; render states\n");

		// Write pass render states.
		nwFOR_EACH(const idMaterialPassDesc& material_pass_desc, material_pass_descs)
		{
			SON::Save(material_pass_desc.render_state_desc, stream);
		}
	}

	//
#if SKIP_FAILED_MATERIALS_SILENTLY
	if( runtime_data.passes.IsEmpty() ) {
		return ALL_OK;
	}
#else
	mxENSURE(!runtime_data.passes.IsEmpty(), ERR_FAILED_TO_PARSE_DATA, "");
#endif
	

	return ALL_OK;
}

}//namespace AssetBaking
