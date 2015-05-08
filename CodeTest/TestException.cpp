#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Redirect.h"

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

BEGIN_TEST(TestException) {
	Arena arena;
	Listing l;

	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref ex = arena.external(L"throwException", &throwException);

	Variable par = l.frame.createIntParam(intCleanup);

	Block myBlock = l.frame.createChild(l.frame.root());
	Variable result = l.frame.createVariable(myBlock, Size::sInt, intCleanup);

	l << prolog();
	l << begin(myBlock);
	l << mov(result, par);
	l << add(result, intConst(4));
	l << fnCall(ex, Size::sInt);
	l << add(result, eax);
	l << mov(eax, result);
	l << epilog();
	l << ret(Size::sInt);

	Binary output(arena, l);

	typedef cpuInt (*fn)(cpuInt);
	fn p = (fn)output.address();

	destroyed = 0;
	throwError = true;
	CHECK_ERROR((*p)(10), Error);
	CHECK_EQ(destroyed, 24);

	destroyed = 0;
	throwError = false;
	CHECK_EQ((*p)(10), 18);
	CHECK_EQ(destroyed, 28);

} END_TEST

static cpuInt redirectTo() {
	return 3;
}

static void *redirectFn(cpuInt v) {
	if (throwError)
		throw Error();
	return &redirectTo;
}

BEGIN_TEST(TestChainedException) {
	Arena arena;
	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref throwEx = arena.external(L"throwException", &redirectFn);

	// Innermost function is a redirect.
	Redirect redirect;
	redirect.param(Size::sInt, Ref(intCleanup), false);
	redirect.result(Size::sInt, true);
	Binary *inner = new Binary(arena, redirect.code(Ref(throwEx), false, Value()));
	RefSource innerRef(arena, L"inner");
	innerRef.set(inner);

	// Outermost function is a regular function.
	Listing l;
	Variable result = l.frame.createIntVar(l.frame.root(), Ref(intCleanup));
	Variable r2 = l.frame.createIntVar(l.frame.root(), Ref(intCleanup), freeOnBlockExit);

	l << prolog();
	l << mov(result, intConst(3));
	l << mov(r2, intConst(2));
	l << fnParam(result);
	l << fnCall(Ref(innerRef), Size::sInt);
	l << epilog();
	l << ret(Size::sInt);

	Binary outer(arena, l);

	throwError = true;
	destroyed = 0;
	CHECK_ERROR(call<cpuInt>(outer.address(), false), Error);
	CHECK_EQ(destroyed, 6);

	throwError = false;
	destroyed = 0;
	CHECK_EQ(call<cpuInt>(outer.address(), false), 3);
	CHECK_EQ(destroyed, 5);
} END_TEST

BEGIN_TEST(TestNoException) {
	Arena arena;
	Listing l;

	Variable par1 = l.frame.createIntParam();
	Variable par2 = l.frame.createIntParam();

	Variable var1 = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << mov(var1, par1);
	l << add(var1, par2);
	l << mov(eax, var1);
	l << epilog();
	l << ret(Size::sInt);

	Binary output(arena, l);

	typedef cpuInt (*Fn)(cpuInt, cpuInt);
	Fn fn = (Fn)output.address();

	CHECK_EQ((*fn)(1, 2), 3);
} END_TEST

static void destroyPtr(cpuInt *v) {
	destroyed = *v;
}

BEGIN_TEST(TestRefException) {
	Arena arena;
	Listing l;

	Ref intCleanup = arena.external(L"destroyPtr", &destroyPtr);
	Ref throwEx = arena.external(L"throwException", &throwException);

	Variable v = l.frame.createIntVar(l.frame.root(), Ref(intCleanup), freeOnBoth | freePtr);

	l << prolog();
	l << mov(v, intConst(103));
	l << fnCall(Ref(throwEx), Size::sInt);
	l << mov(v, eax);
	l << epilog();
	l << ret(Size::sInt);

	Binary output(arena, l);

	const void *fn = output.address();

	destroyed = 0;
	throwError = true;
	CHECK_ERROR(call<cpuInt>(fn, false), Error);
	CHECK_EQ(destroyed, 103);

	destroyed = 0;
	throwError = false;
	CHECK_EQ(call<cpuInt>(fn, false), 4);
	CHECK_EQ(destroyed, 4);

} END_TEST


  /**
   * For some reason, throwing an exception in a generated function right after we have returned
   * from the same (probably another generated as well), causes a crash. Probably we are not properly
   * cleaning up the SEH stuff on the stack, but not so badly so that C++ can not recover it later.
   */
BEGIN_TEST(TestMultipleEx) {
	Arena arena;
	Listing l;

	Ref intCleanup = arena.external(L"destroyInt", &destroyInt);
	Ref throwEx = arena.external(L"throwException", &throwException);

	Variable v = l.frame.createIntVar(l.frame.root(), Ref(intCleanup));

	l << prolog();
	l << mov(v, intConst(103));
	l << fnCall(Ref(throwEx), Size::sInt);
	l << epilog();
	l << ret(Size());

	Binary output(arena, l);
	typedef void (*FnPtr)();
	FnPtr fn = (FnPtr)output.address();

#if defined(X86) && defined(SEH)
	void *sehTop, *after;

	__asm {
		mov eax, fs:[0];
		mov sehTop, eax;
	};
#endif

	throwError = false;
	CHECK_RUNS((*fn)());

#if defined(X86) && defined(SEH)
	__asm {
		mov eax, fs:[0];
		mov after, eax;
	};

	// Something broke this!
	CHECK_EQ(after, sehTop);
#endif

	// Crashes...
	throwError = true;
	CHECK_ERROR((*fn)(), Error);

} END_TEST

struct Large {
	int a;
	int b;
	int c;

	Large() : a(0), b(10), c(0) {}
};

static bool correct = false;

static void destroyLarge(Large *large) {
	// PVAR(large->a);
	// PVAR(large->b);
	// PVAR(large->c);
	correct &= large->a == 0;
	correct &= large->b == 10;
	correct &= large->c == 0;
}

BEGIN_TEST(TestDtor) {
	Arena arena;
	Ref dtor = arena.external(L"dtor", &destroyLarge);
	Ref error = arena.external(L"error", &throwException);

	Listing l;
	Variable param = l.frame.createParameter(Size::sInt * 3, false, dtor, freeOnBoth | freePtr);
	Variable var = l.frame.createVariable(l.frame.root(), Size::sInt * 3, dtor, freeOnBoth | freePtr);

	l << prolog();

	l << mov(eax, intRel(param, Offset()));
	l << mov(intRel(var, Offset()), eax);
	l << mov(eax, intRel(param, Offset::sInt));
	l << mov(intRel(var, Offset::sInt), eax);
	l << mov(eax, intRel(param, Offset::sInt * 2));
	l << mov(intRel(var, Offset::sInt * 2), eax);

	l << fnCall(error, Size());

	l << epilog();
	l << ret(Size());

	Binary output(arena, l);
	typedef void (*Fn)(Large);
	Fn fn = (Fn)output.address();
	Large obj;

	correct = true;
	throwError = true;
	CHECK_ERROR((*fn)(obj), Error);
	CHECK(correct);

	correct = true;
	throwError = false;
	CHECK_RUNS((*fn)(obj));
	CHECK(correct);
} END_TEST
