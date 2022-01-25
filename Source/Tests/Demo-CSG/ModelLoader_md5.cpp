//#include "stdafx.h"
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Text/Lexer.h>
#include "ModelLoader_md5.h"

#define TO_FLOAT3  *(V3F4*)&
#define TO_FLOAT4  *(V4F4*)&

#define MAX_BONES_PER_VERTEX 4

namespace md5
{

static inline Matrix4 CalcBoneTransform( const md5_joint& joint )
{
	mxWHY("do i need to transpose the rotation matrix?");
	Matrix4	boneMatrix( joint.orientation.ToMat3().Transpose().ToMat4() );
	boneMatrix.SetTranslation( joint.position );
	return boneMatrix;
}

mxBEGIN_FLAGS( FVertexFlagsT )
	mxREFLECT_BIT( vertexColor, EVertexFlags::EVF_VertexColor ),
mxEND_FLAGS;



ERet LoadModel( const char* fileName, Model &model )
{
	mxASSERT(model.IsEmpty());

	ByteBuffer	fileData;
	mxDO(Util_LoadFileToBlob(fileName, fileData));

	Lexer	parser( fileData.ToPtr(), fileData.DataSize(), fileName );

	return LoadModel( parser, model );
}

ERet LoadModel( Lexer& parser, Model &model )
{
	int		version;
	Token	token;
	int		i;

	int	totalVertexCount = 0;
	int	totalIndexCount = 0;

	//MX_STATS( const int startTime = mxGetMilliseconds() );

	//if ( !parser.LoadMemory( data, size, stream->GetURI().ToChars() ) )
	//{
	//	mxErrf( "Failed to load md5 model from file '%s'.\n", stream->GetURI().ToChars() );
	//	return false;
	//}

	model.Clear();

	mxDO(Expect2( parser, MD5_VERSION_STRING ));
	version = expectInt( parser );
	if ( version < MD5_VERSION_DOOM3 || version > MD5_VERSION_ETQW ) {
		parser.Error( "Invalid version %d.  Should be version %d or %d\n", version, MD5_VERSION_DOOM3, MD5_VERSION_ETQW );
		return ERR_UNSUPPORTED_VERSION;
	}

	// skip the command line
  	mxDO(Expect2( parser, "commandline" ));
	parser.ReadToken( token );

	int		numJoints = 0;
	int		numMeshes = 0;

	// parse num joints
	mxDO(Expect2( parser, "numJoints" ));
	numJoints = expectInt( parser  );
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse num meshes
	mxDO(Expect2( parser, "numMeshes" ));
	numMeshes = expectInt( parser );
	if ( numMeshes <= 0 ) {
		parser.Error( "Invalid number of meshes: %d", numMeshes );
	}

	// parse the joints
	model.joints.SetNum( numJoints );
	
	md5_joint* joints = model.joints.ToPtr();

	mxDO(Expect2( parser, "joints" ));
	mxDO(Expect2( parser, "{" ));
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
		mxDO(Expect2( parser, "("));
		joints[ i ].position.x = expectFloat( parser );
		joints[ i ].position.y = expectFloat( parser );
		joints[ i ].position.z = expectFloat( parser );
		mxDO(Expect2( parser, ")"));

		// parse joint orientation in object local space
		mxDO(Expect2( parser, "("));
		joints[ i ].orientation.x = expectFloat( parser );
		joints[ i ].orientation.y = expectFloat( parser );
		joints[ i ].orientation.z = expectFloat( parser );
		joints[ i ].orientation.w = joints[ i ].orientation.CalcW();
		mxDO(Expect2( parser, ")"));
	}

	mxDO(Expect2( parser, "}" ));

	// parse meshes
	model.meshes.SetNum( numMeshes );

	Mesh* meshes = model.meshes.ToPtr();

	for( i = 0; i < numMeshes; i++ )
	{
		Mesh & mesh = meshes[ i ];

		mxDO(Expect2( parser, "mesh" ));
		mxDO(Expect2( parser, "{" ));

		MetaFlags::Mask flags = 0;
		if( version == MD5_VERSION_ETQW )
		{
			// read mesh name
			mxDO(Expect2( parser, "name" ));
			parser.ReadToken( token );

			// parse shader
			mxDO(Expect2( parser, "shader" ));
			parser.ReadToken( token );
			mxASSERT( token.type == TT_String );
			mesh.shader = token.text;

			// parse flags
			mxDO(Expect2( parser, "flags" ));
			mxDO(ParseFlags( parser, GetTypeOf_FVertexFlagsT(), flags ));
		}

		// parse vertices
		mxDO(Expect2( parser, "numverts" ));
		const int numVertices = expectInt( parser );

		mesh.vertices.SetNum( numVertices );
		for( int iVertex = 0; iVertex < mesh.vertices.Num(); iVertex++ )
		{
			md5_vertex & vertex = mesh.vertices[ iVertex ];

			// parse vertex index
			mxDO(Expect2( parser, "vert" ));
			const int vertexIndex = expectInt( parser );
			mxASSERT( vertexIndex == iVertex );

			// parse texture coordinates
			mxDO(Parse1DMatrix2( parser, 2, vertex.uv.ToPtr() ));

			// parse weights
			vertex.startWeight = expectInt( parser );
			vertex.numWeights = expectInt( parser );

			if( flags & EVF_VertexColor )
			{
				float rgba[4];
				mxDO(Parse1DMatrix2( parser, 4, rgba ));
			}
		}
		totalVertexCount += mesh.vertices.Num();

		// parse triangles
		mxDO(Expect2( parser, "numtris" ));
		const int numTriangles = expectInt( parser );

		mesh.triangles.SetNum( numTriangles );
		for( int iTri = 0; iTri < mesh.triangles.Num(); iTri++ )
		{
			md5_triangle & tri = mesh.triangles[ iTri ];

			// parse triangle index
			mxDO(Expect2( parser, "tri" ));
			int triIndex = expectInt( parser );
			mxASSERT( (int)iTri == triIndex );

			// parse vertex indices
			tri.indices[ 0 ] = expectInt( parser );
			tri.indices[ 1 ] = expectInt( parser );
			tri.indices[ 2 ] = expectInt( parser );
		}
		totalIndexCount += (mesh.triangles.Num() * 3);

		// parse weights

		mxDO(Expect2( parser, "numweights" ));
		const int numWeights = expectInt( parser );

		mesh.weights.SetNum( numWeights );
		for( int iWeight = 0; iWeight < mesh.weights.Num(); iWeight++ )
		{
			md5_weight & weight = mesh.weights[ iWeight ];

			// parse weight
			mxDO(Expect2( parser, "weight" ));
			const int weightIndex = expectInt( parser );
			mxASSERT( (int)weightIndex == iWeight );

			const int jointIndex = expectInt( parser );
			weight.jointIndex = (int)jointIndex;
			weight.bias = expectFloat( parser );

			mxDO(Parse1DMatrix2( parser, 3, weight.position.ToPtr() ));
		}
		mxDO(Expect2( parser, "}"));
	}//end of meshes

	//MX_STATS( mxPutf("Parsed mesh '%s' in %u milliseconds\n",
	//	stream->GetURI().ToChars(), mxGetMilliseconds() - startTime )
	//);

	// Compute the mesh's vertex positions in object-local space
	// based on the model's joints and the mesh's weights array.

	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		Mesh & mesh = meshes[ iMesh ];

		mesh.bindPosePositions.SetNum( mesh.vertices.Num() );
		mesh.transformedPositions.SetNum( mesh.vertices.Num() );

		for( int iVertex = 0; iVertex < mesh.vertices.Num(); iVertex++ )
		{
			/*const*/ md5_vertex & vertex = mesh.vertices[ iVertex ];

			Vec3D	position( 0.0f );

			for( int iWeight = 0; iWeight < vertex.numWeights; iWeight++ )
			{
				const md5_weight & weight = mesh.weights[ vertex.startWeight + iWeight ];
				const md5_joint & joint = joints[ weight.jointIndex ];

				// each vertex' weight position must be converted
				// from joint local space to object space

				// Convert the weight position from joint's local space to object space.

				Vec3D	weightPos = joint.position + (joint.orientation * weight.position);

				position += weightPos * weight.bias;
			}

			mesh.bindPosePositions[ iVertex ] = position;
			mesh.transformedPositions[ iVertex ] = position;

		}//verts

	}//meshes

	mxASSERT( numJoints < 256 );	// so that we can pack joint indices into bytes

	model.bindPose.SetNum( numJoints );
	model.inverseBindPose.SetNum( numJoints );

	for( int iJoint = 0; iJoint < numJoints; iJoint++ )
	{
		md5_joint & joint = joints[ iJoint ];


Vec3D vZero;

Vec3D v0 = joint.position + (joint.orientation * vZero);

		Matrix4 rotation = joint.orientation.ToMat4();
		Matrix4 translation(_InitIdentity);
		translation.SetTranslation(joint.position);

		joint.bindMatrix = rotation * translation;
		//joint.bindMatrix = CalcBoneTransform(joint);

		
Vec3D v1 = joint.bindMatrix.TransformPoint(vZero);

		joint.bindMatrix.BuildTransform( joint.position, joint.orientation );

Vec3D v2 = joint.bindMatrix.TransformPoint(vZero);


		model.bindPose[iJoint].BuildTransform( joint.position, joint.orientation.Inverse() );
		model.inverseBindPose[iJoint] = model.bindPose[iJoint].Inverse();
	}

	return ALL_OK;
}

// Build skeleton for a given frame data.
static void BuildFrameSkeleton(
	const Animation& anim,
	int frameNum,
	md5_skeleton &skeleton
)
{
	const int numJoints = anim.NumJoints();
	const jointAnimInfo_t* jointInfos = anim.jointInfo.ToPtr();
	const md5_baseFrameJoint* baseFrames = anim.baseFrame.ToPtr();
	const float* animFrameData = anim.componentFrames.ToPtr() + anim.numAnimatedComponents * frameNum;

	skeleton.SetNum( numJoints );

	for( int iJoint = 0; iJoint < numJoints; ++iJoint )
	{
		const jointAnimInfo_t& jointInfo = jointInfos[ iJoint ];
		const md5_baseFrameJoint& baseFrameJoint = baseFrames[iJoint];

		md5_joint & skeletonJoint = skeleton[ iJoint ];

		// Start with the base frame position and orientation.

		skeletonJoint.orientation	= baseFrameJoint.q;
		skeletonJoint.position		= baseFrameJoint.t;
		skeletonJoint.parentNum		= jointInfo.parentNum;

		// Build the skeleton pose for a single frame of the animation.
		// This is done by combining the base frames with the frame components array.

		const int animFlags = jointInfo.animBits;
		const int firstAnimComponent = jointInfo.firstComponent;
		int j = 0;

		if ( animFlags & ANIM_TX ) // Pos.x
		{
			skeletonJoint.position.x = animFrameData[ firstAnimComponent + j++ ];
		}
		if ( animFlags & ANIM_TY ) // Pos.y
		{
			skeletonJoint.position.y = animFrameData[ firstAnimComponent + j++ ];
		}
		if ( animFlags & ANIM_TZ ) // Pos.x
		{
			skeletonJoint.position.z  = animFrameData[ firstAnimComponent + j++ ];
		}
		if ( animFlags & ANIM_QX ) // Orient.x
		{
			skeletonJoint.orientation.x = animFrameData[ firstAnimComponent + j++ ];
		}
		if ( animFlags & ANIM_QY ) // Orient.y
		{
			skeletonJoint.orientation.y = animFrameData[ firstAnimComponent + j++ ];
		}
		if ( animFlags & ANIM_QZ ) // Orient.z
		{
			skeletonJoint.orientation.z = animFrameData[ firstAnimComponent + j++ ];
		}
		skeletonJoint.orientation.w = skeletonJoint.orientation.CalcW();

		const int jointParentIndex = jointInfo.parentNum;

		if( jointParentIndex != -1 )	// Has a parent joint
		{
			const md5_joint& parentJoint = skeleton[ jointParentIndex ];

			// Concatenate transforms.
			Vec3D	rotatedPos = parentJoint.orientation * skeletonJoint.position;
			skeletonJoint.position = parentJoint.position + rotatedPos;

			skeletonJoint.orientation = skeletonJoint.orientation * parentJoint.orientation;
			skeletonJoint.orientation.NormalizeFast();
		}
	}
}

ERet LoadAnim( const char* fileName, Animation &anim )
{
	ByteBuffer	fileData;
	mxDO(Util_LoadFileToBlob(fileName, fileData));
	return LoadAnim( fileData.ToPtr(), fileData.DataSize(), anim );
}

ERet LoadAnim( const void* data, size_t size, Animation &anim )
{
	int		version;
	Lexer	parser( data, size );
	Token	token;
	int		i, j;

	//MX_STATS( const int startTime = mxGetMilliseconds() );

	//const char* data = cast(const char*) stream->Map();
	//const int length = cast(const int) stream->GetSize();

	//if ( !parser.LoadMemory( data, length, stream->GetURI().ToChars() ) ) {
	//	return false;
	//}

	Expect( parser, MD5_VERSION_STRING );
	version = expectInt( parser );
	if ( version != MD5_VERSION_DOOM3 ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION_DOOM3 );
	}

	// skip the command line
	Expect( parser, "commandline" );
	parser.ReadToken( token );

	anim.Clear();

	// parse num frames
	Expect( parser, "numFrames" );
	anim.numFrames = expectInt( parser );
	if ( anim.numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", anim.numFrames );
	}

	// parse num joints
	Expect( parser, "numJoints" );
	anim.numJoints = expectInt( parser );
	if ( anim.numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", anim.numJoints );
	}

	// parse frame rate
	Expect( parser, "frameRate" );
	anim.frameRate = expectInt( parser );
	if ( anim.frameRate <= 0 ) {
		parser.Error( "Invalid frame rate: %d", anim.frameRate );
	}

	// parse num animated components
	Expect( parser, "numAnimatedComponents" );
	anim.numAnimatedComponents = expectInt( parser );
	if ( anim.numAnimatedComponents <= 0 ) {
		parser.Error( "Invalid number of animated components: %d", anim.numAnimatedComponents );
	}


	// parse the hierarchy
	anim.jointInfo.SetNum( anim.numJoints );
	Expect( parser, "hierarchy" );
	Expect( parser, "{" );
	for( i = 0; i < anim.numJoints; i++ )
	{
		parser.ReadToken( token );
		anim.jointInfo[ i ].name = token.text;

		// parse parent num
		anim.jointInfo[ i ].parentNum = expectInt( parser );
		if ( anim.jointInfo[ i ].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", anim.jointInfo[ i ].parentNum );
		}

		if ( ( i != 0 ) && ( anim.jointInfo[ i ].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		anim.jointInfo[ i ].animBits = expectInt( parser );
		if ( anim.jointInfo[ i ].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", anim.jointInfo[ i ].animBits );
		}

		// parse first component
		anim.jointInfo[ i ].firstComponent = expectInt( parser );
		if ( ( anim.numAnimatedComponents > 0 ) && ( ( anim.jointInfo[ i ].firstComponent < 0 ) || ( anim.jointInfo[ i ].firstComponent >= anim.numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", anim.jointInfo[ i ].firstComponent );
		}
	}

	Expect( parser, "}" );

	// parse bounds
	Expect( parser, "bounds" );
	Expect( parser, "{" );
	anim.bounds.SetNum( anim.numFrames );
	for( i = 0; i < anim.numFrames; i++ ) {
		Parse1DMatrix( parser, 3, (float*)&(anim.bounds[ i ].min_point) );
		Parse1DMatrix( parser, 3, (float*)&(anim.bounds[ i ].max_point) );
	}
	Expect( parser, "}" );

	// parse base frame
	anim.baseFrame.SetNum( anim.numJoints );
	Expect( parser, "baseframe" );
	Expect( parser, "{" );
	for( i = 0; i < anim.numJoints; i++ ) {
		CQuat q;
		Parse1DMatrix( parser, 3, anim.baseFrame[ i ].t.ToPtr() );
		Parse1DMatrix( parser, 3, q.ToPtr() );//baseFrame[ i ].q.ToPtr() );
		anim.baseFrame[ i ].q = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();
	}
	Expect( parser, "}" );

	// parse frames
	anim.componentFrames.SetNum( anim.numAnimatedComponents * anim.numFrames );

	float *componentPtr = anim.componentFrames.ToPtr();
	for( i = 0; i < anim.numFrames; i++ ) {
		Expect( parser, "frame" );
		int num = expectInt( parser );
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		Expect( parser, "{" );

		for( j = 0; j < anim.numAnimatedComponents; j++, componentPtr++ ) {
			*componentPtr = expectFloat( parser );
		}

		Expect( parser, "}" );
	}

/*
	// get total move delta
	if ( !numAnimatedComponents ) {
		totaldelta.Zero();
	} else {
		componentPtr = &componentFrames[ jointInfo[ 0 ].firstComponent ];
		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			for( iJoint = 0; iJoint < numFrames; iJoint++ ) {
				componentPtr[ numAnimatedComponents * iJoint ] -= baseFrame[ 0 ].t.x;
			}
			totaldelta.x = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.x = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			for( iJoint = 0; iJoint < numFrames; iJoint++ ) {
				componentPtr[ numAnimatedComponents * iJoint ] -= baseFrame[ 0 ].t.y;
			}
			totaldelta.y = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.y = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			for( iJoint = 0; iJoint < numFrames; iJoint++ ) {
				componentPtr[ numAnimatedComponents * iJoint ] -= baseFrame[ 0 ].t.z;
			}
			totaldelta.z = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
		} else {
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[ 0 ].t.Zero();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;
*/


	//MX_STATS( mxPutf("Parsed animation '%s' in %u milliseconds\n",
	//	stream->GetURI().ToChars(), mxGetMilliseconds() - startTime )
	//	);


	// Build the skeleton pose for a single frame of the animation.
	anim.frameSkeletons.SetNum( anim.numFrames );
	for( int iFrame = 0; iFrame < anim.numFrames; iFrame++ )
	{
		BuildFrameSkeleton( anim, iFrame, anim.frameSkeletons[ iFrame ] );
	}

	// done
	return ALL_OK;
}

static inline void Dbg_DrawJoint( AuxRenderer& dbgDrawer, const md5_joint& joint, const md5_joint& parentJoint, const Matrix4& transform, const RGBAf& color )
{
	Vec3D transformedPositionA = transform.TransformPoint( joint.position );
	Vec3D transformedPositionB = transform.TransformPoint( parentJoint.position );

	dbgDrawer.DrawLine( TO_FLOAT3 transformedPositionA, TO_FLOAT3 transformedPositionB, color, color );

	Matrix4 mat;
	mat.BuildTransform(joint.position, joint.orientation);

	M44F4 jointMat = (M44F4&) mat;
	const V3F4 position = Matrix_GetTranslation(jointMat);
	const V3F4 axisX = V3_Normalized(V4F4_As_V3F4(jointMat.r0));
	const V3F4 axisY = V3_Normalized(V4F4_As_V3F4(jointMat.r1));
	const V3F4 axisZ = V3_Normalized(V4F4_As_V3F4(jointMat.r2));
	dbgDrawer.DrawLine(position, position+axisX, RGBAf::RED, RGBAf::RED);
	dbgDrawer.DrawLine(position, position+axisY, RGBAf::GREEN, RGBAf::GREEN);
	dbgDrawer.DrawLine(position, position+axisZ, RGBAf::BLUE, RGBAf::BLUE);
}

void Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const md5_joint* joints, int numJoints, const Matrix4& transform, const RGBAf& color )
{
	// root joint has index 0 so start with index 1
	for( int iJoint = 1; iJoint < numJoints; iJoint++ )
	{
		const md5_joint& joint = joints[ iJoint ];
		mxASSERT( joint.parentNum >= 0 );
		const md5_joint& parentJoint = joints[ joint.parentNum ];
		Dbg_DrawJoint( dbgDrawer, joint, parentJoint, transform, color );
	}
}

void Model::Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color )
{
	::md5::Dbg_DrawSkeleton( dbgDrawer, joints.ToPtr(), joints.Num(), transform, color );
}

void CalculateVertexIndexCounts( const Model& md5Model, int &outNumVertices, int &outNumIndices )
{
	int	numVertices = 0;
	int	numIndices = 0;

	for( int iMesh = 0; iMesh < md5Model.meshes.Num(); iMesh++ )
	{
		const Mesh& mesh = md5Model.meshes[ iMesh ];
		numVertices += mesh.vertices.Num();
		numIndices += (mesh.triangles.Num() * 3);
	}

	outNumVertices = numVertices;
	outNumIndices = numIndices;
}

/*================================
		SkeletalMesh
================================*/

md5SkeletalMesh::md5SkeletalMesh()
{
	Clear();
}

void md5SkeletalMesh::SetModel( Model* md5Model )
{
	Clear();

	this->model = md5Model;

	animatedSkeleton = this->model->joints;
}

void md5SkeletalMesh::SetAnim( Animation* md5Anim )
{
	mxASSERT(CheckModelAnim( *model, *md5Anim ));
	anim = md5Anim;

	m_iFrameRate = anim->frameRate;
	m_fFrameDuration = 1.0f / (float)anim->frameRate;
	m_fAnimDuration = ( m_fFrameDuration * (float)anim->numFrames );
	m_iNumFrames = anim->numFrames;

	m_currentTime = 0.0f;
}

void md5SkeletalMesh::Clear()
{
	model = nil;
	anim = nil;

	m_iFrameRate = 0;
	m_fFrameDuration = 0;
	m_fAnimDuration = 0;
	m_currentTime = 0;
	m_iNumFrames = 0;

	animatedSkeleton.Empty();

	AABB24_Clear(&localBounds);
}

void InterpolateSkeletons(
	const md5_joint* skeletonA, const md5_joint* skeletonB, int numJoints,
	FLOAT blend,
	md5_joint *skeleton )
{
	for ( int iJoint = 0; iJoint < numJoints; ++iJoint )
	{
		const md5_joint& joint0 = skeletonA[ iJoint ];
		const md5_joint& joint1 = skeletonB[ iJoint ];
		md5_joint & finalJoint = skeleton[iJoint];

		finalJoint.orientation.Slerp( joint0.orientation, joint1.orientation, blend );
		finalJoint.position.Lerp( joint0.position, joint1.position, blend );
		finalJoint.parentNum = joint0.parentNum;
	}
}

void md5SkeletalMesh::Animate( FLOAT deltaSeconds )
{
	// Animate only if there is an animation...
	if( anim != nil )
	{
		if( anim->numFrames < 1 ) return;

		m_currentTime += deltaSeconds;

		// Clamp m_currentTime between 0 and m_fAnimDuration.
		while ( m_currentTime > m_fAnimDuration ) m_currentTime -= m_fAnimDuration;
		while ( m_currentTime < 0.0f ) m_currentTime += m_fAnimDuration;

		// Figure out which frame we're on
		float fFramNum = m_currentTime * (float)m_iFrameRate;
		int iFrame0 = (int)floorf( fFramNum );
		int iFrame1 = (int)ceilf( fFramNum );
		iFrame0 = iFrame0 % m_iNumFrames;
		iFrame1 = iFrame1 % m_iNumFrames;

		float fInterpolate = fmodf( m_currentTime, m_fFrameDuration ) / m_fFrameDuration;
		//DBGOUT("frame0: %i, frame1: %i, blend: %f\n",iFrame0,iFrame1,fInterpolate);
		InterpolateSkeletons(
			anim->frameSkeletons[ iFrame0 ].ToPtr(),
			anim->frameSkeletons[ iFrame1 ].ToPtr(),
			anim->NumJoints(),
			fInterpolate,
			animatedSkeleton.ToPtr()
		);

		UpdateMesh();
	}
}

void md5SkeletalMesh::UpdateMesh()
{
	const md5_skeleton& animatedSkeleton = this->animatedSkeleton;
	const int numMeshes = model->NumMeshes();
	Mesh* meshes = model->meshes.ToPtr();

	const md5_joint* joints = animatedSkeleton.ToPtr();

	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		Mesh & mesh = meshes[ iMesh ];

		mesh.transformedPositions.SetNum( mesh.vertices.Num() );

		for( int iVertex = 0; iVertex < mesh.vertices.Num(); iVertex++ )
		{
			const md5_vertex & vertex = mesh.vertices[ iVertex ];

			Vec3D	position( 0.0f );

			//AlwaysAssertX( vertex.numWeights <= MAX_BONES_PER_VERTEX,
			//	"you cannot have more than " TO_STR2(MAX_BONES_PER_VERTEX) " bones per vertex when using GPU skinning" );

			for( int iWeight = 0; iWeight < vertex.numWeights; iWeight++ )
			{
				const md5_weight & weight = mesh.weights[ vertex.startWeight + iWeight ];
				const md5_joint & joint = joints[ weight.jointIndex ];

				// each vertex' weight position must be converted
				// from joint local space to object space

				Vec3D	weightPos = joint.position + (joint.orientation * weight.position);

				position += weightPos * weight.bias;
			}

			mesh.transformedPositions[ iVertex ] = position;

		}//verts

	}//meshes
}

void md5SkeletalMesh::Dbg_Draw( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color )
{
//	model->Dbg_DrawSkeleton( transform, RGBAf::RED );
	model->Dbg_DrawMesh( dbgDrawer, transform, color );
	Dbg_DrawSkeleton( dbgDrawer, transform, RGBAf::ORANGE );
}

void md5SkeletalMesh::Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color )
{
	::md5::Dbg_DrawSkeleton( dbgDrawer, animatedSkeleton.ToPtr(), animatedSkeleton.Num(), transform, RGBAf::ORANGE );
}
#if 0

void SetupSkinnedModel( const SkeletalMesh& skeletalMesh, mxSkinnedModel &skinnedModel )
{
	// Pack all submeshes in a single big skeletalMesh.

	const md5_model& model = *skeletalMesh.model;

	int	numVertices, numIndices;
	CalculateVertexIndexCounts( model, numVertices, numIndices );


	TArray< Vec3D >			positions;
	TArray< Vec2D >			texCoords;
	TArray< Vec3D >			normals;
	TArray< BoneWeights >	boneWeights;
	TArray< BoneIndices >	boneIndices;

	positions.AddZeroed( numVertices );
	texCoords.AddZeroed( numVertices );
	normals.AddZeroed( numVertices );
	boneWeights.AddZeroed( numVertices );
	boneIndices.AddZeroed( numVertices );
	MemSet( boneIndices.ToPtr(), -1, boneIndices.DataSize() );

	//AssertX( numVertices < MAX_UINT16, "only 16-bit indices supported" );
	TArray< U32 >& indices = skinnedModel.indices;
	indices.AddZeroed( numIndices );

	int	indexBufferOffset = 0;
	int	vertexBufferOffset = 0;

	int	iCurrentVertex = 0;

	const int numMeshes = model.meshes.Num();
	const md5_mesh* meshes = model.meshes.ToPtr();
	const md5_joint* joints = model.joints.ToPtr();

	mxASSERT( model.NumJoints() <= MAX_BONES_PER_MODEL );

	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		const md5_mesh & mesh = meshes[ iMesh ];
		//if( mesh.shader == "skip" ) {
		//	continue;
		//}

		// vertices
		for( int iVertex = 0; iVertex < mesh.vertices.Num(); iVertex++ )
		{
			const md5_vertex & vertex = mesh.vertices[ iVertex ];

			const int numWeights = vertex.numWeights;

			AlwaysAssertX( numWeights <= MAX_BONES_PER_VERTEX,
				"you cannot have more than " TO_STR2(MAX_BONES_PER_VERTEX) " bones per vertex when using GPU skinning" );

			Vec3D position( 0.0f );

			for( int iWeight = 0; iWeight < numWeights; iWeight++ )
			{
				const md5_weight & weight = mesh.weights[ vertex.startWeight + iWeight ];

				boneWeights[ iCurrentVertex ]._[ iWeight ] = weight.bias;
				boneIndices[ iCurrentVertex ]._[ iWeight ] = weight.jointIndex;

				// Convert the weight position from joint's local space to object space.

				const md5_joint & joint = joints[ weight.jointIndex ];

				Vec3D	weightPos = joint.position + (joint.orientation * weight.position);

				position += weightPos * weight.bias;
			}

			positions[ iCurrentVertex ] = position;
			texCoords[ iCurrentVertex ] = vertex.uv;

			iCurrentVertex++;
		}

		// indices
		for( int iTri = 0; iTri < mesh.triangles.Num(); iTri++ )
		{
			const md5_triangle & tri = mesh.triangles[ iTri ];

			// seems i need to reverse the winding

			indices[ indexBufferOffset + 0 ] = tri.indices[ 2 ] + vertexBufferOffset;
			indices[ indexBufferOffset + 1 ] = tri.indices[ 1 ] + vertexBufferOffset;
			indices[ indexBufferOffset + 2 ] = tri.indices[ 0 ] + vertexBufferOffset;

			indexBufferOffset += 3;
		}

		vertexBufferOffset += mesh.vertices.Num();
	}

	// calculate vertex normals and tangents

	// calculate face normals

	TArray< Vec3D >		faceNormals;
	faceNormals.SetNum( numIndices / 3 );

	for( int iFace = 0; iFace < faceNormals.Num(); iFace++ )
	{
		const Vec3D & v0 = positions[ indices[ iFace * 3 + 0 ] ];
		const Vec3D & v1 = positions[ indices[ iFace * 3 + 1 ] ];
		const Vec3D & v2 = positions[ indices[ iFace * 3 + 2 ] ];

		const Vec3D   n1 ( v1 - v0 );
		const Vec3D   n2 ( v2 - v0 );
		faceNormals[ iFace ] = n1.Cross( n2 ).GetNormalized();
	}

	for( int iFace = 0; iFace < faceNormals.Num(); iFace++ )
	{
		normals[ indices[ iFace * 3 + 0 ] ] += faceNormals[ iFace ];
		normals[ indices[ iFace * 3 + 1 ] ] += faceNormals[ iFace ];
		normals[ indices[ iFace * 3 + 2 ] ] += faceNormals[ iFace ];
	}
	for( int iNormal = 0; iNormal < normals.Num(); iNormal++ )
	{
		//if( normals[ iNormal ].LengthSqr() > 1.e-4f )
		{
			normals[ iNormal ].NormalizeFast();
		}
	}









	rxModel & renderModel = skinnedModel.renderModel;

	xna_aabb_copy( renderModel.mLocalBounds, skeletalMesh.localBounds );

	skinnedModel.skinnedVerts.SetNum( numVertices );
	vtx_skinned* skinnedVerts = skinnedModel.skinnedVerts.ToPtr();
	MemZero( skinnedVerts, sizeof(skinnedVerts[0]) * numVertices );

	for( int iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		vtx_skinned & vertex = skinnedVerts[ iVertex ];

		vertex.xyz		= positions[ iVertex ];
		vertex.tangent.Pack( Vec3D::vec3_unit_y );MX_UNDONE
		vertex.normal.Pack( normals[ iVertex ] );
		vertex.uv		= texCoords[ iVertex ];

		for( int iBone = 0; iBone < MAX_BONES_PER_VERTEX; iBone++ )
		{
			vertex.boneIndices[ iBone ] = boneIndices[ iVertex ]._[ iBone ];
			vertex.boneWeights[ iBone ] = boneWeights[ iVertex ]._[ iBone ];
		}
	}


	rxMeshManager & meshFactory = rxMeshManager::Get();
	renderModel.mMesh = meshFactory.CreateRenderMesh< vtx_skinned, U32 >(
		skinnedVerts,
		numVertices,
		indices.ToPtr(),
		numIndices
	);

	rxModelBatch& batch = renderModel.mBatches.Add();

	batch.bounds.SetInfinity();
	batch.subset.baseVertex = 0;
	batch.subset.startIndex = 0;
	batch.subset.indexCount = numIndices;
	batch.subset.vertexCount = numVertices;
	batch.material = rxMaterialServer::Get().GetMaterialByName("skinned");

	renderModel.mNumOpaqueSurfaces = 1;

}
#endif
#if 0

void SkeletalMesh::UpdateGPUBuffers()
{
	Matrix4 * bones = cast(Matrix4*) gfxFrontEnd.constants.mCBPerSkinned.bones;

	const int numJoints = animatedSkeleton.Num();
	const md5_joint* joints = animatedSkeleton.ToPtr();

	for( int iJoint = 0; iJoint < numJoints; iJoint++ )
	{
		const md5_joint & joint = joints[ iJoint ];

		#if(1)
		{
			Matrix4	boneMatrix = CalcBoneTransform( joint );
			Matrix4	finalBoneMatrix = model->inverseBindPose[ iJoint ] * boneMatrix;

			bones[ iJoint ] = finalBoneMatrix;
		}
		#else
		{
			bones[ iJoint ].SetIdentity();
		}
		#endif
	}

	gfxFrontEnd.constants.mCBPerSkinned.Update();
}
MX_BUG("GPU skinning")
void TempHack_DrawSkinnedModel( mxSkinnedModel& skinnedModel, const RGBAf& color )
{
	const int numTriangles = skinnedModel.indices.Num() / 3;
	const U32* indices = skinnedModel.indices.ToPtr();
	const int numVertices = skinnedModel.skinnedVerts.Num();
	const vtx_skinned* verts = skinnedModel.skinnedVerts.ToPtr();
	
	const Matrix4 * bones = cast(const Matrix4*) gfxFrontEnd.constants.mCBPerSkinned.bones;

	AuxRenderer& dbgDrawer = gfxFrontEnd.dbgLineDrawer;

	const Matrix4& transform = as_matrix4(skinnedModel.renderModel.mLocalToWorld);

#if 1
	vtx_skinned* transformedVerts = cast(vtx_skinned*)mxStackAlloc16( skinnedModel.skinnedVerts.DataSize() );
	for( int iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		const vtx_skinned & src = verts[ iVertex ];
		vtx_skinned & dest = transformedVerts[ iVertex ];

		if(0)
		{
			dest.xyz.SetZero();
			for( int iBone = 0; iBone < MAX_BONES_PER_VERTEX; iBone++ )
			{
				if( -1 == iBone ) {
					continue;
				}
				dest.xyz += bones[ (int)src.boneIndices[ iBone ] ].TransformPoint( src.xyz )
							* src.boneWeights[ iBone ];
			}
		}
		else
		{
			Matrix4 m0 = bones[ (int)src.boneIndices[ 0 ] ] * src.boneWeights[ 0 ];
			Matrix4 m1 = bones[ (int)src.boneIndices[ 1 ] ] * src.boneWeights[ 1 ];
			Matrix4 m2 = bones[ (int)src.boneIndices[ 2 ] ] * src.boneWeights[ 2 ];
			Matrix4 m3 = bones[ (int)src.boneIndices[ 3 ] ] * src.boneWeights[ 3 ];
			Matrix4 mt = m0 + m1 + m2 + m3;
			dest.xyz = mt.TransformPoint( src.xyz );
		}
	}
	const vtx_skinned* drawVerts = transformedVerts;

#else

	const vtx_skinned* drawVerts = verts;

#endif

	for( int iTri = 0; iTri < numTriangles; iTri++ )
	{
		Vec3D pA = transform.TransformPoint( drawVerts[ indices[ iTri*3 + 0 ] ].xyz );
		Vec3D pB = transform.TransformPoint( drawVerts[ indices[ iTri*3 + 1 ] ].xyz );
		Vec3D pC = transform.TransformPoint( drawVerts[ indices[ iTri*3 + 2 ] ].xyz );

		dbgDrawer.DrawTriangle3D( pA, pB, pC, color );
	}
}
#endif
void Model::Dbg_DrawMesh( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color )
{
	const Model& model = *this;

	const int numMeshes = model.meshes.Num();
	const Mesh* meshes = model.meshes.ToPtr();
	//const md5_joint* joints = model.joints.ToPtr();

	// Draw each triangle of the model. Slow!
	for( int iMesh = 0; iMesh < numMeshes; iMesh++ )
	{
		const Mesh & mesh = meshes[ iMesh ];

		const Vec3D* positions = mesh.transformedPositions.ToPtr();

		for( int iTri = 0; iTri < mesh.triangles.Num(); iTri++ )
		{
			const md5_triangle & tri = mesh.triangles[ iTri ];

			// seems i need to reverse the winding

			Vec3D pA = transform.TransformPoint( positions[ tri.indices[ 2 ] ] );
			Vec3D pB = transform.TransformPoint( positions[ tri.indices[ 1 ] ] );
			Vec3D pC = transform.TransformPoint( positions[ tri.indices[ 0 ] ] );

			dbgDrawer.DrawWireTriangle( TO_FLOAT3 pA, TO_FLOAT3 pB, TO_FLOAT3 pC, color );
		}
	}
}

bool CheckModelAnim( const Model& model, const Animation& anim )
{
	bool bOk = true;

	if ( anim.NumJoints() != model.NumJoints() ) {
		//mxErrf( "Model '%s' has different # of joints than anim '%s'", model.Name(), name.c_str() );
		ptERROR("Mismatch in number of joints.\n");
		bOk &= false;
	}

#if 0

	int	i;
	int	jointNum;
	int	parent;

	const md5_joint *modelJoints = model.joints.ToPtr();
	for( i = 0; i < anim.jointInfo.Num(); i++ )
	{
		jointNum = anim.jointInfo[ i ].nameIndex;
		if ( modelJoints[ i ].name != animationLib.JointName( jointNum ) ) {
			//mxErrf( "Model '%s''s joint names don't match anim '%s''s", model.Name(), name.c_str() );
			ptERROR("Mismatch in joint names.\n");
			bOk &= false;
		}
		if ( modelJoints[ i ].parentNum ) {
			parent = modelJoints[ i ].parentNum - modelJoints;
		} else {
			parent = -1;
		}
		if ( parent != anim.jointInfo[ i ].parentNum ) {
			//mxErrf( "Model '%s' has different joint hierarchy than anim '%s'", model.Name(), name.c_str() );
			ptERROR("Mismatch in joint hierarchy.\n");
			bOk &= false;
		}
	}
#endif

	return bOk;
}

#if 0
ERet MySkeleton_FromMd5Mesh( const md5_model& _md5model, rxSkeleton &_skel )
{
	const int numBones = _md5model.joints.Num();

	_skel.joints.SetNum(numBones);
	_skel.jointNames.SetNum(numBones);
	_skel.invBindPoses.SetNum(numBones);

	for( int i = 0; i < numBones; i++ )
	{
		_skel.joints[i].orientation = (V4F4&) _md5model.joints[i].orientation;
		_skel.joints[i].position = (V3F4&) _md5model.joints[i].position;
		_skel.joints[i].parent = _md5model.joints[i].parentNum;

		_skel.jointNames[i] = _md5model.joints[i].name;

		_skel.invBindPoses[i] = (M44F4&)(_md5model.inverseBindPose[i]);
	}

	return ALL_OK;
}
ERet MyAnimClip_FromMd5Anim( const md5_anim& _md5anim, AnimClip &_animClip )
{
	TStaticArray< md5_joint, 256 > skeletonJoints;

	const int numJoints = _md5anim.NumJoints();
	_animClip.tracks.SetNum(numJoints);
	_animClip.nodes.SetNum(numJoints);

	const float secondsPerFrame = 1.0f / _md5anim.frameRate;

	_animClip.length = _md5anim.numFrames * secondsPerFrame;

	//_animClip.name = _md5anim.name;

	for( int iJoint = 0; iJoint < numJoints; ++iJoint )
	{
		const jointAnimInfo_t* jointInfos = _md5anim.jointInfo.ToPtr();
		const md5_baseFrameJoint* baseFrames = _md5anim.baseFrame.ToPtr();

		const jointAnimInfo_t& jointInfo = jointInfos[ iJoint ];
		const md5_baseFrameJoint& baseFrameJoint = baseFrames[iJoint];

		_animClip.tracks[iJoint].translations.keys.SetNum(_md5anim.numFrames);
		_animClip.tracks[iJoint].translations.times.SetNum(_md5anim.numFrames);
		_animClip.tracks[iJoint].rotations.keys.SetNum(_md5anim.numFrames);
		_animClip.tracks[iJoint].rotations.times.SetNum(_md5anim.numFrames);
		_animClip.nodes[iJoint] = jointInfo.name;

		md5_joint & skeletonJoint = skeletonJoints[ iJoint ];

		for( int frameNum = 0; frameNum < _md5anim.numFrames; ++frameNum )
		{
			const float frameTime = frameNum * secondsPerFrame;
			const float* animFrameData = _md5anim.componentFrames.ToPtr() + _md5anim.numAnimatedComponents * frameNum;

			// Start with the base frame position and orientation.

			skeletonJoint.orientation	= baseFrameJoint.q;
			skeletonJoint.position		= baseFrameJoint.t;
			skeletonJoint.parentNum		= jointInfo.parentNum;
			skeletonJoint.name			= jointInfo.name;

			// Build the skeleton pose for a single frame of the animation.
			// This is done by combining the base frames with the frame components array.

			const int animFlags = jointInfo.animBits;
			const int firstAnimComponent = jointInfo.firstComponent;
			int j = 0;

			if ( animFlags & ANIM_TX ) // Pos.x
			{
				skeletonJoint.position.x = animFrameData[ firstAnimComponent + j++ ];
			}
			if ( animFlags & ANIM_TY ) // Pos.y
			{
				skeletonJoint.position.y = animFrameData[ firstAnimComponent + j++ ];
			}
			if ( animFlags & ANIM_TZ ) // Pos.x
			{
				skeletonJoint.position.z  = animFrameData[ firstAnimComponent + j++ ];
			}
			if ( animFlags & ANIM_QX ) // Orient.x
			{
				skeletonJoint.orientation.x = animFrameData[ firstAnimComponent + j++ ];
			}
			if ( animFlags & ANIM_QY ) // Orient.y
			{
				skeletonJoint.orientation.y = animFrameData[ firstAnimComponent + j++ ];
			}
			if ( animFlags & ANIM_QZ ) // Orient.z
			{
				skeletonJoint.orientation.z = animFrameData[ firstAnimComponent + j++ ];
			}
			skeletonJoint.orientation.w = skeletonJoint.orientation.CalcW();
#if 0
			const int jointParentIndex = jointInfo.parentNum;

			if( jointParentIndex != -1 )	// Has a parent joint
			{
				const md5_joint& parentJoint = skeletonJoints[ jointParentIndex ];

				// Concatenate transforms.
				Vec3D	rotatedPos = parentJoint.orientation * skeletonJoint.position;
				skeletonJoint.position = parentJoint.position + rotatedPos;

				skeletonJoint.orientation = skeletonJoint.orientation * parentJoint.orientation;
				skeletonJoint.orientation.NormalizeFast();
			}
#endif
			_animClip.tracks[iJoint].translations.keys[frameNum] = TO_FLOAT3 skeletonJoint.position;
			_animClip.tracks[iJoint].translations.times[frameNum] = frameTime;
			_animClip.tracks[iJoint].rotations.keys[frameNum] = TO_FLOAT4 skeletonJoint.orientation;
			_animClip.tracks[iJoint].rotations.times[frameNum] = frameTime;
		}
	}
	return ALL_OK;
}
#endif

}//namespace md5
