#include <Base/Base.h>
#pragma hdrstop
#include <Graphics/Public/graphics_utilities.h>	// AuxVertex
#include <Rendering/Public/Core/VertexFormats.h>


TbVertexFormat *	TbVertexFormat::s_all = nil;

/*
===============================================================================
*/

//{
//	IF_DEVELOPER vertex_description.name.setReference("Pos3F");
//	vertex_description.Begin();
//	vertex_description.add(AttributeType::Float3, AttributeUsage::Position, 0);
//	vertex_description.End();
//	inputLayouts[VTX_Pos3F] = GL::CreateInputLayout(vertex_description);
//}
//{
//	IF_DEVELOPER vertex_description.name.setReference("Pos4F");
//	vertex_description.Begin();
//	vertex_description.add(AttributeType::Float4, AttributeUsage::Position, 0);
//	vertex_description.End();
//	inputLayouts[VTX_Pos4F] = GL::CreateInputLayout(vertex_description);
//}
//{
//	IF_DEVELOPER vertex_description.name.setReference("Pos3F_Tex2F");
//	vertex_description.Begin();
//	vertex_description.add(AttributeType::Float3, AttributeUsage::Position, 0);
//	vertex_description.add(AttributeType::Float2, AttributeUsage::TexCoord, 0);
//	vertex_description.End();
//	inputLayouts[VTX_Pos3F_Tex2F] = GL::CreateInputLayout(vertex_description);
//}
//{
//	IF_DEVELOPER vertex_description.name.setReference("Pos4F_Tex2F");
//	vertex_description.Begin();
//	vertex_description.add(AttributeType::Float4, AttributeUsage::Position, 0);
//	vertex_description.add(AttributeType::Float2, AttributeUsage::TexCoord, 0);
//	vertex_description.End();
//	inputLayouts[VTX_Pos4F_Tex2F] = GL::CreateInputLayout(vertex_description);
//}

tbDEFINE_VERTEX_FORMAT( Vertex_ImGui );
mxBEGIN_REFLECTION( Vertex_ImGui )
	mxMEMBER_FIELD( pos ),
	mxMEMBER_FIELD( uv ),
	mxMEMBER_FIELD( col ),
mxEND_REFLECTION
void Vertex_ImGui::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float2, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float2, NwAttributeUsage::TexCoord, 0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Color,    0, true)
		.end();
}

#if 0

tbDEFINE_VERTEX_FORMAT( Vertex_Nuklear );
mxBEGIN_REFLECTION( Vertex_Nuklear )
	mxMEMBER_FIELD( pos ),
	mxMEMBER_FIELD( uv ),
	mxMEMBER_FIELD( col ),
mxEND_REFLECTION
void Vertex_Nuklear::buildVertexDescription( VertexDescription *description_ )
{
	description_->begin()
		.add(AttributeType::Float3, AttributeUsage::Position, 0)
		.add(AttributeType::Float2, AttributeUsage::TexCoord, 0)
		.add(AttributeType::UByte4, AttributeUsage::Color,    0, true)
		.end();
}

#endif

tbDEFINE_VERTEX_FORMAT(DrawVertex);
mxBEGIN_REFLECTION( DrawVertex )
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( uv ),
	mxMEMBER_FIELD( N ),
	mxMEMBER_FIELD( T ),
	mxMEMBER_FIELD( indices ),
	mxMEMBER_FIELD( weights ),
mxEND_REFLECTION
void DrawVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Half2,  NwAttributeUsage::TexCoord, 0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Normal,   0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Tangent,  0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::BoneIndices, 0, false)
		.add(NwAttributeType::UByte4, NwAttributeUsage::BoneWeights, 0, true)
		.end();
}

#if 0
void VoxelVertex8::buildVertexDescription( VertexDescription *description_ )
{
	
	description_->add(AttributeType::Float1, 1, VertexAttribute::Position, false, 0);
	description_->add(AttributeType::UByte4, VertexAttribute::Normal, false, 0);
	
}
void VTX_Morph_Fat::buildVertexDescription( VertexDescription *description_ )
{
	
	description_->add(AttributeType::Float3, AttributeUsage::Position, 0);
	description_->add(AttributeType::Float3, AttributeUsage::TexCoord, 0);
	
}
#endif

tbDEFINE_VERTEX_FORMAT(Vertex_DLOD);
mxBEGIN_REFLECTION( Vertex_DLOD )
	mxMEMBER_FIELD( xy ),
	mxMEMBER_FIELD( z_and_mat ),
	mxMEMBER_FIELD( N ),
	mxMEMBER_FIELD( color ),
mxEND_REFLECTION
void Vertex_DLOD::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::UInt1, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::UInt1, NwAttributeUsage::Position, 1)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Normal, 0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Color, 0, true)
		.end();
}

#if CLOD_USE_PACKED_VERTICES
	void Vertex_CLOD::buildVertexDescription( NwVertexDescription *description_ )
	{
		
		description_->add(NwAttributeType::UInt1, NwAttributeUsage::Position, 0);
		description_->add(NwAttributeType::UInt1, NwAttributeUsage::Position, 1);
		description_->add(NwAttributeType::UByte4, NwAttributeUsage::Normal, 0);
		description_->add(NwAttributeType::UByte4, NwAttributeUsage::Normal, 1);
		description_->add(NwAttributeType::UByte4, NwAttributeUsage::Color, 0, true);
		description_->add(NwAttributeType::UByte4, NwAttributeUsage::Color, 1, true);
		description_->add(NwAttributeType::UByte4, NwAttributeUsage::TexCoord, 0);
		description_->add(NwAttributeType::UInt1, NwAttributeUsage::TexCoord, 1);
		
	}
#else
	void Vertex_CLOD::buildVertexDescription( NwVertexDescription *description_ )
	{
		description_->begin()
			.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
			.add(NwAttributeType::UByte4, NwAttributeUsage::Normal, 0)
			.add(NwAttributeType::Float3, NwAttributeUsage::Position, 1)		
			.add(NwAttributeType::UByte4, NwAttributeUsage::Normal, 1)
			.add(NwAttributeType::UByte4, NwAttributeUsage::Color, 0, true)
			.add(NwAttributeType::UByte4, NwAttributeUsage::Color, 1, true)
			.add(NwAttributeType::UByte4, NwAttributeUsage::TexCoord, 0)
			.add(NwAttributeType::UInt1, NwAttributeUsage::TexCoord, 1)
			.end();
	}
#endif

void P3f::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.end();
}

tbDEFINE_VERTEX_FORMAT(P3f_TEX2f);
mxBEGIN_REFLECTION( P3f_TEX2f )
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( uv ),
mxEND_REFLECTION
void P3f_TEX2f::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float2, NwAttributeUsage::TexCoord, 0)
		.end();
}

void VTX_P3f_TEX2h::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Half2, NwAttributeUsage::TexCoord, 0)
		.end();
}

tbDEFINE_VERTEX_FORMAT(Vertex_Skinned);
mxBEGIN_REFLECTION( Vertex_Skinned )
	mxMEMBER_FIELD( position ),
	mxMEMBER_FIELD( texCoord ),
	mxMEMBER_FIELD( normal ),
	mxMEMBER_FIELD( tangent ),
	mxMEMBER_FIELD( indices ),
	mxMEMBER_FIELD( weights ),
mxEND_REFLECTION
void Vertex_Skinned::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Half2,  NwAttributeUsage::TexCoord, 0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Normal,   0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Tangent,  0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::BoneIndices, 0, false)
		.add(NwAttributeType::UByte4, NwAttributeUsage::BoneWeights, 0, true)
		.end();
}

tbDEFINE_VERTEX_FORMAT(Vertex_Static);
mxBEGIN_REFLECTION( Vertex_Static )
	mxMEMBER_FIELD( position ),
	mxMEMBER_FIELD( tex_coord ),
	mxMEMBER_FIELD( normal ),
	mxMEMBER_FIELD( tangent ),
	mxMEMBER_FIELD( color ),
	mxMEMBER_FIELD( normal_for_UV_calc ),
mxEND_REFLECTION
void Vertex_Static::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Half2,  NwAttributeUsage::TexCoord, 0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Normal,   0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Tangent,  0)
		.add(NwAttributeType::UByte4, NwAttributeUsage::Color, 0, true)
		.add(NwAttributeType::R10G10B10A2_UNORM, NwAttributeUsage::Normal, 1, true)
		.end();

	//
	mxASSERT(description_->streamStrides[0] == sizeof(Vertex_Static));
}

tbDEFINE_VERTEX_FORMAT(ParticleVertex);
mxBEGIN_REFLECTION( ParticleVertex )
	mxMEMBER_FIELD( position_and_size ),
	mxMEMBER_FIELD( color_and_life ),
mxEND_REFLECTION
void ParticleVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float4, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0)
		.end();
}

#if 0

tbDEFINE_VERTEX_FORMAT(MeshStampVertex);
mxBEGIN_REFLECTION( MeshStampVertex )
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( uv ),
mxEND_REFLECTION
void MeshStampVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float2, NwAttributeUsage::TexCoord, 0)
		.end();
}

#else

tbDEFINE_VERTEX_FORMAT(MeshStampVertex);
mxBEGIN_REFLECTION( MeshStampVertex )
	mxMEMBER_FIELD( xyz ),
mxEND_REFLECTION
void MeshStampVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float3, NwAttributeUsage::Position, 0)
		.end();
}

#endif





tbDEFINE_VERTEX_FORMAT(SpaceshipVertex);
mxBEGIN_REFLECTION( SpaceshipVertex )
	mxMEMBER_FIELD( position ),
	//mxMEMBER_FIELD( uv ),
	//mxMEMBER_FIELD( normal ),
mxEND_REFLECTION
void SpaceshipVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Half4,	NwAttributeUsage::Position, 0)
		//.add(NwAttributeType::Half2,	NwAttributeUsage::TexCoord, 0)
		//.add(NwAttributeType::UByte4,	NwAttributeUsage::Normal,   0)
		.end();
	//
	mxASSERT(description_->streamStrides[0] == sizeof(SpaceshipVertex));
}





TbVertexFormat	AuxVertex_vertex_format(
										mxEXTRACT_TYPE_NAME(AuxVertex),
										mxEXTRACT_TYPE_GUID(AuxVertex),
										nil,
										SClassDescription::For_Class_With_Default_Ctor< AuxVertex >(),
										TbClassLayout::dummy,
										&AuxVertex::buildVertexDescription
										);


tbDEFINE_VERTEX_FORMAT(RrBillboardVertex);
mxBEGIN_REFLECTION( RrBillboardVertex )
	mxMEMBER_FIELD( position_and_size ),
	mxMEMBER_FIELD( color_and_life ),
mxEND_REFLECTION
void RrBillboardVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float4, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0)
		.end();
}
