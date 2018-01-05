#pragma once

namespace sound {

	/**
	 * Platform specific types in a generic way so that we can use them inside Storm types.
	 */


	/**
	 * A SoundBuffer is an object that contains a playable buffer of sound samples.
	 */
	class SoundBuffer {
		STORM_VALUE;
	public:
#ifdef SOUND_DX
		SoundBuffer(IDirectSoundBuffer8 *buffer) { data = (size_t)buffer; }

		inline IDirectSoundBuffer8 *operator ->() const {
			return (IDirectSoundBuffer8 *)data;
		}
#endif
#ifdef SOUND_AL
		SoundBuffer(ALuint buffer) { data = (size_t)buffer; }

		inline operator ALuint() const {
			return (ALuint)data;
		}
#endif
		SoundBuffer() { data = 0; }

		inline operator bool() const {
			return data != 0;
		}

	private:
		// The actual storage.
		UNKNOWN(PTR_NOGC) size_t data;
	};


	/**
	 * A SoundDevice represents the device onto which we will play sound.
	 */
	class SoundDevice {
		STORM_VALUE;
	public:
#ifdef SOUND_DX
		SoundDevice(IDirectSound8 *device) { data = (size_t)device; }

		inline IDirectSound8 *operator ->() const {
			return (IDirectSound8 *)data;
		}
#endif
#ifdef SOUND_AL
		SoundDevice(ALCdevice *device) { data = (size_t)device; }

		inline operator ALCdevice *() const {
			return (ALCdevice *)data;
		}
#endif
		SoundDevice() { data = 0; }

		inline operator bool() const {
			return data != 0;
		}

	private:
		// The actual storage.
		UNKNOWN(PTR_NOGC) size_t data;
	};

	/**
	 * A SoundContext represents the device onto which we will play sound.
	 */
	class SoundContext {
		STORM_VALUE;
	public:
#ifdef SOUND_DX
		// A context does not exist in DirectSound.
#endif
#ifdef SOUND_AL
		SoundContext(ALCcontext *ctx) { data = (size_t)ctx; }

		inline operator ALCcontext *() const {
			return (ALCcontext *)data;
		}
#endif
		SoundContext() { data = 0; }

		inline operator bool() const {
			return data != 0;
		}

	private:
		// The actual storage.
		UNKNOWN(PTR_NOGC) size_t data;
	};

}
