#include "stdafx.h"
#include "Core/Array.h"
#include "Core/Str.h"

#include "Core/Random.h"
#include "Core/Timing.h"

BEGIN_TEST(ArrayTest, Core) {
	Engine &e = gEngine();

	Array<Str *> *t = new (e) Array<Str *>();
	CHECK_EQ(toS(t), L"[]");

	t->push(new (e) Str(L"Hello"));
	t->push(new (e) Str(L"World"));
	CHECK_EQ(toS(t), L"[Hello, World]");

	t->insert(0, new (e) Str(L"Well"));
	CHECK_EQ(toS(t), L"[Well, Hello, World]");

	t->append(t);
	CHECK_EQ(toS(t), L"[Well, Hello, World, Well, Hello, World]");

} END_TEST

BEGIN_TEST(ArrayExTest, CoreEx) {
	Engine &e = gEngine();

	// Check automatically generated toS functions.

	Array<Value> *v = new (e) Array<Value>();
	CHECK_EQ(toS(v), L"[]");

	v->push(Value(Str::stormType(e)));
	CHECK_EQ(toS(v), L"[core.Str]");

} END_TEST


BEGIN_TEST(ArraySortTest, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	for (Int i = 0; i < 10; i++)
		*v << (10 - i);

	v->sort();
	CHECK_EQ(toS(v), L"[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");
} END_TEST


// Performance of sort()
BEGIN_TESTX(ArraySortPerf, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	vector<Int> s;
	for (Int i = 0; i < 100000; i++) {
		Int val = rand(0, 1000000);
		*v << val;
		s.push_back(val);
	}

	Moment start;
	v->sort();
	Moment half;
	std::sort(s.begin(), s.end());
	Moment end;

	PLN(L"Sorted " << v->count() << L" integers:");
	PLN(L"  " << (half - start) << L" using storm::sort");
	PLN(L"  " << (end - half)   << L" using std::sort");
} END_TEST
