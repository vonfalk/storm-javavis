#include "stdafx.h"
#include "Name.h"
#include "Lib/Str.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Type.h"
#include "Io/Url.h"
#include <limits>

namespace storm {

	static vector<Value> toVec(Par<Array<Value>> values) {
		vector<Value> r(values->count());
		for (nat i = 0; i < values->count(); i++)
			r[i] = values->at(i);
		return r;
	}

	/**
	 * Name part
	 */

	NamePart::NamePart(Par<Str> name) :
		name(name->v), params() {}

	NamePart::NamePart(Par<Str> name, Par<Array<Value>> values) :
		name(name->v), params(toVec(values)) {}

	NamePart::NamePart(const String &name) :
		name(name), params() {}

	NamePart::NamePart(const String &name, const vector<Value> &params) :
		name(name), params(params) {}

	void NamePart::output(wostream &to) const {
		to << name;
		if (!params.empty()) {
			to << L"(";
			join(to, params, L", ");
			to << L")";
		}
	}

	bool NamePart::operator ==(const NamePart &o) const {
		return name == o.name
			&& params == o.params;
	}

	Named *NamePart::choose(Par<NameOverloads> from) {
		vector<Named *> candidates;
		int worst = std::numeric_limits<int>::max();

		for (nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);
			int badness = matches(candidate);
			if (badness >= 0 && badness <= worst) {
				if (badness != worst)
					candidates.clear();
				candidates.push_back(candidate);
			}
		}

		if (candidates.size() == 0) {
			return null;
		} else if (candidates.size() == 1) {
			return capture(candidates[0]).ret();
		} else {
			std::wostringstream msg;
			msg << L"Multiple possible matches for: " << from << endl;
			for (nat i = 0; i < candidates.size(); i++) {
				msg << L"Could be: " << candidates[i]->identifier();
			}
			PVAR(msg.str());
			TODO(L"Require a SrcPos for NameParamList!");
			throw TypeError(SrcPos(), msg.str());
		}
	}

	int NamePart::matches(Named *candidate) const {
		const vector<Value> &c = candidate->params;
		if (c.size() != params.size())
			return -1;

		for (nat i = 0; i < c.size(); i++) {
			if (!c[i].matches(params[i], candidate->matchFlags))
				return -1;
		}

		TODO(L"Compute the actual distance!");
		return 0;
	}


	/**
	 * Name
	 */

	Name::Name() {}

	Name::Name(Par<NamePart> part) : parts(1, part) {}

	Name::Name(Par<Str> part) {
		Auto<NamePart> v = CREATE(NamePart, this, part);
		add(v);
	}

	Name::Name(const String &part) {
		add(part);
	}

	Name::Name(Par<const Name> o) : parts(o->parts) {}

	void Name::add(Par<NamePart> part) {
		parts.push_back(part);
	}

	void Name::add(const String &part) {
		parts.push_back(CREATE(NamePart, this, part));
	}

	void Name::add(const String &part, const vector<Value> &params) {
		parts.push_back(CREATE(NamePart, this, part, params));
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

	Name *Name::withParams(const vector<Value> &v) const {
		assert(!root());
		Name *r = parent();
		r->add(parts.back()->name, v);
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
		if (parts.size() != o.parts.size())
			return false;

		for (nat i = 0; i < parts.size(); i++)
			if (*parts[i] != *o.parts[i])
				return false;

		return true;
	}

	size_t Name::hash() const {
		// djb2 hash
		size_t r = 5381;
		for (nat i = 0; i < parts.size(); i++) {
			const String &s = parts[i]->name;
			for (nat j = 0; j < s.size(); j++)
				r = ((r << 5) + r) + s[j];

			// We are ignoring the actual values, since hash collisions will probably
			// be rare enough as it is. The current use of hashes is to keep track of
			// vastly different packages anyway.
			r = ((r << 5) + r) + parts[i]->params.size();
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
	 * parseTemplatename.
	 */

	static Name *parseName(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok);

	static NamePart *parseNamePart(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok) {
		String name = tok.next().token;
		vector<Value> params;

		if (tok.more() && tok.peek().token == L"<") {
			tok.next();

			while (tok.more() && tok.peek().token != L">") {
				Auto<Name> n = parseName(e, scope, pos, tok);
				if (Type *t = as<Type>(scope.find(n)))
					params.push_back(Value(t));
				else
					throw SyntaxError(pos, L"Unknown type: " + ::toS(n));

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

		return CREATE(NamePart, e, name, params);
	}

	static Name *parseName(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok) {
		Auto<Name> r = CREATE(Name, e);
		Auto<NamePart> part;

		while (tok.more()) {
			Auto<NamePart> part = parseNamePart(e, scope, pos, tok);
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

	Name *parseTemplateName(Engine &e, const Scope &scope, const SrcPos &pos, const String &src) {
		Auto<Url> path = CREATE(Url, e); // TODO: Null?
		Tokenizer tok(path, src, 0);
		return parseName(e, scope, pos, tok);
	}


}
