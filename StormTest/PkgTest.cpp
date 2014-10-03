#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/PkgPath.h"
#include "Storm/Engine.h"
#include "Storm/Exception.h"

using namespace storm;

BEGIN_TEST(PkgTest) {

	String tc1[3] = { L"a", L"b", L"c" };
	CHECK_EQ(split(L"a, b, c", L", "), vector<String>(tc1, tc1 + 3));
	CHECK_EQ(split(L", a, b, c, ", L", "), vector<String>(tc1, tc1 + 3));

	CHECK_EQ(toS(PkgPath(L"a.b.c")), L"a.b.c");
	CHECK_EQ(PkgPath(L"a.b") + PkgPath(L"c"), PkgPath(L"a.b.c"));
	CHECK_EQ(PkgPath(L"a.b.c.").parent(), PkgPath(L"a.b"));

	// Do some real things!
	Path root = Path::executable() + Path(L"../root/");
	Engine e(root);

	Package *rootPkg = e.package(PkgPath());
	CHECK(rootPkg != null);
	Package *coreSto = e.package(PkgPath(L"core.sto"));
	CHECK(coreSto != null);

} END_TEST
