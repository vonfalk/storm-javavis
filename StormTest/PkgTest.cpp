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

	// Do some real things!
	Engine &e = *gEngine;

	Auto<SimpleName> name = CREATE(SimpleName, e);
	Package *rootPkg = e.package(name);
	CHECK(rootPkg != null);

	name->add(L"lang");
	name->add(L"bs");
	CHECK_EQ(toS(name), L"lang.bs");
	Package *coreBs = e.package(name);
	CHECK(coreBs != null);

} END_TEST
