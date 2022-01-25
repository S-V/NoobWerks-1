// Sky rendering.
// UNFINISHED
#include <Base/Base.h>
#pragma hdrstop
#include <Core/ObjectModel/Clump.h>
#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Private/Modules/Sky/SkyModel.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/Shaders/Shared/nw_shared_globals.h>
#include <Rendering/Public/Settings.h>
#include <Rendering/Public/Core/RenderPipeline.h>

namespace Rendering
{
ERet DrawSkybox_TriStrip_NoVBIB(
	NGpu::NwRenderContext & render_context,
	const NwRenderState32& states,
	const HProgram program
	)
{
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	IF_DEVELOPER NGpu::Dbg::GpuScope	perfScope( render_context._command_buffer, "Skybox", RGBAf::lightskyblue.ToRGBAi().u );

	//
	NGpu::Cmd_Draw	dip;
	dip.program = program;
	dip.input_layout.SetNil();
	dip.primitive_topology = NwTopology::TriangleStrip;
	dip.use_32_bit_indices = false;
	dip.VB.SetNil();
	dip.IB.SetNil();
	dip.base_vertex = 0;
	dip.vertex_count = 14;	// TriStrip
	dip.start_index = 0;
	dip.index_count = 0;

	IF_DEBUG dip.src_loc = GFX_SRCFILE_STRING;

	cmd_writer.SetRenderState( states );
	cmd_writer.Draw( dip );

	return ALL_OK;
}

ERet renderSkybox_Analytic(
	NGpu::NwRenderContext & render_context
	, const NGpu::LayerID viewId
	)
{
	NwShaderEffect* technique = nil;
	mxDO(Resources::Load( technique, MakeAssetID("skybox") ));

	// 0 = analytic model
	const NwShaderFeatureBitMask variantId
		= technique->composeFeatureMask(
		mxHASH_STR("SKY_RENDERING_MODE")
		, 0
		);

	const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

	//
	NGpu::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		NGpu::buildSortKey( viewId, pass0.program_handles[variantId], 0 )
			nwDBG_CMD_SRCFILE_STRING
		);

	mxDO(DrawSkybox_TriStrip_NoVBIB(
		render_context
		, pass0.render_state
		, pass0.program_handles[variantId]
		));

	return ALL_OK;
}

ERet renderSkybox_EnvironmentMap(
	NGpu::NwRenderContext & render_context
	, const NGpu::LayerID viewId
	, const AssetID& environmentMapId
)
{
	//
	NwTexture* texture = nil;
	mxDO(Resources::Load( texture, environmentMapId ));

	//
	NwShaderEffect* technique = nil;
	mxDO(Resources::Load( technique, MakeAssetID("skybox") ));

	// 1 - skybox cube map
	const NwShaderFeatureBitMask variantId = technique->composeFeatureMask(mxHASH_STR("SKY_RENDERING_MODE"), 1);

	const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

	technique->SetInput(
		nwNAMEHASH("t_cubeMap"),
		texture->m_resource
		);

	//
	NGpu::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		NGpu::buildSortKey( viewId, pass0.program_handles[variantId], 0 )
			nwDBG_CMD_SRCFILE_STRING
		);

	mxDO(technique->pushShaderParameters( render_context._command_buffer ));

	mxDO(DrawSkybox_TriStrip_NoVBIB(
		render_context
		, pass0.render_state
		, pass0.program_handles[variantId]
		));

	return ALL_OK;
}

#if 0
void DetermineUVSphereSize(
						   int numStacks,	//!< horizontal layers
						   int numSlices,	//!< radial divisions
						   int &totalVertices
						   int &totalVertices
						   )
{
	// calculate the total number of vertices and indices
	const int numRings = numStacks-1;	// do not count the poles as rings
	const int numRingVertices = numSlices + 1;	// number of vertices in each horizontal ring
	const int totalVertices = numRings * numRingVertices + 2;	// number of vertices in each ring plus poles
	const int totalIndices = (numSlices * (numStacks-2)) * 6 + (numSlices*3)*2;	// quads in each ring plus triangles in poles

}
#endif

mxTEMP mxTODO("move to TbPrimitiveBatcher::DrawSolidSphere()")
/// Creates a UVSphere in Blender terms.
/// NOTE: Z is up (X - right, Y - forward).
/// NOTE: the density of vertices is greater around the poles, and UV coords are distorted.
/// see http://paulbourke.net/geometry/circlesphere/
/// http://vterrain.org/Textures/spherical.html
///
ERet GenerateUVSphere(
					  const V3f& center, float radius,
					  int numStacks,	//!< horizontal layers
					  int numSlices,	//!< radial divisions
					  DynamicArray< AuxVertex > &vertices,
					  DynamicArray< U32 > &indices
					  )
{
	UByte4 rgbaColor;mxTEMP
		rgbaColor.v = RGBAf::RED.ToRGBAi().u;

	// This approach is much like a globe in that there is a certain number of vertical lines of latitude
	// and horizontal lines of longitude which break the sphere up into many rectangular (4 sided) parts.
	// Note however there will be a point at the top and bottom (north and south pole)
	// and polygons attached to these will be triangular (3 sided).
	// These are easiest to generate
	// and the number of polygons will be equal to: LONGITUDE_LINES * (LATITUDE_LINES + 1).
	// Of these (LATITUDE_LINES*2) will be triangular and connected to a pole, and the rest rectangular.

	// calculate the total number of vertices and indices
	const int numRings = numStacks-1;	// do not count the poles as rings
	const int numRingVertices = numSlices + 1;	// number of vertices in each horizontal ring
	const int totalVertices = numRings * numRingVertices + 2;	// number of vertices in each ring plus poles
	const int totalIndices = (numSlices * (numStacks-2)) * 6 + (numSlices*3)*2;	// quads in each ring plus triangles in poles

	mxDO(vertices.setNum( totalVertices ));
	mxDO(indices.setNum( totalIndices ));

	const int iBaseVertex = 0;

	int	createdVertices = 0;
	int	createdIndices = 0;

	// use polar coordinates to generate slices of the sphere

	//       Z   Phi (Zenith)
	//       |  /
	//       | / /'
	//       |  / '
	//       | /  '
	//       |/___'________Y
	//      / \ - .
	//     /   \  '\
	//    /     \ ' Theta (Azimuth)
	//   X       \'

	const float phiStep = mxPI / (float) numStacks;	// vertical angle delta

	// Compute vertices for each stack ring.
	for ( int i = 1; i <= numRings; ++i )
	{
		const float phi = i * phiStep;	// polar angle (vertical) or inclination, latitude, [0..PI]

		float sinPhi, cosPhi;
		mmSinCos( phi, sinPhi, cosPhi );

		// vertices of ring
		const float thetaStep = 2.0f * mxPI / numSlices;	// horizontal angle delta
		for ( int j = 0; j <= numSlices; ++j )
		{
			const float theta = j * thetaStep;	// azimuthal angle (horizontal) or azimuth, longitude, [0..2*PI]

			float sinTheta, cosTheta;			
			mmSinCos( theta, sinTheta, cosTheta );

			AuxVertex & vertex = vertices[ createdVertices++ ];

			// spherical to cartesian
			vertex.xyz.x = radius * sinPhi * cosTheta;
			vertex.xyz.y = radius * sinPhi * sinTheta;
			vertex.xyz.z = radius * cosPhi;
			vertex.xyz += center;
			vertex.rgba = rgbaColor;

			V3f normal = V3_Normalized(vertex.xyz);
			vertex.N = PackNormal(normal.x, normal.y, normal.z);

			V3f tangent;
			// partial derivative of P with respect to theta
			tangent.x = -radius * sinPhi * sinTheta;
			tangent.y = radius * sinPhi * cosTheta;
			tangent.z = 0.0f;
			//vertex.T = PackNormal(tangent.x, tangent.y, tangent.z);

			V2f	uv;
			uv.x = theta / (2.0f*mxPI );
			uv.y = phi / mxPI;

			vertex.uv = V2_To_Half2( uv );
		}
	}

	// square faces between intermediate points:

	// Compute indices for inner stacks (not connected to poles).
	for ( int i = 0; i < numStacks-2; ++i )	// number of horizontal layers, excluding caps
	{
		for( int j = 0; j < numSlices; ++j )	// number of radial divisions
		{
			// one quad, two triangles (CCW is front)
			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * i     + j;	// 1
			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * i     + j+1;	// 2
			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * (i+1) + j;	// 3

			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * (i+1) + j;	// 3
			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * i     + j+1;	// 2
			indices[ createdIndices++ ] = iBaseVertex + numRingVertices * (i+1) + j+1;	// 4
		}
	}

	const AuxIndex northPoleIndex = iBaseVertex + createdVertices;
	const AuxIndex southPoleIndex = northPoleIndex + 1;

	// poles: note that there will be texture coordinate distortion
	AuxVertex & northPole = vertices[ createdVertices++ ];
	northPole.xyz = center;
	northPole.xyz.z += radius;
	northPole.uv = V2_To_Half2(V2_Set(0.0f, 1.0f));
	northPole.N = PackNormal( 0.0f, 0.0f, 1.0f );
	//northPole.T = PackNormal( 1.0f, 0.0f, 0.0f );
	northPole.rgba = rgbaColor;

	AuxVertex & southPole = vertices[ createdVertices++ ];
	southPole.xyz = center;
	southPole.xyz.z -= radius;
	southPole.uv = V2_To_Half2(V2_Set(0.0f, 1.0f));
	southPole.N = PackNormal( 0.0f, 0.0f, -1.0f );
	//southPole.T = PackNormal( 1.0f, 0.0f, 0.0f );
	southPole.rgba = rgbaColor;

	// triangle faces connecting to top and bottom vertex:

	// Compute indices for top stack.  The top stack was written 
	// first to the vertex buffer.
	for( int i = 0; i < numSlices; ++i )
	{
		indices[ createdIndices++ ] = northPoleIndex;
		indices[ createdIndices++ ] = iBaseVertex + i + 1;
		indices[ createdIndices++ ] = iBaseVertex + i;
	}

	// Compute indices for bottom stack.  The bottom stack was written
	// last to the vertex buffer, so we need to offset to the index
	// of first vertex in the last ring.
	int baseIndex = ( numRings - 1 ) * numRingVertices;
	for( int i = 0; i < numSlices; ++i )
	{
		indices[ createdIndices++ ] = southPoleIndex;
		indices[ createdIndices++ ] = baseIndex + i;
		indices[ createdIndices++ ] = baseIndex + i + 1;
	}

	return ALL_OK;
}


static
ERet GenerateUVSphere2(
					  const V3f& center, float radius,
					  int numStacks,	//!< horizontal layers
					  int numSlices,	//!< radial divisions
					  DynamicArray< AuxVertex > &vertices,
					  DynamicArray< U32 > &indices
						 )
{
	float spherePercentage = 1;
	float texturePercentage = 1;

	// Create sky dome geometry.

	FLOAT azimuth;
	UINT k;

	const FLOAT azimuth_step = (mxPI * 2.f) / numStacks;
	if( spherePercentage < 0.f ) {
		spherePercentage = -spherePercentage;
	}
	if( spherePercentage > 2.f ) {
		spherePercentage = 2.f;
	}
	const FLOAT elevation_step = spherePercentage * mxHALF_PI / (FLOAT)numSlices;

	const UINT numVertices = (numStacks + 1) * (numSlices + 1);
	const UINT numIndices = 3 * (2*numSlices - 1) * numStacks;

	//const UINT vertex_data_size = numVertices * sizeof(DomeVertex);
	//const UINT sizeOfIndexData = numIndices * sizeof(SkyIndex);

	vertices.setNum( numVertices );
	indices.setNum( numIndices );

	AuxVertex vertex;

	UINT iVertex = 0;
	UINT iIndex = 0;

	const FLOAT tcV = texturePercentage / numSlices;

	for( k = 0, azimuth = 0; k <= numStacks; ++k )
	{
		FLOAT elevation = mxHALF_PI;

		const FLOAT tcU = (FLOAT)k / (FLOAT)numStacks;

		FLOAT sinA, cosA;
		mmSinCos( azimuth, sinA, cosA );

		for( UINT j = 0; j <= numSlices; ++j )
		{
			FLOAT sinEl, cosEl;
			mmSinCos( elevation, sinEl, cosEl );

			const FLOAT cosEr = radius * cosEl;
			vertex.xyz = V3_Set( cosEr*sinA, radius*sinEl, cosEr*cosA );
			vertex.uv = V2_To_Half2(V2_Set( tcU, j*tcV ));

			//vertex.Normal = -vertex.XYZ;
			//vertex.Normal.Normalize();

			vertices[ iVertex++ ] = vertex;

			elevation -= elevation_step;
		}
		azimuth += azimuth_step;
	}

	for( k = 0; k < numStacks; ++k )
	{
		indices[ iIndex++ ] = (numSlices + 2 + (numSlices + 1)*k);
		indices[ iIndex++ ] = (1 + (numSlices + 1)*k);
		indices[ iIndex++ ] = (0 + (numSlices + 1)*k);

		for( UINT j = 1; j < numSlices; ++j )
		{
			indices[ iIndex++ ] = (numSlices + 2 + (numSlices + 1)*k + j);
			indices[ iIndex++ ] = (1 + (numSlices + 1)*k + j);
			indices[ iIndex++ ] = (0 + (numSlices + 1)*k + j);

			indices[ iIndex++ ] = (numSlices + 1 + (numSlices + 1)*k + j);
			indices[ iIndex++ ] = (numSlices + 2 + (numSlices + 1)*k + j);
			indices[ iIndex++ ] = (0 + (numSlices + 1)*k + j);
		}
	}

	return ALL_OK;
}

mxDEFINE_CLASS( SkyDome );
mxBEGIN_REFLECTION( SkyDome )
	mxMEMBER_FIELD( m_skyTexture ),
mxEND_REFLECTION
SkyDome::SkyDome()
{
}

ERet SkyDome::Initialize( const AssetID& _textureId, NwClump * _storage )
{
	AllocatorI &	scratch = MemoryHeaps::temporary();

	mxDO(Resources::Load( m_skyTexture._ptr, _textureId, _storage ));

	// Create sky dome geometry.
	DynamicArray< AuxVertex >	vertices( scratch );
	DynamicArray< U32 >	indices( scratch );

	mxDO(GenerateUVSphere(
		V3_Zero(),
		1.0f,
		16,
		16,
		vertices,
		indices
	));

	m_numVertices = vertices.num();
	m_numIndices = indices.num();
UNDONE;
	//NwBufferDescription	VBdesc(_InitDefault);
	//VBdesc.type = Buffer_Vertex;
	//VBdesc.size = vertices.rawSize();
	//VBdesc.stride = sizeof(vertices[0]);
	//IF_DEVELOPER VBdesc.name.setReference("SkyDomeVB");
	//m_VB = NGpu::CreateBuffer( VBdesc, NGpu::copy( vertices.raw(), vertices.rawSize() ) );

	//NwBufferDescription	IBdesc(_InitDefault);
	//IBdesc.type = Buffer_Index;
	//IBdesc.size = indices.rawSize();
	//IBdesc.stride = sizeof(indices[0]);
	//IF_DEVELOPER IBdesc.name.setReference("SkyDomeIB");
	//m_IB = NGpu::CreateBuffer( IBdesc, NGpu::copy( indices.raw(), indices.rawSize() ) );

	return ALL_OK;
}

void SkyDome::Shutdown()
{
	mxTEMP mxUNDONE
	//Resources::Unload
	NGpu::DeleteBuffer( m_VB );	m_VB.SetNil();
	NGpu::DeleteBuffer( m_IB );	m_IB.SetNil();
}

ERet SkyDome::Render(
	NGpu::NwRenderContext & context,
	const RenderPath& _path,
	const NwCameraView& _camera,
	const AssetID& _textureId,
	NwClump * _storage
)
{UNDONE;
#if 0
	const U32 passIndex = _path.getPassDrawingOrder( ScenePasses::SkyLast );
	mxENSURE( passIndex != ~0, ERR_OBJECT_NOT_FOUND, "No skydome!");

	RendererGlobals & globals = tr;

	NwRenderState *	renderState;
	mxDO(Resources::Load(renderState, MakeAssetID("skybox"), _storage));

	TbShader* shader;
	mxDO(Resources::Load(shader, MakeAssetID("skydome"), _storage));

	NwTexture *	skyTexture;
	mxDO(Resources::Load( skyTexture, _textureId, _storage ));
	mxDO(FxSlow_SetInput(shader, "t_skyTexture", skyTexture->m_resource));

#endif

	//const NGpu::BufferToUpdate dependency1 = SetupLights( sceneView, sceneData, context, stats, lightCount );

#if 0
	G_PerObject *	cb_per_objectData;
mxTEMP
	GL::BufferToUpdate	dependency;
	dependency.memory = context->Allocate( (void**)&cb_per_objectData, sizeof(*cb_per_objectData) );
	dependency.handle = globals._global_uniforms.per_model.handle;

	const float skyDomeScale = _camera.far_clip * 0.9f;
	M44f skyDomeWorldMatrix = M44_Scaling( skyDomeScale, skyDomeScale, skyDomeScale );
	M44_SetTranslation( &skyDomeWorldMatrix, _camera.origin );

	//const float skyDomeScale = 10;
	//M44f skyDomeWorldMatrix = M44_Scaling( skyDomeScale, skyDomeScale, skyDomeScale );
	//M44_SetTranslation( &skyDomeWorldMatrix, _camera.origin );

	M44f skyDomeWorldViewMatrix = skyDomeWorldMatrix * _camera.view_matrix;

	cb_per_objectData->g_world_matrix = M44_Transpose( skyDomeWorldMatrix );
	cb_per_objectData->g_world_view_matrix = M44_Transpose( skyDomeWorldViewMatrix );
	cb_per_objectData->g_world_view_projection_matrix = M44_Transpose( skyDomeWorldViewMatrix * _camera.projection_matrix );

#if 1
	GL::Cmd_Draw	batch;
	batch.clear();

	batch.state = *renderState;

	batch.buffersToUpdate.add( dependency );

	BindShaderInputs( batch, *shader );
	batch.program = shader->programs[0];

	batch.inputLayout = globals.inputLayouts[VTX_Aux];
	batch.topology = Topology::TriangleList;

	batch.VB[0] = m_VB;
	batch.IB = m_IB;
	batch.flags = MESH_USES_32BIT_IB;

	batch.base_vertex = 0;
	batch.vertex_count = m_numVertices;
	batch.start_index = 0;
	batch.index_count = m_numIndices;

	IF_DEVELOPER batch.name = "Skydome";

	context->Draw( passIndex, batch, 0 );
#endif
#endif
	return ALL_OK;
}


SkyModel_ClearSky::SkyModel_ClearSky()
{
}

ERet SkyModel_ClearSky::Render(
							   NGpu::NwRenderContext & context,
							   const RenderPath& _path,
							   const NwCameraView& _camera,
							   NwClump * _storage
							   )
{
	UNDONE;
#if 0
	const RrGlobalSettings& settings = g_settings;

	const GL::ViewIndex viewId = _path.getPassDrawingOrder(ScenePasses::SkyLast);
	mxASSERT( viewId != INDEX_NONE );

	TPtr< NwRenderState >	skybox_state;
	mxDO(Resources::Load(skybox_state._ptr, MakeAssetID("skybox"), _storage));

	TbShader* shader;
	mxDO(Resources::Load(shader, MakeAssetID("skybox_clearsky"), _storage));

	mxDO(DrawSkybox(
		context,
		viewId,
		*skybox_state,
		*shader
		));
#endif
	return ALL_OK;
}

#if 0
	// We'll use spherical coordinates to get the new position for the
	// sun. These are pretty simple to use and adapt pretty well to 
	// what we want to achieve here.

	float rho = DEFAULT_SPHERE_RADIUS * 0.95f;
	float phi = PI * currentTime * 0.01f;
	float theta = 0;

	// We want 12 hours of virtual time to correspond to the day cycle,
	// so the sun should start at half-sphere when time reaches a given
	// sunrise time (say 6am), and go on until the sunset time.

	float x = rho*sin(phi)*cos(theta);
	float y = rho*sin(phi)*sin(theta);
	float z = rho*cos(phi);

	return Vector3(x, y, z);
#endif

V3f spherical(float theta, float phi)
{
	float	sinPhi, cosPhi;
	mmSinCos( phi, sinPhi, cosPhi );

	float	sinTheta, cosTheta;
	mmSinCos( theta, sinTheta, cosTheta );

    return CV3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
}


SkyModel_Preetham::SkyModel_Preetham()
{
}

ERet SkyModel_Preetham::Initialize( const AssetID& _textureId, NwClump * _storage )
{
	return ALL_OK;
}

void SkyModel_Preetham::Shutdown()
{

}

ERet SkyModel_Preetham::Render(
	NGpu::NwRenderContext & context,
	const RenderPath& _path,
	const NwCameraView& _camera,
	NwClump * _storage
)
{
	UNDONE;
#if 0
const RrGlobalSettings& settings = g_settings;

float sunTheta = settings.sky.sunThetaDegrees;
float sunPhi = settings.sky.sunPhiDegrees;
float skyTurbidity = settings.sky.skyTurbidity;
Parameters skyParams = compute(DEG2RAD(sunTheta), skyTurbidity, 1.1f);


	TPtr< NwMesh >	unitSphereMesh;
	mxDO(Resources::Load( unitSphereMesh._ptr, MakeAssetID("unit_sphere_ico"), _storage ));	// [-1..+1]

	const float sphereRadius = _camera.far_clip * 0.999f;	// the largest sphere that is not depth-culled

	TPtr< NwRenderState >	renderStates;
	mxDO(Resources::Load(renderStates._ptr, MakeAssetID("skybox"), _storage));

	TPtr< TbShader >		skyShader;
	mxDO(Resources::Load(skyShader._ptr, MakeAssetID("sky_preetham"), _storage));


	GL::Cmd_Draw	batch;
	batch.clear();

	batch.state = *renderStates;

	const M44f worldMatrix = M44_Scaling(sphereRadius) * M44_buildTranslationMatrix( _camera.origin );
	const M44f worldViewMatrix = worldMatrix * _camera.view_matrix;
	const M44f worldViewProjection = worldViewMatrix * _camera.projection_matrix;

	SkyParameters	uniforms;
uniforms.worldViewProjection = M44_Transpose(worldViewProjection);
uniforms.sunDirection = Vector4_Set( spherical(DEG2RAD(sunTheta), DEG2RAD(sunPhi)), 0.0f );
uniforms.A = Vector4_Set( skyParams.A, 0.0f );
uniforms.B = Vector4_Set( skyParams.B, 0.0f );
uniforms.C = Vector4_Set( skyParams.C, 0.0f );
uniforms.D = Vector4_Set( skyParams.D, 0.0f );
uniforms.E = Vector4_Set( skyParams.E, 0.0f );
uniforms.Z = Vector4_Set( skyParams.Z, 0.0f );


mxDO(FxSlow_SetUniform(skyShader,"u_data",&uniforms));

const U32 passIndex = _path.getPassDrawingOrder(ScenePasses::SkyLast);

	const U32 seq = context->NextSequence();
	FxSlow_Commit( context, passIndex, skyShader, seq );


	BindShaderInputs( batch, *skyShader );
	batch.program = skyShader->programs[0];

	RendererGlobals & globals = tr;
	batch.inputLayout = globals.inputLayouts[ unitSphereMesh->m_vertexFormat ];

	batch.topology = unitSphereMesh->m_topology;

	batch.VB[0] = unitSphereMesh->m_vertexBuffer->m_handle;
	batch.IB = unitSphereMesh->m_indexBuffer->m_handle;
	batch.flags = unitSphereMesh->m_flags;

	batch.base_vertex = 0;
	batch.vertex_count = unitSphereMesh->m_numVertices;
	batch.start_index = 0;
	batch.index_count = unitSphereMesh->m_numIndices;

	IF_DEVELOPER batch.name = "SkyModel_Preetham";

	context->Draw( passIndex, batch, seq+1 );
#endif
	return ALL_OK;
}

namespace
{
	// A Practical Analytic Model for Daylight (A. J. Preetham, Peter Shirley, Brian Smits)

	static float perez(float theta, float gamma, float A, float B, float C, float D, float E)
	{
		return (1.f + A * exp(B / (cos(theta) + 0.01))) * (1.f + C * exp(D * gamma) + E * cos(gamma) * cos(gamma));
	}

	static float zenithLuminance(float sunTheta, float turbidity)
	{
		float chi = (4.f / 9.f - turbidity / 120) * (mxPI - 2 * sunTheta);

		return (4.0453 * turbidity - 4.9710) * tan(chi) - 0.2155 * turbidity + 2.4192;
	}

	static float zenithChromacity(const V4f& c0, const V4f& c1, const V4f& c2, float sunTheta, float turbidity)
	{
		V4f thetav = V4f::set( sunTheta * sunTheta * sunTheta, sunTheta * sunTheta, sunTheta, 1.0f );

		return (CV3f(turbidity * turbidity, turbidity, 1) * CV3f(V4f::dot(thetav, c0))) * (V4f::dot(thetav, c1) * V4f::dot(thetav, c2));
	}
}

SkyModel_Preetham::Parameters SkyModel_Preetham::compute(float sunTheta, float turbidity, float normalizedSunY)
{
    mxASSERT(sunTheta >= 0 && sunTheta <= mxPI / 2);
    mxASSERT(turbidity >= 1);
    
    // A.2 Skylight Distribution Coefficients and Zenith Values: compute Perez distribution coefficients
    V3f A = CV3f(-0.0193, -0.0167,  0.1787) * turbidity + CV3f(-0.2592, -0.2608, -1.4630);
    V3f B = CV3f(-0.0665, -0.0950, -0.3554) * turbidity + CV3f( 0.0008,  0.0092,  0.4275);
    V3f C = CV3f(-0.0004, -0.0079, -0.0227) * turbidity + CV3f( 0.2125,  0.2102,  5.3251);
    V3f D = CV3f(-0.0641, -0.0441,  0.1206) * turbidity + CV3f(-0.8989, -1.6537, -2.5771);
    V3f E = CV3f(-0.0033, -0.0109, -0.0670) * turbidity + CV3f( 0.0452,  0.0529,  0.3703);

    // A.2 Skylight Distribution Coefficients and Zenith Values: compute zenith color
    V3f Z;
	Z.x = zenithChromacity(V4f::set(0.00166, -0.00375, 0.00209, 0), V4f::set(-0.02903, 0.06377, -0.03202, 0.00394), V4f::set(0.11693, -0.21196, 0.06052, 0.25886), sunTheta, turbidity);
    Z.y = zenithChromacity(V4f::set(0.00275, -0.00610, 0.00317, 0), V4f::set(-0.04214, 0.08970, -0.04153, 0.00516), V4f::set(0.15346, -0.26756, 0.06670, 0.26688), sunTheta, turbidity);
    Z.z = zenithLuminance(sunTheta, turbidity);
    Z.z *= 1000; // conversion from kcd/m^2 to cd/m^2
    
    // 3.2 Skylight Model: pre-divide zenith color by distribution denominator
    Z.x /= perez(0, sunTheta, A.x, B.x, C.x, D.x, E.x);
    Z.y /= perez(0, sunTheta, A.y, B.y, C.y, D.y, E.y);
    Z.z /= perez(0, sunTheta, A.z, B.z, C.z, D.z, E.z);
    
    // For low dynamic range simulation, normalize luminance to have a fixed value for sun
    if (normalizedSunY)
    {
        Z.z = normalizedSunY / perez(sunTheta, 0, A.z, B.z, C.z, D.z, E.z);
    }

	TSwap(Z.z, Z.y);

	SkyModel_Preetham::Parameters	result = { A, B, C, D, E, Z };
    return result;
}

#if 0

// An Analytic Model for Full Spectral Sky-Dome Radiance (Lukas Hosek, Alexander Wilkie)

namespace
{
#include "Skymodel_Hosek_RGB.inl"

	static double evaluateSpline(const double* spline, size_t stride, double value)
	{
		return
			1 * pow(1 - value, 5) *                 spline[0 * stride] +
			5 * pow(1 - value, 4) * pow(value, 1) * spline[1 * stride] +
			10 * pow(1 - value, 3) * pow(value, 2) * spline[2 * stride] +
			10 * pow(1 - value, 2) * pow(value, 3) * spline[3 * stride] +
			5 * pow(1 - value, 1) * pow(value, 4) * spline[4 * stride] +
			1 *                     pow(value, 5) * spline[5 * stride];
	}

	static double evaluate(const double* dataset, size_t stride, float turbidity, float albedo, float sunTheta)
	{
		// splines are functions of elevation^1/3
		double elevationK = pow( (double)maxf(0.f, 1 - sunTheta / (mxPI / 2)), 1 / 3.0 );

		// table has values for turbidity 1..10
		int turbidity0 = clampf(static_cast<int>(turbidity), 1, 10);
		int turbidity1 = Min(turbidity0 + 1, 10);
		float turbidityK = clampf(turbidity - turbidity0, 0.f, 1.f);

		const double* datasetA0 = dataset;
		const double* datasetA1 = dataset + stride * 6 * 10;

		double a0t0 = evaluateSpline(datasetA0 + stride * 6 * (turbidity0 - 1), stride, elevationK);
		double a1t0 = evaluateSpline(datasetA1 + stride * 6 * (turbidity0 - 1), stride, elevationK);

		double a0t1 = evaluateSpline(datasetA0 + stride * 6 * (turbidity1 - 1), stride, elevationK);
		double a1t1 = evaluateSpline(datasetA1 + stride * 6 * (turbidity1 - 1), stride, elevationK);

		return
			a0t0 * (1 - albedo) * (1 - turbidityK) +
			a1t0 * albedo * (1 - turbidityK) +
			a0t1 * (1 - albedo) * turbidityK +
			a1t1 * albedo * turbidityK;
	}

	static V3f perezExt(float cos_theta, float gamma, float cos_gamma, V3f A, V3f B, V3f C, V3f D, V3f E, V3f F, V3f G, V3f H, V3f I)
	{
		V3f chi = (1 + cos_gamma * cos_gamma) / pow(1.f + H * H - 2.f * cos_gamma * H, CV3f(1.5f));

		return (1.f + A * expf(B / (cos_theta + 0.01f))) * (C + D * expf(E * gamma) + F * (cos_gamma * cos_gamma) + G * chi + I * sqrt(maxf(0.f, cos_theta)));
	}
}

SkyModel_Hosek::Parameters SkyModel_Hosek::compute(float sunTheta, float turbidity, float normalizedSunY)
{
    V3f A, B, C, D, E, F, G, H, I;
    V3f Z;
    
    for (int i = 0; i < 3; ++i)
    {
        float albedo = 0;
        
        A[i] = evaluate(datasetsRGB[i] + 0, 9, turbidity, albedo, sunTheta);
        B[i] = evaluate(datasetsRGB[i] + 1, 9, turbidity, albedo, sunTheta);
        C[i] = evaluate(datasetsRGB[i] + 2, 9, turbidity, albedo, sunTheta);
        D[i] = evaluate(datasetsRGB[i] + 3, 9, turbidity, albedo, sunTheta);
        E[i] = evaluate(datasetsRGB[i] + 4, 9, turbidity, albedo, sunTheta);
        F[i] = evaluate(datasetsRGB[i] + 5, 9, turbidity, albedo, sunTheta);
        G[i] = evaluate(datasetsRGB[i] + 6, 9, turbidity, albedo, sunTheta);
        
        // Note: H and I are swapped in the dataset; we use notation from paper
        H[i] = evaluate(datasetsRGB[i] + 8, 9, turbidity, albedo, sunTheta);
        I[i] = evaluate(datasetsRGB[i] + 7, 9, turbidity, albedo, sunTheta);
        
        Z[i] = evaluate(datasetsRGBRad[i], 1, turbidity, albedo, sunTheta);
    }

    if (normalizedSunY)
    {
		V3f S = V3_Multiply( perezExt(cosf(sunTheta), 0, 1.f, A, B, C, D, E, F, G, H, I), Z );
        
        Z /= V3f::dot(S, CV3f(0.2126, 0.7152, 0.0722));
        Z *= normalizedSunY;
    }

	SkyModel_Hosek::Parameters	result = { A, B, C, D, E, F, G, H, I, Z };
    return result;
}
#endif

Planet_Atmosphere::Planet_Atmosphere()
{
	m_fInnerRadius = 1000.0f;
	m_fOuterRadius = m_fInnerRadius * 1.025f;
	//m_fScale = 1 / (m_fOuterRadius - m_fInnerRadius);

	m_fWavelength[0] = 0.650f;		// 650 nm for red
	m_fWavelength[1] = 0.570f;		// 570 nm for green
	m_fWavelength[2] = 0.475f;		// 475 nm for blue

//	m_nSamples = 3;		// Number of sample rays to use in integral equation
	m_fKr = 0.0025f;		// Rayleigh scattering constant
//	m_Kr4PI = m_Kr*4.0f*PI;
	m_fKm = 0.0010f;		// Mie scattering constant
//	m_Km4PI = m_Km*4.0f*PI;
	m_fESun = 20.0f;		// Sun brightness constant
	m_fG = -0.990f;		// The Mie phase asymmetry factor
}

ERet Planet_Atmosphere::Initialize( const AssetID& _textureId, NwClump * _storage )
{
#if 0
	AllocatorI &	scratch = MemoryHeaps::temporary();

	mxDO(Resources::Load( m_skyTexture._ptr, _textureId, _storage ));

	// Create sky dome geometry.
	DynamicArray< AuxVertex >	vertices( scratch );
	DynamicArray< U32 >	indices( scratch );

	mxDO(GenerateUVSphere(
		V3_Zero(),
		1.0f,
		16,
		16,
		vertices,
		indices
	));

	m_numVertices = vertices.num();
	m_numIndices = indices.num();

	BufferDescription	VBdesc(_InitDefault);
	VBdesc.type = Buffer_Vertex;
	VBdesc.size = vertices.rawSize();
	VBdesc.stride = sizeof(vertices[0]);
	IF_DEVELOPER VBdesc.name.setReference("SkyDomeVB");
	m_VB = GL::CreateBuffer( VBdesc, vertices.raw() );

	BufferDescription	IBdesc(_InitDefault);
	IBdesc.type = Buffer_Index;
	IBdesc.size = indices.rawSize();
	IBdesc.stride = sizeof(indices[0]);
	IF_DEVELOPER IBdesc.name.setReference("SkyDomeIB");
	m_IB = GL::CreateBuffer( IBdesc, indices.raw() );
#endif
	return ALL_OK;
}

void Planet_Atmosphere::Shutdown()
{
#if 0
	mxTEMP mxUNDONE
	//Resources::Unload
	GL::DeleteBuffer( m_VB );	m_VB.SetNil();
	GL::DeleteBuffer( m_IB );	m_IB.SetNil();
#endif
}


ERet Planet_Atmosphere::Render(
	NGpu::NwRenderContext & context,
	const RenderPath& _path,
	const NwCameraView& _camera,
	NwClump * _storage
)
{
	return ALL_OK;
}


}//namespace Rendering


/* References:
 * Preetham, Shirley, Smits, "A practical model for daylight",
 *     http://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf
 * Nishita, Sirai, Tadamura, Nakamae, "Display of the Earth taking into account atmospheric scattering",
 *     http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.145.5229&rep=rep1&type=pdf
 * Hoffman, Preetham, "Rendering outdoor light scattering in real time"
 *     http://developer.amd.com/media/gpu_assets/ATI-LightScattering.pdf
 * Bruneton, Neyret, "Precomputed atmospheric scattering",
 *     http://hal.archives-ouvertes.fr/docs/00/28/87/58/PDF/article.pdf
 * Preetham, "Modeling skylight and aerial perspective",
 *     http://developer.amd.com/media/gpu_assets/PreethamSig2003CourseNotes.pdf
 * http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
 * http://www.springerlink.com/content/nrq497r40xmw4821/fulltext.pdf
 */
