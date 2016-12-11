#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"

using syntax::Parser;

BEGIN_TEST_(ParserTest) {
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, L"test.syntax")));
	VERIFY(pkg);

	Parser *p = Parser::create(pkg, L"Sentence");
	CHECK(p->parse(new (e) Str(L"the cat runs"), new (e) Url()));
	PVAR(p);

} END_TEST
