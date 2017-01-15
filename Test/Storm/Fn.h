#pragma once
#include "Compiler/Engine.h"
#include "OS/FnCall.h"

template <class Res>
inline Res runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	Value r(StormInfo<Res>::type(e));
	assert(r.canStore(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	return os::call<Res>(f->ref().address(), false);
}

template <>
inline void runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	assert(f->result == Value(), L"Invalid return type of " + ::toS(sName) + L"!");
	return os::call<void>(f->ref().address(), false);
}

template <class Res, class T>
inline Res runFn(const wchar *name, T t) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	sName->last()->params->push(Value(StormInfo<T>::type(e)));
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	Value r(StormInfo<Res>::type(e));
	assert(r.canStore(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	os::FnParams p;
	p.add(t);
	return os::call<Res>(f->ref().address(), false, p);
}
