#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/BuildConfig.h>
#include <Rendering/Public/Scene/CameraView.h>


namespace Rendering
{

void NwCameraView::recomputeDerivedMatrices()
{
	this->near_clipping_plane = Plane_FromPointNormal( this->eye_pos, this->look_dir );

	const V3f up_dir = getUpDir();
	this->view_matrix = M44_BuildView( this->right_dir, this->look_dir, up_dir, this->eye_pos );
	this->inverse_view_matrix = M44_OrthoInverse( this->view_matrix );

#if nwRENDERER_USE_REVERSED_DEPTH

	M44_ProjectionAndInverse_D3D_ReverseDepth_InfiniteFarPlane(
		&this->projection_matrix, &this->inverse_projection_matrix
		, this->half_FoV_Y*2.0f
		, this->aspect_ratio
		, this->near_clip
		);

#else

	M44_ProjectionAndInverse_D3D(
		&this->projection_matrix, &this->inverse_projection_matrix
		, this->half_FoV_Y*2.0f
		, this->aspect_ratio
		, this->near_clip
		, this->far_clip
		);

#endif

	this->view_projection_matrix = M44_Multiply( this->view_matrix, projection_matrix );
	this->inverse_view_projection_matrix = M44_Inverse( this->view_projection_matrix );
}

bool NwCameraView::projectViewSpaceToScreenSpace(
	const V3f& position_in_view_space
	, V2f *position_in_screen_space_
	) const
{
	const V4f posH = view_projection_matrix.projectPoint( position_in_view_space );
	if( posH.w < 0.0f ) {
		return false;	// behind the eye
	}

	// [-1..+1] -> [0..W]
	float x = (posH.x + 1.0f) * (screen_width * 0.5f);
	// [-1..+1] -> [H..0]
	float y = (1.0f - posH.y) * (screen_height * 0.5f);

	*position_in_screen_space_ = V2f( x, y );

	return true;
}

const ViewFrustum NwCameraView::BuildViewFrustum() const
{
	ViewFrustum	view_frustum;
	view_frustum.extractFrustumPlanes_Generic(
		this->eye_pos
		, this->right_dir
		, this->look_dir
		, this->getUpDir()
		, this->half_FoV_Y
		, this->aspect_ratio
		, this->near_clip
		, this->far_clip
		);
	return view_frustum;
}

}//namespace Rendering
