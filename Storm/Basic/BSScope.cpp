#include "stdafx.h"
#include "BSScope.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "PkgReader.h"

namespace storm {

	bs::BSScope::BSScope() {}

	bs::BSScope::BSScope(Par<Url> file) : file(file) {}

	Named *bs::BSScope::findHelper(const Scope &s, Par<Name> name) {
		if (Named *found = ScopeLookup::find(s, name))
			return found;

		for (nat i = 0; i < includes.size(); i++) {
			if (Named *found = storm::find(includes[i], name))
				return found;
		}

		return null;
	}

	Named *bs::BSScope::find(const Scope &s, Par<Name> name) {
		if (Named *n = findHelper(s, name))
			return n;

		// Expressions of the form foo(x, y, z) are equal to x.foo(y, z).
		if (name->size() > 1)
			return null;

		Auto<NamePart> last = name->last();
		if (last->params.size() == 0)
			return null;

		const Value &v = last->params[0];
		if (v == Value())
			return null;

		return v.type->find(last);
	}

	void bs::BSScope::addSyntax(const Scope &from, Par<SyntaxSet> to) {
		if (file) {
			Auto<Name> syntax = syntaxPkg(file);
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
			return null;
		}
	}

}
