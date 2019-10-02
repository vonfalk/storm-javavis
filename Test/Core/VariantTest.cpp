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

	/**
	 * Check the high-level C++ API as well!
	 */

	{
		Variant empty;

		// Should call the pointer overload.
		Variant str(new (e) Str(S("Hello!")), e);
		CHECK_EQ(::toS(str), L"Hello!");

		// Should call the template overload.
		Variant num(Int(100), e);
		CHECK_EQ(::toS(num), L"100");

		// Check contents.
		CHECK(str.has(StormInfo<Str *>::type(e)));
		CHECK(num.has(StormInfo<Int>::type(e)));

		// Extraction.
		CHECK_EQ(::toS(str.get<Str *>()), L"Hello!");
		CHECK_EQ(num.get<Int>(), 100);
		CHECK_EQ(*num.get<Int *>(), 100);
		CHECK_EQ(empty.get<Int *>(), (Int *)null);
		CHECK_EQ(str.get<Int *>(), (Int *)null);

		// Cant easily test this, as it prints.
		// CHECK_ERROR(str.get<Str>(), AssertionException);
	}

} END_TEST
