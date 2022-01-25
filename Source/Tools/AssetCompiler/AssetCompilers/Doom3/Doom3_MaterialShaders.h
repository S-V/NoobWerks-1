// https://www.iddevnet.com/doom3/materials.html
#pragma once

#include <Rendering/Private/Modules/idTech4/idMaterial.h>
//#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>


namespace Doom3
{


mxSTOLEN("Doom3 source code, Material.h");

// moved from image.h for default parm
typedef enum {
	TF_LINEAR,
	TF_NEAREST,
	TF_DEFAULT				// use the user-specified r_textureFilter
} textureFilter_t;

typedef enum {
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures
	TR_CLAMP_TO_ZERO_ALPHA	// guarantee 0 alpha edge for projected textures
} textureRepeat_t;

typedef struct {
	int		stayTime;		// msec for no change
	int		fadeTime;		// msec to fade vertex colors over
	float	start[4];		// vertex color at spawn (possibly out of 0.0 - 1.0 range, will clamp after calc)
	float	end[4];			// vertex color at fade-out (possibly out of 0.0 - 1.0 range, will clamp after calc)
} decalInfo_t;

typedef enum {
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
	DFRM_PARTICLE,
	DFRM_PARTICLE2,
	DFRM_TURB
} deform_t;

typedef enum {
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
	DI_XRAY_RENDER,
	DI_REMOTE_RENDER
} dynamicidImage_t;

typedef struct {
	Rendering::RegisterIndex_t		registers[4];
} colorStage_t;

typedef enum {
	TG_EXPLICIT,
	TG_DIFFUSE_CUBE,
	TG_REFLECT_CUBE,
	TG_SKYBOX_CUBE,
	TG_WOBBLESKY_CUBE,
	TG_SCREEN,			// screen aligned, for mirrorRenders and screen space temporaries
	TG_SCREEN2,
	TG_GLASSWARP
} texgen_t;

// we only allow a subset of the full projection matrix
typedef Rendering::RegisterIndex_t TextureMatrix[2][3];

typedef struct {
	//idCinematic *		cinematic;
	//idImage *			image;

	AssetID				texture_id;

	texgen_t			texgen;
	bool				hasMatrix;
	TextureMatrix		texture_matrix;	// we only allow a subset of the full projection texture_matrix

	// dynamic image variables
	dynamicidImage_t	dynamic;
	int					width, height;
	int					dynamicFrameCount;
} textureStage_t;

// the order BUMP / DIFFUSE / SPECULAR is necessary for interactions to draw correctly on low end cards
enum stageLighting_t
{
	SL_AMBIENT,						// execute after lighting
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR,
	SL_COVERAGE,
};
const char* stageLightingToString(stageLighting_t sl);

static const int	NUM_STAGE_LIGHTING_TYPES = SL_COVERAGE + 1;

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
typedef enum {
	SVC_IGNORE,
	SVC_MODULATE,
	SVC_INVERSE_MODULATE
} stageVertexColor_t;

static const int	MAX_FRAGMENT_IMAGES = 8;
static const int	MAX_VERTEX_PARMS = 4;

struct newShaderStage_t
{
	AssetID				vertexProgram;
	int					numVertexParms;
	int					vertexParms[MAX_VERTEX_PARMS][4];	// evaluated register indexes

	AssetID				fragmentProgram;
	int					glslProgram;
	int					numFragmentProgramImages;
//	idImage *			fragmentProgramImages[MAX_FRAGMENT_IMAGES];

public:
	newShaderStage_t()
	{
		numVertexParms = 0;
		mxZERO_OUT(vertexParms);

		glslProgram = 0;
		numFragmentProgramImages = 0;
	}
};


/// index of an expOp_t
typedef int OpIndex_t;


struct shaderStage_t
{
	Rendering::RegisterIndex_t			conditionRegister;	// if registers[conditionRegister] == 0, skip stage

	/// index of the conditional ("if (expr)") expression op; -1 by default
	OpIndex_t				condition_op_index;

	String32				condition_expr_string;

	// true if this stage is not used in my demo (e.g. monster dissolve effect)
	bool					should_be_removed;


	//
	stageLighting_t			lighting;			// determines which passes interact with lights

	U64						drawStateBits;

	//

	colorStage_t			color;

	bool					hasAlphaTest;
	Rendering::RegisterIndex_t			alphaTestRegister;

	textureStage_t			texture;
	stageVertexColor_t		vertexColor;
	bool					ignoreAlphaTest;	// this stage should act as translucent, even
												// if the surface is alpha tested
	float					privatePolygonOffset;	// a per-stage polygon offset

	newShaderStage_t *		newStage;			// vertex / fragment program based stage

public:
	shaderStage_t()
	{
		conditionRegister = 0;

		condition_op_index = -1;

		should_be_removed = false;

		lighting = SL_DIFFUSE;
	}
};

typedef enum {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum {
	SS_SUBVIEW = -3,	// mirrors, viewscreens, etc
	SS_GUI = -2,		// guis
	SS_BAD = -1,
	SS_OPAQUE,			// opaque

	SS_PORTAL_SKY,

	SS_DECAL,			// scorch marks, etc.

	SS_FAR,
	SS_MEDIUM,			// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST,			// screen blood blobs

	SS_POST_PROCESS = 100	// after a screen copy to texture
} materialSort_t;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES			= 256;

const int MAX_TEXGEN_REGISTERS		= 4;

// material flags
typedef enum {
	MF_DEFAULTED				= BIT(0),
	MF_POLYGONOFFSET			= BIT(1),
	MF_NOSHADOWS				= BIT(2),
	MF_FORCESHADOWS				= BIT(3),
	MF_NOSELFSHADOW				= BIT(4),
	MF_NOPORTALFOG				= BIT(5),	// this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE			= BIT(6)	// in use (visible) per editor
} materialFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
typedef enum {
	CONTENTS_SOLID				= BIT(0),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT(1),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT(2),	// used for water
	CONTENTS_PLAYERCLIP			= BIT(3),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT(4),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT(5),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT(6),	// solid to IK
	CONTENTS_BLOOD				= BIT(7),	// used to detect blood decals
	CONTENTS_BODY				= BIT(8),	// used for actors
	CONTENTS_PROJECTILE			= BIT(9),	// used for projectiles
	CONTENTS_CORPSE				= BIT(10),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT(11),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT(12),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT(13),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT(14),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER	= BIT(15),	// used for triggers that are activated by the flashlight

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT(20),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT(21),	// don't cut this brush with CSG operations in the editor

	CONTENTS_REMOVE_UTIL		= ~(CONTENTS_AREAPORTAL|CONTENTS_NOCSG)
} contentsFlags_t;

// surface types
const int NUM_SURFACE_BITS		= 4;
const int MAX_SURFACE_TYPES		= 1 << NUM_SURFACE_BITS;

typedef enum {
	SURFTYPE_NONE,					// default type
    SURFTYPE_METAL,
	SURFTYPE_STONE,
	SURFTYPE_FLESH,
	SURFTYPE_WOOD,
	SURFTYPE_CARDBOARD,
	SURFTYPE_LIQUID,
	SURFTYPE_GLASS,
	SURFTYPE_PLASTIC,
	SURFTYPE_RICOCHET,
	SURFTYPE_10,
	SURFTYPE_11,
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
} surfTypes_t;

// surface flags
typedef enum {
	SURF_TYPE_BIT0				= BIT(0),	// encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1				= BIT(1),	// "
	SURF_TYPE_BIT2				= BIT(2),	// "
	SURF_TYPE_BIT3				= BIT(3),	// "
	SURF_TYPE_MASK				= ( 1 << NUM_SURFACE_BITS ) - 1,

	SURF_NODAMAGE				= BIT(4),	// never give falling damage
	SURF_SLICK					= BIT(5),	// effects game physics
	SURF_COLLISION				= BIT(6),	// collision surface
	SURF_LADDER					= BIT(7),	// player can climb up this surface
	SURF_NOIMPACT				= BIT(8),	// don't make missile explosions
	SURF_NOSTEPS				= BIT(9),	// no footstep sounds
	SURF_DISCRETE				= BIT(10),	// not clipped or merged by utilities
	SURF_NOFRAGMENT				= BIT(11),	// dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL				= BIT(12)	// renderbump will draw this surface as 0x80 0x80 0x80, which
											// won't collect light from any angle
} surfaceFlags_t;



struct mtrParsingData_t;

}//namespace Doom3
