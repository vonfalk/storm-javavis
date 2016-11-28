#include "stdafx.h"
#include "Str.h"
#include "StrBuf.h"
#include "GcType.h"
#include "Utf.h"

namespace storm {

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(wchar), // element size
		0,
		{},
	};

	static GcArray<wchar> empty = {
		1,
	};

	static void check(GcArray<wchar> *data) {
		for (nat i = 0; i < data->count - 1; i++)
			if (data->v[i] == 0)
				DebugBreak();
	}

	Str::Str() : data(&storm::empty) {}

	Str::Str(const wchar *s) {
		nat count = wcslen(s);
		allocData(count + 1);
		for (nat i = 0; i < count; i++)
			data->v[i] = s[i];
		data->v[count] = 0;
		validate();
	}

	Str::Str(const wchar *from, const wchar *to) {
		nat count = to - from;
		allocData(count + 1);
		for (nat i = 0; i < count; i++)
			data->v[i] = from[i];
		data->v[count] = 0;
		validate();
	}

	Str::Str(Char ch) {
		wchar lead = ch.leading();
		wchar trail = ch.trailing();

		if (lead) {
			allocData(3);
			data->v[0] = lead;
			data->v[1] = trail;
		} else {
			allocData(2);
			data->v[0] = trail;
		}
		validate();
	}

	Str::Str(Char ch, Nat times) {
		wchar lead = ch.leading();
		wchar trail = ch.trailing();

		if (lead) {
			allocData(2*times + 1);
			for (nat i = 0; i < times; i++) {
				data->v[i*2] = lead;
				data->v[i*2 + 1] = trail;
			}
		} else {
			allocData(times + 1);
			for (nat i = 0; i < times; i++) {
				data->v[i] = trail;
			}
		}
		validate();
	}

	Str::Str(GcArray<wchar> *data) : data(data) {
		validate();
	}

	// The data is immutable, no need to copy it!
	Str::Str(Str *o) : data(o->data) {
		validate();
	}

	void Str::validate() const {
#ifdef DEBUG
		for (nat i = 0; i < data->count - 1; i++)
			if (data->v[i] == 0)
				assert(false, L"String contains a premature null terminator!");
		assert(data->v[data->count - 1] == 0, L"String is missing a null terminator!");
#endif
	}

	Bool Str::empty() const {
		return data->count == 1;
	}

	Bool Str::any() const {
		return !empty();
	}

	Str *Str::operator +(Str *o) const {
		return new (this) Str(this, o);
	}

	Str *Str::operator +(const wchar *o) const {
		return new (this) Str(this, o);
	}

	Str::Str(const Str *a, const Str *b) {
		size_t aSize = a->data->count - 1;
		size_t bSize = b->data->count - 1;

		allocData(aSize + bSize + 1);
		for (size_t i = 0; i < aSize; i++)
			data->v[i] = a->data->v[i];
		for (size_t i = 0; i < bSize; i++)
			data->v[i + aSize] = b->data->v[i];
		data->v[aSize + bSize] = 0;
	}

	Str::Str(const Str *a, const wchar *b) {
		size_t aSize = a->data->count - 1;
		size_t bSize = wcslen(b);

		allocData(aSize + bSize + 1);
		for (size_t i = 0; i < aSize; i++)
			data->v[i] = a->data->v[i];
		for (size_t i = 0; i < bSize; i++)
			data->v[i + aSize] = b[i];
		data->v[aSize + bSize] = 0;
	}

	Str *Str::operator *(Nat times) const {
		return new (this) Str(this, times);
	}

	Str::Str(const Str *a, Nat times) {
		size_t s = a->data->count - 1;
		allocData(s*times + 1);

		size_t at = 0;
		for (Nat i = 0; i < times; i++) {
			for (size_t j = 0; j < s; j++) {
				data->v[at++] = a->data->v[j];
			}
		}
	}

	Bool Str::equals(Object *o) const {
		if (!Object::equals(o))
			return false;

		Str *other = (Str *)o;
		return wcscmp(c_str(), other->c_str()) == 0;
	}

	Nat Str::hash() const {
		// djb2 hash
		Nat r = 5381;
		size_t to = data->count - 1;
		for (size_t j = 0; j < to; j++)
			r = ((r << 5) + r) + data->v[j];

		return r;
	}

	Int Str::toInt() const {
		wchar_t *end;
		Int r = wcstol(data->v, &end, 10);
		if (end != data->v + data->count)
			throw StrError(L"Not a number");
		return r;
	}

	Nat Str::toNat() const {
		wchar_t *end;
		Nat r = wcstoul(data->v, &end, 10);
		if (end != data->v + data->count)
			throw StrError(L"Not a number");
		return r;
	}

	Long Str::toLong() const {
		wchar_t *end;
		Long r = _wcstoi64(data->v, &end, 10);
		if (end != data->v + data->count)
			throw StrError(L"Not a number");
		return r;
	}

	Word Str::toWord() const {
		wchar_t *end;
		Word r = _wcstoui64(data->v, &end, 10);
		if (end != data->v + data->count)
			throw StrError(L"Not a number");
		return r;
	}

	Float Str::toFloat() const {
		wchar_t *end;
		Float r = (float)wcstod(data->v, &end);
		if (end != data->v + data->count)
			throw StrError(L"Not a floating-point number");
		return r;
	}

	void Str::deepCopy(CloneEnv *env) {
		// We don't have any mutable data we need to clone.
	}

	Str *Str::toS() const {
		// We're not mutable anyway...
		return (Str *)this;
	}

	void Str::toS(StrBuf *buf) const {
		buf->add(this);
	}

	wchar *Str::c_str() const {
		return data->v;
	}

	void Str::allocData(nat count) {
		data = runtime::allocArray<wchar>(engine(), &bufType, count);
	}

	Str::Iter Str::begin() const {
		return Iter(this);
	}

	Str::Iter Str::end() const {
		return Iter();
	}

	Str *Str::substr(Iter start) {
		return substr(start, end());
	}

	wchar *Str::toPtr(const Iter &i) {
		if (i.atEnd())
			return data->v + data->count - 1;
		else if (i.owner == this)
			return data->v + i.pos;
		else
			// Fallback if it is referring to the wrong object.
			return data->v;
	}

	Str *Str::substr(Iter start, Iter end) {
		return new (this) Str(toPtr(start), toPtr(end));
	}

	Str::Iter::Iter() : owner(null), pos(0) {}

	Str::Iter::Iter(const Str *owner) : owner(owner), pos(0) {}

	void Str::Iter::deepCopy(CloneEnv *env) {}

	Str::Iter &Str::Iter::operator ++() {
		if (atEnd())
			return *this;

		if (utf16::leading(owner->data->v[pos]))
			pos += 2;
		else
			pos++;

		return *this;
	}

	Str::Iter Str::Iter::operator ++(int dummy) {
		Iter t = *this;
		++*this;
		return t;
	}

	Bool Str::Iter::operator ==(const Iter &o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();

		return owner == o.owner && pos == o.pos;
	}

	Bool Str::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	// Get the value.
	Char Str::Iter::operator *() const {
		return v();
	}

	Char Str::Iter::v() const {
		if (atEnd())
			return Char(Nat(0));

		wchar p = owner->data->v[pos];
		if (utf16::leading(p)) {
			return Char(utf16::assemble(p, owner->data->v[pos + 1]));
		} else {
			return Char(p);
		}
	}

	Bool Str::Iter::atEnd() const {
		return owner ? pos + 1 == owner->data->count : true;
	}

}
