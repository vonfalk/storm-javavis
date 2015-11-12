#include "stdafx.h"
#include "Str.h"

namespace storm {

	Str::Str() : Object() {}

	Str::Str(Par<Str> o) : Object(), v(o->v) {}

	Str::Str(const String &o) : Object(), v(o) {}

	Str::Str(const wchar *s) : Object(), v(s) {}

	nat Str::count() const {
		return v.size();
	}

	Str *Str::operator +(Par<Str> o) {
		return CREATE(Str, this, v + o->v);
	}

	Str *Str::operator *(Nat times) {
		String result(v.size() * times, ' ');
		for (nat i = 0; i < times; i++) {
			for (nat j = 0; j < v.size(); j++) {
				result[i*v.size() + j] = v[j];
			}
		}
		return CREATE(Str, this, result);
	}

	Bool Str::equals(Par<Object> o) {
		if (!Object::equals(o))
			return false;
		Str *other = (Str *)o.borrow();
		return v == other->v;
	}

	Nat Str::hash() {
		// djb2 hash
		size_t r = 5381;
		for (nat j = 0; j < v.size(); j++)
			r = ((r << 5) + r) + v[j];

		return r;
	}

	Int Str::toInt() const {
		return v.toInt();
	}

	Nat Str::toNat() const {
		return v.toNat();
	}

	Long Str::toLong() const {
		return v.toInt64();
	}

	Word Str::toWord() const {
		return v.toNat64();
	}

	Float Str::toFloat() const {
		return (Float)v.toDouble();
	}

	void Str::output(wostream &to) const {
		to << v;
	}

	Str *Str::toS() {
		addRef();
		return this;
	}

	Str *Str::createStr(Type *type, const wchar *str) {
		return new (type) Str(str);
	}


	// Indentation...
	struct Indent {
		wchar ch;
		nat count;
	};

	static Indent indentOf(const String &str, nat start) {
		Indent r = { 0, 0 };

		if (str[start] != ' ' && str[start] != '\t')
			return r;

		r.ch = str[start];
		for (r.count = 0; str[start + r.count] == r.ch; r.count++)
			;

		return r;
	}

	static nat nextLine(const String &str, nat start) {
		for (; start < str.size() && str[start] != '\n'; start++)
			;

		if (start < str.size())
			start++;

		if (start < str.size() && str[start] == '\r')
			start++;

		return start;
	}

	static bool emptyLine(const String &str, nat start) {
		nat end = nextLine(str, start);
		for (nat i = start; i < end; i++) {
			switch (str[i]) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				break;
			default:
				return false;
			}
		}

		return true;
	}

	static Indent min(Indent a, Indent b) {
		if (a.ch == 0)
			return b;
		if (b.ch == 0)
			return a;

		if (a.ch != b.ch) {
			a.count = 0;
			return a;
		}

		a.count = ::min(a.count, b.count);
		return a;
	}

	Str *removeIndent(Par<Str> str) {
		const String &src = str->v;

		// Examine the indentation of all lines...
		Indent remove = { 0, 0 };
		for (nat at = 0; at < src.size(); at = nextLine(src, at)) {
			if (!emptyLine(src, at))
				remove = min(remove, indentOf(src, at));
		}

		if (remove.count == 0 || remove.ch == 0)
			return str.ret();

		// Now we have some kind of indentation.
		std::wostringstream to;

		nat at = 0;
		nat end = 0;
		while (at < src.size()) {
			end = nextLine(src, at);

			if (emptyLine(src, at)) {
				to << src.substr(at, end - at);
			} else {
				at += remove.count;
				to << src.substr(at, end - at);
			}

			at = end;
		}

		return CREATE(Str, str, to.str());
	}

	Str *trimBlankLines(Par<Str> str) {
		const String &src = str->v;

		nat start = 0;
		nat end = 0;
		nat at = 0;

		for (nat at = 0; at < src.size(); at = nextLine(src, at)) {
			if (!emptyLine(src, at)) {
				end = start = at;
				break;
			}
		}

		for (nat at = start; at < src.size(); at = nextLine(src, at)) {
			if (!emptyLine(src, at)) {
				end = at;
			}
		}

		end = nextLine(src, end);
		while (end > 0) {
			wchar ch = src[end-1];
			if (ch == '\n' || ch == '\r')
				end--;
			else
				break;
		}

		return CREATE(Str, str, src.substr(start, end - start));
	}

	static void indent(wostream &to, const String &src, const wchar* prepend) {
		nat at = 0;
		while (at < src.size()) {
			nat next = nextLine(src, at);
			to << prepend;
			for (nat i = at; i < next; i++)
				to << src[i];
			at = next;
		}
	}

	Str *indent(Par<Str> str) {
		std::wostringstream o;
		indent(o, str->v, L"    ");
		return CREATE(Str, str, o.str());
	}

	Str *indent(Par<Str> str, Par<Str> prepend) {
		std::wostringstream o;
		indent(o, str->v, prepend->v.c_str());
		return CREATE(Str, str, o.str());
	}
}
