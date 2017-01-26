#include "stdafx.h"
#include "MemStream.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {

	IMemStream::IMemStream(Buffer b) : data(buffer(engine(), b.filled())), pos(0) {
		// Copy the filled data.
		memcpy(data.dataPtr(), b.dataPtr(), b.filled());
		data.filled(b.filled());
	}

	IMemStream::IMemStream(Buffer b, Nat start) : data(buffer(engine(), b.filled())), pos(start) {
		// Copy the filled data.
		memcpy(data.dataPtr(), b.dataPtr(), b.filled());
		data.filled(b.filled());
	}

	IMemStream::IMemStream(const IMemStream &o) : data(o.data) {}

	void IMemStream::deepCopy(CloneEnv *env) {
		// We never change 'data', so we do not need to clone it here.
	}

	Bool IMemStream::more() {
		return pos < data.count();
	}

	Buffer IMemStream::read(Buffer to) {
		Nat start = to.filled();
		to = peek(to);
		pos += to.filled() - start;
		return to;
	}

	Buffer IMemStream::peek(Buffer to) {
		Nat start = to.filled();
		Nat copy = min(to.count() - start, data.count() - pos);
		memcpy(to.dataPtr() + start, data.dataPtr() + pos, copy);
		to.filled(copy + start);
		return to;
	}

	void IMemStream::seek(Word to) {
		pos = min(nat(to), data.count());
	}

	Word IMemStream::tell() {
		return pos;
	}

	Word IMemStream::length() {
		return data.count();
	}

	RIStream *IMemStream::randomAccess() {
		return this;
	}

	void IMemStream::toS(StrBuf *to) const {
		outputMark(*to, data, pos);
	}


	OMemStream::OMemStream() {}

	OMemStream::OMemStream(Buffer appendTo) : data(appendTo) {}

	OMemStream::OMemStream(const OMemStream &o) : data(o.data) {
		data.deepCopy(null);
	}

	void OMemStream::deepCopy(CloneEnv *env) {
		// Nothing to do...
	}

	void OMemStream::write(Buffer src, Nat start) {
		start = min(start, src.filled());
		Nat copy = src.filled() - start;
		Nat filled = data.filled();
		if (filled + copy >= data.count()) {
			// Grow the buffer...
			Nat growTo = max(max(data.count() * 2, copy), Nat(1024));
			data = grow(engine(), data, growTo);
		}

		// Copy data.
		memcpy(data.dataPtr() + filled, src.dataPtr() + start, copy);
		data.filled(filled + copy);
	}

	Buffer OMemStream::buffer() {
		return data;
	}

	void OMemStream::toS(StrBuf *to) const {
		*to << data;
	}

}
