#include "stdafx.h"
#include "Sound.h"
#include "Core/StrBuf.h"

namespace sound {

	Sound::Sound() {}

	void Sound::close() {}

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
		return 0;
	}

	Buffer Sound::read(Nat samples) {
		return read(sound::buffer(engine(), samples));
	}

	Buffer Sound::read(Buffer to) {
		return to;
	}

	Bool Sound::more() {
		return false;
	}

	void Sound::toS(StrBuf *to) const {
		*to << L"Sound: " << sampleFreq() << L" Hz, " << channels() << L" channels";
	}

}
