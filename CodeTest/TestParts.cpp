#include "stdafx.h"
#include "Test/Test.h"

static int throwError(nat on, nat id) {
	if (id == on)
		throw InvalidValue(L"ERROR");
	if (id + 100 == on)
		return 1;
	return 0;
}

static int sum = 0;

static void destroyInt(int v) {
	sum += v;
}

BEGIN_TEST(TestParts) {
	Arena arena;

	Ref error = arena.external(L"error", &throwError);
	Ref dtor = arena.external(L"dtor", &destroyInt);

	Listing l;
	Variable p = l.frame.createIntParam();

	Block root = l.frame.root();
	Part part1 = root;
	Part part2 = l.frame.createPart(part1);
	Part part3 = l.frame.createPart(part2);
	Variable a = l.frame.createIntVar(part1, dtor);
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

	Binary bin(arena, L"TestParts", l);

	int v = 0;
	FnParams params;
	params.add(v);

	v = 0;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params), InvalidValue);
	CHECK_EQ(sum, 1);

	v = 1;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params), InvalidValue);
	CHECK_EQ(sum, 3);

	v = 2;
	sum = 0;
	CHECK_ERROR(call<void>(bin.getData(), params), InvalidValue);
	CHECK_EQ(sum, 6);

	v = 3;
	sum = 0;
	CHECK_RUNS(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 6);

} END_TEST

void breakOn(Listing &to, Ref error, Variable p, int n) {
	to << fnParam(p);
	to << fnParam(intConst(n));
	to << fnCall(error, Size::sInt);
	Label l = to.label();
	to << cmp(eax, intConst(0));
	to << jmp(l, ifEqual);
	to << epilog();
	to << ret(Size());
	to << l;
}

BEGIN_TEST(TestNestedPartsException) {
	Arena arena;

	Ref error = arena.external(L"error", &throwError);
	Ref dtor = arena.external(L"dtor", &destroyInt);

	Listing l;
	Variable p = l.frame.createIntParam();

	Block root = l.frame.root();
	Part part1 = root;
	Part part2 = l.frame.createPart(part1);
	Part part3 = l.frame.createPart(part2);
	Variable a = l.frame.createIntVar(part1, dtor, freeOnException);
	Variable b = l.frame.createIntVar(part2, dtor, freeOnException);
	Variable c = l.frame.createIntVar(part3, dtor, freeOnException);

	Block sub1 = l.frame.createChild(part1);
	Block sub2 = l.frame.createChild(part2);
	Block sub3 = l.frame.createChild(part3);
	Variable d = l.frame.createIntVar(sub1, dtor, freeOnException);
	Variable e = l.frame.createIntVar(sub2, dtor, freeOnException);
	Variable f = l.frame.createIntVar(sub3, dtor, freeOnException);

	l << prolog();
	l << mov(a, intConst(1));
	l << mov(b, intConst(2));
	l << mov(c, intConst(3));
	breakOn(l, error, p, 0);

	l << begin(sub1);
	l << mov(d, intConst(4));
	breakOn(l, error, p, 1);
	l << end(sub1);
	breakOn(l, error, p, 2);

	l << begin(part2);
	breakOn(l, error, p, 3);
	l << begin(sub2);
	l << mov(e, intConst(5));
	breakOn(l, error, p, 4);
	l << end(sub2);
	breakOn(l, error, p, 5);

	l << begin(part3);
	breakOn(l, error, p, 6);
	l << begin(sub3);
	l << mov(f, intConst(6));
	breakOn(l, error, p, 7);
	l << end(sub3);
	breakOn(l, error, p, 8);

	l << epilog();
	l << ret(Size());

	Binary bin(arena, L"TestParts", l);

	nat results[] = {
		1, 5, 1, 3, 8, 3, 6, 12, 6
	};

	int v = 0;
	FnParams params;
	params.add(v);

	for (nat i = 0; i < ARRAY_SIZE(results); i++) {
		v = i;
		sum = 0;
		CHECK_ERROR(call<void>(bin.getData(), params), InvalidValue);
		CHECK_EQ(sum, results[i]);
	}

	v = ARRAY_SIZE(results);
	sum = 0;
	CHECK_RUNS(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 0);

} END_TEST


BEGIN_TEST(TestNestedParts) {
	Arena arena;

	Ref error = arena.external(L"error", &throwError);
	Ref dtor = arena.external(L"dtor", &destroyInt);

	Listing l;
	Variable p = l.frame.createIntParam();

	Block root = l.frame.root();
	Part part1 = root;
	Part part2 = l.frame.createPart(part1);
	Part part3 = l.frame.createPart(part2);
	Variable a = l.frame.createIntVar(part1, dtor);
	Variable b = l.frame.createIntVar(part2, dtor);
	Variable c = l.frame.createIntVar(part3, dtor);

	Block sub1 = l.frame.createChild(part1);
	Block sub2 = l.frame.createChild(part2);
	Block sub3 = l.frame.createChild(part3);
	Variable d = l.frame.createIntVar(sub1, dtor);
	Variable e = l.frame.createIntVar(sub2, dtor);
	Variable f = l.frame.createIntVar(sub3, dtor);

	l << prolog();
	l << mov(a, intConst(1));
	l << mov(b, intConst(2));
	l << mov(c, intConst(3));
	breakOn(l, error, p, 0);

	l << begin(sub1);
	l << mov(d, intConst(4));
	breakOn(l, error, p, 1);
	l << end(sub1);
	breakOn(l, error, p, 2);

	l << begin(part2);
	breakOn(l, error, p, 3);
	l << begin(sub2);
	l << mov(e, intConst(5));
	breakOn(l, error, p, 4);
	l << end(sub2);
	breakOn(l, error, p, 5);

	l << begin(part3);
	breakOn(l, error, p, 6);
	l << begin(sub3);
	l << mov(f, intConst(6));
	breakOn(l, error, p, 7);
	l << end(sub3);
	breakOn(l, error, p, 8);

	l << epilog();
	l << ret(Size());

	Binary bin(arena, L"TestParts", l);

	nat results[] = {
		1, 5, 5, 7, 12, 12, 15, 21, 21
	};

	int v = 0;
	FnParams params;
	params.add(v);

	for (nat i = 0; i < ARRAY_SIZE(results); i++) {
		v = i;
		sum = 0;
		CHECK_ERROR(call<void>(bin.getData(), params), InvalidValue);
		CHECK_EQ(sum, results[i]);

		v = i + 100;
		sum = 0;
		CHECK_RUNS(call<void>(bin.getData(), params));
		CHECK_EQ(sum, results[i]);
	}

	v = ARRAY_SIZE(results);
	sum = 0;
	CHECK_RUNS(call<void>(bin.getData(), params));
	CHECK_EQ(sum, 21);

} END_TEST
