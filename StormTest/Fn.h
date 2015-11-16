#pragma once

#include "Storm/Function.h"

template <class T>
T runFn(const String &fn) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	Auto<Function> fun = steal(e.scope()->find(fName)).as<Function>();
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");
	return fun->call<T>();
}

template <class T, class ParT>
T runFn(const String &fn, const ParT &par) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, stormType<ParT>(e)));
	Auto<Function> fun = steal(e.scope()->find(fName)).as<Function>();
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
	params[0] = stormType<ParT>(e);
	params[1] = stormType<ParU>(e);
	fName = fName->withParams(params);
	Auto<Function> fun = steal(e.scope()->find(fName)).as<Function>();
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");

	return fun->call<T>(os::FnParams().add(par).add(qar));
}

template <class T, class ParT, class ParU, class ParV>
T runFn(const String &fn, const ParT &par, const ParU &qar, const ParV &rar) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	vector<Value> params(3);
	params[0] = stormType<ParT>(e);
	params[1] = stormType<ParU>(e);
	params[2] = stormType<ParV>(e);
	fName = fName->withParams(params);
	Auto<Function> fun = steal(e.scope()->find(fName)).as<Function>();
	if (!fun)
		throw TestError(L"Function " + ::toS(fName) + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + ::toS(fName) + L" did not return any code.");

	return fun->call<T>(os::FnParams().add(par).add(qar).add(rar));
}

inline Int runFnInt(const String &fn) {
	return runFn<Int>(fn);
}

template <class T>
inline Int runFnInt(const String &fn, T p) {
	return runFn<Int, typename AsPar<T>::v>(fn, p);
}

template <class T, class U>
inline Int runFnInt(const String &fn, T p, U q) {
	return runFn<Int, typename AsPar<T>::v, typename AsPar<U>::v>(fn, p, q);
}
