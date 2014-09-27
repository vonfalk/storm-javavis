#include "stdafx.h"

#include "Test/TestMgr.h"
#include <afxwin.h>

void myFn() {
	PLN("In the new function!");
}

void paramFn(cpuInt a, cpuInt b) {
	PLN("Got parameters " << a << ", " << b);
}

void testReplace(Arena &arena) {
	Ref fun = arena.external(L"myFn", &myFn);
	Ref paramFun = arena.external(L"paramFn", &paramFn);

	Listing l2;
	l2 << call(fun, 0);
	l2 << ret(0);
	PLN(l2);

	Binary middle(arena, L"middle");
	middle.set(l2);

	Listing l;
	Label lbl = l.label();

	l << jmp(lbl);
	l << call(fun, 0);
	l << lbl;
	l << call(Ref(middle), 0);
	l << ret(0);
	PLN(l);

	Binary output(arena, L"Test");
	output.set(l);

	void (*p)();
	p = (void (*)())output.getData();
	(*p)();

	l2 = Listing();
	l2 << fnParam(natConst(128));
	l2 << fnParam(natConst(20));
	l2 << fnCall(paramFun, 0);
	l2 << ret(0);
	PLN(l2);
	middle.set(l2);

	p = (void (*)())output.getData();
	(*p)();
}

void testPush(Arena &arena) {
	Binary b(arena, L"TestFn");

	for (nat i = 0; i < 0xFFFFF; i++) {
		if ((i & 0xFF) == 0)
			PLN("Now at " << toHex(i));

		Listing l;
		l << push(natConst(i));
		l << pop(eax);
		l << ret(4);

		b.set(l);

		typedef cpuInt (*fn)();
		fn p = (fn)b.getData();
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

void throwException() {
	throw TestError();
}

void testFunction(Arena &arena) {
	Listing l;

	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref ex = arena.external(L"throwException", &throwException);

	Variable par = l.frame.createParameter(4, false);

	Block myBlock = l.frame.createChild(l.frame.root());
	Variable result = l.frame.createVariable(myBlock, 4, intCleanup);

	l << prolog();
	l << begin(myBlock);
	l << mov(result, natConst(1));
	l << add(result, par);
	l << fnCall(ex, 4);
	l << mov(eax, result);
	l << epilog() << ret(4);

	l << end(myBlock);
	l << epilog() << ret(4);

	PLN("Before:" << l);

	Binary output(arena, L"MyFunction");
	output.set(l);

	typedef cpuInt (*fn)(cpuInt);
	fn p = (fn)output.getData();
	try {
		cpuInt r = (*p)(10);
		PLN("Result: " << r);
	} catch (const Exception &e) {
		PLN("Got exception! " << e.what());
	}
}


int _tmain(int argc, _TCHAR* argv[]) {
	TODO(L"Test without SEH as well!");

	Tests::run();

	//testReplace(arena);

	//testPush(arena);

	//testFunction(arena);


	waitForReturn();
	return 0;
}

