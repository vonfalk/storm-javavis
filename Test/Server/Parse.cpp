#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Syntax/GLR/Parser.h"
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


BEGIN_TEST(InfoError, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	InfoParser *p = InfoParser::create(pkg, L"SExpr", new (e) glr::Parser());

	{
		Str *src = new (e) Str(L"foo +");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		VERIFY(s.success());

		InfoNode *tree = p->infoTree();
		VERIFY(tree->length() == 5);
		CHECK_EQ(tree->leafAt(0)->color, tVarName);
		CHECK_EQ(tree->leafAt(4)->color, tNone);
	}

	{
		Str *src = new (e) Str(L"foo bar");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		VERIFY(s.success());

		InfoNode *tree = p->infoTree();
		VERIFY(tree->length() == 7);
		CHECK_EQ(tree->leafAt(0)->color, tVarName);
		CHECK_EQ(tree->leafAt(3)->color, tNone);
		CHECK_EQ(tree->leafAt(4)->color, tVarName);
	}

	{
		// How are strings with unknown tokens handled?
		Str *src = new (e) Str(L"foo +;");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		VERIFY(s.success());

		// This checks to see if trees are properly padded to their full length.
		InfoNode *tree = p->fullInfoTree();
		VERIFY(tree->length() == 6);
		CHECK_EQ(tree->leafAt(0)->color, tVarName);
		CHECK_EQ(tree->leafAt(4)->color, tNone);
	}

	{
		// Can we skip unknown characters?
		Str *src = new (e) Str(L"foo ? bar");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		VERIFY(s.success());

		InfoNode *tree = p->infoTree();
		VERIFY(tree->length() == 9);
		CHECK_EQ(tree->leafAt(0)->color, tVarName);
		CHECK_EQ(tree->leafAt(6)->color, tVarName);
		CHECK_EQ(tree->leafAt(8)->color, tVarName);
	}

} END_TEST

BEGIN_TEST(JavaError, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.java");
	InfoParser *p = InfoParser::create(pkg, L"SStmt");

	{
		// Previously, this failed...
		Str *src = new (e) Str(L"1 +");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		CHECK(s.success());
	}

	{
		// And this...
		Str *src = new (e) Str(L"try { foo(bar, } catch (Type v) {}");
		InfoErrors s = p->parseApprox(src, new (e) Url());
		CHECK(s.success());

		if (!s.success())
			p->throwError();
	}
} END_TEST
