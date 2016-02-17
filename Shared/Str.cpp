#include "stdafx.h"
#include "Str.h"
#include "Utf.h"

namespace storm {

	Str::Str() : Object() {}

	Str::Str(Par<Str> o) : Object(), v(o->v) {}

	Str::Str(const String &o) : Object(), v(o) {}

	Str::Str(Char ch) : Object(), v(toString(ch)) {}

	static String repeat(const String &str, Nat times) {
		String r(str.size() * times, ' ');
		nat dest = 0;
		for (nat i = 0; i < times; i++) {
			for (nat j = 0; j < str.size(); j++)
				r[dest++] = str[j];
		}

		return r;
	}

	Str::Str(Char ch, Nat times) : Object(), v(repeat(toString(ch), times)) {}

	Str::Str(const wchar *s) : Object(), v(s) {}

	String Str::toString(const Char &ch) {
		wchar buf[3];

		if (!utf16::valid(ch.value)) {
			buf[0] = '?';
			buf[1] = 0;
		} else if (utf16::split(ch.value)) {
			buf[0] = utf16::splitLeading(ch.value);
			buf[1] = utf16::splitTrailing(ch.value);
			buf[2] = 0;
		} else {
			buf[0] = wchar(ch.value);
			buf[1] = 0;
		}

		return String(buf);
	}

	Bool Str::empty() const {
		return v.empty();
	}

	Bool Str::any() const {
		return !v.empty();
	}

	Str *Str::operator +(Par<Str> o) {
		return CREATE(Str, this, v + o->v);
	}

	Str *Str::operator *(Nat times) {
		return CREATE(Str, this, repeat(v, times));
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

	Str::Iter Str::begin() {
		return Iter(this);
	}

	Str::Iter Str::end() {
		return Iter();
	}

	/**
	 * Iterator.
	 */

	Str::Iter::Iter() : index(0) {}

	Str::Iter::Iter(Par<Str> owner) : owner(owner), index(0) {}

	void Str::Iter::deepCopy(Par<CloneEnv> x) {
		// No need to do anything. Strings are immutable in Storm!
	}

	bool Str::Iter::atEnd() const {
		return !owner || index >= owner->v.size();
	}

	Str::Iter &Str::Iter::operator ++() {
		if (!atEnd()) {
			if (utf16::leading(owner->v[index])) {
				// Skip the trailing one as well!
				index += 2;
			} else {
				// Single skip.
				index++;
			}
		}

		return *this;
	}

	Str::Iter Str::Iter::operator ++(int) {
		Iter tmp(*this);
		++(*this);
		return tmp;
	}

	Bool Str::Iter::operator ==(const Iter &o) const {
		if (atEnd() == o.atEnd() && atEnd())
			return true;

		return owner.borrow() == o.owner.borrow() && index == o.index;
	}

	Bool Str::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	Char Str::Iter::operator *() const {
		if (atEnd())
			throw StrError(L"Trying to dereference an invalid iterator!");

		wchar now = owner->v[index];
		if (utf16::leading(now)) {
			wchar next = owner->v[index + 1];
			nat joined = utf16::assemble(now, next);
			return Char(joined);
		} else {
			return Char(now);
		}
	}

	Char Str::Iter::v() const {
		return **this;
	}

	Nat Str::Iter::charIndex() const {
		return index;
	}

	void Str::Iter::charIndex(Nat i) {
		index = i;
	}

	/**
	 * Utility functions.
	 */

	// Indentation...
	struct Indent {
		wchar ch;
		nat count;

		static const nat invalid = -1;
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

	static Indent min(const Indent &a, const Indent &b) {
		nat count = Indent::invalid;

		if (a.count == Indent::invalid)
			count = b.count;
		else if (b.count == Indent::invalid)
			count = a.count;
		else
			count = ::min(a.count, b.count);

		wchar ch = 0;
		if (a.ch == 0)
			ch = b.ch;
		else if (b.ch == 0)
			ch = a.ch;
		else if (a.ch == b.ch)
			ch = a.ch;

		Indent r = { ch, count };
		return r;
	}

	Str *removeIndent(Par<Str> str) {
		const String &src = str->v;

		// Examine the indentation of all lines...
		Indent remove = { 0, Indent::invalid };
		for (nat at = 0; at < src.size(); at = nextLine(src, at)) {
			if (!emptyLine(src, at))
				remove = min(remove, indentOf(src, at));
		}

		if (remove.count == Indent::invalid)
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
