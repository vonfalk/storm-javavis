#include "stdafx.h"
#include "Compiler/Syntax/Regex.h"

using namespace storm::syntax;


BEGIN_TEST_(RegexTest) {
	Engine &e = gEngine();

	Str *match = new (e) Str(L"ab[abc]");
	Regex r(match);
	PVAR(r);

	

} END_TEST
