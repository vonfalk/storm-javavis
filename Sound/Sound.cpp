#include "stdafx.h"
#include "Sound.h"
#include "OggSound.h"
#include <iomanip>

namespace sound {

	/**
	 * Buffer
	 */

	SoundBuffer::SoundBuffer(float *buffer, nat size) : size(size), data(buffer), owner(false) {}

	SoundBuffer::SoundBuffer(Nat size) : size(size), data(new float[size]), owner(true) {}

	SoundBuffer::SoundBuffer(const SoundBuffer &o) : size(o.size), data(new float[o.size]), owner(true) {
		copyArray(data, o.data, size);
	}

	SoundBuffer &SoundBuffer::operator =(const SoundBuffer &o) {
		if (owner)
			delete []data;
		size = o.size;
		data = new float[size];
		copyArray(data, o.data, size);
		return *this;
	}

	SoundBuffer::~SoundBuffer() {
		if (owner)
			delete []data;
	}

	wostream &operator <<(wostream &to, const SoundBuffer &b) {
		for (nat i = 0; i < b.count(); i++) {
			to << std::setw(10) << b[i];
			if (i != b.count() - 1) {
				if (i % 8 == 7)
					to << endl;
				else
					to << ' ';
			}
		}

		return to;
	}

	Str *toS(EnginePtr e, SoundBuffer b) {
		return CREATE(Str, e.v, ::toS(b));
	}

	/**
	 * Sound interface.
	 */

	Sound::Sound() {}

	Word Sound::tell() {
		return 0;
	}

	Bool Sound::seek(Word to) {
		return false;
	}

	Word Sound::length() {
		return 0;
	}

	Nat Sound::sampleFreq() const {
		return 0;
	}

	Nat Sound::channels() const {
		return 1;
	}

	Nat Sound::read(SoundBuffer &to) {
		return 0;
	}

	void Sound::output(wostream &to) const {
		to << L"Sound " << channels() << L"ch @" << sampleFreq() << L"Hz";
	}


	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	bool checkHeader(Par<storm::IStream> file, char *header, bool zeroTerm) {
		nat len = strlen(header);
		if (zeroTerm)
			len++;

		Buffer buffer(len);
		nat r = file->peek(buffer);
		if (r != len) {
			return false;
		}

		for (nat i = 0; i < len; i++) {
			if (byte(buffer[i]) != byte(header[i])) {
				return false;
			}
		}

		return true;
	}


	Sound *openSound(Par<IStream> src) {
		Auto<Sound> loaded;

		if (checkHeader(src, "OggS", false)) {
			return openOgg(steal(src->randomAccess()));
		}

		throw SoundOpenError(L"Unknown file format.");
	}

	Sound *openStreamingSound(Par<IStream> src) {
		Auto<Sound> loaded;

		if (checkHeader(src, "OggS", false)) {
			return openStreamingOgg(src);
		}

		throw SoundOpenError(L"Unknown file format.");
	}

}
