#include "stdafx.h"
#include "Compiler/Syntax/Tokenizer.h"

using namespace storm::syntax;

BEGIN_TEST(TokenizerTest, Storm) {
	Engine &e = gEngine();

	{
		const wchar *parse = L"abc +de";
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(L"abc"));
		CHECK(tok.skipIf(L"+"));
		CHECK(tok.skipIf(L"de"));
	}


	{
		const wchar *parse = L"ab\"cd\\\"\"de//comment\n   f";
		Tokenizer tok(null, new (e) Str(parse), 0);

		CHECK(tok.skipIf(L"ab"));
		CHECK(tok.skipIf(L"\"cd\\\"\""));
		CHECK(tok.skipIf(L"de"));
		CHECK(tok.skipIf(L"f"));
	}


} END_TEST
