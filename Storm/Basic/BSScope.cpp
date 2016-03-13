#include "stdafx.h"
#include "BSScope.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "PkgReader.h"
#include "Scope.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	bs::BSScope::BSScope() : ScopeLookup(L"void") {}

	bs::BSScope::BSScope(Par<Url> file) : ScopeLookup(L"void"), file(file) {}

	Named *bs::BSScope::findHelper(const Scope &s, Par<SimpleName> name) {
		if (Named *found = ScopeLookup::find(s, name))
			return found;

		for (nat i = 0; i < includes.size(); i++) {
			if (Named *found = storm::find(includes[i], name))
				return found;
		}

		return null;
	}

	Named *bs::BSScope::find(const Scope &s, Par<SimpleName> name) {
		// Expressions of the form foo(x, y, z) are equal to x.foo(y, z),
		// but only if name only contains one part (ie. not foo:bar(y, z)).
		if (name->count() == 1) {
			SimplePart *last = name->last().borrow();
			if (last->any() && last->param(0) != Value()) {
				if (Named *r = last->param(0).type->find(last))
					return r;
			}
		}


		return findHelper(s, name);
	}

	void bs::BSScope::addSyntax(const Scope &from, Par<SyntaxSet> to) {
		if (file) {
			Auto<SimpleName> syntax = syntaxPkg(file);
			to->add(engine().package(syntax));
		}

		// Current package.
		to->add(firstPkg(from.top));

		for (nat i = 0; i < includes.size(); i++)
			to->add(includes[i]);
	}

	Bool bs::addInclude(const Scope &to, Par<Package> pkg) {
		if (Auto<BSScope> s = to.lookup.as<BSScope>()) {
			for (nat i = 0; i < s->includes.size(); i++)
				if (s->includes[i] == pkg.borrow())
					return false;
			s->includes.push_back(pkg.borrow());
			return true;
		} else {
			WARNING(L"This is not what you want to do!");
			return false;
		}
	}

	SyntaxSet *bs::getSyntax(const Scope &scope) {
		if (Auto<BSScope> s = scope.lookup.as<BSScope>()) {
			Auto<SyntaxSet> to = CREATE(SyntaxSet, s);
			s->addSyntax(scope, to);
			return to.ret();
		} else {
			WARNING(L"This is probably not what you want to do!");
			Object *o = scope.top;
			if (!o)
				o = scope.lookup.borrow();
			assert(o, L"Nothing useful in the scope.");
			return CREATE(SyntaxSet, o);
		}
	}

}
