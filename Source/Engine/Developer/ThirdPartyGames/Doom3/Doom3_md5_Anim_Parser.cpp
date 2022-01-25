#include <Base/Base.h>
#pragma hdrstop
#include <algorithm>
#include <Base/Template/Containers/Blob.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Text/Lexer.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_Constants.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_MD5_Model_Parser.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_md5_Anim_Parser.h>


namespace Doom3
{

// Build skeleton for a given frame data.
static void BuildFrameSkeleton(
	const md5Animation& anim
	, const MD5LoadOptions& options
	, const int frameNum
	, md5Skeleton &skeleton
)
{
	const int numJoints = anim.NumJoints();
	const md5JointAnimInfo* jointInfos = anim.jointInfo.raw();
	const md5_baseFrameJoint* baseFrames = anim.baseFrame.raw();
	const float* animFrameData = anim.componentFrames.raw() + anim.numAnimatedComponents * frameNum;

	skeleton.setNum( numJoints );

	for( int iJoint = 0; iJoint < numJoints; ++iJoint )
	{
		const md5JointAnimInfo& jointInfo = jointInfos[ iJoint ];
		const md5_baseFrameJoint& baseFrameJoint = baseFrames[iJoint];

		md5Joint & skeletonJoint = skeleton[ iJoint ];

		// Start with the base frame position and orientation.
		// It is described relative to the joint's parent.

		skeletonJoint.orientation	= baseFrameJoint.q;
		skeletonJoint.position		= baseFrameJoint.t;
		skeletonJoint.parentNum		= jointInfo.parentNum;

		// Build the skeleton pose for a single frame of the animation.
		// This is done by combining the base frames with the frame components array.

		const int animFlags = jointInfo.animBits;
		const int firstAnimComponent = jointInfo.firstComponent;
		int j = 0;



// bit flags which tell you how to compute the skeleton of a frame for a joint
//
#define	ANIM_TX		BIT( 0 )
#define	ANIM_TY		BIT( 1 )
#define	ANIM_TZ		BIT( 2 )
#define	ANIM_QX		BIT( 3 )
#define	ANIM_QY		BIT( 4 )
#define	ANIM_QZ		BIT( 5 )



		if ( animFlags & ANIM_TX ) // Pos.x
		{
			if(options.scale_to_meters) {
				skeletonJoint.position.x = animFrameData[ firstAnimComponent + j++ ] * DOOM3_TO_METERS;
			} else {
				skeletonJoint.position.x = animFrameData[ firstAnimComponent + j++ ];
			}
		}
		if ( animFlags & ANIM_TY ) // Pos.y
		{
			if(options.scale_to_meters) {
				skeletonJoint.position.y = animFrameData[ firstAnimComponent + j++ ] * DOOM3_TO_METERS;
			} else {
				skeletonJoint.position.y = animFrameData[ firstAnimComponent + j++ ];
			}
		}
		if ( animFlags & ANIM_TZ ) // Pos.x
		{
			if(options.scale_to_meters) {
				skeletonJoint.position.z = animFrameData[ firstAnimComponent + j++ ] * DOOM3_TO_METERS;
			} else {
				skeletonJoint.position.z = animFrameData[ firstAnimComponent + j++ ];
			}
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


		mxWHY("need to use inverse orientation quaternion to convert from md5 to ozz?");
		skeletonJoint.ozz_orientation = skeletonJoint.orientation.inverse();
		skeletonJoint.ozz_position = skeletonJoint.position;

		// To build the final skeleton joint in object-local space,
		// you have to add the position and orientation of the joint’s parent.

		const int jointParentIndex = jointInfo.parentNum;

		if( jointParentIndex != -1 )	// Has a parent joint
		{
			const md5Joint& parentJoint = skeleton[ jointParentIndex ];

			// Concatenate transforms.
			V3f	rotatedPos = parentJoint.orientation.rotateVector( skeletonJoint.position );
			skeletonJoint.position = parentJoint.position + rotatedPos;

			skeletonJoint.orientation = skeletonJoint.orientation.MulWith( parentJoint.orientation );
			skeletonJoint.orientation.NormalizeSelf();
		}
	}
}

ERet MD5_LoadAnimFromFile( md5Animation &anim_
						  , const char* fileName
						  , const MD5LoadOptions& options
						  )
{
	NwBlob	fileData(MemoryHeaps::temporary());
	mxTRY(NwBlob_::loadBlobFromFile( fileData, fileName ));

	return md5_LoadAnimFromMemory(
		anim_
		, fileData.raw()
		, fileData.rawSize()
		, options
		);
}

ERet md5_LoadAnimFromMemory( md5Animation &anim_
							, const void* data
							, size_t size
							, const MD5LoadOptions& options
							)
{
	int		version;
	Lexer	parser( data, size );
	TbToken	token;
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

	anim_.clear();

	// parse num frames
	Expect( parser, "numFrames" );
	anim_.numFrames = expectInt( parser );
	if ( anim_.numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", anim_.numFrames );
	}

	// parse num joints
	Expect( parser, "numJoints" );
	anim_.numJoints = expectInt( parser );
	if ( anim_.numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", anim_.numJoints );
	}

	// parse frame rate
	Expect( parser, "frameRate" );
	anim_.frameRate = expectInt( parser );
	if ( anim_.frameRate <= 0 ) {
		parser.Error( "Invalid frame rate: %d", anim_.frameRate );
	}

	// parse num animated components
	Expect( parser, "numAnimatedComponents" );
	anim_.numAnimatedComponents = expectInt( parser );
	if ( anim_.numAnimatedComponents <= 0 ) {
		parser.Error( "Invalid number of animated components: %d", anim_.numAnimatedComponents );
	}


	// parse the hierarchy
	mxDO(anim_.jointInfo.setNum( anim_.numJoints ));
	Expect( parser, "hierarchy" );
	Expect( parser, "{" );
	for( i = 0; i < anim_.numJoints; i++ )
	{
		parser.ReadToken( token );
		anim_.jointInfo[ i ].name = token.text;

		// parse parent num
		anim_.jointInfo[ i ].parentNum = expectInt( parser );
		if ( anim_.jointInfo[ i ].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", anim_.jointInfo[ i ].parentNum );
		}

		if ( ( i != 0 ) && ( anim_.jointInfo[ i ].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		anim_.jointInfo[ i ].animBits = expectInt( parser );
		if ( anim_.jointInfo[ i ].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", anim_.jointInfo[ i ].animBits );
		}

		// parse first component
		anim_.jointInfo[ i ].firstComponent = expectInt( parser );
		if ( ( anim_.numAnimatedComponents > 0 ) && ( ( anim_.jointInfo[ i ].firstComponent < 0 ) || ( anim_.jointInfo[ i ].firstComponent >= anim_.numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", anim_.jointInfo[ i ].firstComponent );
		}
	}
	Expect( parser, "}" );

	// parse bounds
	Expect( parser, "bounds" );
	Expect( parser, "{" );
	anim_.bounds.setNum( anim_.numFrames );
	for( i = 0; i < anim_.numFrames; i++ )
	{
		Parse1DMatrix( parser, 3, (float*)&(anim_.bounds[ i ].min_corner) );
		Parse1DMatrix( parser, 3, (float*)&(anim_.bounds[ i ].max_corner) );

		if(options.scale_to_meters) {
			anim_.bounds[ i ].min_corner *= DOOM3_TO_METERS;
			anim_.bounds[ i ].min_corner *= DOOM3_TO_METERS;
		}
	}
	Expect( parser, "}" );

	// parse base frame
	mxDO(anim_.baseFrame.setNum( anim_.numJoints ));
	Expect( parser, "baseframe" );
	Expect( parser, "{" );
	for( i = 0; i < anim_.numJoints; i++ )
	{
		Q3f q;
		Parse1DMatrix( parser, 3, (float*)&anim_.baseFrame[ i ].t );
		Parse1DMatrix( parser, 3, (float*)&q );//baseFrame[ i ].q.raw() );
		anim_.baseFrame[ i ].q = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();

		if(options.scale_to_meters) {
			anim_.baseFrame[ i ].t *= DOOM3_TO_METERS;
		}
	}
	Expect( parser, "}" );

	anim_.baseFrame[MD5_ROOT_JOINT_INDEX].t += options.root_joint_offset;

	// parse frames
	mxDO(anim_.componentFrames.setNum( anim_.numAnimatedComponents * anim_.numFrames ));

	float *componentPtr = anim_.componentFrames.raw();
	for( i = 0; i < anim_.numFrames; i++ ) {
		Expect( parser, "frame" );
		int num = expectInt( parser );
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		Expect( parser, "{" );

		for( j = 0; j < anim_.numAnimatedComponents; j++, componentPtr++ ) {
			*componentPtr = expectFloat( parser );
		}

		Expect( parser, "}" );
	}

	//MX_STATS( mxPutf("Parsed animation '%s' in %u milliseconds\n",
	//	stream->GetURI().ToChars(), mxGetMilliseconds() - startTime )
	//	);

	// Build the skeleton pose for a single frame of the animation.
	anim_.frameSkeletons.setNum( anim_.numFrames );
	for( int iFrame = 0; iFrame < anim_.numFrames; iFrame++ )
	{
		BuildFrameSkeleton(
			anim_
			, options
			, iFrame
			, anim_.frameSkeletons[ iFrame ]
		);
	}

	// done
	return ALL_OK;
}


bool CheckModelAnim( const md5Model& model, const md5Animation& anim )
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

	const md5_joint *modelJoints = model.joints.raw();
	for( i = 0; i < anim.jointInfo.num(); i++ )
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

}//namespace Doom3
