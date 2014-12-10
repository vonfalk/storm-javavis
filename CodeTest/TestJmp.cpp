#include "stdafx.h"
#include "Test/Test.h"

static int sumVar = 0;

static void printVal(cpuInt i) {
	sumVar += i;
}

BEGIN_TEST(TestJmp) {
	Arena arena;

	Ref pv = arena.external(L"printVal", &printVal);

	Listing l;
	Variable v = l.frame.createIntVar(l.frame.root());
	Label done = l.label();
	Label begin = l.label();

	l << prolog();
	l << mov(v, intConst(10));
	l << begin;
	l << cmp(v, intConst(0));
	l << jmp(done, ifEqual);
	l << fnParam(v);
	l << fnCall(pv, 0);
	l << sub(v, intConst(1));
	l << jmp(begin);
	l << done;
	l << epilog();
	l << ret(0);

	sumVar = 0;

	Binary b(arena, L"TestJmp", l);
	typedef void (*Fn)();
	Fn f = (Fn)b.getData();
	(*f)();

	CHECK_EQ(sumVar, 55);

} END_TEST


BEGIN_TEST(TestSetCond) {
	Arena arena;

	Ref pv = arena.external(L"printVal", &printVal);

	Listing l;
	Variable v = l.frame.createIntVar(l.frame.root());
	Variable b = l.frame.createByteVar(l.frame.root());
	Label done = l.label();
	Label begin = l.label();

	l << prolog();
	l << mov(v, intConst(10));
	l << begin;
	l << cmp(v, intConst(0));
	l << setCond(al, ifEqual);
	l << cmp(al, byteConst(0));
	l << jmp(done, ifNotEqual);
	l << fnParam(v);
	l << fnCall(pv, 0);
	l << sub(v, intConst(1));
	l << jmp(begin);
	l << done;
	l << epilog();
	l << ret(0);

	sumVar = 0;

	Binary bin(arena, L"TestSetCond", l);
	typedef void (*Fn)();
	Fn f = (Fn)bin.getData();
	(*f)();

	CHECK_EQ(sumVar, 55);

} END_TEST
