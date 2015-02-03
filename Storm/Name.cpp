#include "stdafx.h"
#include "Name.h"
#include "Lib/Str.h"

namespace storm {

	NamePart::NamePart(Par<Str> name) : name(name->v) {}

	NamePart::NamePart(const String &name) : name(name) {}

	NamePart::NamePart(const String &name, const vector<Value> &params) : name(name), params(params) {}

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

	Name::Name() {}

	Name::Name(Par<NamePart> part) : parts(1, part) {}

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

	// Name &Name::operator +=(const Name &o) {
	// 	for (nat i = 0; i < o.parts.size(); i++)
	// 		parts.push_back(o.parts[i]);
	// 	return *this;
	// }

	// Name Name::operator +(const Name &o) const {
	// 	Name t(*this);
	// 	t += o;
	// 	return t;
	// }

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
		PLN("CHECKING EQUALITY");
		if (parts.size() != o.parts.size())
			return false;

		for (nat i = 0; i < parts.size(); i++)
			if (*parts[i] != *o.parts[i])
				return false;

		return true;
	}

	size_t Name::hash() const {
		PLN("COMPUTING HASH");
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

}
