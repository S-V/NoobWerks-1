/*
=============================================================================
	File:	FPSTracker.h
	Desc:	
=============================================================================
*/

#ifndef __MX_FPS_TRACKER_H__
#define __MX_FPS_TRACKER_H__



//----------------------------------------------------------//

// MJP's
template< UINT NUM_SAMPLES = 64 >
class FPSTracker
{
	FLOAT	m_samples[ NUM_SAMPLES ];	// time deltas for smoothing
	UINT	m_currentSampleIndex;

public:
	FPSTracker()
	{
		this->Reset();
	}
	void Reset()
	{
		mxZERO_OUT(m_samples);
		m_currentSampleIndex = 0;
	}
	// Call this function once per frame.
	void Update( FLOAT deltaSeconds )
	{
		//mxASSERT(deltaSeconds > 0.0f);
		if( deltaSeconds == 0.0f ) {
			deltaSeconds = 9999.0f;
		}

		m_samples[ m_currentSampleIndex ] = deltaSeconds;
		m_currentSampleIndex = (m_currentSampleIndex + 1) % NUM_SAMPLES;
	}
	FLOAT CalculateFPS() const
	{
		FLOAT averageDelta = 0;
		for(UINT i = 0; i < NUM_SAMPLES; ++i) {
			averageDelta += m_samples[i];
		}
		averageDelta /= NUM_SAMPLES;
		mxASSERT(averageDelta > 0.0f);
		return floor( (1.0f / averageDelta) + 0.5f );
	}
};

#endif // !__MX_FPS_TRACKER_H__

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
