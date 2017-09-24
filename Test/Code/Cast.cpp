#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(CastIntLong, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();
	Var r = l->createLongVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(rax, r);

	*l << epilog();
	*l << ret(valLong());

	Binary *b = new (e) Binary(arena, l);
	typedef Long (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastIntLong2, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();
	Var r = l->createLongVar(l->root());

	*l << prolog();

	*l << lea(ptrA, r);
	*l << icast(longRel(ptrA, Offset()), p);
	*l << mov(rax, r);

	*l << epilog();
	*l << ret(valLong());

	Binary *b = new (e) Binary(arena, l);
	typedef Long (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastCharInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createByteParam();
	Var r = l->createIntVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(eax, r);

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(char);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastCharLong, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createByteParam();
	Var r = l->createLongVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(rax, r);

	*l << epilog();
	*l << ret(valLong());

	Binary *b = new (e) Binary(arena, l);
	typedef Long (*Fn)(char);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastLongInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createLongParam();
	Var r = l->createIntVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(eax, r);

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Long);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastIntChar, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();
	Var r = l->createByteVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(al, r);

	*l << epilog();
	*l << ret(valByte());

	Binary *b = new (e) Binary(arena, l);
	typedef char (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

BEGIN_TEST(CastLongChar, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createLongParam();
	Var r = l->createByteVar(l->root());

	*l << prolog();

	*l << icast(r, p);
	*l << mov(al, r);

	*l << epilog();
	*l << ret(valByte());

	Binary *b = new (e) Binary(arena, l);
	typedef char (*Fn)(Long);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(-2), -2);

} END_TEST

/**
 * Unsigned:
 */

BEGIN_TEST(CastNatWord, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();
	Var r = l->createLongVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(rax, r);

	*l << epilog();
	*l << ret(valLong());

	Binary *b = new (e) Binary(arena, l);
	typedef Word (*Fn)(Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(0xFF00FF00), 0xFF00FF00);

} END_TEST

BEGIN_TEST(CastByteNat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createByteParam();
	Var r = l->createIntVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(eax, r);

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Byte);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(0xFF), 0xFF);

} END_TEST

BEGIN_TEST(CastByteWord, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createByteParam();
	Var r = l->createLongVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(rax, r);

	*l << epilog();
	*l << ret(valLong());

	Binary *b = new (e) Binary(arena, l);
	typedef Word (*Fn)(Byte);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);
	CHECK_EQ((*fn)(0xFF), 0xFF);

} END_TEST

BEGIN_TEST(CastWordNat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createLongParam();
	Var r = l->createIntVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(eax, r);

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Word);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);

} END_TEST

BEGIN_TEST(CastNatByte, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();
	Var r = l->createByteVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(al, r);

	*l << epilog();
	*l << ret(valByte());

	Binary *b = new (e) Binary(arena, l);
	typedef Byte (*Fn)(Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);

} END_TEST

BEGIN_TEST(CastWordByte, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createLongParam();
	Var r = l->createByteVar(l->root());

	*l << prolog();

	*l << ucast(r, p);
	*l << mov(al, r);

	*l << epilog();
	*l << ret(valByte());

	Binary *b = new (e) Binary(arena, l);
	typedef Byte (*Fn)(Word);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(2), 2);

} END_TEST
