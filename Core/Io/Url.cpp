#include "stdafx.h"
#include "Url.h"
#include "Str.h"
#include "StrBuf.h"
#include "CloneEnv.h"
#include "Protocol.h"
#include "Serialization.h"
#include "SerializationUtils.h"
#include "Core/Convert.h"
#include "Core/Exception.h"

namespace storm {

	static inline bool isDot(const wchar *str) {
		return str[0] == '.' && str[1] == '\0';
	}

	static inline bool isParent(const wchar *str) {
		return str[0] == '.' && str[1] == '.' && str[2] == '\0';
	}

	static inline bool separator(wchar c) {
		return (c == '/') || (c == '\\');
	}

	// Make sure 'str' do not contain any forbidden characters, and is not empty.
	static void validate(Str *str) {
		if (str->empty())
			throw new (str) InvalidName();

		for (const wchar *s = str->c_str(); *s; s++) {
			// Now, we only disallow separators in parts.
			if (separator(*s))
				throw new (str) InvalidName(str);
		}
	}

	static void validate(Array<Str *> *data) {
		for (Nat i = 0; i < data->count(); i++)
			validate(data->at(i));
	}

	static Array<Str *> *simplify(Array<Str *> *parts) {
		Array<Str *> *result = new (parts) Array<Str *>();
		result->reserve(parts->count());

		for (nat i = 0; i < parts->count(); i++) {
			Str *p = parts->at(i);
			if (isDot(p->c_str())) {
				// Ignore it.
			} else if (isParent(p->c_str()) && result->any() && !isParent(result->last()->c_str())) {
				result->remove(result->count() - 1);
			} else {
				result->push(p);
			}
		}

		return result;
	}

	static void simplifyInplace(Array<Str *> *&parts) {
		for (nat i = 0; i < parts->count(); i++) {
			Str *p = parts->at(i);
			if (isDot(p->c_str()) || isParent(p->c_str())) {
				parts = simplify(parts);
				return;
			}
		}
	}

	Url::Url() : flags(nothing) {
		parts = new (this) Array<Str *>();
		protocol = new (this) RelativeProtocol();
	}

	Url::Url(Protocol *p, Array<Str *> *parts) : protocol(p), parts(parts), flags(nothing) {
		dbg_assert(p, L"Need a protocol!");
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Array<Str *> *parts) : protocol(new (engine()) RelativeProtocol()), parts(parts), flags(nothing) {
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Protocol *p, Array<Str *> *parts, UrlFlags flags) : protocol(p), parts(parts), flags(flags) {
		dbg_assert(p, L"Need a protocol!");
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	Url::Url(Array<Str *> *parts, UrlFlags flags) : protocol(new (engine()) RelativeProtocol()), parts(parts), flags(flags) {
		validate(this->parts);
		simplifyInplace(this->parts);
	}

	void Url::toS(StrBuf *to) const {
		*to << protocol;

		if (parts->count() > 0)
			*to << parts->at(0);

		for (nat i = 1; i < parts->count(); i++)
			*to << L"/" << parts->at(i);

		if (flags & isDir)
			*to << L"/";
	}

	Url::Url(ObjIStream *from) {
		// Read data here so that we can initialize Url later.
		protocol = (Protocol *)Protocol::read(from);
		parts = Serialize<Array<Str *> *>::read(from);
		flags = UrlFlags(Serialize<Nat>::read(from));
	}

	SerializedType *Url::serializedType(EnginePtr e) {
		SerializedStdType *t = serializedStdType<Url>(e.v);
		t->add(new (e.v) Str(S("protocol")), StormInfo<Protocol>::type(e.v));
		t->add(new (e.v) Str(S("parts")), StormInfo<Array<Str *>>::type(e.v));
		t->add(new (e.v) Str(S("flags")), StormInfo<Nat>::type(e.v));
		return t;
	}

	Url *Url::read(ObjIStream *from) {
		return (Url *)from->readClass(StormInfo<Url>::type(from->engine()));
	}

	void Url::write(ObjOStream *to) const {
		if (to->startClass(StormInfo<Url>::type(engine()), (Object *)this)) {
			protocol->write(to);
			Serialize<Array<Str *> *>::write(parts, to);
			Serialize<Nat>::write(Nat(flags), to);
			to->end();
		}
	}

	void Url::deepCopy(CloneEnv *e) {
		cloned(parts, e);
	}

	Bool Url::operator ==(const Url &o) const {
		if (!sameType(this, &o))
			return false;

		if (*protocol != *o.protocol)
			return false;

		if (parts->count() != o.parts->count())
			return false;

		for (Nat i = 0; i < parts->count(); i++) {
			if (!protocol->partEq(parts->at(i), o.parts->at(i)))
				return false;
		}

		return true;
	}

	Nat Url::hash() const {
		Nat r = 5381;
		for (nat i = 0; i < parts->count(); i++) {
			r = ((r << 5) + r) + protocol->partHash(parts->at(i));
		}
		return r;
	}

	Array<Str *> *Url::getParts() const {
		return new (this) Array<Str *>(*parts);
	}

	Url *Url::copy() const {
		Url *c = new (this) Url(*this);
		c->parts = new (this) Array<Str *>(*c->parts);
		return c;
	}

	Url *Url::push(Str *p) {
		validate(p);

		Url *c = copy();
		if (p->empty())
			return c;

		c->parts->push(p);
		simplifyInplace(c->parts);
		c->flags &= ~isDir;
		return c;
	}

	Url *Url::operator /(Str *p) {
		return push(p);
	}

	Url *Url::pushDir(Str *p) {
		validate(p);

		Url *c = copy();
		if (p->empty())
			return c;

		c->parts->push(p);
		simplifyInplace(c->parts);
		c->flags |= isDir;
		return c;
	}

	Url *Url::push(Url *url) {
		if (url->absolute())
			throw new (this) InvalidName(url->toS());

		Url *c = copy();
		for (nat i = 0; i < url->parts->count(); i++)
			c->parts->push(url->parts->at(i));

		simplifyInplace(c->parts);
		c->flags = (flags & ~isDir) | (url->flags & isDir);
		return c;
	}

	Url *Url::operator /(Url *p) {
		return push(p);
	}

	Url *Url::parent() const {
		Array<Str *> *p = new (this) Array<Str *>();

		for (nat i = 0; i < parts->count() - 1; i++)
			p->push(parts->at(i));

		return new (this) Url(protocol, p, flags | isDir);
	}

	Bool Url::dir() const {
		return (flags & isDir) != nothing;
	}

	Bool Url::absolute() const {
		return protocol->absolute();
	}

	Str *Url::name() const {
		if (parts->any())
			return parts->last();
		else
			return new (this) Str(L"");
	}

	static Str::Iter divideName(Str *str) {
		if (str->empty())
			return str->begin();

		Str::Iter last = str->end();
		for (Str::Iter at = str->begin(); at != str->end(); at++) {
			if (at.v() == Char(L'.'))
				last = at;
		}

		if (last == str->begin())
			last = str->end();
		return last;
	}

	Str *Url::ext() const {
		Str *n = name();
		Str::Iter d = divideName(n);
		d++;
		return n->substr(d);
	}

	Url *Url::withExt(Str *ext) const {
		Url *c = copy();
		if (parts->empty())
			return c;

		// Sorry about the derefs, the + operator is intended for use in Storm...
		c->parts->last() = *(*this->title() + S(".")) + ext;
		return c;
	}

	Str *Url::title() const {
		Str *n = name();
		Str::Iter d = divideName(n);
		return n->substr(n->begin(), d);
	}

	Url *Url::relative(Url *to) {
		if (!absolute() || !to->absolute())
			throw new (this) InvalidName(new (this) Str(S("Both paths to 'relative' must be absolute.")));

		// Different protocols, not possible...
		if (*protocol != *to->protocol)
			return this;

		Array<Str *> *rel = new (this) Array<Str *>();
		Str *up = new (this) Str(L"..");

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

		return new (this) Url(rel, flags);
	}

	/**
	 * Forward to the protocol.
	 */

	// Find all children URL:s.
	Array<Url *> *Url::children() {
		return protocol->children(this);
	}

	// Open this Url for reading.
	IStream *Url::read() {
		return protocol->read(this);
	}

	// Open this Url for writing.
	OStream *Url::write() {
		return protocol->write(this);
	}

	// Does this Url exist?
	Bool Url::exists() {
		return protocol->exists(this);
	}

	// Create a directory.
	Bool Url::createDir() {
		return protocol->createDir(this);
	}

	// Format.
	Str *Url::format() {
		return protocol->format(this);
	}


	/**
	 * Parsing
	 */


	Url *parsePath(Str *s) {
		return parsePath(s->engine(), s->c_str());
	}

	Url *parsePath(Engine &e, const wchar *src) {
		Array<Str *> *parts = new (e) Array<Str *>();
		Protocol *protocol = new (e) FileProtocol();
		UrlFlags flags = nothing;

		if (*src == 0)
			return new (e) Url(null, parts, flags);

		const wchar *start = src;
		// UNIX absolute path?
		if (separator(*start))
			start++;
		// Windows absulute path?
		else if (src[0] != 0 && src[1] == ':')
			start = src;
		// Windows network share?
		else if (separator(src[0]) && separator(src[1]))
			start = src + 2;
		// Relative path?
		else
			protocol = new (e) RelativeProtocol();

		if (*start == 0)
			return new (e) Url(protocol, parts, flags);

		const wchar *end = start;
		for (; end[1]; end++)
			;

		if (separator(*end)) {
			flags |= isDir;
			end--;
		}

		const wchar *last = start;
		for (const wchar *i = start; i < end; i++) {
			if (separator(*i)) {
				if (i > last) {
					parts->push(new (e) Str(last, i));
				}
				last = i + 1;
			}
		}
		if (end + 1 > last)
			parts->push(new (e) Str(last, end + 1));

		return new (e) Url(protocol, parts, flags);
	}

#if defined(WINDOWS)
	Url *cwdUrl(EnginePtr e) {
		wchar_t tmp[MAX_PATH + 1];
		tmp[0] = 0;
		GetCurrentDirectory(MAX_PATH + 1, tmp);
		return parsePath(e.v, tmp);
	}

	Url *executableFileUrl(Engine &e) {
		wchar_t tmp[MAX_PATH + 1];
		GetModuleFileName(NULL, tmp, MAX_PATH + 1);
		return parsePath(e, tmp);
	}
#elif defined(POSIX)
	Url *cwdUrl(EnginePtr e) {
		char path[PATH_MAX + 1] = { 0 };
		if (!getcwd(path, PATH_MAX))
			throw new (e.v) InternalError(S("Failed to get the current working directory."));
		return parsePath(e.v, toWChar(e.v, path)->v);
	}

	Url *executableFileUrl(Engine &e) {
		char path[PATH_MAX + 1] = { 0 };
		ssize_t r = readlink("/proc/self/exe", path, PATH_MAX);
		if (r >= PATH_MAX || r < 0)
			throw new (e) InternalError(S("Failed to get the path of the executable."));
		return parsePath(e, toWChar(e, path)->v);
	}
#else
#error "Please implement executableFileUrl and cwdUrl for your OS!"
#endif

	Url *executableUrl(Engine &e) {
		Url *v = executableFileUrl(e);
		return v->parent();
	}

	Url *dbgRootUrl(Engine &e) {
#ifndef DEBUG
		WARNING(L"Using dbgRoot in release!");
#endif
		Url *v = executableUrl(e);
		return v->parent();
	}

	Url *httpUrl(Str *host) {
		Protocol *p = new (host) HttpProtocol(false);
		return new (host) Url(p, new (host) Array<Str *>(1, host));
	}

	Url *httpsUrl(Str *host) {
		Protocol *p = new (host) HttpProtocol(true);
		return new (host) Url(p, new (host) Array<Str *>(1, host));
	}

}
