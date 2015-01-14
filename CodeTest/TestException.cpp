#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Redirect.h"
#include "Code/Function.h"

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
	redirect.param(Size::sInt, Ref(intCleanup));
	redirect.result(Size::sInt, true);
	Binary inner(arena, L"inner", redirect.code(Ref(throwEx), Value()));
	RefSource innerRef(arena, L"inner");
	inner.update(innerRef);

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

	Binary outer(arena, L"outer", l);

	throwError = true;
	destroyed = 0;
	CHECK_ERROR(FnCall().call<cpuInt>(outer.getData()));
	CHECK_EQ(destroyed, 6);

	throwError = false;
	destroyed = 0;
	CHECK_EQ(FnCall().call<cpuInt>(outer.getData()), 3);
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

	Binary output(arena, L"MyFn", l);

	typedef cpuInt (*Fn)(cpuInt, cpuInt);
	Fn fn = (Fn)output.getData();

	CHECK_EQ((*fn)(1, 2), 3);
} END_TEST
