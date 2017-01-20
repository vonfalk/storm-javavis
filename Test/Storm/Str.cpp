#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Exception.h"

BEGIN_TEST(StrConcatTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.strConcatTest")), L"ab1cvoid");
	CHECK_ERROR(runFn<Str *>(L"test.bs.strConcatError"), SyntaxError);
} END_TEST


BEGIN_TEST(RemoveIndentTest, BS) {
	Engine &e = gEngine();

	Str *v = new (e) Str(L"  hello\n  world");
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\nworld");

	v = new (e) Str(L"hello\n  world");
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\n  world");

	v = new (e) Str(L"\n  hello\n  world");
	CHECK_EQ(::toS(removeIndentation(v)), L"\nhello\nworld");

	v = new (e) Str(L"\nhello\n  world");
	CHECK_EQ(::toS(removeIndentation(v)), L"\nhello\n  world");

	v = new (e) Str(L"  hello\n  \n  world");
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\n\nworld");

} END_TEST

static wchar *utfString = L"a\u00D6\u0D36\u3042\u79C1\U0001F639e";
static nat codepoints[] = {
	nat('a'), 0xD6, 0x0D36, 0x3042, 0x79C1, 0x0001F639, nat('e')
};

BEGIN_TEST(StrIteratorTest, BS) {
	Engine &e = gEngine();

	Str *v = new (e) Str(utfString);
	nat at = 0;
	for (Str::Iter i = v->begin(); i != v->end(); i++, at++) {
		int cp = 0;
		if (at < ARRAY_COUNT(codepoints))
			cp = codepoints[at];
		CHECK_EQ(i.v().codepoint(), cp);
	}

	CHECK_EQ(at, ARRAY_COUNT(codepoints));


	CHECK_EQ(runFn<Int>(L"test.bs.iterateStr", v), ARRAY_COUNT(codepoints));

	Array<Char> *ref = new (e) Array<Char>();
	for (nat i = 0; i < ARRAY_COUNT(codepoints); i++) {
		ref->push(Char(codepoints[i]));
	}

	CHECK_EQ(runFn<Int>(L"test.bs.verifyStr", v, ref), 1);

} END_TEST