#pragma once
#include "Sound.h"

namespace sound {

	/**
	 * Play sound in a loop. This assumes the stream is seekable, otherwise the looping will not
	 * have any effect.
	 */
	class SoundLoop : public Sound {
		STORM_CLASS;
	public:
		STORM_CTOR SoundLoop(Par<Sound> sound);

		// Position.
		virtual Word STORM_FN tell();

		// Seek.
		virtual Bool STORM_FN seek(Word to);

		// Length.
		virtual Word STORM_FN length();

		// Sample frequency.
		virtual Nat STORM_FN sampleFreq() const;

		// Channels.
		virtual Nat STORM_FN channels() const;

		// Read.
		virtual Nat STORM_FN read(SoundBuffer &to);

		// More data?
		virtual Bool STORM_FN more();

	private:
		// Data source.
		Auto<Sound> src;

		// # of repeats so far (to report accurate position).
		Word repeat;

		// Seek discovered to not be supported?
		Bool noSeek;
	};

	// Create nicely.
	SoundLoop *STORM_FN loop(Par<Sound> sound);

}
