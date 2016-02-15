#include "stdafx.h"
#include "Option.h"
#include "Rule.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		Option::Option(Par<Str> name, Par<OptionDecl> decl, Scope scope) :
			Type(name->v, typeClass),
			pos(pos),
			scope(scope) {

			Auto<Named> found = scope.find(decl->rule);
			if (!found)
				throw SyntaxError(pos, L"The rule " + ::toS(decl->rule) + L" was not found!");
			Auto<Rule> r = found.as<Rule>();
			if (!r)
				throw SyntaxError(pos, ::toS(decl->rule) + L" is not a rule.");
			setSuper(r);
		}

		bool Option::loadAll() {
			// Load our functions!

			// Add these last.
			add(steal(CREATE(TypeDefaultCtor, this, this)));
			add(steal(CREATE(TypeCopyCtor, this, this)));
			add(steal(CREATE(TypeDeepCopy, this, this)));
			add(steal(CREATE(TypeDefaultDtor, this, this)));

			return Type::loadAll();
		}

	}
}
