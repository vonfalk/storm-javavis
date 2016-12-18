#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"

using syntax::Parser;

BEGIN_TEST_(ParserTest) {
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, L"test.grammar")));
	VERIFY(pkg);

	Parser *p = Parser::create(pkg, L"Sentence");
	Str *s = new (e) Str(L"the cat runs");
	CHECK(p->parse(s, new (e) Url()));
	CHECK(!p->hasError());
	CHECK(p->hasTree());
	CHECK(p->matchEnd() == s->end());

	syntax::Node *tree = p->tree();
	Str *r = syntax::transformNode<Str>(tree);
	CHECK_EQ(::toS(r), L"cat");

	CHECK(p->parse(new (e) Str(L"the cat runs!"), new (e) Url()));
	CHECK(p->hasError());
	CHECK(p->hasTree());
	CHECK_EQ(p->matchEnd().v(), Char('!'));

} END_TEST
