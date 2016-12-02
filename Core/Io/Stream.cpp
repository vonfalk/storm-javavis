#include "stdafx.h"
#include "Stream.h"

namespace storm {

	/**
	 * IStream.
	 */

	IStream::IStream() {}

	IStream::IStream(IStream *o) {}

	Bool IStream::more() {
		return false;
	}

	Buffer IStream::read(Nat c) {
		return read(buffer(engine(), c));
	}

	Buffer IStream::read(Buffer to) {
		to.filled(0);
		return to;
	}

	Buffer IStream::peek(Nat c) {
		return peek(buffer(engine(), c));
	}

	Buffer IStream::peek(Buffer to) {
		to.filled(0);
		return to;
	}

	RIStream *IStream::randomAccess() {
		assert(false, L"Use lazy memory stream!");
		// return lazyIMemStream(this);
		return null;
	}

	void IStream::close() {}

	/**
	 * RIStream.
	 */

	RIStream::RIStream() {}
	RIStream::RIStream(RIStream *o) {}

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

	OStream::OStream(OStream *o) {}

	void OStream::write(Buffer buf) {}

	void OStream::close() {}

}
