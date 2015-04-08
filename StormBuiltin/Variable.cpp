#include "stdafx.h"
#include "Variable.h"

Variable Variable::read(const CppScope &scope, Tokenizer &tok) {
	Variable r;
	r.cppScope = scope;
	r.type = CppType::read(tok);
	r.name = tok.next();
	tok.expect(L";");
	return r;
}
