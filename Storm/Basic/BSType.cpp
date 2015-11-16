#include "stdafx.h"
#include "BSType.h"
#include "Exception.h"
#include "Scope.h"
#include "Type.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		bs::TypePart::TypePart(Par<SStr> name) : name(name->v) {
			pos = name->pos;
		}

		bs::TypePart::TypePart(Par<Str> name) : name(name) {}

		bs::TypePart::TypePart(Par<NamePart> src) {
			name = CREATE(Str, this, src->name);
			for (nat i = 0; i < src->params.size(); i++) {
				params.push_back(CREATE(TypeName, this, src->params[i]));
			}
		}

		Str *bs::TypePart::title() const {
			return name.ret();
		}

		Nat bs::TypePart::count() const {
			return params.size();
		}

		TypeName *bs::TypePart::operator [](Nat id) const {
			return params[id].ret();
		}

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

		bs::TypeName::TypeName(Value v) {
			Type *from = v.type;
			if (from == null) {
				Auto<Str> s = CREATE(Str, this, L"void");
				parts.push_back(CREATE(TypePart, this, s));
			} else {
				Auto<Name> p = from->path();
				for (nat i = 0; i < p->size(); i++) {
					parts.push_back(CREATE(TypePart, this, p->at(i)));
				}
			}
		}

		void bs::TypeName::add(Par<TypePart> part) {
			parts.push_back(part);
		}

		Nat bs::TypeName::count() const {
			return parts.size();
		}

		TypePart *bs::TypeName::operator [](Nat i) const {
			return parts[i].ret();
		}

		void bs::TypeName::output(wostream &to) const {
			join(to, parts, L".");
		}

		Name *bs::TypeName::toName(const Scope &scope) {
			Auto<Name> n = CREATE(Name, this);

			for (nat i = 0; i < parts.size(); i++)
				n->add(steal(parts[i]->toPart(scope)));

			return n.ret();
		}

		Named *bs::TypeName::find(const Scope &scope) {
			Auto<Name> name = toName(scope);
			return scope.find(name);
		}

		Value bs::TypeName::resolve(const Scope &scope) {
			Auto<Name> name = toName(scope);
			if (name->size() == 1 && name->at(0)->name == L"void")
				return Value();

			if (Auto<Type> found = steal(scope.find(name)).as<Type>())
				return Value(found);

			throw SyntaxError(pos, L"Can not find the type " + ::toS(name) + L".");
		}

	}
}
