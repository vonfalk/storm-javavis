#include "stdafx.h"
#include "Test/Test.h"

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

} END_TEST
