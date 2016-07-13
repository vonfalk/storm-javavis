#include "stdafx.h"
#include "StrBuf.h"
#include "Engine.h"
#include "Str.h"

namespace storm {

	static const GcType bufType = {
		GcType::tArray,
		null,
		sizeof(wchar),
		0,
		{},
	};

	StrBuf::StrBuf() : buf(null) {
		clear();
	}

	StrBuf::StrBuf(StrBuf *o) {
		clear();
		buf = copyBuf(o->buf);
	}

	StrBuf::StrBuf(Str *src) {
		clear();
		buf = copyBuf(src->data);
	}

	void StrBuf::deepCopy(CloneEnv *env) {
		buf = copyBuf(buf);
	}

	Str *StrBuf::toS() const {
		return new (this) Str(copyBuf(buf));
	}

	wchar *StrBuf::c_str() const {
		return buf->v;
	}

	bool StrBuf::lastNewline() const {
		if (pos == 0)
			return true;
		return buf->v[pos - 1] == '\n';
	}

	void StrBuf::insertIndent() {
		if (!lastNewline())
			return;

		nat iLen = indentStr->charCount();
		nat iTot = iLen * indentation;
		ensure(pos + iTot);

		for (nat i = 0; i < indentation; i++) {
			for (nat j = 0; j < iLen; j++) {
				buf->v[pos++] = indentStr->data->v[j];
			}
		}
	}

	StrBuf *StrBuf::add(const wchar *data) {
		nat len = 0;
		nat iLen = indentStr->charCount();
		nat iTot = iLen * indentation;

		// Compute the length.
		if (lastNewline())
			len += iTot;

		for (nat at = 0; data[at]; at++) {
			if (data[at] == '\n' && data[at + 1] != '\0')
				len += iLen;
			len++;
		}

		// Reserve space.
		ensure(pos + len);

		// Copy.
		for (nat at = 0; data[at]; at++) {
			insertIndent();
			buf->v[pos++] = data[at];
		}

		return this;
	}

	StrBuf *StrBuf::add(Str *str) {
		return add(str->c_str());
	}

	StrBuf *StrBuf::add(Bool b) {
		return add(b ? L"true" : L"false");
	}

	StrBuf *StrBuf::add(Byte i) {
		return add(Word(i));
	}

	StrBuf *StrBuf::add(Int i) {
		return add(Long(i));
	}

	StrBuf *StrBuf::add(Nat i) {
		return add(Word(i));
	}

	// 64-bit numbers never need more than 21 digits, including a - sign.
	static const nat maxDigits = 21;

	static void reverse(wchar *begin, wchar *end) {
		while ((begin != end) && (begin != --end)) {
			swap(*(begin++), *end);
		}
	}

	StrBuf *StrBuf::add(Long i) {
		insertIndent();

		ensure(pos + maxDigits);

		bool negative = i < 0;

		nat p = 0;
		while (p < maxDigits) {
			Long last = i % 10;
			if (last > 0 && negative)
				last -= 10;
			buf->v[pos + p++] = wchar(abs(last) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		if (negative)
			buf->v[pos + p++] = '-';

		reverse(buf->v + pos, buf->v + pos + p);

		pos += p;
		return this;
	}

	StrBuf *StrBuf::add(Word i) {
		insertIndent();

		ensure(pos + maxDigits);

		nat p = 0;
		while (p < maxDigits) {
			buf->v[pos + p++] = wchar((i % 10) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		reverse(buf->v + pos, buf->v + pos + p);
		pos += p;

		return this;
	}

	void StrBuf::clear() {
		buf = null;
		pos = 0;
		indentStr = new (this) Str(L"    ");
		indentation = 0;
	}

	void StrBuf::indent() {
		indentation++;
	}

	void StrBuf::dedent() {
		indentation--;
	}

	void StrBuf::indentBy(Str *str) {
		indentStr = str;
	}

	Bool StrBuf::empty() const {
		return pos == 0 || buf == null;
	}

	Bool StrBuf::any() const {
		return !empty();
	}

	void StrBuf::ensure(nat capacity) {
		nat curr = 0;
		if (buf)
			curr = buf->count - 1;

		if (curr >= capacity)
			return;

		// Grow!
		nat newCap = max(capacity, curr * 2);
		newCap = max(newCap, nat(16));

		GcArray<wchar> *to = engine().gc.allocArray<wchar>(&bufType, newCap + 1);
		for (nat i = 0; i < pos; i++)
			to->v[i] = buf->v[i];

		buf = to;
	}

	GcArray<wchar> *StrBuf::copyBuf(GcArray<wchar> *buf) const {
		GcArray<wchar> *to = engine().gc.allocArray<wchar>(&bufType, buf->count);
		for (nat i = 0; i < buf->count; i++)
			to->v[i] = buf->v[i];
		return to;
	}

	/**
	 * Indent.
	 */

	Indent::Indent(StrBuf *buf) : buf(buf) {
		buf->indent();
	}

	Indent::~Indent() {
		buf->dedent();
	}
}
