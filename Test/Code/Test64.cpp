#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST_(Add64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v = l->createLongVar(l->root());
	Var w = l->createLongVar(l->root());

	*l << prolog();

	*l << mov(v, longConst(0x7777777777));
	*l << mov(w, longConst(0x9999999999));
	*l << add(v, w);
	*l << mov(rax, v);

	*l << epilog();
	*l << ret(ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), int64(0)), 0x11111111110);

} END_TEST

BEGIN_TEST(Param64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v = l->createLongParam();
	Var w = l->createLongVar(l->root());

	*l << prolog();

	*l << mov(w, v);
	*l << add(w, longConst(0x1));
	*l << mov(rax, w);

	*l << epilog();
	*l << ret(ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), int64(0x123456789A)), 0x123456789B);

} END_TEST

static Long longValue = 0;
static void CODECALL longFn(Long a) {
	longValue = a;
}

BEGIN_TEST(Call64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Var v = l->createLongParam();

	*l << prolog();
	*l << fnParam(v);
	*l << fnCall(arena->external(S("longFn"), address(&longFn)), valVoid());
	*l << epilog();
	*l << ret(valVoid());

	Binary *b = new (e) Binary(arena, l);
	callFn(b->address(), Long(0x123456789A));
	CHECK_EQ(longValue, 0x123456789A);
} END_TEST

BEGIN_TEST(Sub64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v = l->createLongVar(l->root());
	Var w = l->createLongVar(l->root());

	*l << prolog();

	*l << mov(v, longConst(0xA987654321));
	*l << mov(w, longConst(0x123456789A));
	*l << sub(v, w);
	*l << mov(rax, v);

	*l << epilog();
	*l << ret(ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), int64(0)), 0x97530ECA87);

} END_TEST


BEGIN_TEST(Mul64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v = l->createLongVar(l->root());
	Var w = l->createLongVar(l->root());

	*l << prolog();

	*l << mov(rax, longConst(0x100000001)); // Make sure we preserve other registers.
	*l << mov(v, longConst(0xA987654321));
	*l << mov(w, longConst(0x123456789A));
	*l << mul(v, w);
	*l << add(rax, v);

	*l << epilog();
	*l << ret(ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), int64(0)), 0x2DE2A36D2B77D9DB);

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
	for (nat i = 0; i < ARRAY_COUNT(cf); i++)
		if (cf[i] == f)
			return bit(v, i);

	return false;
}

#define FAIL(msg) { result = false; PLN(msg << " does not work"); }
#define CH(flag, cond) if (bit(r, flag) != (cond)) FAIL(name(flag))

static bool check(Long a, Long b, const void *fn) {
	typedef int (*F)(Long, Long);
	F f = (F)fn;
	int r = (*f)(a, b);

	Word ua = a;
	Word ub = b;

	bool result = true;
	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		if (bit(r, i) != bit(r, i + 16)) {
			PLN(name(cf[i]) << L" differs when using setCond and jmp!");
			result = false;
		}
	}

	// Cases.
	CH(ifAlways, true);
	CH(ifNever, false);
	CH(ifEqual, a == b);
	CH(ifNotEqual, a != b);
	CH(ifBelow, ua < ub);
	CH(ifAbove, ua > ub);
	CH(ifBelowEqual, ua <= ub);
	CH(ifAboveEqual, ua >= ub);
	CH(ifLess, a < b);
	CH(ifGreater, a > b);
	CH(ifLessEqual, a <= b);
	CH(ifGreaterEqual, a >= b);

	return result;
}

static bool checkLarge(int64 a, int64 b, const void *fn) {
	// Place the numbers just at the border between low 32-bit and high 32-bit words.
	int64 f = 0x80000000;
	return check(a * f, b * f, fn);
}

BEGIN_TEST(Cmp64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v1 = l->createLongParam();
	Var v2 = l->createLongParam();
	Var r = l->createIntVar(l->root());

	*l << prolog();

	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		*l << mov(eax, intConst(0));
		*l << cmp(v1, v2);
		*l << setCond(al, cf[i]);
		*l << shl(eax, byteConst(i));
		*l << bor(r, eax);
	}

	for (nat i = 0; i < ARRAY_COUNT(cf); i++) {
		Label lbl = l->label();

		*l << cmp(v1, v2);
		*l << jmp(lbl, inverse(cf[i]));
		*l << bor(r, intConst(1 << (16 + i)));
		*l << lbl;
	}

	*l << mov(eax, r);
	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	const void *p = b->address();

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
