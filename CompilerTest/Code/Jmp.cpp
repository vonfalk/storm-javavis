#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(JmpTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p = l->createIntParam();

	*l << prolog();

	Label loop = l->label();
	Label end = l->label();

	*l << mov(eax, intConst(0));

	*l << loop;
	*l << cmp(p, intConst(0));
	*l << jmp(end, ifEqual);

	*l << sub(p, intConst(1));
	*l << add(eax, intConst(2));
	*l << jmp(loop);

	*l << end;

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->rawPtr();

	CHECK_EQ((*fn)(1), 2);
	CHECK_EQ((*fn)(2), 4);
	CHECK_EQ((*fn)(3), 6);

} END_TEST

static bool called = false;

static Int addFive(Int v) {
	called = true;
	return v + 5;
}

// Main reason for this test is to see that we properly compute the offset for static things. If we
// fail, things will fail miserably later on!
BEGIN_TEST(CallTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p = l->createIntParam();

	*l << prolog();

	*l << push(p);
	// Note: this is not how one should call functions. It works until someone tries to save the
	// listing and restore it somewhere else. So during tests and final backend transformations it
	// is OK, otherwise not. Use a reference instead!
	*l << call(xConst(Size::sPtr, Word(&addFive)), ValType(Size::sInt, false));

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->rawPtr();

	called = false;
	CHECK_EQ((*fn)(1), 6);
	CHECK(called);

} END_TEST
