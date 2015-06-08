#pragma once

#include "Storm/Function.h"

template <class T>
T runFn(const String &fn) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");
	return fun->call<T>();
}

template <class T>
T runFn(const String &fn, Int p) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, Value(intType(e))));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");
	return fun->call<T>(os::FnParams().add(p));
}

template <class T, class ParT>
T runFn(const String &fn, const ParT &par) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, ParT::stormType(e)));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");

	return fun->call<T>(os::FnParams().add(par));
}

template <class T, class ParT, class ParU>
T runFn(const String &fn, const ParT &par, const ParU &qar) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	vector<Value> params(2);
	params[0] = ParT::stormType(e);
	params[1] = ParU::stormType(e);
	fName = fName->withParams(params);
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");

	return fun->call<T>(os::FnParams().add(par).add(qar));
}

inline Int runFn(const String &fn) {
	return runFn<Int>(fn);
}

inline Int runFn(const String &fn, Int p) {
	return runFn<Int>(fn, p);
}

template <class T>
inline Int runFnInt(const String &fn, T p) {
	return runFn<Int, typename AsPar<T>::v>(fn, p);
}

template <class T, class U>
inline Int runFnInt(const String &fn, T p, U q) {
	return runFn<Int, typename AsPar<T>::v, typename AsPar<U>::v>(fn, p, q);
}
