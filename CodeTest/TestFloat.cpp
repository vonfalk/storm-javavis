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
	l << ret(Size::sInt);

	Binary b(arena, l);
	typedef int (*Fn)(float, float);
	Fn fn = (Fn)b.address();

	int r = (*fn)(12.3f, 2.2f); // Should be 27.06
	CHECK_EQ(r, 271); // Rounds up according to the FPU state

} END_TEST


BEGIN_TEST_(TestReturnFloat) {
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
	l << retFloat(Size::sFloat); // Returns the float stored in 'eax'

	Binary b(arena, l);
	typedef Float (*Fn)(Float, Float);
	Fn fn = (Fn)b.address();

	float r = (*fn)(12.3f, 2.2f);
	CHECK_EQ(int(r * 100), 2706);

} END_TEST
