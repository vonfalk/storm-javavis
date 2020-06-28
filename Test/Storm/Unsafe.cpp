#include "stdafx.h"
#include "Compiler/Lib/PinnedSet.h"

BEGIN_TEST(PinnedSetTest, BS) {
	Engine &e = gEngine();

	GcArray<Nat> *data1 = runtime::allocArray<Nat>(e, &natArrayType, 10);
	GcArray<Nat> *data2 = runtime::allocArray<Nat>(e, &natArrayType, 10);

	PinnedSet *s = new (e) PinnedSet();

	s->put(&data1->v[5]);
	s->put(&data1->v[3]);
	s->put(&data1->v[5]);

	CHECK_EQ(s->has(data1), true);
	CHECK_EQ(s->has(data2), false);

	{
		CHECK_EQ(s->has(data1, 0 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data1, 0 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data1, 1 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data1, 2 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data1, 3 * sizeof(Nat)), true);
		CHECK_EQ(s->has(data1, 4 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data1, 5 * sizeof(Nat)), true);
		CHECK_EQ(s->has(data1, 6 * sizeof(Nat)), false);
	}
	{
		Array<Nat> *r = s->offsets(data1);
		CHECK_EQ(r->count(), 2);
		CHECK_EQ(r->at(0), 3 * sizeof(Nat));
		CHECK_EQ(r->at(1), 5 * sizeof(Nat));
	}
	{
		CHECK_EQ(s->has(data2, 0 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 1 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 2 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 3 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 4 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 5 * sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 6 * sizeof(Nat)), false);
	}
	{
		Array<Nat> *r = s->offsets(data2);
		CHECK_EQ(r->count(), 0);
	}

	s->put(&data2->v[8]);
	s->put(&data1->v[1]);
	s->put(&data1->v[5]);

	CHECK_EQ(s->has(data1), true);
	CHECK_EQ(s->has(data2), true);

	{
		Array<Nat> *r = s->offsets(data1);
		CHECK_EQ(r->count(), 3);
		CHECK_EQ(r->at(0), 1 * sizeof(Nat));
		CHECK_EQ(r->at(1), 3 * sizeof(Nat));
		CHECK_EQ(r->at(2), 5 * sizeof(Nat));
	}
	{
		CHECK_EQ(s->has(data2, 7 * sizeof(Nat), sizeof(Nat)), false);
		CHECK_EQ(s->has(data2, 8 * sizeof(Nat), sizeof(Nat)), true);
		CHECK_EQ(s->has(data2, 9 * sizeof(Nat), sizeof(Nat)), false);
	}
	{
		Array<Nat> *r = s->offsets(data2);
		CHECK_EQ(r->count(), 1);
		CHECK_EQ(r->at(0), 8 * sizeof(Nat));
	}

} END_TEST
