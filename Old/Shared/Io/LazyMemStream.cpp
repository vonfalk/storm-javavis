#include "stdafx.h"
#include "LazyMemStream.h"
#include "Shared/CloneEnv.h"

namespace storm {

	LazyIMemStream::LazyIMemStream() : src(), buffer(null), len(0), pos(0) {}

	LazyIMemStream::LazyIMemStream(Par<LazyIMemStream> o) : src(o->src), buffer(null), len(o->len), pos(o->pos) {
		if (o->buffer) {
			buffer = new byte[len];
			memcpy(buffer, o->buffer, len);
		}
		clone(src, steal(CREATE(CloneEnv, this)));
	}

	LazyIMemStream::~LazyIMemStream() {
		delete []buffer;
	}

	void LazyIMemStream::deepCopy(Par<CloneEnv> env) {
		clone(src, env);
	}

	Bool LazyIMemStream::more() {
		return pos < len || src->more();
	}

	Nat LazyIMemStream::read(Buffer &to) {
		ensure(pos + to.count());
		nat l = min(to.count(), len - pos);
		memcpy(to.dataPtr(), buffer + pos, l);
		pos += l;
		return l;
	}

	Nat LazyIMemStream::peek(Buffer &to) {
		ensure(pos + to.count());
		nat l = min(to.count(), len - pos);
		memcpy(to.dataPtr(), buffer + pos, l);
		return l;
	}

	void LazyIMemStream::seek(Word t) {
		nat to = nat(t);
		ensure(to);
		pos = min(to, len);
	}

	Word LazyIMemStream::tell() {
		return pos;
	}

	Word LazyIMemStream::length() {
		return len;
	}

	bool LazyIMemStream::ensure(nat b) {
		if (pos + b <= len)
			return true;

		nat g = max(grow, pos + b - len);
		byte *d = new byte[len + g];
		if (buffer) {
			memcpy(buffer, d, len);
			delete []buffer;
		}
		buffer = d;

		g = src->read(Buffer(buffer + len, g));
		len += g;

		return pos + b <= len;
	}

	LazyIMemStream *lazyIMemStream(Par<IStream> src) {
		LazyIMemStream *s = CREATE(LazyIMemStream, src);
		s->src = src;
		return s;
	}

}
