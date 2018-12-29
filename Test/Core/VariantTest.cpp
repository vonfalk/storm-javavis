#include "stdafx.h"
#include "Core/Variant.h"

BEGIN_TEST(VariantTest, Core) {
	Engine &e = gEngine();

	{
		Variant empty;
		CHECK(empty.empty());
		CHECK_EQ(::toS(empty), L"<empty>");
	}

	{
		Variant s(new (e) Str(S("Hello!")));
		CHECK(s.any());
		CHECK_EQ(::toS(s), L"Hello!");

		Variant c(s);
		CHECK_EQ(::toS(s), L"Hello!");

		c.deepCopy(new (e) CloneEnv());
		CHECK_EQ(::toS(s), L"Hello!");
	}

	{
		Array<Int> *v = new (e) Array<Int>();
		v->push(1); v->push(2); v->push(3);
		Variant a(v);

		CHECK_EQ(::toS(a), L"[1, 2, 3]");

		a.deepCopy(new (e) CloneEnv());
		CHECK_EQ(::toS(a), L"[1, 2, 3]");
	}

	{
		Variant i(20, e);
		CHECK_EQ(::toS(i), L"20");

		Variant j(i);
		CHECK_EQ(::toS(i), L"20");

		j.deepCopy(new (e) CloneEnv());
		CHECK_EQ(::toS(j), L"20");
	}

} END_TEST
