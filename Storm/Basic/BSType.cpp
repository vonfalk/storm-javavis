#include "stdafx.h"
#include "BSType.h"
#include "Exception.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		bs::TypePart::TypePart(Par<SStr> name) : name(name->v) {}

		void bs::TypePart::add(Par<TypeName> param) {
			params.push_back(param);
		}

		NamePart *bs::TypePart::toPart(const Scope &scope) {
			vector<Value> v;
			v.reserve(params.size());

			for (nat i = 0; i < params.size(); i++)
				v.push_back(params[i]->resolve(scope));

			return CREATE(NamePart, this, name->v, v);
		}

		void bs::TypePart::output(wostream &to) const {
			to << name;
			if (params.empty())
				return;

			to << L"<";
			join(to, params, L", ");
			to << L">";
		}

		bs::TypeName::TypeName() {}

		void bs::TypeName::add(Par<TypePart> part) {
			parts.push_back(part);
		}

		void bs::TypeName::output(wostream &to) const {
			join(to, parts, L", ");
		}

		Name *bs::TypeName::toName(const Scope &scope) {
			Auto<Name> n = CREATE(Name, this);

			for (nat i = 0; i < parts.size(); i++)
				n->add(steal(parts[i]->toPart(scope)));

			return n.ret();
		}

		Value bs::TypeName::resolve(const Scope &scope) {
			Auto<Name> name = toName(scope);
			if (name->size() == 1 && name->at(0)->name == L"void")
				return Value();

			if (Type *found = as<Type>(scope.find(name)))
				return Value(found);

			throw SyntaxError(pos, L"Can not find the type " + ::toS(name) + L".");
		}

	}
}
