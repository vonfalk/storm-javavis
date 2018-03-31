#include "stdafx.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

BEGIN_TEST(StrTest, Core) {
	Engine &e = gEngine();

	Str *s = new (e) Str(S("Hello"));
	Str *t = new (e) Str(S(" World"));

	CHECK_EQ(toS(s), L"Hello");
	CHECK_EQ(toS(*s + t), L"Hello World");
	CHECK_EQ(toS(*s * 3), L"HelloHelloHello");

	Str *w = new (e) Str(Char('Z'), 5);
	CHECK_EQ(toS(w), L"ZZZZZ");

	Str *p = toS(e, Char(nat(0x10030)));
	w = new (e) Str(Char(nat(0x10030)), 2);
	CHECK_EQ(toS(w), toS(*p * 2));

	Str *l = new (e) Str(S("Hello World"));
	CHECK(l->startsWith(S("Hello")));
	CHECK(!l->startsWith(S("Hello!")));
	CHECK(l->endsWith(S("World")));
	CHECK(!l->endsWith(S("World!")));
} END_TEST

BEGIN_TEST(StrBufTest, Core) {
	Engine &e = gEngine();

	StrBuf *buf = new (e) StrBuf();
	*buf << 10 << S("z") << -20;
	CHECK_EQ(toS(buf), L"10z-20");

	buf->clear();
	*buf << 10 << S("n");
	CHECK_EQ(toS(buf), L"10n");

	buf->clear();
	buf->indentBy(new (e) Str(S("-->")));
	*buf << S("hello\n");
	buf->indent();
	*buf << S("world\n");
	buf->dedent();
	CHECK_EQ(toS(buf), L"hello\n-->world\n");

	buf->clear();
	buf->indentBy(new (e) Str(S("-->")));
	*buf << S("a\n");
	buf->indent();
	*buf << 20;
	CHECK_EQ(toS(buf), L"a\n-->20");

	buf->clear();
	*buf << width(5) << fill('z') << S("w");
	CHECK_EQ(toS(buf), L"zzzzw");

	buf->clear();
	*buf << width(5) << 20 << width(5) << Nat(100) << width(5) << -3;
	CHECK_EQ(toS(buf), L"   20  100   -3");

	buf->clear();
	*buf << hex(Byte(10)) << S(" ") << hex(Nat(10)) << S(" ") << hex(Word(0x12345));
	CHECK_EQ(toS(buf), L"0A 0000000A 0000000000012345");

	buf->clear();
	*buf << left(3) << -3 << 3 << left(3) << S("a") << S("a") << left(3) << Word(3);
	CHECK_EQ(toS(buf), L"-3 3a  a3  ");

} END_TEST


BEGIN_TEST(StrConvTest, Core) {
	Engine &e = gEngine();

	CHECK_EQ((new (e) Str(L"AF"))->hexToNat(), 0xAF);
	CHECK_EQ(::toS((new (e) Str(L"\\x70"))->unescape()), L"\x70");
	CHECK_EQ(::toS((new (e) Str(L"\\n"))->unescape()), L"\n");
	CHECK_EQ(::toS((new (e) Str(L"\n"))->escape()), L"\\n");

	CHECK_EQ(::toS((new (e) Str(L"\\\""))->unescape(Char('"'))), L"\"");
	CHECK_EQ(::toS((new (e) Str(L"\""))->escape(Char('"'))), L"\\\"");

} END_TEST
