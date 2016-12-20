#include "stdafx.h"
#include "Stream.h"

namespace storm {

	/**
	 * IStream.
	 */

	IStream::IStream() {}

	Bool IStream::more() {
		return false;
	}

	Buffer IStream::read(Nat c) {
		return read(buffer(engine(), c), 0);
	}

	Buffer IStream::read(Buffer to) {
		return read(to, 0);
	}

	Buffer IStream::read(Buffer to, Nat start) {
		to.filled(0);
		return to;
	}

	Buffer IStream::peek(Nat c) {
		return peek(buffer(engine(), c), 0);
	}

	Buffer IStream::peek(Buffer to) {
		return peek(to, 0);
	}

	Buffer IStream::peek(Buffer to, Nat start) {
		to.filled(0);
		return to;
	}

	Buffer IStream::readAll(Nat c) {
		return readAll(buffer(engine(), c));
	}

	Buffer IStream::readAll(Buffer b) {
		b.filled(0);
		while (!b.full() && more()) {
			b = read(b, b.filled());
		}
		return b;
	}

	RIStream *IStream::randomAccess() {
		assert(false, L"TODO: Use lazy memory stream!");
		// return lazyIMemStream(this);
		return null;
	}

	void IStream::close() {}

	/**
	 * RIStream.
	 */

	RIStream::RIStream() {}

	void RIStream::seek(Word to) {}

	Word RIStream::tell() { return 0; }

	Word RIStream::length() { return 0; }

	RIStream *RIStream::randomAccess() {
		return this;
	}


	/**
	 * OStream.
	 */

	OStream::OStream() {}

	void OStream::write(Buffer buf) {}

	void OStream::close() {}

}
