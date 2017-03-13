#include "stdafx.h"
#include "Loop.h"
#include <limits>

namespace sound {

	SoundLoop::SoundLoop(Sound *src) : src(src), noSeek(false) {}

	void SoundLoop::close() {
		src->close();
	}

	Word SoundLoop::tell() {
		return src->length() * repeat + src->tell();
	}

	Bool SoundLoop::seek(Word to) {
		Word l = src->length();
		repeat = to / l;
		return src->seek(to % l);
	}

	Word SoundLoop::length() {
		return std::numeric_limits<Word>::max();
	}

	Nat SoundLoop::sampleFreq() const {
		return src->sampleFreq();
	}

	Nat SoundLoop::channels() const {
		return src->channels();
	}

	Buffer SoundLoop::read(Buffer to) {
		if (!src->more()) {
			if (src->seek(0))
				repeat++;
			else
				noSeek = true;
		}

		return src->read(to);
	}

	Bool SoundLoop::more() {
		if (noSeek)
			return src->more();
		else
			return true;
	}

	SoundLoop *loop(Sound *src) {
		return new (src) SoundLoop(src);
	}

}
