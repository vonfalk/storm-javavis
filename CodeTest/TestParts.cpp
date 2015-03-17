#include "stdafx.h"
#include "Test/Test.h"

static void throwError(nat on, nat id) {
	if (id == on)
		throw InvalidValue(L"ERROR");
}

static int sum = 0;

static void destroyInt(int v) {
	sum += v;
}

BEGIN_TEST_(TestParts) {
	Arena arena;

	Ref error = arena.external(L"error", &throwError);
	Ref dtor = arena.external(L"dtor", &destroyInt);

	Listing l;
	Variable p = l.frame.createIntParam();

	Block root = l.frame.root();
	Block part2 = l.frame.createChild(root);
	Block part3 = l.frame.createChild(part2);
	Variable a = l.frame.createIntVar(root, dtor);
	Variable b = l.frame.createIntVar(part2, dtor);
	Variable c = l.frame.createIntVar(part3, dtor);

	l << prolog();
	l << mov(a, intConst(1));
	l << mov(b, intConst(2));
	l << mov(c, intConst(3));

	l << fnParam(p);
	l << fnParam(intConst(0));
	l << fnCall(error, Size());

	l << begin(part2);
	l << fnParam(p);
	l << fnParam(intConst(1));
	l << fnCall(error, Size());

	l << begin(part3);
	l << fnParam(p);
	l << fnParam(intConst(2));
	l << fnCall(error, Size());

	l << epilog();
	l << ret(Size());

	PVAR(l);

	Binary bin(arena, L"TestParts", l);

	int v = 0;
	FnParams params;
	params.add(v);

	v = 0;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 1);

	v = 1;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 3);

	v = 2;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 6);

	v = 3;
	sum = 0;
	CHECK_RUNS(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 6);

} END_TEST
