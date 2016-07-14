#include "stdafx.h"
#include "Test/Test.h"
#include "Compiler/Str.h"
#include "Compiler/StrBuf.h"

BEGIN_TEST(StrTest) {
	Engine &e = *gEngine;

	Str *s = new (e) Str(L"Hello");
	Str *t = new (e) Str(L" World");

	CHECK_EQ(toS(s), L"Hello");
	CHECK_EQ(toS(*s + t), L"Hello World");
	CHECK_EQ(toS(*s * 3), L"HelloHelloHello");

	Str *w = new (e) Str(Char('Z'), 5);
	CHECK_EQ(toS(w), L"ZZZZZ");

} END_TEST

BEGIN_TEST(StrBufTest) {
	Engine &e = *gEngine;

	StrBuf *buf = new (e) StrBuf();
	*buf << 10 << L"z" << -20;
	CHECK_EQ(toS(buf), L"10z-20");

	buf->clear();
	*buf << 10 << L"n";
	CHECK_EQ(toS(buf), L"10n");

	buf->clear();
	buf->indentBy(new (e) Str(L"-->"));
	*buf << L"hello\n";
	buf->indent();
	*buf << L"world\n";
	buf->dedent();
	CHECK_EQ(toS(buf), L"hello\n-->world\n");

	buf->clear();
	buf->indentBy(new (e) Str(L"-->"));
	*buf << L"a\n";
	buf->indent();
	*buf << 20;
	CHECK_EQ(toS(buf), L"a\n-->20");

	buf->clear();
	*buf << width(5) << fill('z') << L"w";
	CHECK_EQ(toS(buf), L"zzzzw");

	buf->clear();
	*buf << width(5) << 20 << width(5) << Nat(100) << width(5) << -3;
	CHECK_EQ(toS(buf), L"   20  100   -3");

} END_TEST
