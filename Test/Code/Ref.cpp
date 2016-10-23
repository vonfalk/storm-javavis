#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/Reference.h"

using namespace code;

static Int fnA(Int v) {
	return v + 10;
}

static Int fnB(Int v) {
	return v + 20;
}

// Basic test of references:
BEGIN_TEST(RefTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	RefSource *fn = new (e) RefSource(new (e) Str(L"fnA/B"));
	fn->setPtr(&fnA);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();

	*l << prolog();

	*l << fnParam(p);
	*l << fnCall(Ref(fn), valInt());
	*l << add(eax, intConst(5));

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn f = (Fn)b->address();

	CHECK_EQ((*f)(10), 25);

	fn->setPtr(&fnB);
	CHECK_EQ((*f)(10), 35);

} END_TEST


static Int tenFn() {
	return 10;
}

static Int addFn(Int a, Int b) {
	return a + b;
}

// More advanced replacement test.
BEGIN_TEST(ReplaceTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref tenFun = arena->external(L"tenFn", &tenFn);
	Ref addFun = arena->external(L"addFn", &addFn);

	Listing *l = new (e) Listing();
	*l << prolog();
	*l << fnCall(tenFun, valInt());
	*l << epilog();
	*l << ret(valInt());

	Binary *middleBlob = new (e) Binary(arena, l);
	RefSource *middle = new (e) RefSource(new (e) Str(L"middle"));
	middle->set(middleBlob);

	l = new (e) Listing();
	*l << prolog();
	*l << fnCall(Ref(middle), valInt());
	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn f = (Fn)b->address();

	CHECK_EQ((*f)(), 10);

	// Now, replace 'middle'!
	l = new (e) Listing();
	*l << prolog();
	*l << fnParam(intConst(128));
	*l << fnParam(intConst(20));
	*l << fnCall(addFun, valInt());
	*l << epilog();
	*l << ret(valInt());

	middle->set(new (e) Binary(arena, l));

	CHECK_EQ((*f)(), 148);
} END_TEST
