#include "stdafx.h"

#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"

using namespace storm::syntax;

BEGIN_TEST(IndentParse, Server) {
	Engine &e = gEngine();

	Package *pkg = e.package(L"lang.simple");
	Parser *p = Parser::create(pkg, L"SRoot"); //, new (e) storm::syntax::earley::Parser());

	{
		Str *src = new (e) Str(L"foo;\n{\na+b;\n{c;}\n}\nbar;");
		p->parse(src, new (e) Url());
		VERIFY(p->hasTree() && p->matchEnd() == src->end());

		InfoNode *tree = p->infoTree();

		// StrBuf *to = new (e) StrBuf();
		// tree->format(to);

		CHECK_EQ(tree->indentAt(5), indentLevel(0));
		CHECK_EQ(tree->indentAt(6), indentLevel(1));
		CHECK_EQ(tree->indentAt(9), indentAs(8));
		CHECK_EQ(tree->indentAt(12), indentLevel(1));
		CHECK_EQ(tree->indentAt(13), indentLevel(2));
		CHECK_EQ(tree->indentAt(16), indentLevel(1));
		CHECK_EQ(tree->indentAt(17), indentLevel(0));
	}

	// Try negative indentation as well.
	{
		Str *src = new (e) Str(L"{\na+b;\n[\nc;\n]\n}");
		p->parse(src, new (e) Url());
		VERIFY(p->hasTree() && p->matchEnd() == src->end());

		InfoNode *tree = p->infoTree();
		CHECK_EQ(tree->indentAt(7), indentLevel(1));
		CHECK_EQ(tree->indentAt(8), indentLevel(0));
		CHECK_EQ(tree->indentAt(11), indentLevel(0));
		CHECK_EQ(tree->indentAt(12), indentLevel(1));
	}

} END_TEST
