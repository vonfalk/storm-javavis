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

	Name *bs::TypeName::getName() {
		Name *r = CREATE(Name, this);
		if (pkg) {
			for (nat i = 0; i < pkg->parts.size(); i++)
				r->add(steal(CREATE(NamePart, this, pkg->parts[i])));
		}
		r->add(steal(CREATE(NamePart, this, name)));
		return r;
	}

	Value bs::TypeName::value(const Scope &scope) {
		Auto<Name> name = getName();
		if (name->size() == 1 && name->at(0)->name == L"void")
			return Value();

		Named *n = scope.find(name);
		if (Type *t = as<Type>(n)) {
			return Value(t);
		} else {
			throw SyntaxError(pos, L"No type " + ::toS(getName()) + L"!");
		}
	}
}
