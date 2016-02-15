#include "stdafx.h"
#include "Rule.h"
#include "TypeCtor.h"
#include "TypeDtor.h"

namespace storm {
	namespace syntax {

		Rule::Rule(Par<RuleDecl> decl, Scope scope) :
			Type(decl->name->v, typeClass),
			pos(decl->pos),
			scope(scope) {}

		bool Rule::loadAll() {

			// Add these last.
			add(steal(CREATE(TypeDefaultCtor, this, this)));
			add(steal(CREATE(TypeCopyCtor, this, this)));
			add(steal(CREATE(TypeDeepCopy, this, this)));
			add(steal(CREATE(TypeDefaultDtor, this, this)));

			// Load our functions!
			return Type::loadAll();
		}

	}
}
