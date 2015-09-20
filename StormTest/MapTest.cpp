#include "stdafx.h"
#include "Test/Test.h"
#include "Shared/Map.h"
#include "Storm/Url.h"

BEGIN_TEST_(MapTest) {
	Engine &e = *gEngine;

	{
		typedef Map<Auto<Str>, Int> SIMap;
		Auto<SIMap> map = CREATE(SIMap, e);

		map->put(CREATE(Str, e, L"A"), 10);
		map->put(CREATE(Str, e, L"B"), 11);
		map->put(CREATE(Str, e, L"A"), 12);
		map->put(CREATE(Str, e, L"E"), 13);

		map->dbg_print();
		PVAR(map);

		map->remove(CREATE(Str, e, L"A"));

		map->dbg_print();
		PVAR(map);

		CHECK_EQ(map->count(), 2);
	}


	{
		typedef Map<Auto<Str>, Value> SVMap;
		Auto<SVMap> map = CREATE(SVMap, e);

		map->put(CREATE(Str, e, L"A"), StrBuf::stormType(e));
		PVAR(map);
	}

	{
		typedef Map<Auto<Str>, Auto<Url>> SVMap;
		Auto<SVMap> map = CREATE(SVMap, e);

		map->put(CREATE(Str, e, L"A"), rootUrl(e));
		PVAR(map);
	}


} END_TEST
