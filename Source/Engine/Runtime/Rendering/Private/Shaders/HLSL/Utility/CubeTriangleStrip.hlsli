

// Single tri-strip cube, from:
//  Interesting CUBE triangle strip
//   http://www.asmcommunity.net/board/index.php?action=printpage;topic=6284.0
// ; 1-------3-------4-------2   Cube = 8 vertices
// ; |  E __/|\__ A  |  H __/|   =================
// ; | __/   |   \__ | __/   |   Single Strip: 4 3 7 8 5 3 1 4 2 7 6 5 2 1
// ; |/   D  |  B   \|/   I  |   12 triangles:     A B C D E F G H I J K L
// ; 5-------8-------7-------6
// ;         |  C __/|
// ;         | __/   |
// ;         |/   J  |
// ;         5-------6
// ;         |\__ K  |
// ;         |   \__ |
// ;         |  L   \|         Left  D+E
// ;         1-------2        Right  H+I
// ;         |\__ G  |         Back  K+L
// ;         |   \__ |        Front  A+B
// ;         |  F   \|          Top  F+G
// ;         3-------4       Bottom  C+J
// ;
static const int CUBE_TRISTRIP_INDICES[14] =
{
   //4, 3, 7, 8, 5, 3, 1, 4, 2, 7, 6, 5, 2, 1,

   3, 2, 6, 7, 4, 2, 0, 3, 1, 6, 5, 4, 1, 0,
};

//     3   4
//   1   2
//     8   7
//   5   6
static const float3 STRIPIFIED_UNIT_CUBE_VERTS[8] =
{
    float3(-1,-1,+1),
    float3(+1,-1,+1),
    float3(-1,+1,+1),
    float3(+1,+1,+1),

	float3(-1,-1,-1),
    float3(+1,-1,-1),
    float3(+1,+1,-1),
    float3(-1,+1,-1),
};


// Creates a unit cube triangle strip from just vertex ID (14 vertices)
// https://twitter.com/Donzanoid/status/616370134278606848
// https://www.gamedev.net/forums/topic/674733-vertex-to-cube-using-geometry-shader/?do=findComment&comment=5271097
// https://gist.github.com/XProger/66e53d69c7f861a7b2a2
// http://www.asmcommunity.net/forums/topic/?id=6284.
//
inline float3 GetUnitCubeCornerByVertexID( in uint vertexID )
{
	uint b = 1 << vertexID;
	return float3(
		(0x287a & b) != 0,
		(0x02af & b) != 0,
		(0x31e3 & b) != 0
	);
}
