#include <Base/Base.h>
#pragma hdrstop
#include <algorithm>
#include <Base/Template/Containers/Blob.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Text/Lexer.h>

#include <Graphics/Public/graphics_utilities.h>	// ALineRenderer

#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_MD5_Model_Parser.h>



#define TO_FLOAT3  *(V3f*)&
#define TO_FLOAT4  *(V4f*)&

#define MAX_BONES_PER_VERTEX 4


namespace Doom3
{

static bool readOptionalToken( ATokenReader & parser, const char* token_name )
{
	TbToken	next_token;
	if( parser.PeekToken( next_token ) && next_token.type == TT_Name && Str::EqualS( next_token.text, token_name ) )
	{
		parser.ReadToken( next_token );
	}

	return true;
}

static M33f Quat_to_Matrix3x3( const Q4f& q )
{
	float	wx, wy, wz;
	float	xx, yy, yz;
	float	xy, xz, zz;
	float	x2, y2, z2;

	x2 = q.x + q.x;
	y2 = q.y + q.y;
	z2 = q.z + q.z;

	xx = q.x * x2;
	xy = q.x * y2;
	xz = q.x * z2;

	yy = q.y * y2;
	yz = q.y * z2;
	zz = q.z * z2;

	wx = q.w * x2;
	wy = q.w * y2;
	wz = q.w * z2;

	//
	M33f	mat;

	mat[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	mat[ 0 ][ 1 ] = xy - wz;
	mat[ 0 ][ 2 ] = xz + wy;

	mat[ 1 ][ 0 ] = xy + wz;
	mat[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	mat[ 1 ][ 2 ] = yz - wx;

	mat[ 2 ][ 0 ] = xz - wy;
	mat[ 2 ][ 1 ] = yz + wx;
	mat[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	return mat;
}

static M44f Quat_to_Matrix4x4( const Q4f& q )
{
	return M44_FromRotationMatrix(Quat_to_Matrix3x3(q));
}

mxBEGIN_FLAGS( FVertexFlagsT )
	mxREFLECT_BIT( vertexColor, EVertexFlags::EVF_VertexColor ),
mxEND_FLAGS;

ERet MD5_LoadModel(
			   const char* fileName
			   , md5Model &model
			   , const MD5LoadOptions& options
			   )
{
	mxASSERT(model.IsEmpty());

	NwBlob	fileData(MemoryHeaps::temporary());
	mxDO(NwBlob_::loadBlobFromFile( fileData, fileName ));

	Lexer	parser( fileData.raw(), fileData.rawSize(), fileName );

	return MD5_LoadModel(
		parser
		, model
		, options
		);
}

ERet MD5_LoadModel(
				   Lexer& parser
				   , md5Model &model
				   , const MD5LoadOptions& options
				   )
{
	int		version;
	TbToken	token;
	int		i;

	int	totalVertexCount = 0;
	int	totalIndexCount = 0;

	//MX_STATS( const int startTime = mxGetMilliseconds() );

	//if ( !parser.LoadMemory( data, size, stream->GetURI().ToChars() ) )
	//{
	//	mxErrf( "Failed to load md5 model from file '%s'.\n", stream->GetURI().ToChars() );
	//	return false;
	//}

	model.clear();

	mxDO(expectTokenString( parser, MD5_VERSION_STRING ));
	version = expectInt( parser );
	if ( version < MD5_VERSION_DOOM3 || version > MD5_VERSION_ETQW ) {
		parser.Error( "Invalid version %d.  Should be version %d or %d\n", version, MD5_VERSION_DOOM3, MD5_VERSION_ETQW );
		return ERR_UNSUPPORTED_VERSION;
	}

	const bool is_Quake_ETQW_version = ( version == MD5_VERSION_ETQW );

	// skip the command line
  	mxDO(expectTokenString( parser, "commandline" ));
	parser.ReadToken( token );

	int		numJoints = 0;
	int		numMeshes = 0;

	// parse num joints
	mxDO(expectTokenString( parser, "numJoints" ));
	numJoints = expectInt( parser  );
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse num meshes
	mxDO(expectTokenString( parser, "numMeshes" ));
	numMeshes = expectInt( parser );
	if ( numMeshes <= 0 ) {
		parser.Error( "Invalid number of meshes: %d", numMeshes );
	}

	// parse the joints
	model.joints.setNum( numJoints );
	
	md5Joint* joints = model.joints.raw();

	mxDO(expectTokenString( parser, "joints" ));
	mxDO(expectTokenString( parser, "{" ));
	// These joints define the skeleton of the model in the bind pose.
	for( i = 0; i < numJoints; i++ )
	{
		// read name
		parser.ReadToken( token );
		//joints[ i ].nameIndex = animationLib.JointIndex( token );
		joints[ i ].name = token.text;

		// parse parent num
		const int parentNum = expectInt( parser );
		if ( parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", parentNum );
		}
		joints[ i ].parentNum = parentNum;

		if ( ( i != 0 ) && ( parentNum < 0 ) ) {
			parser.Error( "Models can have only one root joint" );
		}

		// parse joint position in object local space
		mxDO(expectTokenString( parser, "("));
		joints[ i ].position.x = expectFloat( parser );
		joints[ i ].position.y = expectFloat( parser );
		joints[ i ].position.z = expectFloat( parser );
		mxDO(expectTokenString( parser, ")"));

		if(options.scale_to_meters)
		{
			joints[ i ].position *= DOOM3_TO_METERS;
		}

		// parse joint orientation in object local space
		mxDO(expectTokenString( parser, "("));
		joints[ i ].orientation.x = expectFloat( parser );
		joints[ i ].orientation.y = expectFloat( parser );
		joints[ i ].orientation.z = expectFloat( parser );
		joints[ i ].orientation.w = joints[ i ].orientation.CalcW();
		mxDO(expectTokenString( parser, ")"));
	}

	mxDO(expectTokenString( parser, "}" ));

	// parse meshes
	model.meshes.setNum( numMeshes );

	md5Mesh* meshes = model.meshes.raw();

	for( i = 0; i < numMeshes; i++ )
	{
		md5Mesh & mesh = meshes[ i ];

		mxDO(expectTokenString( parser, "mesh" ));
		mxDO(expectTokenString( parser, "{" ));

		// read mesh name (MD5_VERSION_ETQW)
		if( is_Quake_ETQW_version )
		{
			if( readOptionalToken( parser, "name" ) )
			{
				TbToken	mesh_name_token;
				parser.ReadToken( mesh_name_token );
			}
		}

		// parse shader
		mxDO(expectTokenString( parser, "shader" ));
		parser.ReadToken( token );
		mxASSERT( token.type == TT_String );
		mesh.shader = token.text;

		MetaFlags::ValueT flags = 0;
		if( is_Quake_ETQW_version )
		{
			// parse flags
			mxDO(expectTokenString( parser, "flags" ));
			mxDO(ParseFlags( parser, GetTypeOf_FVertexFlagsT(), flags ));
		}

		// parse vertices
		mxDO(expectTokenString( parser, "numverts" ));
		const int numVertices = expectInt( parser );

		mesh.vertices.setNum( numVertices );
		for( int iVertex = 0; iVertex < mesh.vertices.num(); iVertex++ )
		{
			md5Vertex & vertex = mesh.vertices[ iVertex ];

			// parse vertex index
			mxDO(expectTokenString( parser, "vert" ));
			const int vertexIndex = expectInt( parser );
			mxASSERT( vertexIndex == iVertex );

			// parse texture coordinates
			mxDO(Parse1DMatrix2( parser, 2, (float*)&vertex.uv ));

			// parse weights
			vertex.startWeight = expectInt( parser );
			vertex.numWeights = expectInt( parser );

			if( flags & EVF_VertexColor )
			{
				float rgba[4];
				mxDO(Parse1DMatrix2( parser, 4, rgba ));
			}
		}
		totalVertexCount += mesh.vertices.num();

		// parse triangles
		mxDO(expectTokenString( parser, "numtris" ));
		const int numTriangles = expectInt( parser );

		mesh.triangles.setNum( numTriangles );
		for( int iTri = 0; iTri < mesh.triangles.num(); iTri++ )
		{
			md5Triangle & tri = mesh.triangles[ iTri ];

			// parse triangle index
			mxDO(expectTokenString( parser, "tri" ));
			int triIndex = expectInt( parser );
			mxASSERT( (int)iTri == triIndex );

			// parse vertex indices
			tri.indices[ 0 ] = expectInt( parser );
			tri.indices[ 1 ] = expectInt( parser );
			tri.indices[ 2 ] = expectInt( parser );
		}
		totalIndexCount += (mesh.triangles.num() * 3);

		// parse weights

		mxDO(expectTokenString( parser, "numweights" ));
		const int numWeights = expectInt( parser );

		mxDO(mesh.weights.setNum( numWeights ));
		for( int iWeight = 0; iWeight < mesh.weights.num(); iWeight++ )
		{
			md5Weight & weight = mesh.weights[ iWeight ];

			// parse weight
			mxDO(expectTokenString( parser, "weight" ));
			const int weightIndex = expectInt( parser );
			mxASSERT( (int)weightIndex == iWeight );

			const int jointIndex = expectInt( parser );
			weight.jointIndex = (int)jointIndex;
			weight.bias = expectFloat( parser );

			mxDO(Parse1DMatrix2( parser, 3, (float*)&weight.position ));

			if(options.scale_to_meters)
			{
				weight.position *= DOOM3_TO_METERS;
			}
		}//For each weight.

		mxDO(expectTokenString( parser, "}"));

	}//end of meshes

	//MX_STATS( mxPutf("Parsed mesh '%s' in %u milliseconds\n",
	//	stream->GetURI().ToChars(), mxGetMilliseconds() - startTime )
	//);

	// Compute the mesh's vertex positions in object-local space
	// based on the model's joints and the mesh's weights array.

	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		md5Mesh & mesh = meshes[ iMesh ];

		mesh.bindPosePositions.setNum( mesh.vertices.num() );

		for( int iVertex = 0; iVertex < mesh.vertices.num(); iVertex++ )
		{
			const md5Vertex& vertex = mesh.vertices[ iVertex ];

			V3f	position = V3_SetAll( 0.0f );

			for( int iWeight = 0; iWeight < vertex.numWeights; iWeight++ )
			{
				const md5Weight& weight = mesh.weights[ vertex.startWeight + iWeight ];
				const md5Joint& joint = joints[ weight.jointIndex ];

				// each vertex' weight position must be converted
				// from joint local space to object space

				// Convert the weight position from joint's local space to object space.

				V3f	weightPos = joint.position
					+ joint.orientation.rotateVector( weight.position )
					;

				position += weightPos * weight.bias;
			}

			mesh.bindPosePositions[ iVertex ] = position;

		}//verts

		mesh.computeBindPoseNormals();
		mesh.computeBindPoseTangents();

	}//meshes

	mxASSERT( numJoints > 0 && numJoints < 256 );	// so that we can pack joint indices into bytes

	//
	model.joints[ 0 ].position += options.root_joint_offset;

	//
	mxDO(model.inverseBindPoseMatrices.setNum( numJoints ));
	for( int iJoint = 0; iJoint < numJoints; ++iJoint )
	{
		const md5Joint& joint = model.joints[ iJoint ];

		// NOTE: first rotation, then translation
		const M44f bindPoseMatrix
			= Quat_to_Matrix4x4(joint.orientation)
			* M44_buildTranslationMatrix(joint.position)
			;

		const M44f inverseBindPoseMatrix = M44_Inverse( bindPoseMatrix );

		model.inverseBindPoseMatrices[iJoint] = inverseBindPoseMatrix;
	}

	return ALL_OK;
}

void md5Mesh::computeBindPoseNormals()
{
	const UINT num_vertices = bindPosePositions.num();
	bindPoseNormals.setNum(num_vertices);

	Arrays::setAll(bindPoseNormals, CV3f(0));

	for( UINT iFace = 0; iFace < triangles.num(); iFace++ )
	{
		const md5Triangle& tri = triangles[iFace];

		const int i0 = tri.indices[0];
		const int i1 = tri.indices[1];
		const int i2 = tri.indices[2];

		V3f face_normal = Triangle_Normal(
			bindPosePositions[i0],
			bindPosePositions[i1],
			bindPosePositions[i2]
		);

		V3_Normalize(&face_normal);

		bindPoseNormals[i0] += face_normal;
		bindPoseNormals[i1] += face_normal;
		bindPoseNormals[i2] += face_normal;
	}

	for( UINT iVert = 0; iVert < num_vertices; iVert++ )
	{
		V3_Normalize(&bindPoseNormals[iVert]);
	}
}

mxSTOLEN("Doom3 BFG edition")
/*
============
R_DeriveNormalsAndTangents

Derives the normal and orthogonal tangent vectors for the triangle vertices.
For each vertex the normal and tangent vectors are derived from all triangles
using the vertex which results in smooth tangents across the mesh.
============
*/
void md5Mesh::computeBindPoseTangents()
{
	AllocatorI & scratch = MemoryHeaps::temporary();

	DynamicArray< V3f > vertexNormals( scratch );
	DynamicArray< V3f > vertexTangents( scratch );
	DynamicArray< V3f > vertexBitangents( scratch );

	vertexNormals.setNum( vertices.num() );
	vertexTangents.setNum( vertices.num() );
	vertexBitangents.setNum( vertices.num() );

	Arrays::setAll(vertexNormals, CV3f(0));
	Arrays::setAll(vertexTangents, CV3f(0));
	Arrays::setAll(vertexBitangents, CV3f(0));

	for ( int i = 0; i < triangles.num(); i++ )
	{
		const md5Triangle& tri = triangles[i];

		const int i0 = tri.indices[0];
		const int i1 = tri.indices[1];
		const int i2 = tri.indices[2];

		const md5Vertex& a = vertices[i0];
		const md5Vertex& b = vertices[i1];
		const md5Vertex& c = vertices[i2];

		const V3f p0 = bindPosePositions[i0];
		const V3f p1 = bindPosePositions[i1];
		const V3f p2 = bindPosePositions[i2];

		const V2f aST = a.uv;
		const V2f bST = b.uv;
		const V2f cST = c.uv;

		float d0[5];
		d0[0] = p1[0] - p0[0];
		d0[1] = p1[1] - p0[1];
		d0[2] = p1[2] - p0[2];
		d0[3] = bST[0] - aST[0];
		d0[4] = bST[1] - aST[1];

		float d1[5];
		d1[0] = p2[0] - p0[0];
		d1[1] = p2[1] - p0[1];
		d1[2] = p2[2] - p0[2];
		d1[3] = cST[0] - aST[0];
		d1[4] = cST[1] - aST[1];

		V3f normal;
		normal[0] = d1[1] * d0[2] - d1[2] * d0[1];
		normal[1] = d1[2] * d0[0] - d1[0] * d0[2];
		normal[2] = d1[0] * d0[1] - d1[1] * d0[0];

		const float f0 = mmInvSqrt( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z );

		normal.x *= f0;
		normal.y *= f0;
		normal.z *= f0;

		// area sign bit
		const float area = d0[3] * d1[4] - d0[4] * d1[3];
		unsigned int signBit = ( *(unsigned int *)&area ) & ( 1 << 31 );

		V3f tangent;
		tangent[0] = d0[0] * d1[4] - d0[4] * d1[0];
		tangent[1] = d0[1] * d1[4] - d0[4] * d1[1];
		tangent[2] = d0[2] * d1[4] - d0[4] * d1[2];

		const float f1 = mmInvSqrt( tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z );
		*(unsigned int *)&f1 ^= signBit;

		tangent.x *= f1;
		tangent.y *= f1;
		tangent.z *= f1;

		V3f bitangent;
		bitangent[0] = d0[3] * d1[0] - d0[0] * d1[3];
		bitangent[1] = d0[3] * d1[1] - d0[1] * d1[3];
		bitangent[2] = d0[3] * d1[2] - d0[2] * d1[3];

		const float f2 = mmInvSqrt( bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z );
		*(unsigned int *)&f2 ^= signBit;

		bitangent.x *= f2;
		bitangent.y *= f2;
		bitangent.z *= f2;

		vertexNormals[i0] += normal;
		vertexTangents[i0] += tangent;
		vertexBitangents[i0] += bitangent;

		vertexNormals[i1] += normal;
		vertexTangents[i1] += tangent;
		vertexBitangents[i1] += bitangent;

		vertexNormals[i2] += normal;
		vertexTangents[i2] += tangent;
		vertexBitangents[i2] += bitangent;
	}

	// Project the summed vectors onto the normal plane and normalize.
	// The tangent vectors will not necessarily be orthogonal to each
	// other, but they will be orthogonal to the surface normal.
	for ( int i = 0; i < vertices.num(); i++ )
	{
		const float normalScale = mmInvSqrt( vertexNormals[i].x * vertexNormals[i].x + vertexNormals[i].y * vertexNormals[i].y + vertexNormals[i].z * vertexNormals[i].z );
		vertexNormals[i].x *= normalScale;
		vertexNormals[i].y *= normalScale;
		vertexNormals[i].z *= normalScale;

		vertexTangents[i] -= V3_Scale( vertexNormals[i], vertexTangents[i] * vertexNormals[i] );
		vertexBitangents[i] -= V3_Scale( vertexNormals[i], vertexBitangents[i] * vertexNormals[i] );

		const float tangentScale = mmInvSqrt( vertexTangents[i].x * vertexTangents[i].x + vertexTangents[i].y * vertexTangents[i].y + vertexTangents[i].z * vertexTangents[i].z );
		vertexTangents[i].x *= tangentScale;
		vertexTangents[i].y *= tangentScale;
		vertexTangents[i].z *= tangentScale;

		const float bitangentScale = mmInvSqrt( vertexBitangents[i].x * vertexBitangents[i].x + vertexBitangents[i].y * vertexBitangents[i].y + vertexBitangents[i].z * vertexBitangents[i].z );
		vertexBitangents[i].x *= bitangentScale;
		vertexBitangents[i].y *= bitangentScale;
		vertexBitangents[i].z *= bitangentScale;
	}

	bindPoseTangents.setNum( vertexTangents.num() );
	memcpy( bindPoseTangents.raw(), vertexTangents.raw(), vertexTangents.rawSize() );
	//transformedTangents = bindPoseTangents;
	//transformedNormals = vertexNormals;
}

static inline void Dbg_DrawJoint( ALineRenderer& dbgDrawer, const md5Joint& joint, const md5Joint& parentJoint, const M44f& transform, const RGBAf& color )
{
	V3f transformedPositionA = M44_TransformPoint( transform, joint.position );
	V3f transformedPositionB = M44_TransformPoint( transform, parentJoint.position );

	dbgDrawer.DrawLine( TO_FLOAT3 transformedPositionA, TO_FLOAT3 transformedPositionB, color, color );
}

void Dbg_DrawSkeleton( ALineRenderer& dbgDrawer, const md5Joint* joints, int numJoints, const M44f& transform, const RGBAf& color )
{
	// root joint has index 0 so start with index 1
	for( int iJoint = 1; iJoint < numJoints; iJoint++ )
	{
		const md5Joint& joint = joints[ iJoint ];
		mxASSERT( joint.parentNum >= 0 );
		const md5Joint& parentJoint = joints[ joint.parentNum ];
		Dbg_DrawJoint( dbgDrawer, joint, parentJoint, transform, color );
	}
}

void md5Model::Dbg_DrawSkeleton( ALineRenderer& dbgDrawer, const M44f& transform, const RGBAf& color )
{
	UNDONE;
	//::md5::Dbg_DrawSkeleton( dbgDrawer, joints.raw(), joints.num(), transform, color );
}

void CalculateVertexIndexCounts( const md5Model& md5Model, int &outNumVertices, int &outNumIndices )
{
	int	numVertices = 0;
	int	numIndices = 0;

	for( int iMesh = 0; iMesh < md5Model.meshes.num(); iMesh++ )
	{
		const md5Mesh& mesh = md5Model.meshes[ iMesh ];
		numVertices += mesh.vertices.num();
		numIndices += (mesh.triangles.num() * 3);
	}

	outNumVertices = numVertices;
	outNumIndices = numIndices;
}

void md5Model::drawMeshInBindPose( ALineRenderer& dbgDrawer, const M44f& transform, const RGBAf& color )
{
	const int numMeshes = this->meshes.num();
	const md5Mesh* meshes = this->meshes.raw();

	// Draw each triangle of the model. Slow!
	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		const md5Mesh & mesh = meshes[ iMesh ];

		const V3f* positions = mesh.bindPosePositions.raw();

		for( int iTri = 0; iTri < mesh.triangles.num(); iTri++ )
		{
			const md5Triangle & tri = mesh.triangles[ iTri ];

			// seems i need to reverse the winding

			V3f pA = M44_TransformPoint( transform, positions[ tri.indices[ 2 ] ] );
			V3f pB = M44_TransformPoint( transform, positions[ tri.indices[ 1 ] ] );
			V3f pC = M44_TransformPoint( transform, positions[ tri.indices[ 0 ] ] );

			dbgDrawer.DrawWireTriangle( TO_FLOAT3 pA, TO_FLOAT3 pB, TO_FLOAT3 pC, color );
		}
	}
}

AABBf md5Model::computeBoundingBox() const
{
	AABBf bounds;
	bounds.clear();

	const int numMeshes = this->meshes.num();
	const md5Mesh* meshes = this->meshes.raw();

	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		const md5Mesh & mesh = meshes[ iMesh ];

		const V3f* positions = mesh.bindPosePositions.raw();

		for( int i = 0; i < mesh.bindPosePositions.num(); i++ )
		{
			bounds.addPoint( positions[i] );
		}
	}

	return bounds;
}

}//namespace Doom3
