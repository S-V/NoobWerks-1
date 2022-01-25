#include "stdafx.h"
#pragma hdrstop

#include <VoxelEngine/VoxelTools.h>

namespace VX
{

mxBEGIN_REFLECT_ENUM( VoxelTool8 )
	mxREFLECT_ENUM_ITEM( CSG_Add_Shape, VoxelToolType::CSG_Add_Shape ),
	mxREFLECT_ENUM_ITEM( CSG_Subtract_Shape, VoxelToolType::CSG_Subtract_Shape ),

	mxREFLECT_ENUM_ITEM( CSG_Add_Model, VoxelToolType::CSG_Add_Model ),
	mxREFLECT_ENUM_ITEM( CSG_Subtract_Model, VoxelToolType::CSG_Subtract_Model ),

	mxREFLECT_ENUM_ITEM( Pick_Material, VoxelToolType::Pick_Material ),
	mxREFLECT_ENUM_ITEM( Paint_Brush, VoxelToolType::Paint_Brush ),
	mxREFLECT_ENUM_ITEM( Paint_Brush_Model, VoxelToolType::Paint_Brush_Model ),

	mxREFLECT_ENUM_ITEM( SmoothGeometry, VoxelToolType::SmoothGeometry ),
	mxREFLECT_ENUM_ITEM( SharpenEdges, VoxelToolType::SharpenEdges ),

	mxREFLECT_ENUM_ITEM( Grow, VoxelToolType::Grow ),
	mxREFLECT_ENUM_ITEM( Shrink, VoxelToolType::Shrink ),

	mxREFLECT_ENUM_ITEM( Copy, VoxelToolType::Copy ),
	mxREFLECT_ENUM_ITEM( Cut, VoxelToolType::Cut ),
	mxREFLECT_ENUM_ITEM( Paste, VoxelToolType::Paste ),
mxEND_REFLECT_ENUM

mxBEGIN_REFLECT_ENUM( VoxelBrushShapeType )
	mxREFLECT_ENUM_ITEM( Sphere, VoxelBrushShape_Sphere ),
	mxREFLECT_ENUM_ITEM( Cube, VoxelBrushShape_Cube ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(SVoxelToolSettingsBase);
mxBEGIN_REFLECTION(SVoxelToolSettingsBase)
	//mxMEMBER_FIELD( scale ),
	//mxMEMBER_FIELD( distance_along_view_direction ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_ShapeBrush);
mxBEGIN_REFLECTION(SVoxelToolSettings_ShapeBrush)
	mxMEMBER_FIELD( brush_shape_type ),
	mxMEMBER_FIELD( brush_radius_in_world_units ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_CSG_Subtract);
mxBEGIN_REFLECTION(SVoxelToolSettings_CSG_Subtract)
	mxMEMBER_FIELD( custom_surface_material ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_CSG_Add);
mxBEGIN_REFLECTION(SVoxelToolSettings_CSG_Add)
	mxMEMBER_FIELD( fill_material ),
	mxMEMBER_FIELD( custom_surface_material ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_Paint_Brush);
mxBEGIN_REFLECTION(SVoxelToolSettings_Paint_Brush)
	mxMEMBER_FIELD( fill_material ),
	mxMEMBER_FIELD( custom_surface_material ),
	mxMEMBER_FIELD( paint_surface_only ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_Place_Model);
mxBEGIN_REFLECTION(SVoxelToolSettings_Place_Model)
	mxMEMBER_FIELD( fill_material ),
	mxMEMBER_FIELD( angles_degrees ),

	mxMEMBER_FIELD( model_name ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_Subtract_Model);
mxBEGIN_REFLECTION(SVoxelToolSettings_Subtract_Model)
	mxMEMBER_FIELD( fill_material ),
	mxMEMBER_FIELD( angles_degrees ),
mxEND_REFLECTION;

mxDEFINE_CLASS(SVoxelToolSettings_SmoothGeometry);
mxBEGIN_REFLECTION(SVoxelToolSettings_SmoothGeometry)
	mxMEMBER_FIELD( brush_radius_in_world_units ),
	mxMEMBER_FIELD( strength ),
	mxMEMBER_FIELD( custom_surface_material ),
mxEND_REFLECTION;


mxDEFINE_CLASS(SVoxelToolSettings_All);
mxBEGIN_REFLECTION(SVoxelToolSettings_All)
	mxMEMBER_FIELD( csg_add ),
	mxMEMBER_FIELD( csg_subtract ),

	mxMEMBER_FIELD( csg_add_model ),
	mxMEMBER_FIELD( csg_subtract_model ),

	mxMEMBER_FIELD( paint_brush ),

	mxMEMBER_FIELD( current_tool ),
mxEND_REFLECTION;

void SVoxelToolSettings_All::resetAllToDefaultSettings()
{

}

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
