#include "stdafx.h"
#include "Variable.h"

Variable Variable::read(bool external, const CppScope &scope, Tokenizer &tok) {
	Variable r;
	r.cppScope = scope;
	r.type = CppType::read(tok);
	r.name = tok.next();
	r.external = external;
	tok.expect(L";");
	return r;
}
