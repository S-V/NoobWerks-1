/*
=============================================================================
	idTech4 (Doom 3) shader/material system
=============================================================================
*/
#pragma once

//#include <Base/Template/Containers/Array/TFixedArray.h>
#include <Core/Assets/Asset.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Core/Material.h>	// NwShaderEffect::Pass


namespace Rendering
{
/// register index for idTech4's material script expression evaluation
typedef U16	RegisterIndex_t;

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.

// Doom3 uses 4096
//const int MAX_EXPRESSION_OPS = 4096;
//const int MAX_EXPRESSION_REGISTERS = 4096;
const int MAX_EXPRESSION_OPS = 1024;
const int MAX_EXPRESSION_REGISTERS = 1024;


// note: keep opNames[] in sync with changes
enum expOpType_t: U16
{
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_TABLE,
	OP_TYPE_GT,
	OP_TYPE_GE,
	OP_TYPE_LT,
	OP_TYPE_LE,
	OP_TYPE_EQ,
	OP_TYPE_NE,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_SOUND
};

typedef enum
{
	EXP_REG_TIME,

	EXP_REG_PARM0,
	EXP_REG_PARM1,
	EXP_REG_PARM2,
	EXP_REG_PARM3,
	EXP_REG_PARM4,
	EXP_REG_PARM5,
	EXP_REG_PARM6,
	EXP_REG_PARM7,
	EXP_REG_PARM8,
	EXP_REG_PARM9,
	EXP_REG_PARM10,
	EXP_REG_PARM11,

	EXP_REG_GLOBAL0,
	EXP_REG_GLOBAL1,
	EXP_REG_GLOBAL2,
	EXP_REG_GLOBAL3,
	EXP_REG_GLOBAL4,
	EXP_REG_GLOBAL5,
	EXP_REG_GLOBAL6,
	EXP_REG_GLOBAL7,

	EXP_REG_NUM_PREDEFINED
} expRegister_t;

struct expOp_t: CStruct
{
	expOpType_t			opType;	
	RegisterIndex_t		lhs, rhs, res;

public:
	mxDECLARE_CLASS(expOp_t, CStruct);
	mxDECLARE_REFLECTION;

	void dbgprint( U32 i ) const;
};
ASSERT_SIZEOF( expOp_t, 8 );

struct idMtrTableRef: CStruct
{
	AssetID	table_id;
	U32		op_index;	// index of the expOp_t
public:
	mxDECLARE_CLASS(idMtrTableRef, CStruct);
	mxDECLARE_REFLECTION;
};

const int MAX_ENTITY_SHADER_PARMS	= 12;	// see EXP_REG_PARM11
const int MAX_GLOBAL_SHADER_PARMS	= 8;	// see EXP_REG_GLOBAL7

struct idLocalShaderParameters: CStruct
{
	float	f[ MAX_ENTITY_SHADER_PARMS ];

public:
	mxDECLARE_CLASS(idLocalShaderParameters, CStruct);
	mxDECLARE_REFLECTION;
};

struct idGlobalShaderParameters: CStruct
{
	float	f[ MAX_GLOBAL_SHADER_PARMS ];
};


mxSTOLEN("Doom3 BFG source code, ./neo/renderer/RenderWorld.h")

// shader parms
const int SHADERPARM_RED			= 0;
const int SHADERPARM_GREEN			= 1;
const int SHADERPARM_BLUE			= 2;
const int SHADERPARM_ALPHA			= 3;
const int SHADERPARM_TIMESCALE		= 3;
const int SHADERPARM_TIMEOFFSET		= 4;
const int SHADERPARM_DIVERSITY		= 5;	// random between 0.0 and 1.0 for some effects (muzzle flashes, etc)
const int SHADERPARM_MODE			= 7;	// for selecting which shader passes to enable
const int SHADERPARM_TIME_OF_DEATH	= 7;	// for the monster skin-burn-away effect enable and time offset

// model parms
const int SHADERPARM_MD3_FRAME		= 8;
const int SHADERPARM_MD3_LASTFRAME	= 9;
const int SHADERPARM_MD3_BACKLERP	= 10;

const int SHADERPARM_BEAM_END_X		= 8;	// for _beam models
const int SHADERPARM_BEAM_END_Y		= 9;
const int SHADERPARM_BEAM_END_Z		= 10;
const int SHADERPARM_BEAM_WIDTH		= 11;

const int SHADERPARM_SPRITE_WIDTH	= 8;
const int SHADERPARM_SPRITE_HEIGHT	= 9;

const int SHADERPARM_PARTICLE_STOPTIME = 8;	// don't spawn any more particles after this time



struct idExpressionEvaluationScratchpad
{
	mxPREALIGN(16) float	regs[ MAX_EXPRESSION_REGISTERS ];
};



mxSTOLEN("Doom3 BFG source code, ./neo/framework/DeclTable.h")
/*
===============================================================================

	tables are used to map a floating point input value to a floating point
	output value, with optional wrap / clamp and interpolation

===============================================================================
*/
class idDeclTable: public NwResource {
public:
	mxDECLARE_CLASS( idDeclTable, NwResource );
	mxDECLARE_REFLECTION;

	idDeclTable();

	float	TableLookup( float index ) const;

public_internal:
	AssetID				_name;

	RegisterIndex_t		_index;

	/// don't wrap around if the index is outside the range of table elements,
	/// (return the first value if less or the last value if it's more)
	bool				_clamp;

	/// jump directly from one value to the other (don't blend between values)
	bool				_snap;

	/// data lookup table then for referencing from materials
	TBuffer< float >	_values;
};


class idDeclTableList: CStruct {
public:
	mxDECLARE_CLASS( idDeclTableList, CStruct );
	mxDECLARE_REFLECTION;

	idDeclTableList();

	static AssetID getAssetId();

public_internal:
	DynamicArray< idDeclTable >		tables;
};


struct idMaterialFlags
{
	enum Enum
	{
		// true if EvaluateExpressions() doesn't have to be called.
		// will be false if material's ops ever reference globalParms or entityParms
		HasOnlyConstantRegisters	= BIT(0),
	};
};
mxDECLARE_FLAGS( idMaterialFlags::Enum, U32, idMaterialFlagsT );

struct idMaterialPassFlags
{
	enum Enum
	{
		HasTextureTransform	= BIT(0),
		HasAlphaTest		= BIT(1),
	};
};
mxDECLARE_FLAGS( idMaterialPassFlags::Enum, U16, idMaterialPassFlagsT );


// must be synced with the shader effect (see idMaterial::EFFECT_FILENAME)
mxPREALIGN(16)
struct idMaterialUniforms: CStruct
{
	V4f		texture_matrix_row0;
	V4f		texture_matrix_row1;
	V3f		color_rgb;
	float	alpha_threshold;

public:
	mxDECLARE_CLASS( idMaterialUniforms, CStruct );
	mxDECLARE_REFLECTION;
};
ASSERT_SIZEOF(idMaterialUniforms, 48);

//struct idTextureStage
//{
//	// we only allow a subset of the full projection matrix
//	// our reflection system does not support 2D arrays yet, so use these instead of a 2x3 TextureMatrix
//	RegisterIndex_t			texture_matrix_row0[3];
//	RegisterIndex_t			texture_matrix_row1[3];
//};

// analog of shaderStage_t
struct idMaterialPass: CStruct
{
	// if registers[conditionRegister] == 0, skip stage
	RegisterIndex_t		condition_register;	//2

	//
	idMaterialPassFlagsT	flags;	//2

	// only if HasAlphaTest flag is set
	RegisterIndex_t			alpha_test_register;	//2

	HProgram				program_handle;	//2

	// only if HasTextureTransform flag is set
	//idTextureStage		texture_stage;
	RegisterIndex_t			texture_matrix_row0[3];
	RegisterIndex_t			texture_matrix_row1[3];

	mxOPTIMIZE("cache texture handles to avoid indirection");
	mxOPTIMIZE("or, better, build a command buffer to avoid checks at runtime");
	TResPtr< NwTexture >	diffuse_map;
	TResPtr< NwTexture >	normal_map;	// can be nil
	TResPtr< NwTexture >	specular_map;	// can be nil

	mxOPTIMIZE("cache to avoid indirection");
	//TResPtr< NwRenderState >	render_state;
	NwRenderState32				render_state;

	NGpu::LayerID					view_id;

	//32: 52

public:
	mxDECLARE_CLASS( idMaterialPass, CStruct );
	mxDECLARE_REFLECTION;

	idMaterialPass();
};


/*
-----------------------------------------------------------------------------
	idMaterial
-----------------------------------------------------------------------------
*/
struct idMaterial: NwSharedResource
{
	idMaterialFlagsT	flags;	//4

	/// pointer to shader effect, must always be valid
	TResPtr< NwShaderEffect >	effect;

	// for debugging
	AssetID			id;


	/// loaded as a single memory blob
	struct LoadedData: CStruct
	{
		// make sure to align the read/write cursor when the serialized data follows a string
		enum { ALIGNMENT = 16 };

		TBuffer< expOp_t >		ops;
		TBuffer< float >		expression_registers;

		/// An idTech 4 material might have multiple passes.
		TBuffer< idMaterialPass >	passes;

	public:
		mxDECLARE_CLASS(LoadedData, CStruct);
		mxDECLARE_REFLECTION;
		LoadedData();

		void dbgprint() const;
	};

	LoadedData *	data;

	//32: 24

public:
	mxDECLARE_CLASS(idMaterial, NwSharedResource);
	mxDECLARE_REFLECTION;
	idMaterial();
	
	ERet setUniform(
		const NameHash32 name_hash
		, const void* uniform_data
		);

public:
	static const char* EFFECT_FILENAME;
};

/*
----------------------------------------------------------
	MaterialLoader
----------------------------------------------------------
*/
class idMaterialLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	idMaterialLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;

public:

	enum { SIGNATURE = MCHAR4('M','T','R','4') };

#pragma pack (push,1)
	struct Header_d
	{
		U32					fourCC;
		idMaterialFlagsT	flags;
		U32					unused[2];
	};
	ASSERT_SIZEOF(Header_d, 16);
#pragma pack (pop)

};


/*
-----------------------------------------------------------------------------
	idMaterialSystem
-----------------------------------------------------------------------------
*/
class idMaterialSystem: TSingleInstance< idMaterialSystem >
{
public:
	// can be used to automatically effect every material in the world that references globalParms
	idGlobalShaderParameters	global_shader_parms;

	TPtr< idDeclTableList >		tables;

	//
	float _absolute_time_in_seconds;

public:
	idMaterialSystem();

	ERet Initialize();
	void Shutdown();

	void Update(
		const NwTime& game_time
		);

	void EvaluateExpressions(
		idMaterial & material
		, const idLocalShaderParameters& shader_params
		, idExpressionEvaluationScratchpad & scratchpad
		) const;

private:
	void _SetupResourceLoaders( AllocatorI & allocator );
	void _CloseResourceLoaders( AllocatorI & allocator );
};

}//namespace Rendering
