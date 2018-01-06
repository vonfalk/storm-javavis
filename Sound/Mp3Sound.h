#pragma once
#include "Sound.h"
#include "mpg123.h"

namespace sound {

	struct Mp3Stream;

	/**
	 * Read sound from an mp3 stream.
	 */
	class Mp3Sound : public Sound {
		STORM_CLASS;
	public:
		// Create the stream.
		Mp3Sound(IStream *src, Bool seekable);

		// Destroy.
		~Mp3Sound();

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
		// Sample frequency.
		Nat rate;

		// Channels.
		Nat ch;

		// The decoder.
		UNKNOWN(PTR_NOGC) mpg123_handle *h;

		// Handle passed to the decoder.
		UNKNOWN(PTR_GC) Mp3Stream *stream;

		// At end?
		Bool atEnd;

		// IO functions called by mpg123.
		static ssize_t read(void *handle, void *to, size_t count);
		static off_t seek(void *handle, off_t offset, int mode);
		static void cleanup(void *handle);
	};

	// Open an mp3 file. Throws on failure.
	Mp3Sound *STORM_FN openMp3(RIStream *src) ON(Audio);
	Mp3Sound *STORM_FN openMp3Stream(IStream *src) ON(Audio);

}
