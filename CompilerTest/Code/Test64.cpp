#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(Add64, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable v = l->createLongVar(l->root());
	Variable w = l->createLongVar(l->root());

	*l << prolog(e);

	*l << mov(e, v, longConst(0x7777777777));
	*l << mov(e, w, longConst(0x9999999999));
	*l << add(e, v, w);
	*l << mov(e, rax, v);

	*l << epilog(e);
	*l << ret(e, ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->rawPtr(), int64(0)), 0x11111111110);

} END_TEST

BEGIN_TEST(Param64, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable v = l->createLongParam();
	Variable w = l->createLongVar(l->root());

	*l << prolog(e);

	*l << mov(e, w, v);
	*l << add(e, w, longConst(0x1));
	*l << mov(e, rax, w);

	*l << epilog(e);
	*l << ret(e, ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->rawPtr(), int64(0x123456789A)), 0x123456789B);

} END_TEST

BEGIN_TEST(Sub64, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable v = l->createLongVar(l->root());
	Variable w = l->createLongVar(l->root());

	*l << prolog(e);

	*l << mov(e, v, longConst(0xA987654321));
	*l << mov(e, w, longConst(0x123456789A));
	*l << sub(e, v, w);
	*l << mov(e, rax, v);

	*l << epilog(e);
	*l << ret(e, ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->rawPtr(), int64(0)), 0x97530ECA87);

} END_TEST


BEGIN_TEST(Mul64, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable v = l->createLongVar(l->root());
	Variable w = l->createLongVar(l->root());

	*l << prolog(e);

	*l << mov(e, rax, longConst(0x100000001)); // Make sure we preserve other registers.
	*l << mov(e, v, longConst(0xA987654321));
	*l << mov(e, w, longConst(0x123456789A));
	*l << mul(e, v, w);
	*l << add(e, rax, v);

	*l << epilog(e);
	*l << ret(e, ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->rawPtr(), int64(0)), 0x2DE2A36D2B77D9DB);

} END_TEST

BEGIN_TEST(Preserve64, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable v = l->createLongVar(l->root());
	Variable w = l->createLongVar(l->root());

	*l << prolog(e);

	*l << mov(e, v, longConst(0x123456789A));
	*l << mov(e, w, longConst(0xA987654321));
	*l << add(e, rax, v);
	*l << mov(e, v, w);

	*l << epilog(e);
	*l << ret(e, ValType(Size::sLong, false));

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->rawPtr(), int64(0)), 0x123456789A);

} END_TEST
