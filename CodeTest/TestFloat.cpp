#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(TestFloat) {
	Arena arena;

	Listing l;

	Variable p1 = l.frame.createFloatParam();
	Variable p2 = l.frame.createFloatParam();
	Variable v1 = l.frame.createIntVar(l.frame.root());

	l << prolog();

	l << fld(p1);
	l << fld(p2);
	l << fmulp();
	l << mov(v1, intConst(10));
	l << fild(v1);
	l << fmulp();
	l << fistp(v1); // seems to round the result.
	l << mov(eax, v1);

	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);

	float pa = 12.3f, pb = 2.2f;
	// Should be 27.06, is then rounded (depending on FPU-flags).
	FnParams p; p.add(pa).add(pb);
	int r = call<int>(b.address(), false, p);
	CHECK_EQ(r, 270); // Rounds up according to the FPU state

} END_TEST


BEGIN_TEST(TestReturnFloat) {
	Arena arena;
	Listing l;

	Variable p1 = l.frame.createFloatParam();
	Variable p2 = l.frame.createFloatParam();
	Variable res = l.frame.createFloatVar(l.frame.root());

	l << prolog();

	l << fld(p1);
	l << fld(p2);
	l << fmulp();
	l << fstp(res);
	l << mov(eax, res);

	l << epilog();
	l << ret(retVal(Size::sFloat, true)); // Returns the float stored in 'eax'

	Binary b(arena, l);

	float pa = 12.3f, pb = 2.2f;
	// Should be 27.06 rounded (depending on FPU-flags).
	FnParams p; p.add(pa).add(pb);
	float r = call<float>(b.address(), false, p);
	CHECK_EQ(int(r * 100), 2706);

} END_TEST

BEGIN_TEST(TestFloatConst) {
	Arena arena;
	Listing l;

	l << prolog();

	l << mov(eax, floatConst(10.2f));

	l << epilog();
	l << ret(retVal(Size::sFloat, true)); // Returns the float stored in 'eax'

	Binary b(arena, l);

	FnParams p;
	float r = call<float>(b.address(), false, p);
	CHECK_EQ(r, 10.2f);

} END_TEST
