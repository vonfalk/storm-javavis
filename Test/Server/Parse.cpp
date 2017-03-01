#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"

#include "Compiler/Syntax/Earley/Parser.h"

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
	CHECK_EQ(tree->leafAt(6)->color, tVarName);
	CHECK_EQ(tree->leafAt(12)->color, tConstant);
	CHECK_EQ(tree->leafAt(13)->color, tNone);

} END_TEST

BEGIN_TEST(IndentParse, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	Parser *p = Parser::create(pkg, L"SRoot"); //, new (e) storm::syntax::earley::Parser());

	Str *src = new (e) Str(L"foo;\n{\na+b;\n{c;}\n}\nbar;");
	p->parse(src, new (e) Url());
	VERIFY(p->hasTree() && p->matchEnd() == src->end());

	InfoNode *tree = p->infoTree();

	StrBuf *to = new (e) StrBuf();
	tree->format(to);

	CHECK_EQ(tree->indentAt(5), indentLevel(0));
	CHECK_EQ(tree->indentAt(6), indentLevel(1));
	CHECK_EQ(tree->indentAt(9), indentAs(8));
	CHECK_EQ(tree->indentAt(12), indentLevel(1));
	CHECK_EQ(tree->indentAt(13), indentLevel(2));
	CHECK_EQ(tree->indentAt(16), indentLevel(1));
	CHECK_EQ(tree->indentAt(17), indentLevel(0));

} END_TEST
