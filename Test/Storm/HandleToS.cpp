#include "stdafx.h"


BEGIN_TEST(HandleToS, Storm) {
	Engine &e = *gEngine;

	Array<Int> *iArr = new (e) Array<Int>();
	iArr->push(1);
	iArr->push(2);
	iArr->push(3);
	iArr->push(4);
	CHECK_EQ(::toS(iArr), L"[1, 2, 3, 4]");

} END_TEST
