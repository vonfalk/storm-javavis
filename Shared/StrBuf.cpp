#include "stdafx.h"
#include "StrBuf.h"
#include "Str.h"

namespace storm {

	StrBuf::StrBuf() : buffer(null), capacity(0), pos(0) {}

	StrBuf::StrBuf(Par<StrBuf> o) : buffer(null), capacity(0), pos(0) {
		ensure(o->pos);
		pos = o->pos;
		copyArray(buffer, o->buffer, pos);
	}

	StrBuf::StrBuf(Par<Str> str) : buffer(null), capacity(0), pos(0) {
		ensure(str->count());
		pos = str->count();
		for (nat i = 0; i < pos; i++)
			buffer[i] = str->v[i];
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
		// Note: We allocate one extra entry for the null terminator here!
		wchar *n = new wchar[capacity + 1];
		if (buffer)
			copyArray(n, buffer, pos);
		swap(n, buffer);
		delete []n;
	}

	void StrBuf::nullTerminate() const {
		buffer[pos] = 0;
	}

	// Get the string we've built!
	Str *StrBuf::toS() {
		if (buffer == null) {
			return CREATE(Str, this, L"");
		} else {
			nullTerminate();
			return CREATE(Str, this, buffer);
		}
	}

	// Append stuff.
	StrBuf *StrBuf::add(Str *str) {
		return add(Par<Str>(str));
	}

	StrBuf *StrBuf::add(Par<Str> str) {
		ensure(pos + str->count());
		for (nat i = 0; i < str->count(); i++) {
			buffer[pos++] = str->v[i];
		}

		addRef();
		return this;
	}

	void StrBuf::add(const String &str) {
		ensure(pos + str.size());
		for (nat i = 0; i < str.size(); i++) {
			buffer[pos++] = str[i];
		}
	}

	void StrBuf::add(const wchar *str) {
		ensure(pos + wcslen(str));
		while (*str) {
			buffer[pos++] = *(str++);
		}
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

	// 64-bit numbers does never need more than 21 digits, including a - sign.
	static const nat maxDigits64 = 21;

	StrBuf *StrBuf::add(Long i) {
		ensure(pos + maxDigits64);

		bool negative = i < 0;
		if (i < 0)
			buffer[pos++] = '-';

		nat p = 0;
		while (p < maxDigits64) {
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

	StrBuf *StrBuf::add(Word i) {
		ensure(pos + maxDigits64);

		nat p = 0;
		while (p < maxDigits64) {
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

	StrBuf *StrBuf::add(Byte i) {
		return add(Nat(i));
	}

	StrBuf *StrBuf::add(Float i) {
		const nat max = 20;
		ensure(pos + max);
		pos += _snwprintf_s(buffer + pos, max, max, L"%f", i);

		addRef();
		return this;
	}

	// Add a single char.
	void StrBuf::addChar(Nat p) {
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
		if (buffer) {
			nullTerminate();
			to << buffer;
		}
	}

}
