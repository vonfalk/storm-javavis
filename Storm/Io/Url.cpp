#include "stdafx.h"
#include "Url.h"
#include "Lib/CloneEnv.h"
#include "Protocol.h"

namespace storm {

	static inline bool separator(wchar c) {
		return (c == '/') || (c == '\\');
	}

	// Make sure 'str' do not contain any forbidden characters.
	static void validate(const String &str) {
		for (nat i = 0; i < str.size(); i++) {
			// Now, we only disallow separators in parts.
			if (separator(str[i]))
				throw InvalidName(str);
		}
	}

	static ArrayP<Str> *simplify(Par<ArrayP<Str>> parts) {
		Auto<ArrayP<Str>> result = CREATE(ArrayP<Str>, parts);
		result->reserve(parts->count());

		for (nat i = 0; i < parts->count(); i++) {
			Auto<Str> &p = parts->at(i);
			if (p->v == L".") {
				// Ignore it.
			} else if (p->v == L".." && result->any() && result->last()->v != L"..") {
				result->erase(result->count() - 1);
			} else {
				result->push(p);
			}
		}

		return result.ret();
	}

	static void simplifyInplace(Auto<ArrayP<Str>> &parts) {
		for (nat i = 0; i < parts->count(); i++) {
			Auto<Str> &p = parts->at(i);
			if (p->v == L"." || p->v == L"..") {
				parts = simplify(parts);
				return;
			}
		}
	}

	Url::Url(Par<Protocol> p, Par<ArrayP<Str>> parts) : protocol(p), parts(parts), flags(nothing) {
		simplifyInplace(this->parts);
	}

	Url::Url(Par<Protocol> p, Par<ArrayP<Str>> parts, Flags flags) : protocol(p), parts(parts), flags(flags) {
		simplifyInplace(this->parts);
	}

	Url::Url(Par<Url> o) {
		protocol = o->protocol;
		parts = o->parts;
		flags = o->flags;
	}

	Url::~Url() {}

	void Url::output(wostream &to) const {
		if (as<FileProtocol>(protocol.borrow()))
			to << "/";
		else if (protocol)
			to << protocol << L"://";
		else
			to << L"./";

		if (parts->count() > 0)
			to << parts->at(0)->v;

		for (nat i = 1; i < parts->count(); i++)
			to << L"/" << parts->at(i)->v;

		if (flags & isDir)
			to << L"/";
	}

	void Url::deepCopy(Par<CloneEnv> e) {
		parts = e->clone(parts);
	}

	// TODO: How do we consider the protocol here? Should we?
	Bool Url::equals(Par<Object> o) {
		if (!Object::equals(o))
			return false;

		Url *u = (Url *)o.borrow();
		if (protocol) {
			if (!protocol->equals(u->protocol))
				return false;
		} else {
			if (u->protocol)
				return false;
		}

		if (parts->count() != u->parts->count())
			return false;

		for (nat i = 0; i < parts->count(); i++) {
			if (!parts->at(i)->equals(u->parts->at(i)))
				return false;
		}

		return true;
	}

	Url *Url::copy() {
		Url *c = COPY(Url, this);
		c->parts = COPY(ArrayP<Str>, c->parts);
		return c;
	}

	Url *Url::push(const String &p) {
		Auto<Str> z = CREATE(Str, engine(), p);
		return push(z);
	}

	Url *Url::push(Par<Str> p) {
		validate(p->v);

		Url *c = copy();
		if (p->count() == 0)
			return c;

		c->parts->push(p);
		simplifyInplace(c->parts);
		c->flags &= ~isDir;
		return c;
	}

	Url *Url::push(Par<Url> url) {
		if (url->absolute())
			throw InvalidName(::toS(url));

		Url *c = copy();
		for (nat i = 0; i < url->parts->count(); i++)
			c->parts->push(url->parts->at(i));

		simplifyInplace(c->parts);
		c->flags = (flags & ~isDir) | (url->flags & isDir);
		return c;
	}

	Url *Url::parent() const {
		Auto<ArrayP<Str>> p = CREATE(ArrayP<Str>, engine());

		for (nat i = 0; i < parts->count() - 1; i++)
			p->push(parts->at(i));

		return CREATE(Url, engine(), protocol, p, flags | isDir);
	}

	Bool Url::dir() const {
		return (flags & isDir) != nothing;
	}

	Bool Url::absolute() const {
		return protocol != null;
	}

	Str *Url::name() const {
		nat last = parts->count();
		if (last == 0)
			return CREATE(Str, this, L"");
		else
			return parts->at(last - 1).ret();
	}

	static nat divideName(const String &str) {
		// Note, we _want_ to ignore files starting with . They are simply hidden files on UNIX.
		for (nat i = str.size() - 1; i > 0; i--)
			if (str[i] == '.')
				return i;
		return str.size();
	}

	Str *Url::ext() const {
		Auto<Str> n = name();
		nat d = divideName(n->v) + 1;
		if (d >= n->v.size())
			return CREATE(Str, this, L"");
		else
			return CREATE(Str, this, n->v.substr(d));
	}

	Str *Url::title() const {
		Auto<Str> n = name();
		nat d = divideName(n->v);
		return CREATE(Str, this, n->v.substr(0, d));
	}

	Url *Url::relative(Par<Url> to) {
		if (!absolute() || !to->absolute())
			throw InvalidName(L"Both paths to 'relative' must be absolute.");

		// Different protocols, not possible...
		if (!protocol->equals(to->protocol))
			return capture(this).ret();

		Auto<ArrayP<Str>> rel = CREATE(ArrayP<Str>, this);
		Auto<Str> up = CREATE(Str, this, L"..");

		nat equalTo = 0; // Equal up to a position
		for (nat i = 0; i < to->parts->count(); i++) {
			if (equalTo == i) {
				if (i >= parts->count()) {
				} else if (protocol->partEq(to->parts->at(i), parts->at(i))) {
					equalTo = i + 1;
				}
			}

			if (equalTo <= i)
				rel->push(up);
		}

		for (nat i = equalTo; i < parts->count(); i++)
			rel->push(parts->at(i));

		return CREATE(Url, this, null, rel, flags);
	}


	/**
	 * Parsing
	 */


	Url *parsePath(Par<Str> s) {
		return parsePath(s->engine(), s->v);
	}

	Url *parsePath(Engine &e, const String &s) {
		Auto<ArrayP<Str>> parts = CREATE(ArrayP<Str>, e);
		Auto<Protocol> protocol = CREATE(FileProtocol, e);
		Url::Flags flags = Url::nothing;

		if (s.size() == 0)
			return CREATE(Url, e, null, parts, flags);

		nat start = 0;
		// UNIX absolute path?
		if (separator(s[0]))
			start = 1;
		// Windows absulute path?
		else if (s.size() >= 2 && s[1] == ':')
			start = 0;
		// Windows network share?
		else if (s.size() >= 2 && separator(s[0]) && separator(s[1]))
			start = 2;
		// Relative path?
		else
			protocol = null;

		nat end = s.size() - 1;
		if (start > end)
			return CREATE(Url, e, protocol, parts, flags);
		if (separator(s[end])) {
			flags |= Url::isDir;
			end--;
		}

		nat last = start;
		for (nat i = start; i <= end; i++) {
			if (separator(s[i])) {
				if (i > last) {
					Str *p = CREATE(Str, e, s.substr(last, i - last));
					parts->push(p);
				}
				last = i + 1;
			}
		}

		if (end + 1 > last)
			parts->push(CREATE(Str, e, s.substr(last, end - last + 1)));

		return CREATE(Url, e, protocol, parts, flags);
	}

}
