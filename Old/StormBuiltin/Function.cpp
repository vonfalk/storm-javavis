#include "stdafx.h"
#include "Function.h"
#include "Tokenizer.h"
#include "Exception.h"

void Function::output(wostream &to) const {
	to << result << L" " << name << L"(";
	join(to, params, L", ");
	to << L"), " << cppScope;
}

Function Function::read(FnFlags flags, const String &package,
						const CppScope &scope, const CppType &result, Tokenizer &tok) {
	Function r;
	r.result = result;
	r.cppScope = scope;
	r.name = tok.next();
	r.flags = flags;
	r.package = package;

	if (r.name == L"operator") {
		r.name = L"operator ";
		while (tok.peek() != L"(") {
			r.name += tok.next();
		}
	}

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
		r.flags |= fnConst;
		tok.next();
	}

	if (tok.peek() == L"ON") {
		tok.next();
		tok.expect(L"(");
		r.thread = CppName::read(tok);
		tok.expect(L")");
	}

	return r;
}

Function Function::dtor(const String &package, const CppScope &scope, bool external) {
	Function r;
	r.result = CppType::tVoid();
	r.cppScope = scope;
	r.name = L"__dtor";
	r.flags = fnVirtual;
	if (external)
		r.flags |= fnExternal;
	r.package = package;
	return r;
}

Function Function::copyCtor(const String &package, const CppScope &scope, bool external) {
	Function r;
	r.result = CppType::tVoid();
	r.cppScope = scope;
	r.name = L"__ctor";
	r.flags = fnNone;
	if (external)
		r.flags |= fnExternal;
	r.package = package;

	CppType z;
	z.isRef = true;
	z.isConst = true;
	z.type = scope.cppName();
	r.params.push_back(z);

	return r;
}

Function Function::assignment(const String &package, const CppScope &scope, bool external) {
	Function r;
	r.result = CppType::tVoid();
	r.cppScope = scope;
	r.name = L"operator =";
	r.flags = fnNone;
	if (external)
		r.flags |= fnExternal;
	r.package = package;

	CppType z;
	z.isRef = true;
	z.isConst = true;
	z.type = scope.cppName();
	r.params.push_back(z);

	r.result = z;
	r.result.isConst = false;

	return r;
}
