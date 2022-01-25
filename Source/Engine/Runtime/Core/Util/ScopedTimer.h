/*
=============================================================================
	File:	ScopedTimer.h
	Desc:	
=============================================================================
*/
#pragma once
mxREFACTOR("delete this file");

class ScopedTimer
{
	U64	m_startTimeMicroseconds;
	const char *	m_name;
public:
	ScopedTimer( const char* _name = NULL )
	{
		this->Reset();
		m_name = _name;
	}
	~ScopedTimer()
	{
		if( m_name )
		{
			ptPRINT( "%s took %u milliseconds\n", m_name, this->ElapsedMilliseconds() );
		}
	}
	void Reset()
	{
		m_startTimeMicroseconds = mxGetTimeInMicroseconds();
	}
	U64 ElapsedMicroseconds() const
	{
		return mxGetTimeInMicroseconds() - m_startTimeMicroseconds;
	}
	U32 ElapsedMilliseconds() const
	{
		return this->ElapsedMicroseconds() / 1000;
	}
	UINT ElapsedSeconds() const
	{
		return this->ElapsedMilliseconds() / 1000;
	}
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
