#include "stdafx.h"
#include "Compiler/Syntax/Tokenizer.h"

using namespace storm::syntax;

BEGIN_TEST(TokenizerTest, Storm) {
	Engine &e = gEngine();

	{
		const wchar *parse = S("abc +de");
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(S("abc")));
		CHECK(tok.skipIf(S("+")));
		CHECK(tok.skipIf(S("de")));
	}


	{
		const wchar *parse = S("ab\"cd\\\"\"de//comment\n   f");
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(S("ab")));
		CHECK(tok.skipIf(S("\"cd\\\"\"")));
		CHECK(tok.skipIf(S("de")));
		CHECK(tok.skipIf(S("f")));
	}

	{
		const wchar *parse = S("(,)");
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(S("(")));
		CHECK(tok.skipIf(S(",")));
		CHECK(tok.skipIf(S(")")));
	}

	{
		const wchar *parse = S("Hello \"string\\\"\"?*op");
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(S("Hello")));
		CHECK(tok.skipIf(S("\"string\\\"\"")));
		CHECK(tok.skipIf(S("?*")));
		CHECK(tok.skipIf(S("op")));
	}

} END_TEST
