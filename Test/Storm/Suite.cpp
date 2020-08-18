#include "stdafx.h"
#include "Compiler/Package.h"

// Run any test suites found in tests/suites
BEGIN_TEST(StormSuites, BS) {
	Engine &e = gEngine();

	// Package to run suites inside.
	Package *pkg = e.package(S("tests.suites"));

	SimpleName *fnName = parseSimpleName(e, S("test.runTests"));
	fnName->last()->params->push(Value(StormInfo<Package>::type(e)));
	fnName->last()->params->push(Value(StormInfo<Bool>::type(e)));
	fnName->last()->params->push(Value(StormInfo<Bool>::type(e)));
	Function *f = as<Function>(e.scope().find(fnName));
	assert(f, L"Function " + ::toS(fnName) + L" for running tests not found!");
	assert(f->result.isClass(), L"Result for " + ::toS(fnName) + L" must be a class-type.");

	Type *resultType = f->result.type;
	bool verbose = false;
	bool recurse = true;
	os::FnCall<RootObject *> c = os::fnCall().add(pkg).add(verbose).add(recurse);
	RootObject *resultVal = c.call(f->ref().address(), false);

	// Inspect the object to find relevant members...
	TestResult our;
	for (NameSet::Iter i = resultType->begin(), end = resultType->end(); i != end; ++i) {
		MemberVar *var = as<MemberVar>(i.v());
		if (!var)
			continue;

		if (*var->name == S("total")) {
			our.total = OFFSET_IN(resultVal, var->rawOffset().current(), Nat);
		} else if (*var->name == S("failed")) {
			our.failed = OFFSET_IN(resultVal, var->rawOffset().current(), Nat);
		} else if (*var->name == S("crashed")) {
			our.crashed = OFFSET_IN(resultVal, var->rawOffset().current(), Nat);
		} else if (*var->name == S("aborted")) {
			our.aborted = OFFSET_IN(resultVal, var->rawOffset().current(), Bool);
		}
	}

	__result__ += our;

} END_TEST
