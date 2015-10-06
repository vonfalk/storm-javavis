#include "stdafx.h"
#include "Test/Test.h"

static cpuInt copied = 0;

static void CODECALL copyInt(cpuInt *dest, cpuInt *src) {
	*dest = *src;
	copied = *src;
}

static cpuInt CODECALL intAdd(cpuInt v) {
	return v + 2;
}

BEGIN_TEST(TestCall) {
	Arena arena;

	Ref copy = arena.external(L"copyInt", &copyInt);
	Ref intFn = arena.external(L"copyInt", &::intAdd);

	Listing l;
	Variable v = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << mov(v, intConst(20));
	l << fnParam(v, copy);
	l << fnCall(intFn, retVal(Size::sInt, false));
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	// PVAR(l);
	Binary b(arena, l);
	const void *f = b.address();

	copied = 0;
	CHECK_EQ(call<cpuInt>(f, false), 22);
	CHECK_EQ(copied, 20);

} END_TEST

BEGIN_TEST(TestCallRef) {
	Arena arena;

	Ref copy = arena.external(L"copyInt", &copyInt);
	Ref intFn = arena.external(L"copyInt", &::intAdd);

	Listing l;
	Variable v = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << mov(v, intConst(20));
	l << lea(ptrA, v);
	l << fnParamRef(ptrA, copy);
	l << fnCall(intFn, retVal(Size::sInt, false));
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	// PVAR(l);
	Binary b(arena, l);
	const void *f = b.address();

	copied = 0;
	CHECK_EQ(call<cpuInt>(f, false), 22);
	CHECK_EQ(copied, 20);

} END_TEST
