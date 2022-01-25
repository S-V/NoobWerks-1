#include <bx/macros.h>

//#include <bgfx/include/bgfx.h>
//#include <bgfx/include/bgfxplatform.h>
#include <ImGui/imgui.h>

#include <FMOD/fmod.h>
#include <FMOD/fmod_errors.h>

#include <Base/Base.h>
#include <Base/Util/LogUtil.h>
#include <Core/Core.h>

#include "SDKwavefile.h"

#if 0

http://wiki.xentax.com/index.php?title=Project_IGI_RES

Format Specifications
char {4}     - Header (ILFF) 
UINT32 {4}   - Archive Size 
UINT32 {4}   - Unknown (4) 
UINT32 {4}   - null 
char {4}     - Resources Header (IRES) 

// for each file 
char {4}     - Entry Header (NAME) 
UINT32 {4}   - Filename Length (including 1 null) 
UINT32 {4}   - Unknown (4) 
UINT32 {4}   - Offset to BODY header (relative to the start of this file entry) 
char {X}     - Filename 
byte {1-4}   - (null) Padding to a multiple of 4 bytes 
char {4}     - Body Header (BODY) 
UINT32 {4}   - File Size (including all file entry data) 
UINT32 {4}   - Unknown (4) 
UINT32 {4}   - File Size (including all file entry data, but excluding this and the previous 3 fields) 
char {4}     - File Data Header (ILSF) 
byte {X}     - File Data 
#endif

struct ResFileHeader
{
	char	fourCC[4];	// ILFF
	UINT32	fileSize;
	UINT32	unknown;
	UINT32	zero;
	char	fourCC2[4];	// IRES
	//20 bytes
};

// for each file
struct ResFileEntry
{
	char	fourCC[4];	// NAME
	UINT32	length;		// Filename Length (including 1 null)
	UINT32	unknown;
	UINT32	bodyOffset;	// Offset to BODY header (relative to the start of this file entry)
	//16 bytes
};

ERet MyEntry()
{
	ByteBuffer	WAVEFORMATEX_data;
	mxDO(Util_LoadFileToBlob(
		"D:/games/Project IGI [2000]/_unpacked/_wav_headers/5DEDF2E7E64D8623.wav_header",
		WAVEFORMATEX_data
	));

	TArray< BYTE >	temporaryBuffer;
	temporaryBuffer.Reserve(mxKiB(128));

	FileReader	reader;
	mxDO(reader.Open(
		//"D:/games/Project IGI [2000]/common/sounds/sounds.res"
		"D:/games/Project IGI 2 [2003]/pc/COMMON/sounds/sounds.res"
		//"G:/Temp/_r/Project IGI 2/Setup/Main/sounds.res"		
		));

	ResFileHeader header;
	mxDO(reader.Get(header));

	int filesRead = 0;

	while( reader.Tell() < header.fileSize )
	{
		const int currentOffset = reader.Tell();

		ResFileEntry	entry;
		mxDO(reader.Get(entry));

		mxASSERT(*(int*)&entry.fourCC == MCHAR4('N','A','M','E'));
		mxASSERT(entry.unknown == 4);
		mxASSERT(entry.length > 0);

		ptPRINT("[%d] BEGIN at %d, hdr.bodyOffset=%d, entry.length=%d",
				filesRead, currentOffset, entry.bodyOffset, entry.length);

		char name[128] = { 0 };
		mxDO(reader.Read(name, entry.bodyOffset-sizeof(entry)));

		UINT32 bodyTag;
		mxDO(reader.Get(bodyTag));

		if( bodyTag == MCHAR4('B','O','D','Y') )
		{
			// File Size (including all file entry data)
			UINT32 fileSize1;
			mxDO(reader.Get(fileSize1));

			// Unknown
			UINT32 unknown1;
			mxDO(reader.Get(unknown1));

			mxASSERT(unknown1 == 4);

			// File Size (including all file entry data, but excluding this and the previous 3 fields) 
			UINT32 fileSize2;
			mxDO(reader.Get(fileSize2));

			// File Data Header (ILSF)
			char	header2[4];
			mxDO(reader.Get(header2));

			mxASSERT(*(int*)header2 == MCHAR4('I','L','S','F'));

			int data_size = fileSize1 - sizeof(UINT32);
			data_size = tbALIGN4(data_size);
			int next_chuck_offset = reader.Tell() + data_size;

			ptPRINT("[%d] Read '%s' at %d, hdr.bodyOffset=%d, fileSize1=%d, fileSize2=%d, next offset: %d",
				filesRead, name, currentOffset, entry.bodyOffset, fileSize1, fileSize2, next_chuck_offset);


			mxASSERT(data_size > 0);

			const bool isWav = (strcmp( Str::FindExtensionS(name), "wav" ) == 0);
			mxASSERT(isWav);



			String256	temp;
			Str::CopyS(temp, name);
			Str::StripLeading(temp, "LOCAL:common/sounds/");
			Str::ReplaceChar(temp,'/','_');
			Str::ReplaceChar(temp,':','_');


			String256	dstFileName("R:/temp/");
			Str::Append(dstFileName, temp);




			//mxDO(Skip_N_bytes(reader, data_size));
			if( isWav )
			{
				temporaryBuffer.Reserve(data_size);
				mxDO(reader.Read(temporaryBuffer.ToPtr(), data_size));


				WAVEFORMATEX* lpwfxFormat = (WAVEFORMATEX*) WAVEFORMATEX_data.ToPtr();

				CWaveFile  waveFile;
				HRESULT hr = waveFile.Open( (char*)dstFileName.c_str(), lpwfxFormat, WAVEFILE_WRITE );
				mxASSERT(SUCCEEDED(hr));

				UINT bytesWritten;
				waveFile.Write( data_size, temporaryBuffer.ToPtr(), &bytesWritten );
				mxASSERT(bytesWritten > 0);

				waveFile.Close();
			}
			else
			{
				FileWriter	writer;
				mxDO(writer.Open(dstFileName.c_str()));

				char	temp_buffer[mxKiB(4)];
				mxDO(Stream_Copy(reader, writer, data_size, temp_buffer, sizeof(temp_buffer)));
			}

			filesRead++;
		}
		else if( bodyTag == MCHAR4('P','A','T','H') )
		{
			ptPRINT("Detected PATH at %d", currentOffset);
			break;
		}
		else {
			ptPRINT("Unknown header at %d", currentOffset);
			break;
		}
	}

	return ALL_OK;
}

int main(int /*_argc*/, char** /*_argv*/)
{
	SetupBaseUtil	setupBase;
	FileLogUtil		fileLog;
	SetupCoreUtil	setupCore;
	//CConsole		consoleWindow;

	MyEntry();

	return 0;
}
