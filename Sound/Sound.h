#pragma once
#include "Buffer.h"
#include "Core/TObject.h"

namespace sound {

	/**
	 * Sound source. Something that provides sound for playback.
	 *
	 * All positions are indicated in samples.
	 */
	class Sound : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Create the sound source.
		STORM_CTOR Sound();

		// Close this stream now.
		virtual void STORM_FN close();

		// Get position.
		virtual Word STORM_FN tell();

		// Seek (if supported), returns false if seeking is not supported.
		virtual Bool STORM_FN seek(Word to);

		// Length (returns 0 if not seekable).
		virtual Word STORM_FN length();

		// Sample frequency (in samples per second).
		virtual Nat STORM_FN sampleFreq() const;

		// Channels.
		virtual Nat STORM_FN channels() const;

		// Read from the buffer. Channels are interleaved as specified in the vorbis format. Only a
		// multiple of the # of channels are read at once. If at end of stream, returns 0 samples.
		// For 2 channels: L, R
		// For 3 channels: L, C, R
		virtual Buffer STORM_FN read(Nat samples);
		virtual Buffer STORM_FN read(Buffer to);

		// More data?
		virtual Bool STORM_FN more();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
