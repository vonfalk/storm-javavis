#include "stdafx.h"
#include "MemStream.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {

	IMemStream::IMemStream(Buffer b) : data(buffer(engine(), b.filled())), pos(0) {
		// Copy the filled data.
		memcpy(data.dataPtr(), b.dataPtr(), b.filled());
	}

	IMemStream::IMemStream(const IMemStream &o) : data(o.data) {}

	void IMemStream::deepCopy(CloneEnv *env) {
		// We never change 'data', so we do not need to clone it here.
	}

	Bool IMemStream::more() {
		return pos < data.count();
	}

	Buffer IMemStream::read(Buffer to, Nat start) {
		start = min(start, to.count());
		to = peek(to, start);
		pos += to.filled() - start;
		return to;
	}

	Buffer IMemStream::peek(Buffer to, Nat start) {
		start = min(start, to.count());
		nat copy = min(data.count() - pos, to.count() - start);
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
