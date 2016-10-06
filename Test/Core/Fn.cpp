#include "stdafx.h"
#include "Core/Fn.h"

static Int addTwo(Int v, Int w) {
	return v + w + 2;
}


BEGIN_TEST(FnFree, Core) {
	Engine &e = *gEngine;

	storm::Fn<Int, Int, Int> *fn = fnPtr(e, &addTwo);
	CHECK_EQ(fn->call(2, 3), 7);

} END_TEST
