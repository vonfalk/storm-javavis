#include "stdafx.h"
#include "Stream.h"

namespace storm {

	/**
	 * Buffer
	 */

	Buffer::Buffer(void *buffer, nat size) : size(size), data((byte *)buffer), owner(false) {}

	Buffer::Buffer(Nat size) : size(size), data(new byte[size]), owner(true) {}

	Buffer::Buffer(const Buffer &o) : size(o.size), data(new byte[size]), owner(true) {
		copyArray(data, o.data, size);
	}

	Buffer &Buffer::operator =(const Buffer &o) {
		if (owner)
			delete []data;
		size = o.size;
		data = new byte[size];
		copyArray(data, o.data, size);
		return *this;
	}

	Buffer::~Buffer() {
		if (owner)
			delete []data;
	}

	/**
	 * IStream.
	 */

	IStream::IStream() {}

	Bool IStream::more() {
		return false;
	}

	Nat IStream::read(Buffer &to) {
		return 0;
	}

	Nat IStream::peek(Buffer &to) {
		return 0;
	}

	/**
	 * OStream.
	 */

	OStream::OStream() {}

	void OStream::write(const Buffer &buf) {}

}
