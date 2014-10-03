#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"

using namespace storm;

BEGIN_TEST(ParserTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	// Try to get the syntax for the 'sto' file format.
	Package *coreSto = engine.package(PkgPath(L"core.sto"));
	coreSto->syntax();

	// Try parsing a file with the simple syntax.
	Package *simple = engine.package(PkgPath(L"core.simple"));
	simple->syntax();

	SyntaxSet parser;
	parser.add(*simple);
	CHECK_EQ(parser.parse(L"Root", L"a + b"), 5);
	CHECK_EQ(parser.parse(L"Root", L"a + b-"), 5);
	CHECK_EQ(parser.parse(L"Root", L"a + "), 1);

} END_TEST
