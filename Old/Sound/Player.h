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

		// Sound volume (0-1)
		void STORM_SETTER volume(Float level);
		Float STORM_FN volume();

		// Play.
		void STORM_FN play();

		// Pause.
		void STORM_FN pause();

		// Stop. This resets the position to zero.
		void STORM_FN stop();

		// Playing?
		Bool STORM_FN playing();

		// Wait until playback is done or until playback is stopped.
		// Do not call from the UThread managing callbacks!
		void STORM_FN wait();

		// Wait until a specific time has passed since start. Returns if the player is stopped as well.
		// Do not call from the UThread managing callbacks!
		void STORM_FN waitUntil(Duration t);

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

		// Current volume (so that we do not have to convert back to a float again...).
		Float fVolume;

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