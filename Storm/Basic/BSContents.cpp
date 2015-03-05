#include "stdafx.h"
#include "BSContents.h"
#include "BSClass.h"

namespace storm {

	bs::Contents::Contents() {}

	void bs::Contents::add(Par<Type> t) {
		types.push_back(t);
	}

	void bs::Contents::add(Par<FunctionDecl> t) {
		functions.push_back(t);
	}

	void bs::Contents::add(Par<NamedThread> t) {
		threads.push_back(t);
	}

	void bs::Contents::setScope(const Scope &scope) {
		for (nat i = 0; i < types.size(); i++) {
			if (bs::Class *c = as<bs::Class>(types[i].borrow())) {
				c->setScope(scope);
			}
		}
	}

}
