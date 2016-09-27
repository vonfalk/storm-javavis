#include "stdafx.h"

#include "Test/Lib/TestMgr.h"

// Long-running test of the push op-code.
void testPush(Arena &arena) {
	for (nat i = 0; i < 0xFFFFF; i++) {
		if ((i & 0xFF) == 0)
			PLN("Now at " << toHex(i));

		Listing l;
		l << push(natConst(i));
		l << pop(eax);
		l << ret(retVal(Size::sInt, false));

		Binary b(arena, l);

		typedef cpuInt (*fn)();
		fn p = (fn)b.address();
		nat r = (*p)();
		if (r != i)
			PLN("Failed at " << toHex(i) << ": " << toHex(r));
	}
}

void destroyInt(cpuInt v) {
	PLN("Destroying " << v);
}

class TestError : public Exception {
public:
	String what() const { return L"Test error"; }
};

static void throwException() {
	throw TestError();
}

void testFunction(Arena &arena) {
	Listing l;

	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref ex = arena.external(L"throwException", &throwException);

	Variable par = l.frame.createIntParam();

	Block myBlock = l.frame.createChild(l.frame.root());
	Variable result = l.frame.createVariable(myBlock, Size::sInt, intCleanup);

	l << prolog();
	l << begin(myBlock);
	l << mov(result, natConst(1));
	l << add(result, par);
	l << fnCall(ex, retVal(Size::sInt, false));
	l << mov(eax, result);
	l << epilog() << ret(retVal(Size::sInt, false));

	l << end(myBlock);
	l << epilog() << ret(retVal(Size::sInt, false));

	PLN("Before:" << l);

	Binary output(arena, l);

	typedef cpuInt (*fn)(cpuInt);
	fn p = (fn)output.address();
	try {
		cpuInt r = (*p)(10);
		PLN("Result: " << r);
	} catch (const Exception &e) {
		PLN("Got exception! " << e.what());
	}
}


int _tmain(int argc, _TCHAR* argv[]) {
	TODO(L"Test without SEH as well!");

	initDebug();

	TestResult r = Tests::run();

	// Arena arena;

	// testPush(arena);

	// testFunction(arena);

	return r.ok() ? 0 : 1;
}

