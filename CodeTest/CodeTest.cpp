#include "stdafx.h"

#include "Test/TestMgr.h"

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
	l2 << call(fun, Size::sPtr);
	l2 << ret(Size::sPtr);
	PLN(l2);

	Binary middleBlob(arena, L"middle", l2);
	RefSource middle(arena, L"middle");
	middleBlob.update(middle);

	Listing l;
	Label lbl = l.label();

	l << jmp(lbl);
	l << call(fun, Size::sPtr);
	l << lbl;
	l << call(Ref(middle), Size::sPtr);
	l << ret(Size::sPtr);
	PLN(l);

	Binary output(arena, L"Test", l);

	void (*p)();
	p = (void (*)())output.getData();
	(*p)();

	l2 = Listing();
	l2 << fnParam(natConst(128));
	l2 << fnParam(natConst(20));
	l2 << fnCall(paramFun, Size::sPtr);
	l2 << ret(Size::sPtr);
	PLN(l2);
	Binary middle2(arena, L"middle", l2);
	middle2.update(middle);

	p = (void (*)())output.getData();
	(*p)();
}

void testPush(Arena &arena) {
	for (nat i = 0; i < 0xFFFFF; i++) {
		if ((i & 0xFF) == 0)
			PLN("Now at " << toHex(i));

		Listing l;
		l << push(natConst(i));
		l << pop(eax);
		l << ret(Size::sInt);

		Binary b(arena, L"TestFn", l);

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
	l << fnCall(ex, Size::sInt);
	l << mov(eax, result);
	l << epilog() << ret(Size::sInt);

	l << end(myBlock);
	l << epilog() << ret(Size::sInt);

	PLN("Before:" << l);

	Binary output(arena, L"MyFunction", l);

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

	initDebug();

	Tests::run();

	// Arena arena;
	// testReplace(arena);

	// testPush(arena);

	// testFunction(arena);

	return 0;
}

