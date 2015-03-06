#pragma once

#include "Storm/Function.h"

template <class T>
T runFn(const String &fn) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::call<T>(ptr);
}

template <class T>
T runFn(const String &fn, Int p) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, Value(intType(e))));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::call<T>(ptr, code::FnParams().add(p));
}

template <class T, class Par>
T runFn(const String &fn, const Par &par) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, Par::type(e)));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");

	return code::call<T>(ptr, code::FnParams().add(par));
}

inline Int runFn(const String &fn) {
	return runFn<Int>(fn);
}

inline Int runFn(const String &fn, Int p) {
	return runFn<Int>(fn, p);
}
