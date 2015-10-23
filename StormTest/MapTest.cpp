#include "stdafx.h"
#include "Fn.h"
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

Array<Int> *createKeys() {
	Array<Int> *keys = CREATE(Array<Int>, *gEngine);
	int v = 20;
	for (int i = 0; i < 20; i++) {
		keys->push(v);
		v = (v * 19) % 23;
	}
	return keys;
}

Array<Int> *createValues() {
	Array<Int> *values = CREATE(Array<Int>, *gEngine);
	for (int i = 0; i < 20; i++) {
		values->push(i);
	}
	return values;
}

BEGIN_TEST(StormMapTest) {
	Engine &e = *gEngine;
	typedef Map<Int, Int> IIMap;

	Auto<Array<Int>> keys = createKeys();
	Auto<Array<Int>> values = createValues();

	// Check if it works if we're creating the map in C++ and filling it from Storm.
	{
		Auto<Map<Int, Int>> map = CREATE(IIMap, e);
		runFn<void>(L"test.bs.intMapAdd", map.borrow(), keys.borrow(), values.borrow());

		for (nat i = 0; i < keys->count(); i++) {
			CHECK_EQ(map->get(keys->at(i)), values->at(i));
		}
	}

	// Check if we can create a map in Storm and fill it there to use it in C++ later.
	{
		Auto<Map<Int, Int>> map = runFn<Map<Int, Int> *>(L"test.bs.intMapTest", keys.borrow(), values.borrow());

		for (nat i = 0; i < keys->count(); i++) {
			CHECK_EQ(map->get(keys->at(i)), values->at(i));
		}
	}


	// Try to get integers from a map.
	{
		Auto<Map<Int, Int>> map = CREATE(IIMap, e);
		map->put(1, 10);
		map->put(2, 12);
		map->put(5, 11);

		CHECK_EQ(runFn<Int>(L"test.bs.readIntMap", map.borrow(), 1), 10);
		CHECK_EQ(runFn<Int>(L"test.bs.readIntMap", map.borrow(), 2), 12);
		CHECK_EQ(runFn<Int>(L"test.bs.readIntMap", map.borrow(), 3), 0);
		CHECK_EQ(runFn<Int>(L"test.bs.readIntMap", map.borrow(), 5), 11);
	}

	// Use the [] operator.
	{
		CHECK_EQ(runFn<Int>(L"test.bs.addMap"), 20);
	}

} END_TEST

BEGIN_TEST(StormMapObjTest) {
	Engine &e = *gEngine;
	typedef Map<Auto<Str>, Auto<Str>> SSMap;

	Auto<ArrayP<Str>> keys = CREATE(ArrayP<Str>, e);
	keys->push(CREATE(Str, e, L"A"));
	keys->push(CREATE(Str, e, L"B"));
	keys->push(CREATE(Str, e, L"C"));
	keys->push(CREATE(Str, e, L"D"));

	Auto<ArrayP<Str>> vals = CREATE(ArrayP<Str>, e);
	vals->push(CREATE(Str, e, L"Z"));
	vals->push(CREATE(Str, e, L"W"));
	vals->push(CREATE(Str, e, L"A"));
	vals->push(CREATE(Str, e, L"Q"));

	// Create a map in C++ and fill it in Storm?
	{
		Auto<SSMap> map = CREATE(SSMap, e);
		runFn<void>(L"test.bs.strMapAdd", map.borrow(), keys.borrow(), vals.borrow());

		for (nat i = 0; i < keys->count(); i++) {
			CHECK_EQ(map->get(keys->at(i))->v, vals->at(i)->v);
		}
	}

	// Create a map in Storm and use in C++?
	{
		Auto<SSMap> map = runFn<SSMap *>(L"test.bs.strMapTest", keys.borrow(), vals.borrow());

		for (nat i = 0; i < keys->count(); i++) {
			CHECK_EQ(map->get(keys->at(i))->v, vals->at(i)->v);
		}
	}

	{
		Auto<SSMap> map = CREATE(SSMap, e);
		for (nat i = 0; i < keys->count(); i++)
			map->put(keys->at(i), vals->at(i));

		CHECK_EQ(steal(runFn<Str *>(L"test.bs.readStrMap", map.borrow(), keys->at(0).borrow()))->v, L"Z");
		CHECK_EQ(steal(runFn<Str *>(L"test.bs.readStrMap", map.borrow(), keys->at(1).borrow()))->v, L"W");
		CHECK_EQ(steal(runFn<Str *>(L"test.bs.readStrMap", map.borrow(), keys->at(2).borrow()))->v, L"A");
		CHECK_EQ(steal(runFn<Str *>(L"test.bs.readStrMap", map.borrow(), keys->at(3).borrow()))->v, L"Q");
		CHECK_EQ(steal(runFn<Str *>(L"test.bs.readStrMap", map.borrow(), steal(CREATE(Str, e, L"Z")).borrow()))->v, L"");
	}

	// Use the [] operator.
	{
		CHECK_EQ(runFn<Int>(L"test.bs.addMapStr"), 25);
	}

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
