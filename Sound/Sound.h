#pragma once
#include "Shared/EnginePtr.h"

namespace sound {

	/**
	 * Basic buffer type to allow fast and efficient interoperability between
	 * storm and C++. This type is a fixed-size array and its size in a value-type
	 * so that we may pass it to and from Storm efficiently.
	 * The buffer object has strict ownership of its contents, and therefore has
	 * to be copied around all the time. Be careful with copies of this one!
	 */
	class SoundBuffer {
		STORM_VALUE;
	public:
		// Create with a pre-allocated buffer (eg on the stack). Only from C++.
		// You are still in charge of deallocating the buffer!
		SoundBuffer(float *buffer, nat size);

		// Create with a specific size.
		STORM_CTOR SoundBuffer(Nat size);

		// Copy.
		SoundBuffer(const SoundBuffer &o);

		// Assign.
		SoundBuffer &operator =(const SoundBuffer &o);

		// Destroy.
		~SoundBuffer();

		// Number of samples.
		inline Nat STORM_FN count() const { return size; }

		// Element access.
		Float &STORM_FN operator [](Nat id) { return data[id]; }
		const Float &operator [](Nat id) const { return data[id]; }

		// Pointer to the first element.
		float *dataPtr() const { return data; }

	private:
		// Contents.
		float *data;

		// Size.
		nat size;

		// Do we own this buffer?
		bool owner;
	};

	// Output
	wostream &operator <<(wostream &to, const SoundBuffer &b);
	Str *STORM_ENGINE_FN toS(EnginePtr e, SoundBuffer b);


	/**
	 * Sound source. Interface that provides sound.
	 */
	class Sound : public Object {
		STORM_CLASS;
	public:
		// Create the sound, indicate if we're seekable or not.
		STORM_CTOR Sound();

		// Get position (in samples).
		virtual Word STORM_FN tell();

		// Seek (if supported). Returns false if not supported.
		virtual Bool STORM_FN seek(Word to);

		// Length (returns 0 if not seekable).
		virtual Word STORM_FN length();

		// Sample frequency.
		virtual Nat STORM_FN sampleFreq() const;

		// Channels (1 or 2).
		virtual Nat STORM_FN channels() const;

		// Read from the buffer. Channels are interleaved as specified in the vorbis format.
		// Only a multiple of the # of channels are read at once.
		// For 2 channels: L, R
		// For 3 channels: L, C, R
		virtual Nat STORM_FN read(SoundBuffer &to);

	protected:
		virtual void output(wostream &to) const;
	};

	// Open a sound file.
	Sound *STORM_FN openSound(Par<IStream> src);

	// Open sound from a streaming source.
	Sound *STORM_FN openStreamingSound(Par<IStream> src);
}
