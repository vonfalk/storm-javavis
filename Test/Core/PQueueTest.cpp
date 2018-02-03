#include "stdafx.h"
#include "Core/PQueue.h"

BEGIN_TEST(PQueueTest, Core) {
	Engine &e = gEngine();

	PQueue<Int> *pq = new (e) PQueue<Int>();

	*pq << 1 << 5 << 8 << 2 << 20;

	CHECK_EQ(pq->top(), 20); pq->pop();
	CHECK_EQ(pq->top(), 8); pq->pop();
	CHECK_EQ(pq->top(), 5); pq->pop();
	CHECK_EQ(pq->top(), 2); pq->pop();
	CHECK_EQ(pq->top(), 1); pq->pop();
	CHECK(pq->empty());

} END_TEST
