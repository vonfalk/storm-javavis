#pragma once
#include "Sound.h"

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

		// Called when we should fill more data into our buffer.
		void onNotify();

	private:
		// Source.
		Auto<Sound> src;

		// Sound buffer.
		IDirectSoundBuffer8 *buffer;

		// Size of the buffer (bytes).
		nat bufferSize;

		// Part size of the buffer.
		nat partSize;

		// Channels.
		nat channels;

		// Last part filled.
		nat lastFilled;

		// Waiting event.
		HANDLE event;

		// Fill the entire buffer with data.
		void fill();

		// Fill a part of the buffer.
		void fill(nat part);

		// Fill a buffer from the device.
		void fill(void *buf, nat size);
	};

}
