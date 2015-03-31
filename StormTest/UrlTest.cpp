#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Io/Url.h"

BEGIN_TEST(UrlTest) {

	Engine &e = *gEngine;
	Auto<Url> root = parsePath(e, L"C:/Home/Dev/");
	Auto<Url> other = parsePath(e, L"C:/home/dev/foo/bar.txt");
	Auto<Url> rel = other->relative(root);
	PVAR(root);
	PVAR(rel);

	TODO(L"Make actual tests here!");

} END_TEST
