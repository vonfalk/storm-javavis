#include "stdafx.h"
#include "Test/Test.h"
#include "Shared/Map.h"

BEGIN_TEST_(MapTest) {
	Engine &e = *gEngine;

	typedef Map<Auto<Str>, Int> SIMap;
	Auto<SIMap> map = CREATE(SIMap, e);
	PLN("Map created!");

	map->put(steal(CREATE(Str, e, L"A")), 10);
	map->put(steal(CREATE(Str, e, L"B")), 11);
	map->put(steal(CREATE(Str, e, L"A")), 12);

	CHECK_EQ(map->count(), 2);

} END_TEST
