#pragma once

#include <Base/Memory/RingBuffer.h>

#include <ImGui/imgui.h>



struct TextBuffer
{
	// each line is 4-byte aligned
	struct Header
	{
		Header *__next;
		U32		__size;
		U32		length;
		U32		severity;	// ELogLevel
		//--- real header ends here ---
		// followed by: char [length+1];
		char	data[1024];	// variable-sized; array can be viewed in debugger
	public:
		enum { SIZE = sizeof(void*) + 3*sizeof(U32) };
		void Initialize()
		{
			length = 0;
			severity = LL_Info;
		}
		// release some blocks in the ring buffer
		static bool FreeSomeSpace( Header **_readPtr, Header* _writePtr, void * _userData )
		{
			TextBuffer* o = (TextBuffer*) _userData;
			while( (*_readPtr) != _writePtr ) {
				ptBREAK_IF(!(*_readPtr)->__next);
				//(*_readPtr)->__size = 0;	// free
				o->m_num_messages--;
				*_readPtr = (*_readPtr)->__next;
			}
			return true;
		}
	};

	TRingBuffer< Header >	m_storage;
	char *	m_buffer;
	U32		m_num_messages;	// @hack: so that we can iterate over valid Headers

public:
	TextBuffer();
	~TextBuffer();

	ERet Initialize( int _capacity = mxKiB(64) );
	void Shutdown();

	void clear();

	ERet add( ELogLevel _severity, const char* _text, int length )
	{
		mxASSERT(length > 0);
//{
//	String512 tmp;
//	Str::Format(tmp, "!!! Adding '%s' (%d)\n", _text, length);
//	::OutputDebugStringA(tmp.c_str());
//}
		//const int length = strlen(_text);
		const int bytes_to_allocate = AlignUp( Header::SIZE + length + 1, 16 );	// +1 for terminating null
		if( bytes_to_allocate > m_storage.capacity() ) {
			return ERR_OUT_OF_MEMORY;
		}

		Header* new_entry = m_storage.Allocate( length + 1, this );
		mxASSERT_PTR(new_entry);

		m_num_messages++;

		new_entry->length = length;
		new_entry->severity = _severity;
		char *data = (char*) mxAddByteOffset( new_entry, Header::SIZE );
		strncpy( data, _text, length );
		data[ length ] = '\0';

		return ALL_OK;
	}
};

// based on ExampleAppConsole sample
struct ImGui_Console : ALog
{
	char              InputBuf[1024];

	TextBuffer	m_text_buffer;

	bool                  ScrollToBottom;
#if 0
	ImVector<char*>       History;
	int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
	ImVector<const char*> Commands;
#endif

public:
	ImGui_Console();
	~ImGui_Console();

	ERet Initialize( int buffer_size = mxKiB(64) );
	void Shutdown();

	virtual void VWrite( ELogLevel _level, const char* _message, int _length ) override;

	void    ClearLog()
	{
		m_text_buffer.clear();
		ScrollToBottom = true;
	}

	void AddLog(const char* fmt, ...) IM_FMTARGS(1)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, mxCOUNT_OF(buf), fmt, args);
		buf[mxCOUNT_OF(buf)-1] = 0;
		va_end(args);
		ScrollToBottom = true;
	}

	void Draw(const char* title, bool* opened);

	static int Stricmp(const char* str1, const char* str2)               { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
	static int Strnicmp(const char* str1, const char* str2, int count)   { int d = 0; while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; } return d; }

	ERet ExecCommand(const char* command_line);

	//int TextEditCallback( ImGuiTextEditCallbackData* data );
};
