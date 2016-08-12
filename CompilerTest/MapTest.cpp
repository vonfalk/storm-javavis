#include "stdafx.h"
#include "Test/Test.h"
#include "Core/Map.h"
#include "Core/Str.h"

BEGIN_TEST(MapTest, Runtime) {
	Engine &e = *gEngine;

	{
		Map<Str *, Str *> *map = new (e) Map<Str *, Str *>();

		map->put(new (e) Str(L"A"), new (e) Str(L"10"));
		map->put(new (e) Str(L"B"), new (e) Str(L"11"));
		map->put(new (e) Str(L"A"), new (e) Str(L"12"));
		map->put(new (e) Str(L"E"), new (e) Str(L"13"));

		CHECK_EQ(map->count(), 3);
		CHECK_EQ(::toS(map->get(new (e) Str(L"A"))), L"12");

		map->remove(new (e) Str(L"A"));

		CHECK_EQ(map->count(), 2);
		CHECK_EQ(::toS(map->get(new (e) Str(L"B"))), L"11");
		CHECK_EQ(::toS(map->get(new (e) Str(L"E"))), L"13");
		CHECK_EQ(::toS(map->get(new (e) Str(L"A"), new (e) Str(L"-"))), L"-")
	}

} END_TEST
