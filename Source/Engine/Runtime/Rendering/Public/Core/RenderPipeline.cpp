#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Public/Core/RenderPipeline.h>


namespace Rendering
{

struct KnownScenePass {
	const char*	name;
	NameHash32	nameHash;
} gs_knownScenePasses[] = {
	#define DECLARE_SCENE_PASS( NAME, FILTER )    { #NAME, mxHASH_STR(TO_STR(##NAME)) },
	  #include <Rendering/Public/Core/RenderPipeline.inl>
	#undef DECLARE_SCENE_PASS
};

const KnownScenePass* findKnownScenePassByNameHash( const NameHash32 nameHash )
{
	for( UINT i = 0; i < mxCOUNT_OF(gs_knownScenePasses); i++ )
	{
		if( gs_knownScenePasses[i].nameHash == nameHash ) {
			return &gs_knownScenePasses[i];
		}
	}
	return nil;
}

mxBEGIN_REFLECT_ENUM( SortModeT )
	mxREFLECT_ENUM_ITEM( FrontToBack, ESortMode::Sort_FrontToBack ),
	mxREFLECT_ENUM_ITEM( BackToFront, ESortMode::Sort_BackToFront ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(ScenePassInfo);
mxBEGIN_REFLECTION(ScenePassInfo)
	mxMEMBER_FIELD(draw_order),
	mxMEMBER_FIELD(num_sublayers),
	mxMEMBER_FIELD(passDataIndex),
mxEND_REFLECTION;
ScenePassInfo::ScenePassInfo()
{
	draw_order = 0;
	num_sublayers = 1;
	filter_index = 0;
	passDataIndex = 0;
}

mxDEFINE_CLASS(ScenePassData);
mxBEGIN_REFLECTION(ScenePassData)
	mxMEMBER_FIELD(sortedNameHashIndex),
	mxMEMBER_FIELD(render_targets),
	mxMEMBER_FIELD(depth_stencil_name_hash),
	mxMEMBER_FIELD(profiling_scope),
	mxMEMBER_FIELD(view_flags),
	mxMEMBER_FIELD(depth_clear_value),
	mxMEMBER_FIELD(stencil_clear_value),
mxEND_REFLECTION;
ScenePassData::ScenePassData()
{
	sortedNameHashIndex = -1;

	depth_stencil_name_hash = 0;
	view_flags = 0;
	depth_clear_value = 1.0f;
	stencil_clear_value = 0;
}

mxDEFINE_CLASS(RenderPath);
mxBEGIN_REFLECTION(RenderPath)
	mxMEMBER_FIELD(sortedPassNameHashes),
	mxMEMBER_FIELD(passInfo),
	mxMEMBER_FIELD(passData),
mxEND_REFLECTION;

void RenderPath::setNumSubpasses( const ScenePassHandle handle, U8 num_sublayers )
{
	mxASSERT(num_sublayers >= 1);
	passInfo[ handle.id ].num_sublayers = num_sublayers;
}

void RenderPath::recalculatePassDrawingOrder()
{
	const UINT numPasses = sortedPassNameHashes.num();

	UINT currentDrawingOrder = 0;

	for( UINT iPass = 0; iPass < numPasses; iPass++ )
	{
		ScenePassData & srcPass = passData[iPass];

		//
		ScenePassInfo	&dstScenePassInfo = passInfo[srcPass.sortedNameHashIndex];
		dstScenePassInfo.draw_order = currentDrawingOrder;
		mxASSERT(dstScenePassInfo.num_sublayers >= 1);

		mxASSERT2(iPass <= 32, "TbPassMaskT has too few bits!");
		dstScenePassInfo.filter_index = iPass;

		//
		const NameHash32 namehash = sortedPassNameHashes[srcPass.sortedNameHashIndex];
		const KnownScenePass* knownScenePass = findKnownScenePassByNameHash( namehash );
		mxASSERT(knownScenePass);

		const char* debugname = srcPass.profiling_scope.IsEmpty() ? knownScenePass->name : srcPass.profiling_scope.c_str();

		DBGOUT("[%d]Pass '%s': id = 0x%p, order = %d (%d subpasses)"
				,iPass, debugname, namehash, currentDrawingOrder, dstScenePassInfo.num_sublayers);

#if MX_DEVELOPER
		for( UINT iSubPass = 0; iSubPass < dstScenePassInfo.num_sublayers; iSubPass++ )
		{
			String64	temp;
			Str::Copy( temp, srcPass.profiling_scope );
			Str::Format( srcPass.profiling_scope, "%s (%d: %d)", temp.c_str(), currentDrawingOrder, iSubPass );
		}
#endif // MX_DEVELOPER

		currentDrawingOrder += dstScenePassInfo.num_sublayers;

		mxASSERT(currentDrawingOrder <= MAX_UINT8);
	}
}

}//namespace Rendering
