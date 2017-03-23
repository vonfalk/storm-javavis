#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Syntax/Glr/Parser.h"
#include "Compiler/Package.h"

using namespace storm::syntax;

BEGIN_TEST(InfoParse, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	Parser *p = Parser::create(pkg, L"SRoot");

	Str *src = new (e) Str(L"foo + bar / 2;");
	Bool ok = p->parse(src, new (e) Url());
	VERIFY(p->hasTree() && p->matchEnd() == src->end());

	InfoNode *tree = p->infoTree();
	CHECK_EQ(tree->length(), 14);
	CHECK_EQ(tree->leafAt(0)->color, tVarName);
	CHECK_EQ(tree->leafAt(4)->color, tNone);
	CHECK_EQ(tree->leafAt(6)->color, tVarName);
	CHECK_EQ(tree->leafAt(12)->color, tConstant);
	CHECK_EQ(tree->leafAt(13)->color, tNone);

} END_TEST


BEGIN_TEST_(InfoError, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	InfoParser *p = InfoParser::create(pkg, L"SExpr", new (e) glr::Parser());

	Str *src = new (e) Str(L"foo +");
	ParseResult r = p->parseApprox(src, new (e) Url());
	VERIFY(r.success);

	InfoNode *tree = p->infoTree();
	CHECK_EQ(tree->length(), 5);
	CHECK_EQ(tree->leafAt(0)->color, tVarName);
	CHECK_EQ(tree->leafAt(4)->color, tNone);
} END_TEST

BEGIN_TESTX(InfoPrefix2, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	InfoParser *p = InfoParser::create(pkg, L"SExpr", new (e) glr::Parser());

	Str *src = new (e) Str(L"foo +;");
	Bool ok = p->parse(src, new (e) Url());
	VERIFY(!p->hasTree() || p->matchEnd() != src->end());

	InfoNode *tree = p->infoTree();
	CHECK_EQ(tree->length(), 6);
	CHECK_EQ(tree->leafAt(0)->color, tVarName);
	CHECK_EQ(tree->leafAt(4)->color, tNone);
} END_TEST
