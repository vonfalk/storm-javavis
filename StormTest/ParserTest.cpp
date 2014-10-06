#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"

using namespace storm;

nat parse(SyntaxSet &set, const String &root, const String &str) {
	Parser p(set, str);
	nat r = p.parse(root);
	SyntaxNode *t = p.tree();
	if (t) {
		// PLN(*t);
		delete t;
	}
	return r;
}

BEGIN_TEST(ParserTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	// Try to get the syntax for the 'sto' file format.
	Package *coreSto = engine.package(PkgPath(L"core.sto"));
	coreSto->syntax();

	// Try parsing a file with the simple syntax.
	Package *simple = engine.package(PkgPath(L"core.simple"));
	simple->syntax();

	SyntaxSet set;
	set.add(*simple);

	CHECK_EQ(parse(set, L"Root", L"a + b"), 5);
	CHECK_EQ(parse(set, L"Root", L"a + b-"), 5);
	CHECK_EQ(parse(set, L"Root", L"a + "), 1);

	CHECK_EQ(parse(set, L"Rep1Root", L"{ a; b; 1 + 2;}"), 15);
	CHECK_EQ(parse(set, L"Rep2Root", L"{ a; b; 1 + 2;}"), 15);
	CHECK_EQ(parse(set, L"Rep2Root", L"{}"), 2);
	CHECK_EQ(parse(set, L"Rep3Root", L"{ a; }"), 6);
	CHECK_EQ(parse(set, L"Rep3Root", L"{}"), 2);
	CHECK_EQ(parse(set, L"Rep3Root", L"{ a; b; }"), Parser::NO_MATCH);

	CHECK_EQ(parse(set, L"Root", L"banana + rock + 33"), 18);

} END_TEST
