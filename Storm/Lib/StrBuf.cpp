#include "stdafx.h"
#include "StrBuf.h"

namespace storm {

	StrBuf::StrBuf() : buffer(null), capacity(0), pos(0) {}

	StrBuf::StrBuf(Par<StrBuf> o) : buffer(null), capacity(0), pos(0) {
		ensure(o->pos);
		pos = o->pos;
		copyArray(buffer, o->buffer, pos);
		buffer[pos] = 0;
	}

	StrBuf::StrBuf(Par<Str> str) : buffer(null), capacity(0), pos(0) {
		ensure(str->count());
		pos = str->count();
		for (nat i = 0; i < pos; i++)
			buffer[i] = str->v[i];
		buffer[pos] = 0;
	}

	void StrBuf::deepCopy(Par<CloneEnv> env) {
		// Nothing needed.
	}

	// Destroy.
	StrBuf::~StrBuf() {
		delete []buffer;
	}

	void StrBuf::ensure(nat count) {
		if (capacity >= count)
			return;

		capacity = max(max(count, capacity * 2), nat(20));
		wchar *n = new wchar[capacity + 1];
		if (buffer)
			copyArray(n, buffer, pos);
		n[pos] = 0;
		swap(n, buffer);
		delete []n;
	}

	// Get the string we've built!
	Str *StrBuf::toS() {
		if (buffer == null)
			return CREATE(Str, this, L"");
		else
			return CREATE(Str, this, buffer);
	}

	// Append stuff.
	StrBuf *StrBuf::add(Par<Str> str) {
		ensure(pos + str->count());
		for (nat i = 0; i < str->count(); i++) {
			buffer[pos++] = str->v[i];
		}

		addRef();
		return this;
	}

	// 32-bit numbers does never need more than 12 digits, including a - sign.
	static const nat maxDigits = 12;

	static void reverse(wchar *begin, wchar *end) {
		while ((begin != end) && (begin != --end)) {
			swap(*(begin++), *end);
		}
	}

	StrBuf *StrBuf::add(Int i) {
		ensure(pos + maxDigits);

		bool negative = i < 0;
		if (i < 0)
			buffer[pos++] = '-';

		nat p = 0;
		while (p < maxDigits) {
			Int last = i % 10;
			// % of negative numbers is not defined by the standard...
			if (last > 0 && negative)
				last -= 10;
			buffer[pos + p++] = wchar(abs(last) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		reverse(buffer + pos, buffer + pos + p);
		pos += p;

		addRef();
		return this;
	}

	StrBuf *StrBuf::add(Nat i) {
		ensure(pos + maxDigits);

		nat p = 0;
		while (p < maxDigits) {
			buffer[pos + p++] = wchar((i % 10) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		reverse(buffer + pos, buffer + pos + p);
		pos += p;

		addRef();
		return this;
	}

	// Add a single char.
	void StrBuf::addChar(Int p) {
		if (p >= 0xD800 && p < 0xE000) {
			// Invalid codepoint.
			// p = '?';
			// We allow this here...
		}

		if (p >= 0x110000) {
			// Too large for UTF16.
			p = '?';
		}

		if (p < 0xFFFF) {
			// One is enough!
			ensure(pos + 1);
			buffer[pos++] = (wchar)p;
		} else {
			// Surrogate pair.
			p -= 0x010000;
			ensure(pos + 2);
			buffer[pos++] = wchar(((p >> 10) & 0x3FF) + 0xD800);
			buffer[pos++] = wchar((p & 0x3FF) + 0xDC00);
		}
	}

	void StrBuf::output(wostream &to) const {
		to << buffer;
	}

}
