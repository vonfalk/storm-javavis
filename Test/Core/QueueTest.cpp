#include "stdafx.h"
#include "Core/Queue.h"

BEGIN_TEST(QueueTest, Core) {
	Engine &e = gEngine();

	Queue<Int> *q = new (e) Queue<Int>();

	*q << 1 << 5 << 8 << 2 << 20;

	CHECK_EQ(q->top(), 1); q->pop();
	CHECK_EQ(q->top(), 5); q->pop();
	CHECK_EQ(q->top(), 8); q->pop();
	CHECK_EQ(q->top(), 2); q->pop();
	CHECK_EQ(q->top(), 20); q->pop();
	CHECK(q->empty());

} END_TEST
