#include "stdafx.h"
#include "StrBuf.h"
#include "Str.h"
#include "Utf.h"
#include "GcType.h"
#include "GcArray.h"

namespace storm {

	StrFmt::StrFmt() : width(0), fill(Char(' ')) {}

	StrFmt::StrFmt(Nat width, Char fill) : width(width), fill(fill) {}

	void StrFmt::reset() {
		width = 0;
	}

	void StrFmt::clear() {
		width = 0;
		fill = Char(' ');
	}

	void StrFmt::merge(const StrFmt &o) {
		if (o.width)
			width = o.width;
		if (o.fill != Char('\0'))
			fill = o.fill;
	}

	StrFmt width(Nat w) {
		return StrFmt(w, Char('\0'));
	}

	StrFmt fill(Char c) {
		return StrFmt(0, c);
	}

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(wchar), // element size
		0,
		{},
	};

	StrBuf::StrBuf() : buf(null) {
		clear();
	}

	StrBuf::StrBuf(StrBuf *o) {
		clear();
		buf = copyBuf(o->buf);
		pos = o->pos;
	}

	StrBuf::StrBuf(Str *src) {
		clear();
		buf = copyBuf(src->data);
		pos = buf->count;
	}

	void StrBuf::deepCopy(CloneEnv *env) {
		buf = copyBuf(buf);
	}

	Str *StrBuf::toS() const {
		// We can not copy any extra data, so we can not use copyBuf()

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &bufType, pos + 1);
		for (nat i = 0; i < pos; i++)
			to->v[i] = buf->v[i];

		return new (this) Str(to);
	}

	void StrBuf::toS(StrBuf *to) const {
		to->add(c_str());
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

	void StrBuf::fill(nat toOutput) {
		nat w = fmt.width;
		Char fill = fmt.fill;
		fmt.reset();

		// Nothing to do?
		if (toOutput >= w)
			return;

		wchar first = fill.leading();
		wchar last = fill.trailing();
		nat toFill = w - toOutput;
		if (first)
			ensure(pos + toFill*2);
		else
			ensure(pos + toFill);

		for (nat i = 0; i < toFill; i++) {
			if (first)
				buf->v[pos++] = first;
			buf->v[pos++] = last;
		}
	}

	StrBuf *StrBuf::add(const wchar *data) {
		nat len = 0;
		nat points = 0;
		nat iLen = indentStr->charCount();
		nat iTot = iLen * indentation;

		// Compute the length.
		if (lastNewline())
			len += iTot;

		for (nat at = 0; data[at]; at++) {
			if (data[at] == '\n' && data[at + 1] != '\0')
				len += iLen;
			len++;
			if (!utf16::leading(data[at]))
				points++;
		}

		// Add fill.
		insertIndent();
		fill(points);

		// Reserve space (sometimes a bit too much, but that is fine).
		ensure(pos + len);

		// Copy.
		for (nat at = 0; data[at]; at++) {
			insertIndent();
			buf->v[pos++] = data[at];
		}

		return this;
	}

	StrBuf *StrBuf::add(const Str *str) {
		return add(str->c_str());
	}

	StrBuf *StrBuf::add(const Object *obj) {
		// We're doing 'toS' to make the formatting predictable.
		if (obj) {
			return add(obj->toS());
		} else {
			return add(L"null");
		}
	}

	StrBuf *StrBuf::add(const TObject *obj) {
		// We're doing 'toS' to make the formatting predictable.
		if (obj) {
			TODO(L"Call on proper thread!");
			return add(obj->toS());
		} else {
			return add(L"null");
		}
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

		nat worst = max(maxDigits, fmt.width * fmt.fill.size());

		ensure(pos + worst);

		bool negative = i < 0;

		nat p = 0;
		while (p < worst) {
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

		// Fill if needed.
		if (p < fmt.width) {
			wchar lead = fmt.fill.leading();
			wchar trail = fmt.fill.trailing();
			for (nat i = p; i < fmt.width; i++) {
				buf->v[pos + p++] = trail;
				if (lead)
					buf->v[pos + p++] = lead;
			}
			fmt.reset();
		}

		reverse(buf->v + pos, buf->v + pos + p);

		pos += p;
		return this;
	}

	StrBuf *StrBuf::add(Word i) {
		insertIndent();

		nat worst = max(maxDigits, fmt.width * fmt.fill.size());

		ensure(pos + worst);

		nat p = 0;
		while (p < worst) {
			buf->v[pos + p++] = wchar((i % 10) + '0');
			i /= 10;
			if (i == 0)
				break;
		}

		// Fill if needed.
		if (p < fmt.width) {
			wchar lead = fmt.fill.leading();
			wchar trail = fmt.fill.trailing();
			for (nat i = p; i < fmt.width; i++) {
				buf->v[pos + p++] = trail;
				if (lead)
					buf->v[pos + p++] = lead;
			}
			fmt.reset();
		}

		reverse(buf->v + pos, buf->v + pos + p);
		pos += p;

		return this;
	}

	StrBuf *StrBuf::add(Float f) {
		const nat size = 100;
		wchar buf[size];
		_snwprintf_s(buf, size, size, L"%f", f);
		// TODO: Improve!
		return add(buf);
	}

	StrBuf *StrBuf::add(Char c) {
		const wchar str[3] = {
			c.leading(),
			c.trailing(),
			0
		};

		if (str[0])
			return add(str);
		else
			return add(str + 1);
	}

	void StrBuf::clear() {
		buf = null;
		pos = 0;
		indentStr = new (this) Str(L"    ");
		indentation = 0;
		fmt.clear();
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

	StrBuf &StrBuf::operator <<(StrFmt f) {
		fmt.merge(f);
		return *this;
	}

	StrBuf &StrBuf::operator <<(const void *ptr) {
		size_t v = (size_t)ptr;
		const nat digits = sizeof(size_t) * 2;
		wchar buf[digits + 3];
		buf[0] = '0';
		buf[1] = 'x';
		buf[digits + 2] = 0;

		for (nat i = 0; i < digits; i++) {
			nat digit = (v >> ((digits - i - 1) * 4)) & 0xF;
			if (digit < 0xA)
				buf[i + 2] = '0' + digit;
			else
				buf[i + 2] = 'A' + digit - 0xA;
		}

		return *add(buf);
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

		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &bufType, newCap + 1);
		for (nat i = 0; i < pos; i++)
			to->v[i] = buf->v[i];

		buf = to;
	}

	GcArray<wchar> *StrBuf::copyBuf(GcArray<wchar> *buf) const {
		GcArray<wchar> *to = runtime::allocArray<wchar>(engine(), &bufType, buf->count);
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
