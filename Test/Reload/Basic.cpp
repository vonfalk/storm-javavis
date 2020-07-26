#include "stdafx.h"
#include "../Storm/Fn.h"
#include "Compiler/Package.h"

BEGIN_TEST_(Basic, Reload) {
	Engine &e = gEngine();

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 2);

	Package *reload = e.package(S("tests.reload"));
	Url *url = reload->url();
	reload->reload((*url / new (e) Str(S("later")))->children());

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 2);
	CHECK_EQ(runFn<Int>(S("tests.reload.testB"), 1), 3);

	TODO(L"Use a fake file (similar to the one needed by the language server) to test changing files as well!");

} END_TEST
