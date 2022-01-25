#include <Base/Base.h>
#pragma hdrstop

#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/Utility/RenderTargetPool.h>


NwRenderTargetPool::NwRenderTargetPool()
{
}

NwRenderTargetPool::~NwRenderTargetPool()
{

}

ERet NwRenderTargetPool::Initialize()
{
	mxDO(m_items.Reserve(16));
	return ALL_OK;
}

void NwRenderTargetPool::Shutdown()
{
	for( UINT iRT = 0; iRT < m_items.num(); iRT++ )
	{
		PooledItem & item = m_items[ iRT ];
		NGpu::DeleteColorTarget( item.handle );
	}
	m_items.clear();
}

ERet NwRenderTargetPool::GetTemporary(
									HColorTarget &_handle,
									unsigned int _width, unsigned int _height,
									NwDataFormat::Enum _format,
									unsigned int _mips /*= 1*/,
									GrTextureCreationFlagsT _flags /*= GrTextureCreationFlags::defaults*/
									)
{
	for( UINT iRT = 0; iRT < m_items.num(); iRT++ )
	{
		PooledItem & item = m_items[ iRT ];
		if( (item.width == _width) && (item.height == _height) && (item.format == _format)
			&& (item.numMips == _mips) && (item.flags == _flags)
			&& item.is_free )
		{
			item.is_free = false;
			_handle = item.handle;
			return ALL_OK;
		}
	}

	PooledItem	newItem;

	NwColorTargetDescription	rtDesc;
	rtDesc.format = _format;
	rtDesc.width = _width;
	rtDesc.height = _height;
	rtDesc.numMips = _mips;
	rtDesc.flags = _flags;
	HColorTarget hNewRT = NGpu::CreateColorTarget( rtDesc );

	newItem.handle = hNewRT;
	newItem.width = _width;
	newItem.height = _height;
	newItem.format = _format;
	newItem.numMips = _mips;
	newItem.flags = _flags;
	newItem.is_free = false;
	mxDO(m_items.add( newItem ));

	_handle = hNewRT;

	return ALL_OK;
}

void NwRenderTargetPool::ReleaseTemporary( HColorTarget & _handle )
{
	bool bFound = false;
	for( UINT iRT = 0; iRT < m_items.num(); iRT++ )
	{
		PooledItem & item = m_items[ iRT ];
		if( item.handle == _handle )
		{
			mxASSERT(!item.is_free);
			item.is_free = true;
			bFound = true;
			break;
		}
	}
	mxASSERT(bFound);
	_handle.SetNil();
}
