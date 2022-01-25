
/*
=============================================================================
	idTech4 (Doom 3) shader/material support
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Client.h>	// NwTime
#include <GPU/Public/graphics_device.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>	// NwShaderEffectData
#include <Rendering/Public/Globals.h>
#include <Rendering/Private/Modules/idTech4/idMaterial.h>


namespace Rendering
{

namespace
{
	AllocatorI & materialDataHeap() { return MemoryHeaps::renderer(); }
}

mxDEFINE_CLASS( expOp_t );
mxBEGIN_REFLECTION( expOp_t )
	mxMEMBER_FIELD_OF_TYPE(opType, T_DeduceTypeInfo<U64>()),
	mxMEMBER_FIELD(lhs),
	mxMEMBER_FIELD(rhs),
	mxMEMBER_FIELD(res),
mxEND_REFLECTION

void expOp_t::dbgprint( U32 i ) const
{
	const expOp_t* op = this;

	switch( op->opType )
	{
		case OP_TYPE_ADD:
			DEVOUT("Op[%d]: ADD( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_SUBTRACT:
			DEVOUT("Op[%d]: SUB( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_MULTIPLY:
			DEVOUT("Op[%d]: MUL( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_DIVIDE:
			DEVOUT("Op[%d]: DIV( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_MOD:
			DEVOUT("Op[%d]: MOD( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_TABLE:
			DEVOUT("Op[%d]: TBL( %d )[ %d ] -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_SOUND:
			DEVOUT("Op[%d]: SoundAmplitude -> %d", i, op->res);
			break;
		case OP_TYPE_GT:
			DEVOUT("Op[%d]: GT( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_GE:
			DEVOUT("Op[%d]: GE( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_LT:
			DEVOUT("Op[%d]: LT( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_LE:
			DEVOUT("Op[%d]: LE( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_EQ:
			DEVOUT("Op[%d]: EQ( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_NE:
			DEVOUT("Op[%d]: NE( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_AND:
			DEVOUT("Op[%d]: AND( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
		case OP_TYPE_OR:
			DEVOUT("Op[%d]: OR( %d, %d ) -> %d", i, op->lhs, op->rhs, op->res);
			break;
			mxDEFAULT_UNREACHABLE(break);
	}
}

mxDEFINE_CLASS( idMtrTableRef );
mxBEGIN_REFLECTION( idMtrTableRef )
	mxMEMBER_FIELD(table_id),
	mxMEMBER_FIELD(op_index),
mxEND_REFLECTION

mxBEGIN_FLAGS( idMaterialFlagsT )
	mxREFLECT_BIT( HasOnlyConstantRegisters,	idMaterialFlags::HasOnlyConstantRegisters ),
mxEND_FLAGS


mxBEGIN_FLAGS( idMaterialPassFlagsT )
	mxREFLECT_BIT( HasTextureTransform,	idMaterialPassFlags::HasTextureTransform ),
	mxREFLECT_BIT( HasAlphaTest,		idMaterialPassFlags::HasAlphaTest ),
mxEND_FLAGS

mxDEFINE_CLASS( idLocalShaderParameters );
mxBEGIN_REFLECTION( idLocalShaderParameters )
	mxMEMBER_FIELD(f),
mxEND_REFLECTION


/*
----------------------------------------------------------
	idDeclTable
----------------------------------------------------------
*/
mxDEFINE_CLASS( idDeclTable );
mxBEGIN_REFLECTION( idDeclTable )
	mxMEMBER_FIELD(_name),
	mxMEMBER_FIELD(_index),
	mxMEMBER_FIELD(_clamp),
	mxMEMBER_FIELD(_snap),
	mxMEMBER_FIELD(_values),
mxEND_REFLECTION
idDeclTable::idDeclTable()
{
	_index = 0;
	_clamp = false;
	_snap = false;
}

/*
========================
idMath::Ftoi
========================
*/
static mxFORCEINLINE int idMath_Ftoi( float f ) {
	// If a converted result is larger than the maximum signed doubleword integer,
	// the floating-point invalid exception is raised, and if this exception is masked,
	// the indefinite integer value (80000000H) is returned.
	__m128 x = _mm_load_ss( &f );
	return _mm_cvttss_si32( x );
}

/*
=================
idDeclTable::TableLookup
=================
*/
float idDeclTable::TableLookup( float index ) const {
	int iIndex;
	float iFrac;

	const int domain = _values.num() - 1;

	if ( domain <= 1 ) {
		return 1.0f;
	}

	if ( _clamp ) {
		index *= (domain-1);
		if ( index >= domain - 1 ) {
			return _values[domain - 1];
		} else if ( index <= 0 ) {
			return _values[0];
		}
		iIndex = idMath_Ftoi( index );
		iFrac = index - iIndex;
	} else {
		index *= domain;

		if ( index < 0 ) {
			index += domain * ceilf( -index / domain );
		}

		iIndex = idMath_Ftoi( floorf( index ) );
		iFrac = index - iIndex;
		iIndex = iIndex % domain;
	}

	if ( !_snap ) {
		// we duplicated the 0 index at the end at creation time, so we
		// don't need to worry about wrapping the filter
		return _values[iIndex] * ( 1.0f - iFrac ) + _values[iIndex + 1] * iFrac;
	}

	return _values[iIndex];
}

mxDEFINE_CLASS( idDeclTableList );
mxBEGIN_REFLECTION( idDeclTableList )
	mxMEMBER_FIELD(tables),
mxEND_REFLECTION
idDeclTableList::idDeclTableList()
	: tables( materialDataHeap() )
{
}

AssetID idDeclTableList::getAssetId()
{
	return make_AssetID_from_raw_string("doom3_material_tables");
}


mxDEFINE_CLASS(idMaterialUniforms);
mxBEGIN_REFLECTION(idMaterialUniforms)
	mxMEMBER_FIELD(texture_matrix_row0),
	mxMEMBER_FIELD(texture_matrix_row1),
	mxMEMBER_FIELD(color_rgb),
	mxMEMBER_FIELD(alpha_threshold),
mxEND_REFLECTION

/*
-----------------------------------------------------------------------------
	idMaterialPass
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( idMaterialPass );
mxBEGIN_REFLECTION( idMaterialPass )
	mxMEMBER_FIELD(condition_register),

	mxMEMBER_FIELD(flags),

	mxMEMBER_FIELD(alpha_test_register),

	mxMEMBER_FIELD(program_handle),

	mxMEMBER_FIELD(texture_matrix_row0),
	mxMEMBER_FIELD(texture_matrix_row1),

	mxMEMBER_FIELD(diffuse_map),
	mxMEMBER_FIELD(normal_map),
	mxMEMBER_FIELD(specular_map),

	mxMEMBER_FIELD(render_state),

	mxMEMBER_FIELD(view_id),
mxEND_REFLECTION

//tbPRINT_SIZE_OF(idMaterialPass);

idMaterialPass::idMaterialPass()
{
	condition_register = ~0;
	flags.raw_value = 0;
	alpha_test_register = ~0;
	TSetStaticArray(texture_matrix_row0, (RegisterIndex_t)0);
	TSetStaticArray(texture_matrix_row1, (RegisterIndex_t)0);
	program_handle.SetNil();
}

/*
-----------------------------------------------------------------------------
	idMaterial
-----------------------------------------------------------------------------
*/

mxDEFINE_CLASS( idMaterial );
mxBEGIN_REFLECTION( idMaterial )
	//mxMEMBER_FIELD(flags),
	//mxMEMBER_FIELD(condition_register),
	//mxMEMBER_FIELD(ops),
	//mxMEMBER_FIELD(expression_registers),
	//mxMEMBER_FIELD(texture_matrix_row0),
	//mxMEMBER_FIELD(texture_matrix_row1),
mxEND_REFLECTION

//tbPRINT_SIZE_OF(idMaterial);

idMaterial::idMaterial()
{
}

ERet idMaterial::setUniform(
	const NameHash32 name_hash
	, const void* uniform_data
	)
{
	UNDONE;
	//mxTRY(effect->data->setUniform(
	//	name_hash
	//	, uniform_data
	//	, data->command_buffer
	//	));

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	idMaterial::LoadedData
-----------------------------------------------------------------------------
*/
const char* idMaterial::EFFECT_FILENAME = "doom3_mesh";;

mxDEFINE_CLASS(idMaterial::LoadedData);
mxBEGIN_REFLECTION(idMaterial::LoadedData)
	//mxMEMBER_FIELD(flags),
	//mxMEMBER_FIELD(condition_register),
	mxMEMBER_FIELD(ops),
	mxMEMBER_FIELD(expression_registers),
	mxMEMBER_FIELD(passes),

	//mxMEMBER_FIELD(texture_matrix_row0),
	//mxMEMBER_FIELD(texture_matrix_row1),

	//mxMEMBER_FIELD(command_buffer),
	//mxMEMBER_FIELD(referenced_textures),
	//mxMEMBER_FIELD(render_state),
	//mxMEMBER_FIELD(selected_pass_index),
mxEND_REFLECTION;

idMaterial::LoadedData::LoadedData()
{
}

void idMaterial::LoadedData::dbgprint() const
{
	DEVOUT("%s", GFX_SRCFILE_STRING);
UNDONE;
	//DEVOUT("%d local expression registers (%d total), %d ops, condition reg = %d",
	//	expression_registers.num() - EXP_REG_NUM_PREDEFINED, expression_registers.num(), ops.num(), condition_register
	//	);

	// copy the material constants
	for ( UINT i = EXP_REG_NUM_PREDEFINED; i < expression_registers.num() ; i++ ) {
		DEVOUT("Reg[%d]: %f", i, expression_registers[i]);
	}

	//
	const expOp_t *	op = ops.raw();

	for ( UINT i = 0 ; i < ops.num(); i++, op++ ) {
		op->dbgprint( i );
	}
}

/*
-----------------------------------------------------------------------------
	idMaterialSystem
-----------------------------------------------------------------------------
*/
idMaterialSystem::idMaterialSystem()
{
	TSetStaticArray( global_shader_parms.f, 0.0f );
	_absolute_time_in_seconds = 0;
}

ERet idMaterialSystem::Initialize()
{
	_SetupResourceLoaders(materialDataHeap());

	AssetKey	key;
	key.id = idDeclTableList::getAssetId();
	key.type = idDeclTableList::metaClass().GetTypeGUID();

	AssetReader	stream;
	mxTRY(Resources::OpenFile( key, &stream ));

	mxDO(Serialization::LoadMemoryImage( tables._ptr, stream, materialDataHeap() ));

	for( UINT i = 0; i < tables->tables.num(); i++ )
	{
		idDeclTable& table = tables->tables[i];

		AssetKey	key;
		key.id = table._name;
		key.type = idDeclTable::metaClass().GetTypeGUID();

		mxDO(Resources::insert( key, &table ));
	}

	return ALL_OK;
}

void idMaterialSystem::Shutdown()
{
	_CloseResourceLoaders(materialDataHeap());

	if( tables != nil )
	{
		for( UINT i = 0; i < tables->tables.num(); i++ )
		{
			idDeclTable& table = tables->tables[i];

			AssetKey	key;
			key.id = table._name;
			key.type = idDeclTable::metaClass().GetTypeGUID();

			Resources::replaceExisting( key, nil );
		}

		mxDELETE( tables._ptr, materialDataHeap() );
		tables = nil;
	}
}

void idMaterialSystem::_SetupResourceLoaders( AllocatorI & allocator )
{
	ProxyAllocator & resource_allocator = MemoryHeaps::resources();
	idMaterial::metaClass().loader = mxNEW( allocator, idMaterialLoader, resource_allocator );
}

void idMaterialSystem::_CloseResourceLoaders( AllocatorI & allocator )
{
	mxDELETE_AND_NIL( idMaterial::metaClass().loader, allocator );
	NwClump::metaClass().loader = nil;
}

void idMaterialSystem::Update(
	const NwTime& game_time
	)
{
//ptWARN("absolute_time_milliseconds: %d", absolute_time_milliseconds);

	_absolute_time_in_seconds = game_time.real.total_elapsed;

	global_shader_parms.f[ EXP_REG_TIME ] = _absolute_time_in_seconds;
}

mxSTOLEN("Doom3 source code, Material.cpp");

/*
===============
idMaterial::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the material registers
set to their appropriate values.
===============
*/
static
void EvaluateRegisters(
	// Regs should point to a float array large enough to hold GetNumRegisters() floats.
	// FloatTime is passed in because different entities, which may be running in parallel,
	// can be in different time groups.
	float * __restrict			registers

	// expression registers from the material
	, const TSpan< float >		expression_registers
	//, const UINT		numRegisters
	//, const UINT		numOps
	//, const expOp_t * 	ops
	, const TSpan< expOp_t >	ops

	, const idLocalShaderParameters& local_shader_params
	, const idGlobalShaderParameters& global_shader_parms
	, const float	absolute_time_in_seconds
	, const idDeclTableList& tables
	//, idSoundEmitter *soundEmitter
	//, const DebugParam& dbgparm
	)
{
	// copy the material constants
	for ( UINT i = EXP_REG_NUM_PREDEFINED; i < expression_registers._count ; i++ ) {
		registers[i] = expression_registers[i];
	}

	// copy the local and global parameters
	registers[EXP_REG_TIME] = absolute_time_in_seconds;

	registers[EXP_REG_PARM0] = local_shader_params.f[0];
	registers[EXP_REG_PARM1] = local_shader_params.f[1];
	registers[EXP_REG_PARM2] = local_shader_params.f[2];
	registers[EXP_REG_PARM3] = local_shader_params.f[3];
	registers[EXP_REG_PARM4] = local_shader_params.f[4];
	registers[EXP_REG_PARM5] = local_shader_params.f[5];
	registers[EXP_REG_PARM6] = local_shader_params.f[6];
	registers[EXP_REG_PARM7] = local_shader_params.f[7];
	registers[EXP_REG_PARM8] = local_shader_params.f[8];
	registers[EXP_REG_PARM9] = local_shader_params.f[9];
	registers[EXP_REG_PARM10] = local_shader_params.f[10];
	registers[EXP_REG_PARM11] = local_shader_params.f[11];

	registers[EXP_REG_GLOBAL0] = global_shader_parms.f[0];
	registers[EXP_REG_GLOBAL1] = global_shader_parms.f[1];
	registers[EXP_REG_GLOBAL2] = global_shader_parms.f[2];
	registers[EXP_REG_GLOBAL3] = global_shader_parms.f[3];
	registers[EXP_REG_GLOBAL4] = global_shader_parms.f[4];
	registers[EXP_REG_GLOBAL5] = global_shader_parms.f[5];
	registers[EXP_REG_GLOBAL6] = global_shader_parms.f[6];
	registers[EXP_REG_GLOBAL7] = global_shader_parms.f[7];

	//
	const expOp_t *	op = ops._data;

	for ( UINT i = 0 ; i < ops._count; i++, op++ )
	{
		switch( op->opType )
		{
		case OP_TYPE_ADD:
			registers[op->res] = registers[op->lhs] + registers[op->rhs];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->res] = registers[op->lhs] - registers[op->rhs];
			break;
		case OP_TYPE_MULTIPLY:
			registers[op->res] = registers[op->lhs] * registers[op->rhs];
			break;
		case OP_TYPE_DIVIDE:
			registers[op->res] = registers[op->lhs] / registers[op->rhs];
			break;
		case OP_TYPE_MOD:
			{
				int b = (int)registers[op->rhs];
				b = b != 0 ? b : 1;
				registers[op->res] = (int)registers[op->lhs] % b;
			} break;

		case OP_TYPE_TABLE:
			{
				const idDeclTable& table = tables.tables[ op->lhs ];

				//NOTE: make sure all values are positive for safety
				//mxASSERT(registers[op->rhs] >= 0);
				const float safe_lookup = maxf( registers[op->rhs], 0 );

				registers[op->res] = table.TableLookup( safe_lookup );
//if(dbgparm.flag){
//	ptWARN("TBL(%d)[%d = %.3f]: -> {%d = %.3f}",
//		op->lhs, op->rhs, registers[op->rhs], op->res, registers[op->res]
//	);
//}
			}
			break;

		case OP_TYPE_GT:
			registers[op->res] = registers[ op->lhs ] > registers[op->rhs];
			break;
		case OP_TYPE_GE:
			registers[op->res] = registers[ op->lhs ] >= registers[op->rhs];
			break;
		case OP_TYPE_LT:
			registers[op->res] = registers[ op->lhs ] < registers[op->rhs];
			break;
		case OP_TYPE_LE:
			registers[op->res] = registers[ op->lhs ] <= registers[op->rhs];
			break;
		case OP_TYPE_EQ:
			registers[op->res] = registers[ op->lhs ] == registers[op->rhs];
			break;
		case OP_TYPE_NE:
			registers[op->res] = registers[ op->lhs ] != registers[op->rhs];
			break;
		case OP_TYPE_AND:
			registers[op->res] = registers[ op->lhs ] && registers[op->rhs];
			break;
		case OP_TYPE_OR:
			registers[op->res] = registers[ op->lhs ] || registers[op->rhs];
			break;

		case OP_TYPE_SOUND:
			UNDONE;
			//if ( r_forceSoundOpAmplitude.GetFloat() > 0 ) {
			//	registers[op->res] = r_forceSoundOpAmplitude.GetFloat();
			//} else if ( soundEmitter ) {
			//	registers[op->res] = soundEmitter->CurrentAmplitude();
			//} else {
			//	registers[op->res] = 0;
			//}
			break;

		mxDEFAULT_UNREACHABLE(break);
		}
	}
}

void idMaterialSystem::EvaluateExpressions(
	idMaterial & material
	, const idLocalShaderParameters& local_shader_params
	, idExpressionEvaluationScratchpad & scratchpad
	) const
{
	idMaterial::LoadedData &	id_material_data = *material.data;

	// process the shader expressions for conditionals / color / texcoords
	EvaluateRegisters(
		scratchpad.regs
		, Arrays::GetSpan(id_material_data.expression_registers)
		, Arrays::GetSpan(id_material_data.ops)
		, local_shader_params
		, global_shader_parms
		, _absolute_time_in_seconds
		, *tables
		);
}

/*
----------------------------------------------------------
	idMaterialLoader
----------------------------------------------------------
*/
idMaterialLoader::idMaterialLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( idMaterial::metaClass(), parent_allocator )
{
}

ERet idMaterialLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	*new_instance_ = context.object_allocator.new_< idMaterial >();
	return ALL_OK;
}

static
ERet ReadAndFixupReferencedTables(
								  idMaterial::LoadedData * material_data
								  , AReader& stream
								  )
{
	U32	num_table_refs = 0;
	mxDO(stream.Get( num_table_refs ));

	for( UINT i = 0; i < num_table_refs; i++ )
	{
		AssetKey	key;
		mxDO(readAssetID( key.id, stream ));

		key.type = idDeclTable::metaClass().GetTypeGUID();

		//
		U32	op_index = 0;
		mxDO(stream.Get( op_index ));

		//
		expOp_t & op = material_data->ops[ op_index ];

		//
		const idDeclTable* table = (idDeclTable*) Resources::find( key );
		if( table )
		{
			op.lhs = table->_index;
		}
		else
		{
			ptWARN("Failed to find table '%s'", AssetId_ToChars(key.id));
			op.lhs = ~0;	// this will cause out-of-bounds error
			return ERR_OBJECT_NOT_FOUND;
		}
	}

	return ALL_OK;
}

ERet idMaterialLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	idMaterial *material_ = static_cast< idMaterial* >( instance );

	material_->id = context.key.id;

	//
	AssetReader	reader_stream;
	mxTRY(Resources::OpenFile( context.key, &reader_stream ));

	//
	Header_d	header;
	mxDO(reader_stream.Get(header));
	mxENSURE(header.fourCC == SIGNATURE, ERR_FAILED_TO_PARSE_DATA, "");

	//
	material_->flags = header.flags;

	//
	TResPtr< NwShaderEffect >		effect(MakeAssetID(idMaterial::EFFECT_FILENAME));
	mxDO(effect.Load(&context.object_allocator));

	material_->effect = effect;

	//
	idMaterial::LoadedData *	material_data;
	mxDO(AReader_::Align( reader_stream, idMaterial::LoadedData::ALIGNMENT ));
	mxDO(Serialization::LoadMemoryImage( material_data, reader_stream, context.raw_allocator ));

	//
	const RenderPath& render_path = Globals::GetRenderPath();

	//
	const UINT num_passes = material_data->passes.num();
	nwFOR_LOOP(UINT, iPass, num_passes)
	{
		idMaterialPass &pass = material_data->passes._data[ iPass ];

		// Load textures.
		mxDO(pass.diffuse_map.Load(&context.object_allocator));

		mxOPTIMIZE("branches; use cmd buf");
		if(pass.normal_map._id.IsValid())
			mxDO(pass.normal_map.Load(&context.object_allocator));

		if(pass.specular_map._id.IsValid())
			mxDO(pass.specular_map.Load(&context.object_allocator));

		// Setp program handle.
		const NwShaderEffect::Pass& effect_pass = effect->getPasses()[ pass.view_id ];

		const ScenePassInfo pass_info = render_path.getPassInfo( effect_pass.name_hash );

		pass.program_handle = effect_pass.program_handles[ pass.program_handle.id ];
		pass.view_id = pass_info.draw_order;
	}

	//
	mxDO(AReader_::Align(reader_stream, idMaterial::LoadedData::ALIGNMENT));

	//
	// Create pass render states.
	//
	nwFOR_LOOP(UINT, iPass, num_passes)
	{
		NwRenderStateDesc	pass_render_state_desc;
		mxDO(reader_stream.Get(pass_render_state_desc));
		//
		idMaterialPass &pass = material_data->passes._data[ iPass ];
		NwRenderState32_::Create(pass.render_state, pass_render_state_desc);
	}

	//
	mxDO(ReadAndFixupReferencedTables(
		material_data
		, reader_stream
		));

	//
	material_->data = material_data;

	return ALL_OK;
}

void idMaterialLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	UNDONE;
}

ERet idMaterialLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

}//namespace Rendering
