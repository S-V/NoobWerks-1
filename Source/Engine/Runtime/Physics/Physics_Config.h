// Compile-time settings.
#pragma once

typedef float pxReal;



//------------------------------------------------------------------------
// PX_COLLISION_MARGIN - used by near-phase collision detection.
//
// From Havok SDK:
// Adding a radius to a shape can improve performance. The core convex-convex
// collision detection algorithm is fast when shapes are not interpenetrating, and slower when they are.
// Adding a radius makes it less likely that the shapes themselves will interpenetrate, thus reducing the
// likelihood of the slower algorithm being used. The shell is thus faster in situations where there is a risk
// of shapes interpenetrating - for instance, when an object is settling or sliding on a surface, when there is
// a stack of objects, or when many objects are jostling together.
//
mxGLOBAL_CONST pxReal COLLISION_MARGIN = pxReal(0.05);	//aka 'collision skin width'

//------------------------------------------------------------------------
// PX_CONTACT_THRESHOLD - used by near-phase collision detection.
//
// if separating distance (distance between two closest points) is larger
// than this value, the contact won't be processed.
//
mxGLOBAL_CONST pxReal CONTACT_THRESHOLD = pxReal(0.02);

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
