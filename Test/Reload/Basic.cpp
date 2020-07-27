#include "stdafx.h"
#include "Util.h"

BEGIN_TEST_(Basic, Reload) {
	Engine &e = gEngine();

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 2);

	Package *reload = e.package(S("tests.reload"));
	reloadFile(reload, S("a.bs"), S("Int testA(Int x) { x + 2; }"));

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 3);

} END_TEST
