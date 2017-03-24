#include "stdafx.h"
#include "Compiler/Server/RangeSet.h"

using namespace storm::server;

BEGIN_TEST(RangeTest) {
	Engine &e = gEngine();

	RangeSet *s = new (e) RangeSet();
	s->insert(Range(100, 150));
	s->insert(Range(200, 250));
	s->insert(Range(10, 20));
	CHECK_EQ(::toS(s), L"[(10 - 20), (100 - 150), (200 - 250)]");

	s->insert(Range(140, 170));
	CHECK_EQ(::toS(s), L"[(10 - 20), (100 - 170), (200 - 250)]");

	s->insert(Range(90, 110));
	CHECK_EQ(::toS(s), L"[(10 - 20), (90 - 170), (200 - 250)]");

	s->insert(Range(80, 90));
	CHECK_EQ(::toS(s), L"[(10 - 20), (80 - 170), (200 - 250)]");

	s->insert(Range(1, 2));
	CHECK_EQ(::toS(s), L"[(1 - 2), (10 - 20), (80 - 170), (200 - 250)]");

	s->insert(Range(250, 300));
	CHECK_EQ(::toS(s), L"[(1 - 2), (10 - 20), (80 - 170), (200 - 300)]");

	CHECK(!s->has(0));
	CHECK(s->has(1));
	CHECK(!s->has(2));
	CHECK(s->has(210));
	CHECK(!s->has(300));

	CHECK_EQ(s->cover(1), Range(1, 2));
	CHECK_EQ(s->cover(90), Range(80, 170));

	CHECK_EQ(s->cover(Range(90, 200)), Range(80, 300));

	CHECK_EQ(s->nearest(49), Range(10, 20));
	CHECK_EQ(s->nearest(51), Range(80, 170));

	s->remove(Range(40, 80)); // Nothing shall be removed.
	CHECK_EQ(::toS(s), L"[(1 - 2), (10 - 20), (80 - 170), (200 - 300)]");

	s->remove(Range(0, 4));
	CHECK_EQ(::toS(s), L"[(10 - 20), (80 - 170), (200 - 300)]");

	s->remove(Range(15, 90));
	CHECK_EQ(::toS(s), L"[(10 - 15), (90 - 170), (200 - 300)]");

	s->remove(Range(10, 15));
	CHECK_EQ(::toS(s), L"[(90 - 170), (200 - 300)]");

	s->remove(Range(90, 100));
	CHECK_EQ(::toS(s), L"[(100 - 170), (200 - 300)]");

	s->remove(Range(0, 400));
	CHECK_EQ(::toS(s), L"[]");

} END_TEST
