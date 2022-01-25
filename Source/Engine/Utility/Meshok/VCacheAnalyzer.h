#pragma once

#include <Utility/MeshLib/TriMesh.h>

namespace Meshok
{
	mxSTOLEN("based on meshoptimizer by Zeux");
	// Vertex transform cache analyzer
	// Returns cache hit statistics using a simplified FIFO model
	// Results will not match actual GPU performance

	struct PostTransformCacheStatistics
	{
		unsigned int hits, misses;
		float hit_percent, miss_percent;
		float acmr; //!< average cache miss ratio = transformed vertices / triangle count
	};

	template< typename INDEX_TYPE >
	ERet analyzePostTransformImpl(
		const INDEX_TYPE* _indices, size_t _indexCount
		, size_t _vertex—ount, unsigned int _VCacheSize
		, AllocatorI & scratchpad
		, PostTransformCacheStatistics & _result
		)
	{
		DynamicArray<unsigned int> cache_time_stamps( scratchpad );
		mxDO(cache_time_stamps.setNum( _vertex—ount ));
		cache_time_stamps.setAll( 0 );

		mxZERO_OUT(_result);

		unsigned int time_stamp = _VCacheSize + 1;

		for (const INDEX_TYPE* indices_end = _indices + _indexCount; _indices != indices_end; ++_indices)
		{
			INDEX_TYPE index = *_indices;

			if (time_stamp - cache_time_stamps[index] > _VCacheSize)
			{
				// cache miss
				cache_time_stamps[index] = time_stamp++;
				_result.misses++;
			}
		}


		_result.hits = static_cast<unsigned int>(_indexCount) - _result.misses;

		_result.hit_percent = 100 * static_cast<float>(_result.hits) / _indexCount;
		_result.miss_percent = 100 * static_cast<float>(_result.misses) / _indexCount;

		_result.acmr = static_cast<float>(_result.misses) / (_indexCount / 3);

		return ALL_OK;
	}

	inline
	ERet Analyze(
		const TriMesh& _mesh
		, unsigned int _VCacheSize
		, AllocatorI & scratchpad
		, PostTransformCacheStatistics & _result
	)
	{
		const TriMesh::Index* indices = (TriMesh::Index*) _mesh.triangles.raw();
		const U32 index_count = _mesh.triangles.num() * 3;
		return analyzePostTransformImpl( indices, index_count, _mesh.vertices.num(), _VCacheSize, scratchpad, _result );
	}

}//namespace Meshok

//PostTransformCacheStatistics analyzePostTransform(const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
//{
//	return analyzePostTransformImpl(indices, index_count, vertex_count, cache_size);
//}
//
//PostTransformCacheStatistics analyzePostTransform(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
//{
//	return analyzePostTransformImpl(indices, index_count, vertex_count, cache_size);
//}
