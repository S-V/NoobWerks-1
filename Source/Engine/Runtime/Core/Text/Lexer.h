/*
=============================================================================
	File:	Lexer.h
	Desc:	A simple lexicographical text parser (aka scanner, tokenizer).
=============================================================================
*/
#pragma once

#include <Core/Text/Token.h>

/*
-----------------------------------------------------------------------------
	LexerOptions
-----------------------------------------------------------------------------
*/
struct LexerOptions
{
	enum Flags
	{
		SkipSingleLineComments = BIT(1),	// C++-style comments: //
		SkipMultiLineComments = BIT(2),		// C - style comments: /* ... */


		// allow the dollar sign '$' in identifiers?
		DollarsInIdentifiers = BIT(3),

		IgnoreUnknownCharacterErrors = BIT(4),


		// SON (Simple Object Notation) - style comments:

		// asm-style single-line comments
		SkipSemicolonComments = BIT(5),

		// skip comments inside '|': | ... |
		SkipSingleVerticalLineComments = BIT(6),


		SkipComments = SkipSingleLineComments | SkipMultiLineComments,



		// allow path separators ('/') in names
		AllowPathNames	= BIT(7),
	};
	
	TBits< Flags, U32 >	m_flags;
	
public:
	LexerOptions();
};

struct LexerState
{
	const char *readPtr;
	int	line;
	int	column;

public:
	LexerState();
	void clear();
};

/*
-----------------------------------------------------------------------------
	The Lexer class provides the mechanics of lexing tokens
	out of a source buffer and deciding what they mean.
-----------------------------------------------------------------------------
*/
class Lexer: public ATokenReader
{
public:
	Lexer();
	Lexer( const void* source, int length, const char* file = NULL );
	~Lexer();

	// filename is used for line number info/diagnostics
	void SetFileName( const char* fileName );

	void SetOptions( const LexerOptions& options );

	virtual bool ReadToken( TbToken &newToken ) override;
	virtual bool PeekToken( TbToken &nextToken ) override;
	virtual bool EndOfFile() const override;
	virtual Location GetLocation() const override;

	bool ReadLine( TbToken & token );
	bool SkipRestOfLine();

	// parse the rest of the line
	bool ReadRestOfLine( String &text );

	// parse a braced section into a string
	bool ParseBracedSection( String &text, bool skipBraces = true );
	bool ParseBracedSection( ATokenWriter &destination );

	void SaveState( LexerState &state ) const;
	void RestoreState( const LexerState& state );

	int GetCurrentLine() const;
	int GetCurrentColumn() const;
	const char* GetFileName() const;

	// get location as delta between current and previous states
	void GetLocation( Location & out ) const;

	/// returns the current source pointer/read cursor
	const char* CurrentPtr() const;

public_internal:
	void SetLineNumber( int newLineNum );

private:
	void Initialize();
	void Shutdown();

private:
	bool ReadNextTokenInternal( TbToken &token );
	bool ExpectNextCharInternal( char c );

	bool SkipWhiteSpaces();

	// unsafe internal functions
	char CurrChar() const;	// CurrentChar()
	char PeekChar() const;
	char PrevChar() const;	// PreviousChar()
	char ReadChar();	// eats one char and returns the next char
	void SkipChar();

	bool ReadName( TbToken &token );
	bool ReadString( TbToken &token );
	bool readString_SingleQuotes( TbToken &token );
	bool ReadNumber( TbToken &token );
	bool ReadInteger( TbToken &token );
	bool ReadFloat( TbToken &token );
	bool ReadHexadecimal( TbToken &token );
	bool ReadOctalInteger( TbToken &token );
	bool ReadBinaryInteger( TbToken &token );

	bool ParsePunctuation( TbToken &token );

	void IncrementLineCounter();

private:
	const char* m_source;	// pointer to the buffer containing the script
	int		m_length;	// size of the whole source file

	// current lexer state
	const char*	m_curr;		// current position in the source file
	int		m_currentLine;
	int		m_currentColumn;

	NameID		m_fileName;

	LexerOptions	m_options;
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
