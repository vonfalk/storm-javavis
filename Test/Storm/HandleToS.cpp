#include "stdafx.h"


BEGIN_TEST(HandleToS, Storm) {
	Engine &e = *gEngine;

	Array<Int> *iArr = new (e) Array<Int>();
	iArr->push(1);
	iArr->push(2);
	iArr->push(3);
	iArr->push(4);
	CHECK_EQ(::toS(iArr), L"[1, 2, 3, 4]");

	iArr->clear();
	iArr->push(10);
	CHECK_EQ(::toS(iArr), L"[10]");

	Array<Value> *vArr = new (e) Array<Value>();
	vArr->push(Value(StrBuf::stormType(e)));
	CHECK_EQ(::toS(vArr), L"[core.StrBuf]");

	Array<Str *> *sArr = new (e) Array<Str *>();
	sArr->push(new (e) Str(L"Hello"));
	sArr->push(new (e) Str(L"World"));
	CHECK_EQ(::toS(sArr), L"[Hello, World]");

} END_TEST
