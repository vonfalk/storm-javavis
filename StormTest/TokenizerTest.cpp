#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Tokenizer.h"

using namespace storm;

BEGIN_TEST(TokenizerTest) {

	{
		String s(L"hello world");
		Tokenizer t(Path(), s, 0);

		CHECK_EQ(t.next().token, L"hello");
		CHECK_EQ(t.next().token, L"world");
		CHECK(!t.more());
	}

	{
		String s(L"Hello \"string\\\"\"?(op");
		Tokenizer t(Path(), s, 0);

		CHECK_EQ(t.next().token, L"Hello");
		CHECK_EQ(t.next().token, L"\"string\\\"\"");
		CHECK_EQ(t.next().token, L"?(");
		CHECK_EQ(t.next().token, L"op");
		CHECK(!t.more());
	}

	{
		String s(L"He//llo\ncomment");
		Tokenizer t(Path(), s, 0);

		CHECK_EQ(t.next().token, L"He");
		CHECK_EQ(t.next().token, L"comment");
		CHECK(!t.more());
	}

} END_TEST
