/*
=============================================================================

=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Util/Cubemap_Util.h>

namespace Cubemap_Util
{

///
///
///              +----------+
///              | +---->+x |
///              | |        |
///              | |  +y    |
///              |+z      2 |
///   +----------+----------+----------+----------+
///   | +---->+z | +---->+x | +---->-z | +---->-x |
///   | |        | |        | |        | |        |
///   | |  -x    | |  +z    | |  +x    | |  -z    |
///   |-y      1 |-y      4 |-y      0 |-y      5 |
///   +----------+----------+----------+----------+
///              | +---->+x |
///              | |        |
///              | |  -y    |
///              |-z      3 |
///              +----------+
///
const float s_faceUvVectors[6][3][3] =
{
	{ // +x face
        {  0.0f,  0.0f, -1.0f }, // u -> -z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  1.0f,  0.0f,  0.0f }, // +x face
    },
	{ // -x face
        {  0.0f,  0.0f,  1.0f }, // u -> +z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        { -1.0f,  0.0f,  0.0f }, // -x face
    },

    { // +y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f,  1.0f }, // v -> +z
        {  0.0f,  1.0f,  0.0f }, // +y face
    },
    { // -y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f, -1.0f }, // v -> -z
        {  0.0f, -1.0f,  0.0f }, // -y face
    },

    { // +z face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f,  1.0f }, // +z face
    },
    { // -z face
        { -1.0f,  0.0f,  0.0f }, // u -> -x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f, -1.0f }, // -z face
    },
};

}//namespace Cubemap_Util
