// 
#pragma once

#include <Base/Base.h>
#include <Core/Memory.h>
#include <GPU/Public/graphics_types.h>


class NwRenderTargetPool
{
	struct PooledItem
	{
		HColorTarget	handle;
		GrTextureCreationFlagsT	flags;
		U16				width;
		U16				height;
		DataFormatT		format;
		U8				numMips;
		U8				is_free;
	};
//	mxSTATIC_ASSERT_ISPOW2(sizeof(PooledItem));

	TArray< PooledItem >	m_items;

public:
	//NwRenderTargetPool( AllocatorI & _allocator );
	NwRenderTargetPool();
	~NwRenderTargetPool();

	ERet Initialize();
	void Shutdown();

	ERet GetTemporary(
		HColorTarget &_handle,
		unsigned int _width, unsigned int _height,
		NwDataFormat::Enum _format,
		unsigned int _mips = 1,
		GrTextureCreationFlagsT _flags = NwTextureCreationFlags::defaults
	);
	void ReleaseTemporary( HColorTarget & _handle );
};


/// The reference to a pooled render target
struct NwPooledRT: NonCopyable
{
	HColorTarget		handle;
	U16					width;	//!< for convenience
	U16					height;	//!< for convenience
	DataFormatT			format;
private:
	NwRenderTargetPool &	pool;
public:
	NwPooledRT(
		NwRenderTargetPool & _pool,
		unsigned int _width, unsigned int _height,
		NwDataFormat::Enum _format,
		unsigned int _mips = 1,
		GrTextureCreationFlagsT _flags = NwTextureCreationFlags::defaults
		)
		: pool( _pool )
		, width( _width ), height( _height ), format( _format )
	{
		pool.GetTemporary( handle, _width, _height, _format, _mips, _flags );
	}

	~NwPooledRT()
	{
		pool.ReleaseTemporary( handle );
	}
};
