#include <Base/Base.h>
#pragma hdrstop

#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/Containers/Blob.h> 
#include <Core/Memory.h>	// scratch heap
#include <Core/Text/TextWriter.h>	// Debug_SaveToObj
#include <Core/Util/ScopedTimer.h>

#include <MeshLib/TriMesh.h>
#include <Developer/Mesh/MeshCompiler.h>
#include <Developer/Mesh/MeshImporter.h>


#if MX_DEVELOPER

//
#include <assimp/include/assimp/Importer.hpp>
#include <assimp/include/assimp/Postprocess.h>
#include <assimp/include/assimp/Mesh.h>
#include <assimp/include/assimp/Scene.h>
#include <assimp/include/assimp/DefaultLogger.hpp>
#include <assimp/include/assimp/LogStream.hpp>
#if MX_AUTOLINK && MX_DEVELOPER
#pragma comment( lib, "assimp.lib" )
#endif //MX_AUTOLINK


namespace Meshok
{

static inline V3f Convert( const aiVector3D& xyz ) {
//	const V3f result = { xyz.x, xyz.y, xyz.z };	// for "gargoyle.dae" from NVidia SDK
	const V3f result = { xyz.x, -xyz.z, xyz.y };
	return result;
}

ERet TriMesh::ImportFromFile( const char* filepath )
{
	ScopedTimer	timer;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filepath
		,aiProcess_Triangulate

		|aiProcess_PreTransformVertices	//incompatible with aiProcess_OptimizeGraph
		|aiProcess_JoinIdenticalVertices

		//|aiProcess_OptimizeGraph	//incompatible with aiProcess_PreTransformVertices
		//|aiProcess_OptimizeMeshes
		//|aiProcess_FlipWindingOrder
	);
	mxENSURE(scene, ERR_FAILED_TO_OPEN_FILE, "Failed to open '%s'", filepath);
//	mxENSURE(scene->mNumMeshes == 1, ERR_UNSUPPORTED_FEATURE, "Too many meshes: '%u'", scene->mNumMeshes);
	mxENSURE(scene->mNumMeshes > 0, ERR_INVALID_PARAMETER, "No meshes: '%s'", filepath);

#if 0

	if( scene->mNumMeshes > 1 ) {
		ptWARN("'%s' consists of %d meshes, but only the first will be processed", filepath, scene->mNumMeshes);
	}

	const aiMesh* mesh = scene->mMeshes[ 0 ];

	// Copy vertex positions and calculate local bounding box.
	const UINT numVertices = mesh->mNumVertices;
	mxDO(this->vertices.setNum( numVertices ));
	AABBf_Clear( &this->localAabb );
	for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
	{
		const V3f position = Convert(mesh->mVertices[ iVertex ]);
		AABBf_AddPoint( &this->localAabb, position );
		this->vertices[ iVertex ].xyz = position;
	}

	// Copy triangles.
	const UINT numFaces = mesh->mNumFaces;
	mxDO(this->triangles.setNum( numFaces ));
	for ( unsigned int iFace = 0; iFace < numFaces; ++iFace )
	{
		const aiFace& face = mesh->mFaces[ iFace ];
		mxENSURE(face.mNumIndices == 3, ERR_INVALID_PARAMETER, "Face %u has %u indices (expected 3)", iFace, face.mNumIndices);
		this->triangles[ iFace ].v[0] = face.mIndices[0];
		this->triangles[ iFace ].v[1] = face.mIndices[1];
		this->triangles[ iFace ].v[2] = face.mIndices[2];
	}

	DEVOUT("Loaded mesh '%s': %u triangles, %u vertices (%u milliseconds)", filepath, numFaces, numVertices, timer.ElapsedMilliseconds());

#else

	UINT totalNumVertices = 0;
	UINT totalNumTriangles = 0;
	for( UINT iMesh = 0; iMesh < scene->mNumMeshes; iMesh++ )
	{
		const aiMesh* mesh = scene->mMeshes[ iMesh ];
		mxENSURE(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE, ERR_INVALID_PARAMETER,
			"Mesh [%u] contains other primitives (we work only with triangles)", iMesh);
		totalNumVertices += mesh->mNumVertices;
		totalNumTriangles += mesh->mNumFaces;
	}

	mxDO(this->vertices.setNum( totalNumVertices ));
	mxDO(this->triangles.setNum( totalNumTriangles ));

	UINT	vertsAddedSoFar = 0;
	UINT	trisAddedSoFar = 0;

	this->localAabb.clear();
	for( UINT iMesh = 0; iMesh < scene->mNumMeshes; iMesh++ )
	{
		const aiMesh* mesh = scene->mMeshes[ iMesh ];

		const UINT numVertices = mesh->mNumVertices;
		for( UINT iVertex = 0; iVertex < numVertices; iVertex++ )
		{
			const V3f position = Convert(mesh->mVertices[ iVertex ]);
			this->localAabb.addPoint( position );
			this->vertices[ vertsAddedSoFar + iVertex ].xyz = position;
		}

		const UINT numFaces = mesh->mNumFaces;
		for ( unsigned int iFace = 0; iFace < numFaces; ++iFace )
		{
			const aiFace& face = mesh->mFaces[ iFace ];
			mxENSURE(face.mNumIndices == 3, ERR_INVALID_PARAMETER, "Face %u has %u indices (expected 3)", iFace, face.mNumIndices);
			this->triangles[ trisAddedSoFar ].a[0] = face.mIndices[0] + vertsAddedSoFar;
			this->triangles[ trisAddedSoFar ].a[1] = face.mIndices[1] + vertsAddedSoFar;
			this->triangles[ trisAddedSoFar ].a[2] = face.mIndices[2] + vertsAddedSoFar;
			trisAddedSoFar++;
		}

		vertsAddedSoFar += numVertices;
	}

	DEVOUT("Loaded mesh '%s': %u triangles, %u vertices (%u milliseconds)", filepath, totalNumTriangles, totalNumVertices, timer.ElapsedMilliseconds());

#endif

	return ALL_OK;
}

}//namespace Meshok

#endif // MX_DEVELOPER
