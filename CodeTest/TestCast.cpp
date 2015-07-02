#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(TestCastIntLong) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createIntParam();
	Variable r = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(rax, r);
	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	typedef Long (*F)(Int);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

BEGIN_TEST(TestCastCharInt) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createByteParam();
	Variable r = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(eax, r);
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);
	typedef Int (*F)(Char);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

BEGIN_TEST(TestCastCharLong) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createByteParam();
	Variable r = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(rax, r);
	l << epilog();
	l << ret(retVal(Size::sLong, false));

	Binary b(arena, l);
	typedef Long (*F)(Char);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

BEGIN_TEST(TestCastLongInt) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createLongParam();
	Variable r = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(eax, r);
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);
	typedef Int (*F)(Long);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

BEGIN_TEST(TestCastIntChar) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createIntParam();
	Variable r = l.frame.createByteVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(al, r);
	l << epilog();
	l << ret(retVal(Size::sByte, false));

	Binary b(arena, l);
	typedef Char (*F)(Int);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

BEGIN_TEST(TestCastLongChar) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createLongParam();
	Variable r = l.frame.createByteVar(l.frame.root());

	l << prolog();
	l << icast(r, p);
	l << mov(al, r);
	l << epilog();
	l << ret(retVal(Size::sByte, false));

	Binary b(arena, l);
	typedef Char (*F)(Long);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(-2), -2);

} END_TEST

/**
 * Unsigned
 */

BEGIN_TEST(TestCastNatWord) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createIntParam();
	Variable r = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(rax, r);
	l << epilog();
	l << ret(retVal(Size::sWord, false));

	Binary b(arena, l);
	typedef Word (*F)(Nat);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(0x80000000), 0x80000000);

} END_TEST

BEGIN_TEST(TestCastByteNat) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createByteParam();
	Variable r = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(eax, r);
	l << epilog();
	l << ret(retVal(Size::sNat, false));

	Binary b(arena, l);
	typedef Nat (*F)(Byte);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(0x80), 0x80);

} END_TEST

BEGIN_TEST(TestCastByteWord) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createByteParam();
	Variable r = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(rax, r);
	l << epilog();
	l << ret(retVal(Size::sWord, false));

	Binary b(arena, l);
	typedef Word (*F)(Byte);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);
	CHECK_EQ((*f)(0x80), 0x80);

} END_TEST

BEGIN_TEST(TestCastWordNat) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createLongParam();
	Variable r = l.frame.createIntVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(eax, r);
	l << epilog();
	l << ret(retVal(Size::sNat, false));

	Binary b(arena, l);
	typedef Nat (*F)(Word);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);

} END_TEST

BEGIN_TEST(TestCastNatByte) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createIntParam();
	Variable r = l.frame.createByteVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(al, r);
	l << epilog();
	l << ret(retVal(Size::sByte, false));

	Binary b(arena, l);
	typedef Byte (*F)(Nat);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);

} END_TEST

BEGIN_TEST(TestCastWordByte) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createLongParam();
	Variable r = l.frame.createByteVar(l.frame.root());

	l << prolog();
	l << ucast(r, p);
	l << mov(al, r);
	l << epilog();
	l << ret(retVal(Size::sByte, false));

	Binary b(arena, l);
	typedef Byte (*F)(Word);
	F f = (F)b.address();

	CHECK_EQ((*f)(2), 2);

} END_TEST
