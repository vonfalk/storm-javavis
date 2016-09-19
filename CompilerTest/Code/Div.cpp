#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(DivITest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();
	Variable p2 = l->createIntParam();

	*l << prolog();

	*l << idiv(p1, p2);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(-8, 2), -4);
	CHECK_EQ((*fn)(8, -2), -4);
	CHECK_EQ((*fn)(-8, -2), 4);

} END_TEST

BEGIN_TEST(DivI2Test, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();
	Variable p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p1);
	*l << mov(ebx, p2);
	*l << idiv(eax, ebx);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(-8, 2), -4);
	CHECK_EQ((*fn)(8, -2), -4);
	CHECK_EQ((*fn)(-8, -2), 4);

} END_TEST

BEGIN_TEST(DivUTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();
	Variable p2 = l->createIntParam();

	*l << prolog();

	*l << udiv(p1, p2);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Nat, Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(0x80000000, 2), 0x40000000);

} END_TEST

BEGIN_TEST(ModITest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();
	Variable p2 = l->createIntParam();

	*l << prolog();

	*l << imod(p1, p2);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(18, 10), 8);
	CHECK_EQ((*fn)(-18, 10), -8);
	CHECK_EQ((*fn)(18, -10), 8);
	CHECK_EQ((*fn)(-18, -10), -8);

} END_TEST

BEGIN_TEST(ModUTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();
	Variable p2 = l->createIntParam();

	*l << prolog();

	*l << umod(p1, p2);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Nat, Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(14, 10), 4);
	CHECK_EQ((*fn)(0x80000000, 3), 2);

} END_TEST

BEGIN_TEST(DivIConstTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();

	*l << prolog();

	*l << idiv(p1, intConst(-2));
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(8), -4);
	CHECK_EQ((*fn)(-8), 4);

} END_TEST

BEGIN_TEST(ModIConstTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Variable p1 = l->createIntParam();

	*l << prolog();

	*l << imod(p1, intConst(-10));
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(18), 8);
	CHECK_EQ((*fn)(-18), -8);

} END_TEST
