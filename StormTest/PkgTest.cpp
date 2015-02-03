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

	Auto<Name> name = CREATE(Name, e);
	Package *rootPkg = e.package(name);
	CHECK(rootPkg != null);

	name->add(L"lang");
	name->add(L"sto");
	CHECK_EQ(toS(name), L"lang.sto");
	Package *coreSto = e.package(name);
	CHECK(coreSto != null);

} END_TEST
