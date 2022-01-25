#pragma once

#include <SoftRender/SoftRender.h>
#include <SoftRender/SoftRender_Internal.h>

namespace SoftRenderer
{

class srImmediateRenderer : public ATriangleRenderer
	, SingleInstance<srImmediateRenderer>
{
	// current states
	M44f	m_worldMatrix;
	M44f	m_viewMatrix;
	M44f	m_projectionMatrix;

	F_VertexShader *	m_vertexShader;
	F_PixelShader *		m_pixelShader;

	ECullMode	m_cullMode;
	EFillMode	m_fillMode;

	TPtr< SoftTexture2D >	m_texture;

	F_RenderTriangles *			m_ftblProcessTriangles[Fill_MAX][Cull_MAX];
	F_RenderSingleTriangle *	m_ftblDrawTriangle[Fill_MAX];

public:
	srImmediateRenderer();
	~srImmediateRenderer();

	void SetWorldMatrix( const M44f& newWorldMatrix ) override;
	void SetViewMatrix( const M44f& newViewMatrix ) override;
	void SetProjectionMatrix( const M44f& newProjectionMatrix ) override;

	void SetCullMode( ECullMode newCullMode ) override;
	void SetFillMode( EFillMode newFillMode ) override;

	void SetVertexShader( F_VertexShader* newVertexShader ) override;
	void SetPixelShader( F_PixelShader* newPixelShader ) override;

	void SetTexture( SoftTexture2D* newTexture2D ) override;

	void DrawTriangles( SoftFrameBuffer& frameBuffer, const SVertex* vertices, UINT numVertices, const SIndex* indices, UINT numIndices ) override;
};

}//namespace SoftRenderer
