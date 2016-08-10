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

	// Nat IStream::read(Buffer &to) {
	// 	return 0;
	// }

	// Nat IStream::peek(Buffer &to) {
	// 	return 0;
	// }

	RIStream *IStream::randomAccess() {
		assert(false, L"Use lazy memory stream!");
		// return lazyIMemStream(this);
		return null;
	}

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

	// void OStream::write(const Buffer &buf) {}


}
