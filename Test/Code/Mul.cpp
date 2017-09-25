#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(Mul, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mul(p1, p2);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2, 2), 4);
	CHECK_EQ((*fn)(-2, 2), -4);
	CHECK_EQ((*fn)(2, -2), -4);
	CHECK_EQ((*fn)(-2, -2), 4);

} END_TEST

BEGIN_TEST(MulReg, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p2);
	*l << mul(p1, eax);
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2, 2), 4);
	CHECK_EQ((*fn)(-2, 2), -4);
	CHECK_EQ((*fn)(2, -2), -4);
	CHECK_EQ((*fn)(-2, -2), 4);

} END_TEST

BEGIN_TEST(MulReg2, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p2);
	*l << mov(ebx, p1);
	*l << mul(eax, ebx);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2, 2), 4);
	CHECK_EQ((*fn)(-2, 2), -4);
	CHECK_EQ((*fn)(2, -2), -4);
	CHECK_EQ((*fn)(-2, -2), 4);

} END_TEST

BEGIN_TEST(MulReg3, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p2);
	*l << mov(ebx, p1);
	*l << mul(ebx, eax);
	*l << mov(eax, ebx);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2, 2), 4);
	CHECK_EQ((*fn)(-2, 2), -4);
	CHECK_EQ((*fn)(2, -2), -4);
	CHECK_EQ((*fn)(-2, -2), 4);

} END_TEST

BEGIN_TEST(MulConst, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();

	*l << prolog();

	*l << mul(p1, intConst(-2));
	*l << mov(eax, p1);

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), -4);
	CHECK_EQ((*fn)(-2), 4);

} END_TEST

BEGIN_TEST(MulByte, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createByteParam();
	Var p2 = l->createByteParam();

	*l << prolog();

	*l << mul(p1, p2);
	*l << mov(al, p1);

	*l << epilog();
	*l << ret(ValType(Size::sByte, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Byte (*Fn)(Byte, Byte);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2, 2), 4);

} END_TEST

