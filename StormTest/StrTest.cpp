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
