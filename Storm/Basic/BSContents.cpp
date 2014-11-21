#include "stdafx.h"
#include "BSContents.h"

namespace storm {

	bs::Contents::Contents() {}

	void bs::Contents::add(Auto<Type> t) {
		types.push_back(t);
	}

	void bs::Contents::add(Auto<FunctionDecl> t) {
		functions.push_back(t);
	}

}
