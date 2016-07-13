#include "stdafx.h"
#include "Str.h"
#include "Engine.h"

namespace storm {

	static const GcType bufType = {
		GcType::tArray,
		null,
		sizeof(wchar),
		0,
		{},
	};

	static GcArray<wchar> empty = {
		1,
	};

	Str::Str() : data(&storm::empty) {}

	Str::Str(const wchar *s) {
		nat count = wcslen(s);
		allocData(count + 1);
		for (nat i = 0; i < count; i++)
			data->v[i] = s[i];
		data->v[count] = 0;
	}

	Str::Str(GcArray<wchar> *data) : data(data) {}

	// The data is immutable, no need to copy it!
	Str::Str(Str *o) : data(o->data) {}

	Bool Str::empty() const {
		return data->count == 1;
	}

	Bool Str::any() const {
		return !empty();
	}

	Str *Str::operator +(Str *o) const {
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
		TODO(L"Implement me! (we need as<>)");
		return false;
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

	wchar *Str::c_str() const {
		return data->v;
	}

	void Str::allocData(nat count) {
		data = engine().gc.allocArray<wchar>(&bufType, count);
	}

}
