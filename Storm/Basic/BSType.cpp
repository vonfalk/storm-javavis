#include "stdafx.h"
#include "BSType.h"
#include "Exception.h"

namespace storm {

	bs::PkgName::PkgName() {}

	void bs::PkgName::add(Par<SStr> name) {
		parts.push_back(name->v);
	}

	void bs::PkgName::output(wostream &to) const {
		join(to, parts, L", ");
	}

	bs::TypeName::TypeName(Par<SStr> name) : name(name->v) {}
	bs::TypeName::TypeName(Par<PkgName> pkg, Par<SStr> name) : pkg(pkg), name(name->v) {}

	void bs::TypeName::output(wostream &to) const {
		to << pkg << L"." << name;
	}

	Name bs::TypeName::getName() {
		vector<String> p;
		if (pkg) {
			for (nat i = 0; i < pkg->parts.size(); i++)
				p.push_back(pkg->parts[i]->v);
		}
		p.push_back(name->v);
		return Name(p);
	}

	Value bs::TypeName::value(const Scope &scope) {
		Name name = getName();
		if (name.size() == 1 && name[0] == L"void")
			return Value();

		Named *n = scope.find(name);
		if (Type *t = as<Type>(n)) {
			return Value(t);
		} else {
			throw SyntaxError(pos, L"No type " + ::toS(getName()) + L"!");
		}
	}
}
