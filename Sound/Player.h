#pragma once
#include "Sound.h"
#include "Shared/Timing.h"

namespace sound {

	/**
	 * Sound player.
	 */
	class Player : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Create the player. Does not start playing.
		STORM_CTOR Player(Par<Sound> sound);

		// Destroy.
		~Player();

		// Play.
		void STORM_FN play();

		// Pause.
		void STORM_FN pause();

		// Stop. This resets the position to zero.
		void STORM_FN stop();

		// Playing?
		Bool STORM_FN playing();

		// Wait until done. Do not call from the UThread managing callbacks!
		void STORM_FN waitUntilDone();

		// Get the time of playback.
		Duration STORM_FN time();

		// Called when we should fill more data into our buffer.
		void onNotify();

	private:
		// Constants.
		enum {
			// Buffer time in seconds for each part.
			bufferPartTime = 1,

			// Number of parts.
			bufferParts = 3,
		};

		// Source.
		Auto<Sound> src;

		// Sound buffer.
		IDirectSoundBuffer8 *buffer;

		// Size of the buffer (bytes).
		nat bufferSize;

		// Part size of the buffer.
		nat partSize;

		// Size of a single sample (all channels).
		nat sampleSize;

		// Channels.
		nat channels;

		// Sample frequency.
		nat freq;

		// Last part filled.
		nat lastFilled;

		// Starting sample # for each part.
		nat64 partSample[bufferParts];

		// Is the part at index it after the end of stream?
		bool afterEnd[bufferParts];

		// Waiting event.
		HANDLE event;

		// Finished event.
		os::Event finishEvent;

		// Playing?
		bool bufferPlaying;

		// Fill the entire buffer with data.
		void fill();

		// Fill a part of the buffer.
		void fill(nat part);

		// Fill a buffer from the device. Returns true if after end of src.
		bool fill(void *buf, nat size);
	};

}
