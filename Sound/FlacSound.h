#pragma once
#include "Sound.h"
#include "Core/GcType.h"
#include "Core/Io/Stream.h"
#include "FLAC/stream_decoder.h"

namespace sound {

	/**
	 * Read sound from a Flac stream.
	 */
	class FlacSound : public Sound {
		STORM_CLASS;
	public:
		// Create the stream.
		FlacSound(IStream *src, Bool seekable);

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
		// Gc type for the data.
		static const GcType dataType;

		// Data for the decoder. Allocated in non-movable memory as C-code has pointers to it.
		struct Data {
			// The owning sound class.
			FlacSound *owner;

			// Flac allocation (no gc).
			FLAC__StreamDecoder *flac;
		};

		// Data.
		Data *data;

		// Source stream.
		IStream *src;

		// Total samples.
		Word totalSamples;

		// Current position.
		Word currentSample;

		// Bitrate.
		Nat rate;

		// Channels.
		Nat ch;

		// Bits per sample.
		Nat bps;

		// Buffered samples.
		Buffer buffer;

		// # of consumed bytes in the buffer.
		Nat bufferPos;

		// Seekable?
		Bool seekable;

		// At end of stream?
		Bool atEnd;

		// Read more data from libflac into buffer.
		void fillBuffer();

		// Functions called by libflac.
		static FLAC__StreamDecoderReadStatus srcRead(const FLAC__StreamDecoder *decoder,
													FLAC__byte *to,
													size_t *bytes,
													void *data);
		static FLAC__StreamDecoderSeekStatus srcSeek(const FLAC__StreamDecoder *decoder,
													FLAC__uint64 offset,
													void *data);
		static FLAC__StreamDecoderTellStatus srcTell(const FLAC__StreamDecoder *decoder,
													FLAC__uint64 *offset,
													void *data);
		static FLAC__StreamDecoderLengthStatus srcLength(const FLAC__StreamDecoder *decoder,
														FLAC__uint64 *length,
														void *data);
		static FLAC__bool srcEof(const FLAC__StreamDecoder *decoder,
								void *data);
		static FLAC__StreamDecoderWriteStatus write(const FLAC__StreamDecoder *decoder,
													const FLAC__Frame *frame,
													const FLAC__int32 *const buffer[],
													void *data);
		static void metadata(const FLAC__StreamDecoder *decoder,
							const FLAC__StreamMetadata *meta,
							void *data);
		static void error(const FLAC__StreamDecoder *decoder,
						FLAC__StreamDecoderErrorStatus status,
						void *data);

		// Finalizer for data objects.
		static void destroyData(Data *me);
	};

	// Open a vorbis file. Throws on failure.
	FlacSound *STORM_FN openFlac(RIStream *src) ON(Audio);
	FlacSound *STORM_FN openFlacStream(IStream *src) ON(Audio);

}
