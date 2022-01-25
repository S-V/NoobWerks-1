/*
=============================================================================
	File:	TextStream.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop

#include <Core/Text/Token.h>
#include <Core/Text/TextWriter.h>

#define VAL_1X     '\t'

static const char gs_Tabs[ MAX_SCOPE_DEPTH ] = { VAL_32X };

#undef VAL_1X


TextWriter::TextWriter( AWriter& stream )
	: TextStream( stream )
{
	m_scopeDepth = 0;
	m_indentation = 0;
}

TextWriter::~TextWriter()
{
	while( m_scopeDepth > 0 )
	{
		this->LeaveScope();
	}
}

void TextWriter::VWrite( const char* text, int length )
{
	this->InsertTabs();
	if( length > 0 )
	{
		mStream.Write( text, length );
	}
}

TextWriter& TextWriter::Emit( const char* s )
{
	const UINT length = strlen( s );
	return Emit( s, length );
}

TextWriter& TextWriter::Emit( const char* s, UINT length )
{
	mStream.Write( s, length );
	return *this;
}

TextWriter& TextWriter::Emitf( const char* fmt, ... )
{
	char	buffer[ 4096 ];
	mxGET_VARARGS_A( buffer, fmt );
	return Emit( buffer );
}

void TextWriter::EnterScope()
{
	mxASSERT( m_scopeDepth < MAX_SCOPE_DEPTH );
	(*this) << Chars("{\n");
	++m_scopeDepth;
	this->IncreaseIndentation();
}

void TextWriter::LeaveScope( EScopeClosingMode scope_closing_mode )
{
	mxASSERT( m_scopeDepth > 0 );
	--m_scopeDepth;
	this->DecreaseIndentation();

	switch( scope_closing_mode )
	{
	case JustCloseBraces:
		(*this) << Chars("}\n");
		break;

	case EndScopeWithComma:
		(*this) << Chars("},\n");
		break;

	case EndScopeWithSemicolon:
		(*this) << Chars("};\n");
		break;
	}
}

TextWriter& TextWriter::InsertTabs()
{
	// don't indent if vertical whitespace
	if( m_indentation > 0 )
	{
		const UINT numTabs = smallest( m_indentation, MAX_SCOPE_DEPTH );
		mStream.Write( gs_Tabs, numTabs );
	}
	return *this;
}

UINT TextWriter::GetIndentation() const
{
	return m_indentation;
}

TextWriter& TextWriter::IncreaseIndentation()
{
	mxASSERT(m_indentation < MAX_INDENT_LEVEL);
	m_indentation++;
	return *this;
}

TextWriter& TextWriter::DecreaseIndentation()
{
	//mxASSERT(m_indentation > 0);
	m_indentation--;
	m_indentation = Clamp<int>(m_indentation, 0, MAX_INDENT_LEVEL);
	return *this;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
