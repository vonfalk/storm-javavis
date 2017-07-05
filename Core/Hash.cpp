#include "stdafx.h"
#include "Hash.h"
#include "Handle.h"

namespace storm {

	/**
	 * Inspired from http://burtleburtle.net/bob/hash/integer.html (public domain) and
	 * http://www.burtleburtle.net/bob/hash/doobs.html (also public domain)
	 */

	Nat byteHash(Byte v) {
		return natHash(Nat(v));
	}

	Nat intHash(Int v) {
		return natHash(Nat(v));
	}

	Nat natHash(Nat v) {
		v = (v ^ 0xDEADBEEF) + (v << 4);
		v = v ^ (v >> 10);
		v = v + (v << 7);
		v = v ^ (v >> 13);
		return v;
	}

	Nat longHash(Long v) {
		return wordHash(Word(v));
	}

	Nat wordHash(Word v) {
		v += ~(v << 32);
        v ^= (v >> 22);
        v += ~(v << 13);
        v ^= (v >> 8);
        v += (v << 3);
        v ^= (v >> 15);
        v += ~(v << 27);
        v ^= (v >> 31);
		return Nat(v);
	}

	Nat ptrHash(const void *v) {
		if (sizeof(v) == sizeof(Nat))
			return natHash(Nat(size_t(v)));
		else
			return wordHash(Word(size_t(v)));
	}

	void checkHashHandle(const Handle &h) {
		assert(h.hashFn && h.equalFn, L"Incomplete handle for use in a hash container.");
	}

}
