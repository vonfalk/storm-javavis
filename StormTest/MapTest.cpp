#include "stdafx.h"
#include "Test/Test.h"
#include "Shared/Map.h"
#include "Storm/Url.h"
#include "Shared/Timing.h"

#include "Utils/DetectOperator.h"

// Benchmark the map.
void benchmark();

BEGIN_TEST(MapTest) {
	Engine &e = *gEngine;

	{
		typedef Map<Auto<Str>, Int> SIMap;
		Auto<SIMap> map = CREATE(SIMap, e);

		map->put(CREATE(Str, e, L"A"), 10);
		map->put(CREATE(Str, e, L"B"), 11);
		map->put(CREATE(Str, e, L"A"), 12);
		map->put(CREATE(Str, e, L"E"), 13);

		CHECK_EQ(map->count(), 3);
		CHECK_EQ(map->get(steal(CREATE(Str, e, L"A"))), 12);

		map->remove(CREATE(Str, e, L"A"));

		CHECK_EQ(map->count(), 2);
		CHECK_EQ(map->get(steal(CREATE(Str, e, L"B"))), 11);
		CHECK_EQ(map->get(steal(CREATE(Str, e, L"E"))), 13);
	}


	{
		typedef Map<Auto<Str>, Value> SVMap;
		Auto<SVMap> map = CREATE(SVMap, e);

		map->put(CREATE(Str, e, L"A"), StrBuf::stormType(e));
		map->put(CREATE(Str, e, L"A"), Str::stormType(e));
		map->put(CREATE(Str, e, L"B"), intType(e));

		CHECK_EQ(map->count(), 2);
		CHECK_EQ(map->get(steal(CREATE(Str, e, L"A"))), Value(Str::stormType(e)));
		CHECK_EQ(map->get(steal(CREATE(Str, e, L"B"))), Value(intType(e)));
	}

	{
		typedef Map<Int, Auto<Str>> ISMap;
		Auto<ISMap> map = CREATE(ISMap, e);

		map->put(1, CREATE(Str, e, L"He"));
		map->put(1, CREATE(Str, e, L"Hello"));
		map->put(3, CREATE(Str, e, L"World"));

		CHECK_EQ(map->count(), 2);
		CHECK_OBJ_EQ(map->get(1), steal(CREATE(Str, e, L"Hello")));
		CHECK_OBJ_EQ(map->get(3), steal(CREATE(Str, e, L"World")));
	}


	// We do not need the benchmark in regular use.
	//benchmark();

} END_TEST


/**
 * Benchmarking code.
 */

nat cppCollisions(const hash_map<Int, Int> &cpp) {
	size_t r = 0;
	for (nat i = 0; i < cpp.bucket_count(); i++)
		r += max(size_t(1), cpp.bucket_size(i)) - 1;
	return nat(r);
}

nat cppLongest(const hash_map<Int, Int> &cpp) {
	size_t r = 0;
	for (nat i = 0; i < cpp.bucket_count(); i++)
		r = max(r, cpp.bucket_size(i));
	return nat(r);
}

void benchmark() {
	Engine &e = *gEngine;

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

	assert(storm->count() == cpp.size());

	PLN("Storm:");
	PLN(" unique elements: " << storm->count());
	PLN(" collisions: " << storm->collisions());
	PLN(" longest chain: " << storm->maxChain());

	PLN("C++:");
	PLN(" unique elements: " << cpp.size());
	PLN(" buckets: " << cpp.bucket_count());
	PLN(" collisions: " << cppCollisions(cpp));
	PLN(" longest chain: " << cppLongest(cpp));

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

	assert(storm->count() == cpp.size());

	PLN("Storm:");
	PLN(" insert: " << (stormInsertEnd - stormInsertStart));
	PLN(" remove: " << (stormRemoveEnd - stormRemoveStart));

	PLN("C++:");
	PLN(" insert: " << (cppInsertEnd - cppInsertStart));
	PLN(" remove: " << (cppRemoveEnd - cppRemoveStart));
}
