#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Name.h"
#include "Storm/Engine.h"
#include "Storm/Exception.h"

using namespace storm;

BEGIN_TEST(PkgTest) {

	String tc1[3] = { L"a", L"b", L"c" };
	String tc2[4] = { L"a", L"b", L"", L"c" };
	CHECK_EQ(split(L"a, b, c", L", "), vector<String>(tc1, tc1 + 3));
	CHECK_EQ(split(L"a, b, , c", L", "), vector<String>(tc2, tc2 + 4));

	CHECK_EQ(toS(Name(L"a.b.c")), L"a.b.c");
	CHECK_EQ(Name(L"a.b") + Name(L"c"), Name(L"a.b.c"));
	CHECK_EQ(Name(L"a.b.c.").parent(), Name(L"a.b"));

	// Do some real things!
	Path root = Path::executable() + Path(L"../root/");
	Engine e(root);

	Package *rootPkg = e.package(Name());
	CHECK(rootPkg != null);
	Package *coreSto = e.package(Name(L"lang.sto"));
	CHECK(coreSto != null);

} END_TEST
