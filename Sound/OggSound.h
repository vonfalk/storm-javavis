#pragma once
#include "Sound.h"

#include "vorbis/vorbisfile.h"

namespace sound {

	/**
	 * Read sound from an Ogg stream.
	 */
	class OggSound : public Sound {
		STORM_CLASS;
	public:
		// Create the stream.
		OggSound(OggVorbis_File file, bool seekable);

		// Destroy.
		~OggSound();

		// Position.
		virtual Word STORM_FN tell();
		virtual Bool STORM_FN seek(Word to);
		virtual Word STORM_FN length();

		// Format information.
		virtual Nat STORM_FN sampleFreq() const;
		virtual Nat STORM_FN channels() const;

		// Get data.
		virtual Nat STORM_FN read(SoundBuffer &to);
		virtual Bool STORM_FN more();

	private:
		// The ogg file.
		OggVorbis_File file;

		// Information.
		vorbis_info *vi;

		// Seekable?
		bool seekable;
	};


	// Open a vorbis file. Returns null on failure.
	OggSound *STORM_FN openOgg(Par<RIStream> src) ON(Audio);
	OggSound *STORM_FN openStreamingOgg(Par<IStream> src) ON(Audio);

}
