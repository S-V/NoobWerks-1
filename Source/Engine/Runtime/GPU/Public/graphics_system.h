// 
#pragma once

#include <Base/Template/Containers/HashMap/TSimpleHashTable.h>

#include <Core/NameHash.h>

#include <Graphics/ForwardDecls.h>
#include <GPU/Public/graphics_types.h>


namespace NGraphics
{
	ERet Initialize( ProxyAllocator & allocator );
	void Shutdown();


	//
	HBuffer CreateGlobalConstantBuffer(
		const U32 cbuffer_size_in_bytes
		, const NwNameHash& name_hash = NwNameHash()
		);
	void DestroyGlobalConstantBuffer(
		HBuffer & cbuffer_handle
		);

	/// Contains references to global constant buffers and textures
	/// for automatic binding of shader parameters.
	///
	class ShaderResourceLUT
	{
		enum {
			MAX_CBUFFER_TABLE_SIZE = 16,
			MAX_RESOURCE_TABLE_SIZE = 64,
		};

		TSimpleHashTable< NameHash32, HBuffer >			cbuffer_table;
		TSimpleHashTable< NameHash32, HShaderInput >	resource_table;

		NameHash32		cbuffer_namehashes[ MAX_CBUFFER_TABLE_SIZE ];
		HBuffer			cbuffer_handles[ MAX_CBUFFER_TABLE_SIZE ];

		NameHash32		resource_namehashes[ MAX_RESOURCE_TABLE_SIZE ];
		HShaderInput	resource_handles[ MAX_RESOURCE_TABLE_SIZE ];

	public:
		ERet Initialize();

		const HBuffer* GetCBuffer(
			const NwNameHash& name_hash
			) const
		{
			const HBuffer* buffer_handle = cbuffer_table.FindValue(nwNAMEHASH_VAL(name_hash));
			mxASSERT2(buffer_handle && buffer_handle->IsValid(),
				"Couldn't find global constant buffer name '%s'", nwNAMEHASH_STR(name_hash)
				);
			return buffer_handle;
		}

		const HShaderInput* GetResource(
			const NwNameHash& name_hash
			) const
		{
			const HShaderInput* resource_handle = resource_table.FindValue(nwNAMEHASH_VAL(name_hash));
			mxASSERT2(resource_handle && resource_handle->IsValid(),
				"Couldn't find global shader resource name '%s'", nwNAMEHASH_STR(name_hash)
				);
			return resource_handle;
		}

	public:
		///
		ERet RegisterCBuffer(
			const NwNameHash& name_hash
			, const HBuffer handle
			);
		ERet RegisterResource(
			const NwNameHash& name_hash
			, const HShaderInput handle
			);
	};

	///
	ShaderResourceLUT& GetGlobalResourcesLUT();

}//namespace NGraphics



/*
=======================================================================
	USEFUL MACROS
=======================================================================
*/
#define nwCREATE_GLOBAL_CONSTANT_BUFFER( cb_name )\
	NGraphics::CreateGlobalConstantBuffer(\
		sizeof(cb_name)\
		, nwNAMEHASH(TO_STR(cb_name))\
		)

#define nwDESTROY_GLOBAL_CONSTANT_BUFFER( cb_handle )\
	NGraphics::DestroyGlobalConstantBuffer(cb_handle)
