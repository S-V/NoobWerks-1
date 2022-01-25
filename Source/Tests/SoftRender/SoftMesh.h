#pragma once

//#include <Graphics/Graphics.h>

#include <SoftRender/SoftRender.h>

typedef TArray< SVertex >	SVertexBuffer;
typedef TArray< SIndex >		SIndexBuffer;

struct SoftMesh 
{
	SVertexBuffer	m_vb;
	SIndexBuffer	m_ib;	// triangle list
	AABB			m_aabb;

public:
	SoftMesh();

	void SetVertices( const SVertex* p, UINT numPoints );
	void SetIndices( const SIndex* p, UINT numIndices );

	UINT NumVertices() const
	{
		return m_vb.Num();
	}
	const SVertex* GetVerticesArray() const
	{
		return m_vb.Raw();
	}
	UINT NumIndices() const
	{
		return m_ib.Num();
	}
	const SIndex* GetIndicesArray() const
	{
		return m_ib.Raw();
	}
	void Clear()
	{
		m_vb.Empty();
		m_ib.Empty();
	}

	void RecalculateAABB();

public:
	void SetupCube();
	void SetupTeapot();
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
