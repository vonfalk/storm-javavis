#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST_(Shared) {
	Engine &e = gEngine();

	// Try to load an external library.
	CHECK_EQ(runFn<Int>(L"ui.test", 10), 11);

} END_TEST
