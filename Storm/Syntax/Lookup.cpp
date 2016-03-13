#include "stdafx.h"
#include "Lookup.h"
#include "Type.h"
#include "Engine.h"

namespace storm {
	namespace syntax {

		SyntaxLookup::SyntaxLookup() : ScopeExtra(L"void") {
			lang = engine().package(L"core.lang");
			assert(lang, L"core.lang moved!");
		}

		Named *SyntaxLookup::find(const Scope &in, Par<SimpleName> name) {
			if (name->count() == 1) {
				SimplePart *last = name->last().borrow();

				// Resolve SStr properly always.
				if (last->name == L"SStr" && last->empty())
					return lang->find(last);


				// Expressions of the form foo(x, y, z) are equal to x.foo(y, z),
				// but only if name only contains one part (ie. not foo:bar(y, z)).
				if (last->any() && last->param(0) != Value()) {
					if (Named *r = last->param(0).type->find(last))
						return r;
				}
			}

			return ScopeExtra::find(in, name);
		}

	}
}
