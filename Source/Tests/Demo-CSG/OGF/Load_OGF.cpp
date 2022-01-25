// load STALKER's OGF files
#include <OGF/Fmesh.h>
#include <xray_re/xr_ogf_format.h>

#define CFS_CompressMark	(1ul << 31ul)


U32 find_chunk( IOStreamFILE& file, U32 ID, BOOL* bCompressed = 0 )
{
	const size_t fileSize = file.Length();

	file.Seek(0);
	size_t currOffset = 0;

	while (currOffset < fileSize)
	{
		U32 dwType, dwSize;

		mxDO(file.Get(dwType));
		currOffset += sizeof(dwType);

		mxDO(file.Get(dwSize));
		currOffset += sizeof(dwSize);

		if ((dwType&(~CFS_CompressMark)) == ID) {
			mxASSERT(file.Length() + dwSize <= fileSize);
			if (bCompressed) *bCompressed = dwType&CFS_CompressMark;
			return dwSize;
		}
		else
		{
			ptPRINT("Skipping chunk: type=%d, size=%d", dwType, dwSize);
			char	temporary[4096];
			mxDO(Stream_Skip(file,dwSize,temporary,sizeof(temporary)));
			UNDONE;
		}
		currOffset += dwSize;
	}
	return 0;
}

struct OGFHeader_v3 // old
{
	char format_version;
	char type;
	short unknown;
};


ERet LoadOGF( IOStreamFILE& file )
{
	//OGFHeader_v3	hdrV3;
	//mxDO(file.Get(hdrV3));

	//U32 offset = find_chunk(file, OGF_HEADER);

	char temporary[8];
	mxDO(Stream_Skip(file,8, temporary, sizeof(temporary)));

	ogf_header	header;
	mxDO(file.Get(header));

	const xray_re::ogf_model_type model_type = (xray_re::ogf_model_type) header.type;


	mxDO(file.Seek(0));

	if (find_chunk(file, OGF_GCONTAINER))
	{
		// verts
		const U32 vertexFormatID = ReadUInt32(file);
		const U32 vBase = ReadUInt32(file);
		const U32 vCount = ReadUInt32(file);

		//// indices
		//const U32 idx_ID				= data->r_u32				();
		//const U32 iBase				= data->r_u32				();
		//const U32 iCount				= data->r_u32				();
		//const U32 dwPrimitives		= iCount/3;
	}


	if( header.type == MT_NORMAL )
	{
		// IRender_Visual::Load
		if(header.format_version != xrOGF_FormatVersion) {
			return ERR_INCOMPATIBLE_VERSION;
		}
		if (header.shader_id) {
			ptPRINT("Shader: %d", header.shader_id);
		}
		//vis.box.set			(header.bb.min,header.bb.max	);
		//vis.sphere.set		(header.bs.c,	header.bs.r	);

		//Fvisual::Load
		if (find_chunk(file, OGF_GCONTAINER))
		{
			// verts
			const U32 vertexFormatID = ReadUInt32(file);
			const U32 vBase = ReadUInt32(file);
			const U32 vCount = ReadUInt32(file);

			//// indices
			//const U32 idx_ID				= data->r_u32				();
			//const U32 iBase				= data->r_u32				();
			//const U32 iCount				= data->r_u32				();
			//const U32 dwPrimitives		= iCount/3;
		}
	}
	else
	{
		return ERR_OBJECT_OF_WRONG_TYPE;
	}

	return ALL_OK;
}
