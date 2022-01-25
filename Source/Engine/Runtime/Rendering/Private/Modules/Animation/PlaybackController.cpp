#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Private/Modules/Animation/PlaybackController.h>

namespace Rendering
{
//tbPRINT_SIZE_OF(NwPlaybackController);

NwPlaybackController::NwPlaybackController()
	: _time_ratio(0.f)
	, _previous_time_ratio(0.f)
	, _playback_speed(1.f)
	, _is_playing(true)
	, _is_looping(true)
	, did_ever_wrap_around(false)
{}

void NwPlaybackController::AdvanceClock(
	const SecondsF animation_duration
	, const SecondsF delta_seconds
	)
{
	const float prev_time_ratio = _previous_time_ratio;

	float new_time_ratio = _time_ratio;

	if( _is_playing )
	{
		mxOPTIMIZE("mul by inverse animation_duration");
		new_time_ratio = _time_ratio + delta_seconds * _playback_speed / animation_duration;
	}

	// Must be called even if time doesn't change, in order to update previous
	// frame time ratio. Uses setTimeRatio function in order to update
	// previous_time_ an wrap time value in the unit interval (depending on loop
	// mode).
	setTimeRatio(new_time_ratio);

	did_ever_wrap_around |= this->didWrapAroundDuringLastUpdate();
}

void NwPlaybackController::setTimeRatio( float01_t new_time_ratio )
{
	_previous_time_ratio = _time_ratio;
	if( _is_looping ) {
		// Wraps in the unit interval [0:1], even for negative values (the reason
		// for using floorf).
		_time_ratio = new_time_ratio - floorf(new_time_ratio);
	} else {
		// Clamps in the unit interval [0:1].
		_time_ratio = Clamp(new_time_ratio, 0.f, 1.f);
	}
}

// Gets animation current time.
float01_t NwPlaybackController::time_ratio() const { return _time_ratio; }

// Gets animation time of last update.
float01_t NwPlaybackController::previous_time_ratio() const {
	return _previous_time_ratio;
}

void NwPlaybackController::reset( const float01_t start_time_ratio /*= 0*/ )
{
	_previous_time_ratio = _time_ratio = start_time_ratio;
	_playback_speed = 1.f;
	_is_playing = true;
}

}//namespace Rendering
