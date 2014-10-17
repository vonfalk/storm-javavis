#include "stdafx.h"
#include "Type.h"

namespace storm {

	Type::Type(const String &name, TypeFlags f) : name(name), flags(f) {}

	Type::~Type() {
		clearMap(members);
	}

	nat Type::size() const {
		TODO(L"Implement me!");
		return 0;
	}

	void Type::setParentScope(Scope *parent) {
		nameFallback = parent;
	}

	Named *Type::findHere(const Name &name) {
		return null;
	}

	void Type::output(wostream &to) const {
		to << name << ":" << endl;
		Indent i(to);

		for (MemberMap::const_iterator i = members.begin(); i != members.end(); ++i) {
			to << *i->second;
		}
	}

	void Type::add(NameOverload *o) {
		Overload *ovl = null;
		MemberMap::iterator i = members.find(o->name);
		if (i == members.end()) {
			ovl = new Overload(o->name);
			members.insert(make_pair(o->name, ovl));
		} else {
			ovl = i->second;
		}

		ovl->add(o);
	}

	bool Type::operator <(const Type &o) const {
		// Ugly, but works for now.
		return this < &o;
	}

}
