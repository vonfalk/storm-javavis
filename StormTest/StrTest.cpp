#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"

BEGIN_TEST(RemoveIndentTest) {
	Engine &e = *gEngine;

	Auto<Str> v = CREATE(Str, e, L"  hello\n  world");
	CHECK_EQ(steal(removeIndent(v))->v, L"hello\nworld");

	v = CREATE(Str, e, L"hello\n  world");
	CHECK_EQ(steal(removeIndent(v))->v, L"hello\n  world");

	v = CREATE(Str, e, L"\n  hello\n  world");
	CHECK_EQ(steal(removeIndent(v))->v, L"\nhello\nworld");

	v = CREATE(Str, e, L"\nhello\n  world");
	CHECK_EQ(steal(removeIndent(v))->v, L"\nhello\n  world");

} END_TEST

static wchar *utfString = L"a\u00D6\u0D36\u3042\u79C1\U0001F639e";
static nat codepoints[] = {
	nat('a'), 0xD6, 0x0D36, 0x3042, 0x79C1, 0x0001F639, nat('e')
};

BEGIN_TEST(StrIteratorTest) {
	Engine &e = *gEngine;

	Auto<Str> v = CREATE(Str, e, utfString);
	nat at = 0;
	for (Str::Iter i = v->begin(); i != v->end(); i++, at++) {
		int cp = 0;
		if (at < ARRAY_COUNT(codepoints))
			cp = codepoints[at];
		CHECK_EQ((*i).getCodePoint(), cp);
	}

	CHECK_EQ(at, ARRAY_COUNT(codepoints));


	CHECK_EQ(runFn<Int>(L"test.bs.iterateStr", v.borrow()), ARRAY_COUNT(codepoints));

	Auto<Array<Char>> ref = CREATE(Array<Char>, e);
	for (nat i = 0; i < ARRAY_COUNT(codepoints); i++) {
		ref->push(Char(codepoints[i]));
	}

	CHECK_EQ(runFn<Int>(L"test.bs.verifyStr", v.borrow(), ref.borrow()), 1);

} END_TEST
