#pragma once
#include "Sound.h"
#include "Handle.h"
#include "Core/Timing.h"
#include "Core/Event.h"

namespace sound {

	/**
	 * Sound playback.
	 */
	class Player : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Create the player. Does not start playing.
		STORM_CTOR Player(Sound *sound);

		// Destroy.
		~Player();

		// Destroy any resources immediatly.
		void STORM_FN close();

		// Sound volume (0-1).
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

		// Get our event.
		inline Handle waitEvent() const { return event; }

	private:
		// Buffer time in seconds for each part.
		static const Nat bufferPartTime;

		// Number of parts.
		static const Nat bufferParts;

		// Source.
		Sound *src;

		// Current volume (so that we do not have to convert back to a float again...).
		Float fVolume;

		// Sound buffer.
		IDirectSoundBuffer8 *buffer;

		// Size of the buffer (bytes).
		Nat bufferSize;

		// Part size of the buffer.
		Nat partSize;

		// Size of a single sample (all channels).
		Nat sampleSize;

		// Channels.
		Nat channels;

		// Sample frequency.
		Nat freq;

		// Last part filled.
		Nat lastFilled;

		/**
		 * Information about the parts.
		 */
		struct PartInfo {
			// Starting sample # for this part.
			Word sample;

			// Is this part's index after the end of the stream?
			Bool afterEnd;
		};

		static const GcType partInfoType;

		// Starting sample # for each part.
		GcArray<PartInfo> *partInfo;

		// Waiting event.
		Handle event;

		// Finished event.
		Event *finishEvent;

		// Playing?
		Bool bufferPlaying;

		// Fill the entire buffer with data.
		void fill();

		// Fill a part of the buffer.
		void fill(nat part);

		// Fill a buffer from the device. Returns true if after end of src.
		bool fill(void *buf, nat size);

		// Get the next part.
		static Nat next(Nat part);
	};

}
