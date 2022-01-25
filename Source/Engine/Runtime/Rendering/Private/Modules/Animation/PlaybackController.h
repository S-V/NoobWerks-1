#pragma once

namespace Rendering
{
// Utility class that helps with controlling animation playback time. Time is
// computed every update according to the dt given by the caller, playback speed
// and "play" state.
// Internally time is stored as a ratio in unit interval [0,1], as expected by
// ozz runtime animation jobs.
// OnGui function allows to tweak controller parameters through the application
// Gui.
struct NwPlaybackController
{
	/// Current animation time ratio, aka "normalized time",
	/// in the unit interval [0,1], where 0 is the
	/// beginning of the animation, 1 is the end.
	float01_t _time_ratio;

	/// Time ratio of the previous update.
	float01_t _previous_time_ratio;

	/// Playback speed (aka "time scale"). NOTE: cannot be negative - playing the animation backwards is not supported!
	float _playback_speed;

	// how many times the anim has wrapped to the beginning (always 0 for clamped anims)
	//U32	cycle_count;

	/// Animation play mode state: play/pause.
	bool _is_playing;

	/// Animation loop mode.
	bool _is_looping;

	/// can only become true if the animation is looped
	bool did_ever_wrap_around;

	// 16

public:
	// Constructor.
	NwPlaybackController();

	// Sets animation current time.
	void setTimeRatio(float01_t time);

	// Gets animation current time.
	float01_t time_ratio() const;

	// Gets animation time ratio of last update. Useful when the range between
	// previous and current frame needs to pe processed.
	float01_t previous_time_ratio() const;

	// Sets playback speed.
	void set_playback_speed(float new_speed) { _playback_speed = new_speed; }

	// Gets playback speed.
	float playback_speed() const { return _playback_speed; }

	// Sets loop modes. If true, animation time is always clamped between 0 and 1.
	void set_loop(bool loop_anim) { _is_looping = loop_anim; }

	// Gets loop mode.
	bool loop() const { return _is_looping; }

	// Updates animation time if in "play" state, according to playback speed and
	// given frame time _dt.
	// Returns true if animation has looped during update
	void AdvanceClock(
		const SecondsF animation_duration
		, const SecondsF delta_seconds
		);

	// Resets all parameters to their default value.
	void reset( const float01_t start_time_ratio = 0 );

	// assuming anim is always played forward
	mxFORCEINLINE bool didWrapAroundDuringLastUpdate() const
	{
		return _previous_time_ratio > _time_ratio;
	}
};

}//namespace Rendering
