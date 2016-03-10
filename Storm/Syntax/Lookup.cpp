#include "stdafx.h"
#include "Lookup.h"
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
				if (last->name == L"SStr" && last->empty())
					return lang->find(last);
			}

			return ScopeExtra::find(in, name);
		}

	}
}
