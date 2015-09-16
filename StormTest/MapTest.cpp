#include "stdafx.h"
#include "Test/Test.h"
#include "Shared/Map.h"

BEGIN_TEST_(MapTest) {
	Engine &e = *gEngine;

	typedef Map<Auto<Str>, Int> SIMap;
	Auto<SIMap> map = CREATE(SIMap, e);

	map->put(CREATE(Str, e, L"A"), 10);
	map->put(CREATE(Str, e, L"B"), 11);
	map->put(CREATE(Str, e, L"A"), 12);
	map->put(CREATE(Str, e, L"C"), 13);

	map->dbg_print();

	CHECK_EQ(map->count(), 3);

} END_TEST
