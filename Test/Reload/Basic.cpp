#include "stdafx.h"
#include "../Storm/Fn.h"
#include "Compiler/Package.h"
#include "Compiler/UrlWithContents.h"

BEGIN_TEST_(Basic, Reload) {
	Engine &e = gEngine();

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 2);

	Package *reload = e.package(S("tests.reload"));
	Url *url = (*reload->url() / new (e) Str(S("a.bs")));

	const wchar *replace = S("Int testA(Int x) { x + 2; }");
	reload->reload(new (e) UrlWithContents(url, new (e) Str(replace)));

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 3);

} END_TEST
