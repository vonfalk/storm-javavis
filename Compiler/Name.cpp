#include "stdafx.h"
#include "Name.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace storm {

	Name::Name() : parts(new (engine()) Array<NamePart *>()) {}

	Name::Name(NamePart *v) : parts(new (engine()) Array<NamePart *>(v)) {}

	Name::Name(Str *name) : parts(new (engine()) Array<NamePart *>()) {
		add(name);
	}

	Name::Name(Str *name, Array<Value> *p) : parts(new (engine()) Array<NamePart *>()) {
		add(name, p);
	}

	Name::Name(Str *name, Array<Name> *p) : parts(new (engine()) Array<NamePart *>()) {
		add(name, p);
	}

	Name::Name(SimpleName *name) : parts(new (engine()) Array<NamePart *>()) {
		for (nat i = 0; i < name->count(); i++)
			add(name->at(i));
	}

	Name::Name(const Name &o) : parts(new (engine()) Array<NamePart *>(*o.parts)) {}

	void Name::deepCopy(CloneEnv *env) {
		Object::deepCopy(env);
		cloned(parts, env);
	}

	void Name::add(NamePart *v) {
		parts->push(v);
	}

	void Name::add(Str *name) {
		parts->push(new (this) SimplePart(name));
	}

	void Name::add(Str *name, Array<Value> *params) {
		parts->push(new (this) SimplePart(name, params));
	}

	void Name::add(Str *name, Array<Name> *params) {
		parts->push(new (this) RecPart(name));
	}

	Name *Name::parent() const {
		Name *n = new (this) Name();
		for (Nat i = 0; i + 1 < parts->count(); i++)
			n->add(parts->at(i));
		return n;
	}

	MAYBE(SimpleName *) Name::simplify(const Scope &scope) {
		SimpleName *result = new (this) SimpleName();

		for (nat i = 0; i < parts->count(); i++) {
			SimplePart *p = parts->at(i)->find(scope);
			if (!p)
				return null;
			result->add(p);
		}

		return result;
	}

	void Name::toS(StrBuf *to) const {
		if (parts->empty()) {
			*to << L"<root>";
		} else {
			*to << parts->at(0);
			for (nat i = 1; i < parts->count(); i++)
				*to << L"." << parts->at(i);
		}
	}

	/**
	 * SrcName.
	 */

	SrcName::SrcName() : Name() {}

	SrcName::SrcName(SrcPos pos) : Name(), pos(pos) {}

	SrcName::SrcName(Name *o, SrcPos pos) : Name(*o), pos(pos) {}

	SrcName::SrcName(SimpleName *o, SrcPos pos) : Name(o), pos(pos) {}

	void SrcName::deepCopy(CloneEnv *env) {
		Name::deepCopy(env);
		cloned(pos, env);
	}

	/**
	 * SimpleName.
	 */

	SimpleName::SimpleName() : parts(new (engine()) Array<SimplePart *>()) {}

	SimpleName::SimpleName(SimplePart *v) : parts(new (engine()) Array<SimplePart *>(v)) {}

	SimpleName::SimpleName(Str *name) : parts(new (engine()) Array<SimplePart *>()) {
		add(name);
	}

	SimpleName::SimpleName(Str *name, Array<Value> *p) : parts(new (engine()) Array<SimplePart *>()) {
		add(name, p);
	}

	SimpleName::SimpleName(const SimpleName &o) : parts(new (engine()) Array<SimplePart *>(*o.parts)) {}

	void SimpleName::deepCopy(CloneEnv *env) {
		Object::deepCopy(env);
		cloned(parts, env);
	}

	void SimpleName::add(SimplePart *v) {
		parts->push(v);
	}

	void SimpleName::add(Str *name) {
		parts->push(new (this) SimplePart(name));
	}

	void SimpleName::add(Str *name, Array<Value> *params) {
		parts->push(new (this) SimplePart(name, params));
	}

	SimpleName *SimpleName::parent() const {
		SimpleName *n = new (this) SimpleName();
		for (nat i = 1; i < parts->count(); i++)
			n->add(parts->at(i));
		return n;
	}

	SimpleName *SimpleName::from(Nat id) const {
		SimpleName *n = new (this) SimpleName();
		for (nat i = id; i < parts->count(); i++)
			n->add(parts->at(i));
		return n;
	}

	void SimpleName::toS(StrBuf *to) const {
		if (parts->empty()) {
			*to << L"<root>";
		} else {
			*to << parts->at(0);
			for (nat i = 1; i < parts->count(); i++)
				*to << L"." << parts->at(i);
		}
	}

	Bool SimpleName::operator ==(const SimpleName &o) const {
		if (!sameType(this, &o))
			return false;

		if (count() != o.count())
			return false;

		for (Nat i = 0; i < count(); i++)
			if (*at(i) != *o.at(i))
				return false;

		return true;
	}

	Nat SimpleName::hash() const {
		// inspired from djb2
		Nat r = 5381;
		for (nat i = 0; i < parts->count(); i++) {
			r = ((r << 5) + r) + parts->at(i)->hash();
		}
		return r;
	}


	/**
	 * Parsing.
	 */

	SimpleName *parseSimpleName(Str *str) {
		return parseSimpleName(str->engine(), str->c_str());
	}

	SimpleName *parseSimpleName(Engine &e, const wchar *str) {
		SimpleName *r = new (e) SimpleName();

		const wchar *last = str;
		const wchar *at;
		for (at = str; *at; at++) {
			if (*at == '.') {
				if (last < at)
					r->add(new (e) Str(last, at));
				last = at + 1;
			}
		}

		if (last < at)
			r->add(new (e) Str(last, at));

		return r;
	}

	static Str *trimStr(Engine &e, const wchar *start, const wchar *end) {
		while (start < end && *start == ' ')
			start++;

		while (start < end && end[-1] == ' ')
			end--;

		return new (e) Str(start, end);
	}

	static bool atEnd(wchar ch) {
		switch (ch) {
		case '\0':
		case ')':
		case ',':
			return true;
		default:
			return false;
		}
	}

	static MAYBE(Name *) parseComplexName(Engine &e, const wchar *&at) {
		Name *r = new (e) Name();

		const wchar *first = at;
		while (!atEnd(*at)) {
			if (*at == '.') {
				if (first < at)
					r->add(trimStr(e, first, at));
				at++;
				first = at;
			} else if (*at == '(') {
				RecPart *part = new (e) RecPart(trimStr(e, first, at));

				while (*at != ')') {
					if (*at == '\0')
						return null;
					at++;
					if (*at == ' ')
						continue;

					Name *p = parseComplexName(e, at);
					if (!p)
						return null;

					part->params->push(p);
				}
				r->add(part);

				// Find . or end of string.
				at++;
				while (!atEnd(*at)) {
					if (*at == '.') {
						at++;
						break;
					}
					if (*at != ' ')
						return false;
					at++;
				}

				first = at;
			} else {
				at++;
			}
		}

		if (first < at) {
			r->add(trimStr(e, first, at));
		} else if (first == at && at[-1] == '.') {
			r->add(new (e) Str(S("")));
		}

		return r;
	}

	MAYBE(Name *) parseComplexName(Str *str) {
		const wchar *at = str->c_str();
		Name *result = parseComplexName(str->engine(), at);

		if (*at != '\0')
			return null;
		return result;
	}

}
