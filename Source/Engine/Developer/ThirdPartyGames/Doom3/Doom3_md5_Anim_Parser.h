#pragma once

//
#include <Developer/ThirdPartyGames/Doom3/Doom3_md5_Common.h>


namespace Doom3
{

struct md5Model;


struct md5JointAnimInfo
{
	String64	name;
	int			parentNum;	// index of the parent joint; this is the root joint if (-1)
	int			animBits;	// used to compute the frame's skeleton
	int			firstComponent;	// index of the first component for computing the frame's skeleton
};

// base frame joint transform
struct md5_baseFrameJoint
{
	Q4f		q;	// joint's orientation relative to the parent joint
	V3f		t;	// joint's position relative to the parent joint
	int		pad;// so that it's binary compatible with md5Joint
};

typedef TArray< md5Joint >	md5Skeleton;

/// information about skeletal animation of MD5 Mesh model
struct md5Animation
{
	int		numFrames;	// number of frames of the animation
	int		numJoints;	// number of joints of the frame skeletons
	int		frameRate;	// number of frames per second to draw for the animation (24 for MD5)
	int		numAnimatedComponents;	// number of parameters per frame used to compute the frame skeletons

	//float	length_in_seconds;

	String32	name;

	TArray< md5JointAnimInfo >	jointInfo;	// hierarchy information
	TArray< AABBf >				bounds;		// bounding boxes for each frame

	TArray< md5_baseFrameJoint >	baseFrame;	// position and orientation of each joint from which the frame skeletons will be built
	TArray< float >					componentFrames;	// parameters used to compute the frame's skeleton

	// built from the above data
	TArray< md5Skeleton >		frameSkeletons;

public:
	md5Animation()
	{
		this->clear();
	}

	int NumJoints() const { return jointInfo.num(); }

	void clear()
	{
		numFrames = 0;
		numJoints = 0;
		frameRate = 0;
		numAnimatedComponents = 0;

		jointInfo.RemoveAll();
		bounds.RemoveAll();
		baseFrame.RemoveAll();
		componentFrames.RemoveAll();
	}

	bool IsValid() const
	{
		return numFrames > 0;
	}

	float frameDurationInSeconds() const {
		return 1.0f / frameRate;
	}

	float totalDurationInSeconds() const {
		return frameDurationInSeconds() * numFrames;
	}
};

bool CheckModelAnim( const md5Model& model, const md5Animation& anim );





ERet MD5_LoadAnimFromFile( md5Animation &anim_
						  , const char* fileName
						  , const MD5LoadOptions& options = MD5LoadOptions()
						  );

ERet md5_LoadAnimFromMemory( md5Animation &anim_
							, const void* data
							, size_t size
							, const MD5LoadOptions& options = MD5LoadOptions()
							);

}//namespace Doom3
