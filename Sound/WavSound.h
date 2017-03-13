#pragma once
#include "Sound.h"

namespace sound {

	/**
	 * Read sound from Wav stream.
	 */
	class WavSound : public Sound {
		STORM_CLASS;
	public:
		// Create the stream.
		WavSound(IStream *src, Bool seekable);

		// Close.
		virtual void STORM_FN close();

		// Position.
		virtual Word STORM_FN tell();
		virtual Bool STORM_FN seek(Word to);
		virtual Word STORM_FN length();

		// Format information.
		virtual Nat STORM_FN sampleFreq() const;
		virtual Nat STORM_FN channels() const;

		// Get data.
		virtual Buffer STORM_FN read(Buffer to);
		virtual Bool STORM_FN more();

	private:
		// Source stream.
		IStream *src;

		// Sample frequency.
		Nat rate;

		// Channels.
		Nat ch;

		// Bits per sample.
		Nat sampleDepth;

		// Size of each sample for all channels.
		Nat sampleSize;

		// Total length of the data.
		Word dataLength;

		// Current position in the data (counted in samples).
		Word dataPos;

		// Start position of the sound data.
		Word dataStart;

		// Seekable?
		Bool seekable;
	};

	// Open a wave file. Throws on failure.
	WavSound *STORM_FN openWav(RIStream *src) ON(Audio);
	WavSound *STORM_FN openWavStream(IStream *src) ON(Audio);

}
