#include "stdafx.h"
#include "Core/Array.h"
#include "Core/Str.h"

BEGIN_TEST(ArrayTest, Core) {
	Engine &e = gEngine();

	Array<Str *> *t = new (e) Array<Str *>();
	CHECK_EQ(toS(t), L"[]");

	t->push(new (e) Str(L"Hello"));
	t->push(new (e) Str(L"World"));
	CHECK_EQ(toS(t), L"[Hello, World]");

	t->insert(0, new (e) Str(L"Well"));
	CHECK_EQ(toS(t), L"[Well, Hello, World]");

	Array<Value> *v = new (e) Array<Value>();
	CHECK_EQ(toS(v), L"[]");

	v->push(Value(Str::stormType(e)));
	CHECK_EQ(toS(v), L"[core.Str]");

} END_TEST
