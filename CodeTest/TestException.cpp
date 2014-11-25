#include "stdafx.h"
#include "Test/Test.h"

static int destroyed = 0;

static void destroyInt(cpuInt v) {
	destroyed += v;
}

class Error : public Exception {
public:
	String what() const { return L"Test error"; }
};

static bool throwError = false;

static int throwException() {
	if (throwError)
		throw Error();
	return 4;
}

BEGIN_TEST_(TestException) {
	Arena arena;
	Listing l;

	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref ex = arena.external(L"throwException", &throwException);

	Variable par = l.frame.createParameter(4, false, intCleanup);

	Block myBlock = l.frame.createChild(l.frame.root());
	Variable result = l.frame.createVariable(myBlock, 4, intCleanup);

	l << prolog();
	l << begin(myBlock);
	l << mov(result, par);
	l << add(result, intConst(4));
	l << fnCall(ex, 4);
	l << add(result, eax);
	l << mov(eax, result);
	l << epilog();
	l << ret(4);

	Binary output(arena, L"MyFunction", l);

	typedef cpuInt (*fn)(cpuInt);
	fn p = (fn)output.getData();

	destroyed = 0;
	throwError = true;
	CHECK_ERROR((*p)(10));
	CHECK_EQ(destroyed, 24);

	destroyed = 0;
	throwError = false;
	CHECK_EQ((*p)(10), 18);
	CHECK_EQ(destroyed, 28);

} END_TEST

