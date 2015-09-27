#include "stdafx.h"
#include "Test/Test.h"
#include "Shared/Map.h"
#include "Storm/Url.h"
#include "Shared/Timing.h"

#include "Utils/DetectOperator.h"

struct Foo {
	int a;
};

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

	{
		// Simple benchmark:
		const nat count = 1024 * 1024;
		typedef Map<Int, Int> StormMap;

		// Generate data.
		vector<Int> values(count, 0);
		for (nat i = 0; i < count; i++)
			values[i] = rand();

		Auto<StormMap> storm = CREATE(StormMap, e);
		hash_map<Int, Int> cpp;

		// Insert all of them.
		Moment stormInsertStart;
		for (nat i = 0; i < count; i++)
			storm->put(values[i], values[i]);
		Moment stormInsertEnd;

		Moment cppInsertStart;
		for (nat i = 0; i < count; i++)
			cpp.insert(make_pair(values[i], values[i]));
		Moment cppInsertEnd;

		PLN("Unique elements: " << storm->count());
		PLN("Collisions: " << storm->collisions());
		PLN("Longest chain: " << storm->maxChain());

		// Lookup and verify.
		for (nat i = 0; i < count; i++)
			CHECK_EQ(storm->get(values[i]), values[i]);

		// Remove all of them.
		Moment stormRemoveStart;
		for (nat i = 0; i < count; i++) {
			storm->remove(values[i]);
		}
		Moment stormRemoveEnd;

		Moment cppRemoveStart;
		for (nat i = 0; i < count; i++)
			cpp.erase(values[i]);
		Moment cppRemoveEnd;

		CHECK_EQ(storm->count(), cpp.size());

		PLN("Storm:");
		PLN(" insert: " << (stormInsertEnd - stormInsertStart));
		PLN(" remove: " << (stormRemoveEnd - stormRemoveStart));

		PLN("C++:");
		PLN(" insert: " << (cppInsertEnd - cppInsertStart));
		PLN(" remove: " << (cppRemoveEnd - cppRemoveStart));
	}


} END_TEST
