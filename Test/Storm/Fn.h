#pragma once
#include "Compiler/Engine.h"
#include "OS/FnCall.h"

template <class Res>
Res runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	Value r(StormInfo<Res>::type(e));
	assert(r.canStore(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	return os::call<Res>(f->ref().address(), false);
}

template <>
void runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	assert(f->result == Value(), L"Invalid return type of " + ::toS(sName) + L"!");
	return os::call<void>(f->ref().address(), false);
}
