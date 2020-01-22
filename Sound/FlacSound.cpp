#include "stdafx.h"
#include "FlacSound.h"
#include "Exception.h"
#include "Utils/Memory.h"
#include <limits>

namespace sound {

	const GcType FlacSound::dataType = {
		GcType::tFixed,
		null,
		&FlacSound::dataFinalizer,
		sizeof(Data),
		1,
		{ OFFSET_OF(Data, owner) }
	};

	FlacSound::FlacSound(IStream *src, Bool seekable) : src(src), seekable(seekable), atEnd(false) {
		data = (Data *)runtime::allocStaticRaw(engine(), &dataType);
		data->owner = this;
		data->flac = FLAC__stream_decoder_new();
		FLAC__StreamDecoderInitStatus r;
		r = FLAC__stream_decoder_init_stream(data->flac,
											&FlacSound::srcRead,
											seekable ? &FlacSound::srcSeek : null,
											seekable ? &FlacSound::srcTell : null,
											seekable ? &FlacSound::srcLength : null,
											seekable ? &FlacSound::srcEof : null,
											&FlacSound::write,
											&FlacSound::metadata,
											&FlacSound::error,
											data);

		if (r != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			throw new (this) SoundOpenError(TO_S(this, S("Failed initializing FLAC decoder: ") << FLAC__StreamDecoderInitStatusString[r]));

		if (!FLAC__stream_decoder_process_until_end_of_metadata(data->flac))
			throw new (this) SoundOpenError(TO_S(this, S("Failed to decode FLAC metadata: ") << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(data->flac)]));
	}

	void FlacSound::close() {
		if (data) {
			destroyData(data);
			data = null;
		}

		if (src) {
			src->close();
			src = null;
		}
	}

	Bool FlacSound::seek(Word to) {
		if (!seekable)
			return false;

		bufferPos = 0;
		buffer.filled(0);
		if (FLAC__stream_decoder_seek_absolute(data->flac, to)) {
			currentSample = to;
			return true;
		} else {
			return false;
		}
	}

	Word FlacSound::tell() {
		if (!seekable)
			return 0;
		return currentSample;
	}

	Word FlacSound::length() {
		if (!seekable)
			return 0;
		return totalSamples;
	}

	Nat FlacSound::sampleFreq() const {
		return rate;
	}

	Nat FlacSound::channels() const {
		return ch;
	}

	Buffer FlacSound::read(Buffer to) {
		Nat free = to.count() - to.filled();
		Nat samples = free / ch;

		if (bufferPos >= buffer.filled())
			fillBuffer();

		// No more data at all, it seems...
		if (bufferPos >= buffer.filled())
			return to;

		samples = min(samples, (buffer.filled() - bufferPos) / ch);
		memcpy(to.dataPtr() + to.filled(), buffer.dataPtr() + bufferPos, samples*ch*sizeof(Float));
		bufferPos += samples*ch;
		to.filled(to.filled() + samples*ch);
		currentSample += samples;

		return to;
	}

	Bool FlacSound::more() {
		return currentSample < totalSamples;
	}

	void FlacSound::fillBuffer() {
		bufferPos = 0;
		buffer.filled(0);
		FLAC__stream_decoder_process_single(data->flac);
	}

	FLAC__StreamDecoderReadStatus FlacSound::srcRead(const FLAC__StreamDecoder *decoder,
													FLAC__byte *to,
													size_t *bytes,
													void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;

		if (*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

		storm::Buffer r = me->src->read(Nat(*bytes));
		memcpy(to, r.dataPtr(), r.filled());
		*bytes = r.filled();
		if (*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	FLAC__StreamDecoderSeekStatus FlacSound::srcSeek(const FLAC__StreamDecoder *decoder,
													FLAC__uint64 offset,
													void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;
		if (!me->seekable)
			return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;

		RIStream *src = (RIStream *)me->src;
		src->seek(Word(offset));
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}

	FLAC__StreamDecoderTellStatus FlacSound::srcTell(const FLAC__StreamDecoder *decoder,
													FLAC__uint64 *offset,
													void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;
		if (!me->seekable)
			return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;

		RIStream *src = (RIStream *)me->src;
		*offset = FLAC__uint64(src->tell());
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}

	FLAC__StreamDecoderLengthStatus FlacSound::srcLength(const FLAC__StreamDecoder *decoder,
														FLAC__uint64 *length,
														void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;
		if (!me->seekable)
			return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

		RIStream *src = (RIStream *)me->src;
		*length = FLAC__uint64(src->length());
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}

	FLAC__bool FlacSound::srcEof(const FLAC__StreamDecoder *decoder,
								void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;
		return me->src->more() ? false : true;
	}

#define SAMPLE buffer[j][i]
#define CONVERT(expr)							\
	for (Nat i = 0; i < samples; i++) {			\
		for (Nat j = 0; j < ch; j++) {			\
			me->buffer[pos++] = expr;			\
		}										\
	}

	FLAC__StreamDecoderWriteStatus FlacSound::write(const FLAC__StreamDecoder *decoder,
													const FLAC__Frame *frame,
													const FLAC__int32 *const buffer[],
													void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;

		Nat samples = frame->header.blocksize;
		Nat ch = me->ch;
		Nat space = samples * ch;
		if (me->buffer.count() < space)
			me->buffer = sound::buffer(me->engine(), space);
		me->buffer.filled(space);

		Nat pos = 0;
		switch (me->bps) {
		case 8:
			CONVERT(SAMPLE / float(1 << 7));
			break;
		case 12:
			CONVERT(SAMPLE / float(1 << 11));
			break;
		case 16:
			CONVERT(SAMPLE / float(1 << 15));
			break;
		case 20:
			CONVERT(SAMPLE / float(1 << 19));
			break;
		case 24:
			CONVERT(SAMPLE / float(1 << 23));
			break;
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	void FlacSound::metadata(const FLAC__StreamDecoder *decoder,
							const FLAC__StreamMetadata *meta,
							void *data) {
		FlacSound *me = ((FlacSound::Data *)data)->owner;

		me->totalSamples = meta->data.stream_info.total_samples;
		me->rate = meta->data.stream_info.sample_rate;
		me->ch = meta->data.stream_info.channels;
		me->bps = meta->data.stream_info.bits_per_sample;
	}

	void FlacSound::error(const FLAC__StreamDecoder *decoder,
						FLAC__StreamDecoderErrorStatus status,
						void *data) {
		// FlacSound *me = ((FlacSound::Data *)data)->owner;
		// Possibilities:
		switch (status) {
		case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
			PLN(L"Lost sync.");
			break;
		case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
			PLN(L"Bad header.");
			break;
		case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
			PLN(L"CRC mismatch.");
			break;
		case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
			PLN(L"Corrupted stream.");
			break;
		}
	}

	void FlacSound::destroyData(Data *me) {
		if (me->flac) {
			FLAC__stream_decoder_delete(me->flac);
			me->flac = null;
		}
	}

	void FlacSound::dataFinalizer(void *obj, os::Thread *) {
		destroyData((Data *)obj);
	}

	FlacSound *openFlac(RIStream *src) {
		return new (src) FlacSound(src, true);
	}

	FlacSound *openFlacStream(IStream *src) {
		return new (src) FlacSound(src, false);
	}

}
