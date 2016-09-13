#include "stdafx.h"
#include "Test/Test.h"
#include "Core/Array.h"
#include "Core/Str.h"

BEGIN_TEST(ArrayTest, Core) {
	Engine &e = *gEngine;

	Array<Str *> *t = new (e) Array<Str *>();
	CHECK_EQ(toS(t), L"[]");

	t->push(new (e) Str(L"Hello"));
	t->push(new (e) Str(L"World"));
	CHECK_EQ(toS(t), L"[Hello, World]");

} END_TEST
