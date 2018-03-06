#pragma once
#include "Sound.h"
#include "Handle.h"
#include "Types.h"
#include "Core/Timing.h"
#include "Core/Event.h"
#include "Core/Lock.h"

namespace sound {

	/**
	 * Sound playback.
	 *
	 * TODO: Make 'wait' signaling when the buffer is not playing. Esp. if playback was not started
	 * for some reason.
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
		void STORM_ASSIGN volume(Float level);
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

		// Size of a single sample for 1 channel in bytes.
		static const Nat sampleDepth;

		// Lock for keeping calls into the source stream in sync.
		Lock *lock;

		// Source.
		Sound *src;

		// Current volume (so that we do not have to convert back to a float again...).
		Float fVolume;

		// Sound buffer.
		SoundBuffer buffer;

		// Size of the buffer (samples).
		inline Nat bufferSamples() const { return partSamples() * bufferParts; }

		// Size of one part of the buffer (samples).
		inline Nat partSamples() const { return bufferPartTime * freq; }

		// Size of the buffer (bytes).
		inline Nat bufferSize() const { return sampleSize() * bufferSamples(); }

		// Size of one part of the buffer (bytes).
		inline Nat partSize() const { return sampleSize() * partSamples(); }

		// Size of a single sample (bytes, all channels).
		inline Nat sampleSize() const { return channels * sampleDepth; }

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

			// OpenAL buffer name. Unused on DirectSound.
			Word alBuffer;

			// Is this part's index after the end of the stream?
			Bool afterEnd;
		};

		static const GcType partInfoType;

		// Starting sample # for each part.
		GcArray<PartInfo> *partInfo;

		// Temporary buffer usable by the backends.
		void *tmpBuffer;

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


		/**
		 * Backend specific buffer management:
		 */

		// Initialize the sound buffer.
		void initBuffer();

		// Destroy the sound buffer.
		void destroyBuffer();

		// Set playback volume.
		void bufferVolume(float volume);

		// Start playing buffer.
		void bufferPlay();

		// Stop playing buffer.
		void bufferStop();
	};

}
