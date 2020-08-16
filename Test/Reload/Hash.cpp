#include "stdafx.h"
#include "Compiler/Named.h"
#include "Compiler/ReplaceContext.h"
#include "Core/WeakSet.h"
#include "Core/Set.h"
#include "Core/Map.h"

static Nat count(WeakSet<Named> *set) {
	Nat r = 0;
	WeakSet<Named>::Iter i = set->iter();
	while (i.next())
		r++;
	return r;
}

BEGIN_TEST(HashWeakSet, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	WeakSet<Named> *weakSet = new (e) WeakSet<Named>();
	weakSet->put(first);
	weakSet->put(second);

	CHECK_EQ(count(weakSet), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK_EQ(weakSet->has(first), true);
	CHECK_EQ(weakSet->has(second), false); // This will trigger a re-hash. 'second' may linger until this point.

	CHECK_EQ(count(weakSet), 1);
} END_TEST

BEGIN_TEST(RemoveWeakSet, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	WeakSet<Named> *weakSet = new (e) WeakSet<Named>();
	weakSet->put(first);
	weakSet->put(second);

	CHECK_EQ(count(weakSet), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK(weakSet->remove(first));
	CHECK_EQ(count(weakSet), 0);
	CHECK_EQ(weakSet->has(first), false);
} END_TEST

BEGIN_TEST(HashSet, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	Set<Named *> *set = new (e) Set<Named *>();
	set->put(first);
	set->put(second);

	CHECK_EQ(set->count(), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK_EQ(set->has(first), true);
	CHECK_EQ(set->has(second), false); // This will trigger a re-hash. 'second' may linger until this point.

	CHECK_EQ(set->count(), 1);

} END_TEST

BEGIN_TEST(RemoveSet, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	Set<Named *> *set = new (e) Set<Named *>();
	set->put(first);
	set->put(second);

	CHECK_EQ(set->count(), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK(set->remove(first));
	CHECK_EQ(set->count(), 0);
	CHECK_EQ(set->has(first), false);
} END_TEST

BEGIN_TEST(HashMap, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	Map<Named *, Int> *map = new (e) Map<Named *, Int>();
	map->put(first, 1);
	map->put(second, 1);

	CHECK_EQ(map->count(), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK_EQ(map->has(first), true);
	CHECK_EQ(map->has(second), false); // This will trigger a re-hash. 'second' may linger until this point.

	CHECK_EQ(map->count(), 1);

} END_TEST

BEGIN_TEST(RemoveMap, Reload) {
	Engine &e = gEngine();

	Named *first = new (e) Named(new (e) Str(S("A")));
	Named *second = new (e) Named(new (e) Str(S("B")));
	Map<Named *, Int> *map = new (e) Map<Named *, Int>();
	map->put(first, 1);
	map->put(second, 1);

	CHECK_EQ(map->count(), 2);

	ReplaceTasks *replace = new (e) ReplaceTasks();
	replace->replace(second, first);
	replace->apply();

	CHECK(map->remove(first));
	CHECK_EQ(map->count(), 0);
	CHECK_EQ(map->has(first), false);

} END_TEST
