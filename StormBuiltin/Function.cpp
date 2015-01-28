#include "stdafx.h"
#include "Function.h"
#include "Tokenizer.h"
#include "Exception.h"

void Function::output(wostream &to) const {
	to << result << L" " << name << L"(";
	join(to, params, L", ");
	to << L"), " << cppScope;
}

Function Function::read(const String &package, const CppScope &scope, const CppType &result, Tokenizer &tok) {
	Function r;
	r.result = result;
	r.cppScope = scope;
	r.name = tok.next();
	r.isConst = false;
	r.package = package;

	tok.expect(L"(");
	while (tok.peek() != L")") {
		r.params.push_back(CppType::read(tok));
		// Who need names anyway?
		tok.next();

		if (tok.peek() == L",")
			tok.next();
		else
			break;
	}

	tok.expect(L")");

	if (tok.peek() == L"const") {
		r.isConst = true;
		tok.next();
	}

	return r;
}

Function Function::dtor(const String &package, const CppScope &scope) {
	Function r;
	r.result = CppType::tVoid();
	r.cppScope = scope;
	r.name = L"__dtor";
	r.isConst = false;
	r.package = package;
	return r;
}
