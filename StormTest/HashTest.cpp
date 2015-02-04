#include "stdafx.h"
#include "Test/Test.h"

// Check so that hash maps can handle Auto<> correctly. Otherwise, things will break quickly.
BEGIN_TEST(HashTest) {
	hash_map<Auto<Name>, String> c;

	Auto<Name> n = parseSimpleName(*gEngine, L"a.b.c.d");
	c.insert(make_pair(n, L"A"));

	n = parseSimpleName(*gEngine, L"c.d.e.f");
	c.insert(make_pair(n, L"B"));

	CHECK_EQ(c[n], L"B");

	n = parseSimpleName(*gEngine, L"a.b.c.d");
	CHECK_EQ(c[n], L"A");

	c.insert(make_pair(n, L"Z"));

	n = parseSimpleName(*gEngine, L"a.b.c.d");
	CHECK_EQ(c[n], L"A");

} END_TEST
