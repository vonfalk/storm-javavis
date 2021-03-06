#include "stdafx.h"
#include "Lookup.h"
#include "Package.h"

namespace storm {
	namespace bs {

		BSLookup::BSLookup() : ScopeLookup(S("void")) {
			includes = new (this) Array<Package *>();
		}

		Named *BSLookup::findHelper(Scope scope, SimpleName *name) {
			if (Named *found = ScopeLookup::find(scope, name))
				return found;

			for (nat i = 0; i < includes->count(); i++) {
				if (Named *found = storm::find(scope, includes->at(i), name))
					return found;
			}

			return null;
		}

		Named *BSLookup::find(Scope s, SimpleName *name) {
			// Expressions of the form foo(x, y, z) are equal to x.foo(y, z),
			// but only if name only contains one part (ie. not foo:bar(y, z)).
			if (name->count() == 1) {
				SimplePart *last = name->last();
				if (last->params->any() && last->params->at(0) != Value()) {
					Type *firstParam = last->params->at(0).type;
					if (Named *r = firstParam->find(last, s))
						return r;

					// TODO: Also look in the parent scope of the last type? This will fix issues
					// with 2 / 0.2 etc.
				}
			}

			return findHelper(s, name);
		}

		void BSLookup::addSyntax(Scope from, syntax::ParserBase *to) {
			// Current package.
			to->addSyntax(firstPkg(from.top));

			for (nat i = 0; i < includes->count(); i++)
				to->addSyntax(includes->at(i));
		}

		Bool addInclude(Scope to, Package *pkg) {
			if (BSLookup *s = as<BSLookup>(to.lookup)) {
				for (nat i = 0; i < s->includes->count(); i++)
					if (s->includes->at(i) == pkg)
						return false;
				s->includes->push(pkg);
				return true;
			} else {
				WARNING(L"This is not what you want to do!");
				return false;
			}
		}

		void addSyntax(Scope scope, syntax::ParserBase *to) {
			if (BSLookup *s = as<BSLookup>(scope.lookup)) {
				s->addSyntax(scope, to);
			} else {
				WARNING(L"This is probably not what you want to do!");
			}
		}

	}
}
