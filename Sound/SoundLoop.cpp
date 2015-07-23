#include "stdafx.h"
#include "SoundLoop.h"
#include <limits>

namespace sound {

	SoundLoop::SoundLoop(Par<Sound> src) : src(src), noSeek(false) {}

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

	Nat SoundLoop::read(SoundBuffer &to) {
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

	SoundLoop *loop(Par<Sound> src) {
		return CREATE(SoundLoop, src, src);
	}

}
