#include "stdafx.h"
#include "MemStream.h"

namespace storm {

	IMemStream::IMemStream(const Buffer &data) : data(data) {}

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

}
