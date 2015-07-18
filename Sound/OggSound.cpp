#include "stdafx.h"
#include "OggSound.h"
#include <limits>

namespace sound {

	OggSound::OggSound(OggVorbis_File file, bool seekable) : file(file), seekable(seekable) {
		/**
		 * NOTE: There seems to be possible to have multiple streams inside a vorbis file,
		 * we do not handle that correctly at the moment. Now we only show the first stream.
		 */

		vi = ov_info(&file, 0);
	}

	OggSound::~OggSound() {
		ov_clear(&file);
	}

	Word OggSound::tell() {
		return ov_pcm_tell(&file);
	}

	Bool OggSound::seek(Word to) {
		if (!seekable)
			return false;

		ov_pcm_seek(&file, to);
		return true;
	}

	Word OggSound::length() {
		return ov_pcm_total(&file, 0);
	}

	Nat OggSound::sampleFreq() const {
		return vi->rate;
	}

	Nat OggSound::channels() const {
		return vi->channels;
	}

	Nat OggSound::read(SoundBuffer &to) {
		float **buffer;
		nat ch = channels();
		int samples = int(to.count() / ch);
		int bitstream = 0;
		samples = ov_read_float(&file, &buffer, samples, &bitstream);

		// Copy...
		nat at = 0;
		for (int i = 0; i < samples; i++) {
			for (nat c = 0; c < ch; c++) {
				to[at++] = buffer[c][i];
			}
		}

		return samples * ch;
	}

	Bool OggSound::more() {
		return ov_pcm_tell(&file) < ov_pcm_total(&file, 0);
	}

	struct OggData {
		OggData(Par<IStream> src) : stream(src) {}

		Auto<IStream> stream;
	};

	size_t oggRead(void *to, size_t size, size_t nmemb, void *d) {
		OggData *data = (OggData *)d;
		size_t s = size * nmemb;
		s = min(s, std::numeric_limits<Nat>::max());

		return data->stream->read(Buffer(to, s));
	}

	int oggSeek(void *d, ogg_int64_t offset, int whence) {
		OggData *data = (OggData *)d;
		RIStream *s = as<RIStream>(data->stream.borrow());
		if (!s)
			return -1;

		switch (whence) {
		case SEEK_SET:
			s->seek(offset);
			break;
		case SEEK_CUR:
			s->seek(s->tell() + offset);
			break;
		case SEEK_END:
			s->seek(s->length() + offset);
			break;
		}

		return 0;
	}

	int oggClose(void *d) {
		OggData *data = (OggData *)d;
		delete data;
		return 0;
	}

	long oggTell(void *d) {
		OggData *data = (OggData *)d;
		RIStream *s = as<RIStream>(data->stream.borrow());
		if (!s)
			return -1;

		return (long)s->tell();
	}

	OggSound *openOgg(Par<RIStream> src) {
		return openStreamingOgg(src);
	}

	OggSound *openStreamingOgg(Par<IStream> src) {
		OggVorbis_File file;
		OggData *data = new OggData(src);

		ov_callbacks cb = {
			&oggRead, &oggSeek, &oggClose, &oggTell
		};

		switch (ov_open_callbacks(data, &file, null, 0, cb)) {
		case 0:
		{
			bool seekable = as<RIStream>(src.borrow()) != null;
			return CREATE(OggSound, src, file, seekable);
		}
		case OV_ENOTVORBIS:
			throw SoundOpenError(L"Unknown file format.");
		case OV_EVERSION:
			throw SoundOpenError(L"Ogg version mismatch.");
		case OV_EBADHEADER:
			throw SoundOpenError(L"Bad ogg header.");
		default:
			throw SoundOpenError(L"Unknown error.");
		}
	}

}
