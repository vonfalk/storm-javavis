#include "stdafx.h"
#include "Compiler/Lib/PinnedSet.h"

BEGIN_TEST(PinnedSetTest, BS) {
	Engine &e = gEngine();

	GcArray<Nat> *data1 = runtime::allocArray<Nat>(e, &natArrayType, 10);
	GcArray<Nat> *data2 = runtime::allocArray<Nat>(e, &natArrayType, 10);

	PinnedSet *s = new (e) PinnedSet();

	s->add(&data1->v[5]);
	s->add(&data1->v[3]);
	s->add(&data1->v[5]);

	CHECK_EQ(s->has(data1), true);
	CHECK_EQ(s->has(data2), false);

	{
		Array<Nat> *r = s->offsets(data1);
		CHECK_EQ(r->count(), 2);
		CHECK_EQ(r->at(0), OFFSET_OF(GcArray<Nat>, v[3]));
		CHECK_EQ(r->at(1), OFFSET_OF(GcArray<Nat>, v[5]));
	}
	{
		Array<Nat> *r = s->offsets(data2);
		CHECK_EQ(r->count(), 0);
	}

	s->add(&data2->v[8]);
	s->add(&data1->v[1]);
	s->add(&data1->v[5]);

	CHECK_EQ(s->has(data1), true);
	CHECK_EQ(s->has(data2), true);

		{
		Array<Nat> *r = s->offsets(data1);
		CHECK_EQ(r->count(), 3);
		CHECK_EQ(r->at(0), OFFSET_OF(GcArray<Nat>, v[1]));
		CHECK_EQ(r->at(1), OFFSET_OF(GcArray<Nat>, v[3]));
		CHECK_EQ(r->at(2), OFFSET_OF(GcArray<Nat>, v[5]));
	}
	{
		Array<Nat> *r = s->offsets(data2);
		CHECK_EQ(r->count(), 1);
		CHECK_EQ(r->at(0), OFFSET_OF(GcArray<Nat>, v[8]));
	}

} END_TEST
