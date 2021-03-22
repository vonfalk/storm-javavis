#pragma once
#include "Sound.h"
#include "Core/Io/Stream.h"
#include "vorbis.h"

namespace sound {

	/**
	 * Read sound from an Ogg stream.
	 */
	class OggSound : public Sound {
		STORM_CLASS;
	public:
		// Create the stream.
		OggSound(IStream *src, Bool seekable);

		// Destroy.
		~OggSound();

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

		// The ogg file.
		UNKNOWN(PTR_GC) OggVorbis_File *file;

		// Information.
		vorbis_info *vi;

		// Seekable?
		Bool seekable;

		// At end of stream?
		Bool atEnd;

		// Functions called by libogg.
		static size_t srcRead(void *to, size_t size, size_t nmemb, void *data);
		static int srcSeek(void *data, ogg_int64_t offset, int whence);
		static int srcClose(void *data);
		static long srcTell(void *data);
	};

	// Open a vorbis file. Throws on failure.
	OggSound *STORM_FN openOgg(RIStream *src) ON(Audio);
	OggSound *STORM_FN openOggStream(IStream *src) ON(Audio);

}
