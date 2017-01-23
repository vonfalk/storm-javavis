#include "stdafx.h"

// Test 64-bit code.
BEGIN_TEST(Test64Add) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(v, longConst(0x123456789A));
	l << mov(v2, longConst(0xA987654321));
	l << add(v, v2);
	l << mov(rax, v);

	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), int64(0)), 0xBBBBBBBBBB);

} END_TEST

BEGIN_TEST(Test64Sub) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(v, longConst(0xA987654321));
	l << mov(v2, longConst(0x123456789A));
	l << sub(v, v2);
	l << mov(rax, v);

	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), int64(0)), 0x97530ECA87);

} END_TEST

BEGIN_TEST(Test64Mul) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(rax, longConst(0x100000001)); // Make sure we preserve other registers.
	l << mov(v, longConst(0xA987654321));
	l << mov(v2, longConst(0x123456789A));
	l << mul(v, v2);
	l << add(rax, v);

	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), int64(0)), 0x2DE2A36D2B77D9DB);

} END_TEST

BEGIN_TEST(Test64Preserve) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(v, longConst(0x123456789A));
	l << mov(v2, longConst(0xA987654321));
	l << mov(rax, v);
	l << add(v, v2);
	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), int64(0)), 0x123456789A);
} END_TEST

static CondFlag cf[] = {
	ifAlways,
	ifNever,
	ifEqual,
	ifNotEqual,
	ifBelow,
	ifBelowEqual,
	ifAboveEqual,
	ifAbove,
	ifLess,
	ifLessEqual,
	ifGreaterEqual,
	ifGreater,
};


static bool bit(int v, int b) {
	return (v & (1 << b)) != 0;
}

static bool bit(int v, CondFlag f) {
	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		if (cf[i] == f)
			return bit(v, i);
	}
	return false;
}

#define FAIL(msg) { result = false; PLN(msg << " does not work!"); }
#define CH(flag, cond) if (bit(r, flag) != (cond)) FAIL(flag);

static bool check(int64 a, int64 b, const void *fn) {
	typedef int (*F)(int64, int64);
	F f = (F)fn;
	int r = (*f)(a, b);

	nat64 ua = a;
	nat64 ub = b;

	bool result = true;

	// Folded...
	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		if (bit(r, i) != bit(r, i + 16)) {
			PLN(name(cf[i]) << " differs when using setCond and jmp!");
			result = false;
		}
	}

	// cases.
	CH(ifAlways, true);
	CH(ifNever, false);
	CH(ifEqual, a == b);
	CH(ifNotEqual, a != b);
	CH(ifBelow, ua < ub);
	CH(ifAbove, ua > ub);
	CH(ifAboveEqual, ua >= ub);
	CH(ifBelowEqual, ua <= ub);
	CH(ifLess, a < b);
	CH(ifGreater, a > b);
	CH(ifLessEqual, a <= b);
	CH(ifGreaterEqual, a >= b);

	return result;
}

static bool checkLarge(int64 a, int64 b, const void *fn) {
	int64 f = 0x1000000000;
	return check(a * f, b * f, fn);
}

BEGIN_TEST(Test64Cmp) {
	Arena arena;
	Listing l;

	Variable v1 = l.frame.createLongParam();
	Variable v2 = l.frame.createLongParam();
	Variable r = l.frame.createIntVar(l.frame.root());

	l << prolog();

	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		l << mov(eax, intConst(0));
		l << cmp(v1, v2);
		l << setCond(al, cf[i]);
		l << shl(eax, byteConst(i));
		l << or(r, eax);
	}

	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		Label lbl = l.label();

		l << cmp(v1, v2);
		l << jmp(lbl, inverse(cf[i]));
		l << or(r, intConst(1 << (16 + i)));
		l << lbl;
	}

	l << mov(eax, r);
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);
	const void *p = b.address();
	CHECK(check(0, 0, p));
	CHECK(check(1, 0, p));
	CHECK(check(0, 1, p));
	CHECK(check(-1, 0, p));
	CHECK(check(0, -1, p));
	CHECK(check(-100, 100, p));
	CHECK(check(10, 10, p));
	CHECK(checkLarge(0, 0, p));
	CHECK(checkLarge(1, 0, p));
	CHECK(checkLarge(0, 1, p));
	CHECK(checkLarge(-1, 0, p));
	CHECK(checkLarge(0, -1, p));
	CHECK(checkLarge(-100, 100, p));
	CHECK(checkLarge(10, 10, p));

} END_TEST