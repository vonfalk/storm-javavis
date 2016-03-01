#include "stdafx.h"
#include "Name.h"
#include "Shared/Str.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Type.h"
#include "Shared/Io/Url.h"
#include "Utils/PreArray.h"
#include <limits>

namespace storm {

	static vector<Value> toVec(Par<Array<Value>> values) {
		vector<Value> r(values->count());
		for (nat i = 0; i < values->count(); i++)
			r[i] = values->at(i);
		return r;
	}

	static vector<Auto<Name>> toVec(Par<ArrayP<Name>> values) {
		vector<Auto<Name>> r(values->count());
		for (nat i = 0; i < values->count(); i++)
			r[i] = values->at(i);
		return r;
	}


	/**
	 * Name part.
	 */

	NamePart::NamePart(Par<NamePart> o) : name(o->name) {}

	NamePart::NamePart(Par<Str> name) : name(name->v) {}

	NamePart::NamePart(const String &name) : name(name) {}

	Nat NamePart::count() {
		return 0;
	}

	FoundParams *NamePart::find(const Scope &scope) {
		return CREATE(FoundParams, this, name);
	}

	void NamePart::deepCopy(Par<CloneEnv> env) {
		// Nothing needed here.
	}

	void NamePart::output(wostream &to) const {
		to << name;
	}

	NamePart *namePart(Par<Str> name) {
		return CREATE(FoundParams, name, name);
	}

	NamePart *namePart(Par<Str> name, Par<Array<Value>> params) {
		return CREATE(FoundParams, name, name, params);
	}

	NamePart *namePart(Par<Str> name, Par<ArrayP<Name>> params) {
		return CREATE(NameParams, name, name, params);
	}

	Str *name(Par<NamePart> part) {
		return CREATE(Str, part, part->name);
	}

	/**
	 * FoundParams.
	 */

	FoundParams::FoundParams(Par<FoundParams> o) : NamePart(o), params(o->params) {}

	FoundParams::FoundParams(Par<Str> name) : NamePart(name) {}

	FoundParams::FoundParams(Par<Str> name, Par<Array<Value>> values) : NamePart(name), params(toVec(values)) {}

	FoundParams::FoundParams(const String &name) : NamePart(name) {}

	FoundParams::FoundParams(const String &name, const vector<Value> &values) : NamePart(name), params(values) {}

	Nat FoundParams::count() {
		return params.size();
	}

	Value FoundParams::operator[](Nat id) {
		return params[id];
	}

	Named *FoundParams::choose(Par<NameOverloads> from) {
		PreArray<Named *, 4> candidates;
		int best = std::numeric_limits<int>::max();

		for (nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);
			int badness = matches(candidate);
			if (badness >= 0 && badness <= best) {
				if (badness != best)
					candidates.clear();
				best = badness;
				candidates.push(candidate);
			}
		}

		if (candidates.count() == 0) {
			return null;
		} else if (candidates.count() == 1) {
			return capture(candidates[0]).ret();
		} else {
			std::wostringstream msg;
			msg << L"Multiple possible matches for " << *this << L", all with badness " << best << endl;
			for (nat i = 0; i < candidates.count(); i++) {
				msg << L"Could be: " << candidates[i]->identifier() << endl;
			}
			TODO(L"Require a SrcPos for NameParamList!");
			throw TypeError(SrcPos(), msg.str());
		}
	}

	Int FoundParams::matches(Par<Named> candidate) {
		const vector<Value> &c = candidate->params;
		if (c.size() != params.size())
			return -1;

		int distance = 0;

		for (nat i = 0; i < c.size(); i++) {
			if (!c[i].matches(params[i], candidate->flags))
				return -1;
			if (params[i].type)
				distance += params[i].type->distanceFrom(c[i].type);
		}

		return distance;
	}

	FoundParams *FoundParams::find(const Scope &scope) {
		addRef();
		return this;
	}

	void FoundParams::deepCopy(Par<CloneEnv> env) {
		// Nothing needed here.
	}

	void FoundParams::output(wostream &to) const {
		to << name;
		if (!params.empty()) {
			to << L"(";
			join(to, params, L", ");
			to << L")";
		}
	}

	/**
	 * NameParams.
	 */

	NameParams::NameParams(Par<NameParams> o) : NamePart(o), params(o->params) {}

	NameParams::NameParams(Par<Str> name) : NamePart(name) {}

	NameParams::NameParams(Par<Str> name, Par<ArrayP<Name>> values) : NamePart(name), params(toVec(values)) {}

	NameParams::NameParams(const String &name) : NamePart(name) {}

	NameParams::NameParams(const String &name, vector<Auto<Name>> values) : NamePart(name), params(values) {}

	Nat NameParams::count() {
		return params.size();
	}

	FoundParams *NameParams::find(const Scope &scope) {
		vector<Value> v(params.size());
		for (nat i = 0; i < params.size(); i++) {
			Auto<Named> found = scope.find(params[i]);
			if (Type *t = as<Type>(found.borrow())) {
				v[i] = Value(t);
			} else {
				// Failed!
				return null;
			}
		}

		return CREATE(FoundParams, this, name, v);
	}

	void NameParams::deepCopy(Par<CloneEnv> env) {
		for (nat i = 0; i < params.size(); i++)
			params[i].deepCopy(env);
	}

	void NameParams::output(wostream &to) const {
		to << name;
		if (!params.empty()) {
			to << L"(";
			join(to, params, L", ");
			to << L")";
		}
	}

	/**
	 * Name
	 */

	Name::Name() {}

	Name::Name(Par<NamePart> part) : parts(1, part) {}

	Name::Name(Par<Str> part) {
		Auto<NamePart> v = CREATE(FoundParams, this, part);
		add(v);
	}

	Name::Name(Par<Str> name, Par<Array<Value>> values) {
		Auto<NamePart> v = CREATE(FoundParams, this, name, values);
		add(v);
	}

	Name::Name(const String &part, const vector<Value> &params) {
		add(part, params);
	}

	Name::Name(Par<const Name> o) : parts(o->parts) {}

	void Name::deepCopy(Par<CloneEnv> o) {
		for (nat i = 0; i < parts.size(); i++)
			parts[i].deepCopy(o);
	}

	void Name::add(Par<NamePart> part) {
		parts.push_back(part);
	}

	void Name::add(const String &part) {
		parts.push_back(CREATE(FoundParams, this, part));
	}

	void Name::add(const String &part, const vector<Value> &params) {
		parts.push_back(CREATE(FoundParams, this, part, params));
	}

	void Name::add(const String &part, const vector<Auto<Name>> &params) {
		parts.push_back(CREATE(NameParams, this, part, params));
	}

	Name *Name::parent() const {
		Name *n = CREATE(Name, this, this);
		if (!n->root())
			n->parts.pop_back();
		return n;
	}

	NamePart *Name::last() const {
		if (root())
			return null;
		else
			return parts.back().ret();
	}

	const String &Name::lastName() const {
		assert(!root());
		return parts.back()->name;
	}

	Name *Name::withParams(const vector<Value> &v) const {
		assert(!root());
		Name *r = parent();
		r->add(parts.back()->name, v);
		return r;
	}

	Name *Name::withLast(Par<NamePart> last) const {
		assert(!root());
		Name *r = parent();
		r->add(last);
		return r;
	}

	Name *Name::from(Nat f) const {
		Name *r = CREATE(Name, this);
		for (nat i = f; f < parts.size(); f++)
			r->parts.push_back(parts[i]);
		return r;
	}

	void Name::output(std::wostream &to) const {
		join(to, parts, L".");
	}

	bool Name::operator ==(const Name &o) const {
		assert(false, L"TODO!");

		if (parts.size() != o.parts.size())
			return false;

		// for (nat i = 0; i < parts.size(); i++)
		// 	if (*parts[i] != *o.parts[i])
		// 		return false;

		return true;
	}

	Nat Name::hash() {
		// djb2 hash
		size_t r = 5381;
		for (nat i = 0; i < parts.size(); i++) {
			const String &s = parts[i]->name;
			for (nat j = 0; j < s.size(); j++)
				r = ((r << 5) + r) + s[j];

			// We are ignoring the actual values, since hash collisions will probably
			// be rare enough as it is. The current use of hashes is to keep track of
			// vastly different packages anyway.
			r = ((r << 5) + r) + parts[i]->count();
		}

		return r;
	}

	Name *parseSimpleName(Engine &e, const String &name) {
		Auto<Name> r = CREATE(Name, e);

		vector<String> parts = name.split(L".");
		for (nat i = 0; i < parts.size(); i++)
			r->add(parts[i]);

		return r.ret();
	}


	/**
	 * parseTemplateName.
	 */

	static Name *parseName(Engine &e, const SrcPos &pos, Tokenizer &tok);

	static NamePart *parseNamePart(Engine &e, const SrcPos &pos, Tokenizer &tok) {
		String name = tok.next().token;
		vector<Auto<Name>> params;

		if (tok.more() && tok.peek().token == L"<") {
			tok.next();

			while (tok.more() && tok.peek().token != L">") {
				Auto<Name> n = parseName(e, pos, tok);
				params.push_back(n);

				if (!tok.more())
					throw SyntaxError(pos, L"Unbalanced <>");
				Token t = tok.peek();
				if (t.token == L">")
					break;
				if (t.token != L",")
					throw SyntaxError(pos, L"Expected , got" + t.token);
			}

			tok.next();
		}

		return CREATE(NameParams, e, name, params);
	}

	static Name *parseName(Engine &e, const SrcPos &pos, Tokenizer &tok) {
		Auto<Name> r = CREATE(Name, e);
		Auto<NamePart> part;

		while (tok.more()) {
			Auto<NamePart> part = parseNamePart(e, pos, tok);
			r->add(part);

			if (!tok.more())
				break;

			Token t = tok.peek();
			if (t.token == L">")
				break;
			if (t.token == L",")
				break;
			if (t.token != L".")
				throw SyntaxError(pos, L"Expected . got " + t.token);
			tok.next();
		}

		return r.ret();
	}

	Name *parseTemplateName(Engine &e, const SrcPos &pos, const String &src) {
		Auto<Url> path = CREATE(Url, e); // TODO: Null?
		Tokenizer tok(path, src, 0);
		return parseName(e, pos, tok);
	}


}
