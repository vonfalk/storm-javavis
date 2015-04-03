#include "stdafx.h"
#include "MemStream.h"

namespace storm {

	/**
	 * Input.
	 */

	IMemStream::IMemStream(const Buffer &data) : data(data) {}

	IMemStream::IMemStream(Par<IMemStream> o) : data(o->data) {}

	void IMemStream::deepCopy(Par<CloneEnv> env) {
		IStream::deepCopy(env);
		// Nothing to do here.
	}

	Bool IMemStream::more() {
		return data.count() == pos;
	}

	Nat IMemStream::read(Buffer &to) {
		Nat r = peek(to);
		pos += r;
		return r;
	}

	Nat IMemStream::peek(Buffer &to) {
		nat rem = min(data.count() - pos, to.count());
		memcpy(to.dataPtr(), data.dataPtr() + pos, rem);
		return rem;
	}

	/**
	 * Output.
	 */

	OMemStream::OMemStream() : data(null), capacity(0), pos(0) {}

	OMemStream::OMemStream(Par<OMemStream> src) {
		data = new byte[src->capacity];
		pos = src->pos;
		capacity = src->capacity;
		memcpy(data, src->data, src->pos);
	}

	OMemStream::~OMemStream() {
		delete []data;
	}

	void OMemStream::deepCopy(Par<CloneEnv> e) {
		OStream::deepCopy(e);
		// Nothing to do here.
	}

	void OMemStream::write(const Buffer &buf) {
		ensure(pos + buf.count());
		memcpy(data + pos, buf.dataPtr(), buf.count());
		pos += buf.count();
	}

	void OMemStream::ensure(nat size) {
		if (size <= capacity)
			return;

		capacity = max(nat(100), capacity * 2);
		byte *n = new byte[capacity];
		memcpy(n, data, pos);
		swap(n, data);
		delete []n;
	}

	Buffer OMemStream::buffer() {
		return Buffer(data, pos);
	}

}
