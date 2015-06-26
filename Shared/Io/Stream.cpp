#include "stdafx.h"
#include "Stream.h"
#include "LazyMemStream.h"

namespace storm {

	/**
	 * Buffer
	 */

	Buffer::Buffer(void *buffer, nat size) : size(size), data((byte *)buffer), owner(false) {}

	Buffer::Buffer(Nat size) : size(size), data(new byte[size]), owner(true) {}

	Buffer::Buffer(const Buffer &o) : size(o.size), data(new byte[o.size]), owner(true) {
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

	wostream &operator <<(wostream &to, const Buffer &b) {
		for (nat i = 0; i < b.count(); i++) {
			to << toHex(b[i]);
			if (i != b.count() - 1) {
				if (i % 16 == 15)
					to << endl;
				else
					to << ' ';
			}
		}

		return to;
	}

	/**
	 * IStream.
	 */

	IStream::IStream() {}

	IStream::IStream(Par<IStream> o) {}

	Bool IStream::more() {
		return false;
	}

	Nat IStream::read(Buffer &to) {
		return 0;
	}

	Nat IStream::peek(Buffer &to) {
		return 0;
	}

	RIStream *IStream::randomAccess() {
		return lazyIMemStream(this);
	}

	/**
	 * RIStream.
	 */

	RIStream::RIStream() {}
	RIStream::RIStream(Par<RIStream> o) {}

	void RIStream::seek(Word to) {}

	Word RIStream::tell() { return 0; }

	Word RIStream::length() { return 0; }

	RIStream *RIStream::randomAccess() {
		addRef();
		return this;
	}


	/**
	 * OStream.
	 */

	OStream::OStream() {}

	OStream::OStream(Par<OStream> o) {}

	void OStream::write(const Buffer &buf) {}

}
