#include "stdafx.h"

#ifdef POSIX
#define Sleep(X) usleep((X) * 1000LL);
#endif

struct OtherData {
	os::Sema sync;
	os::Sema ready;
	Nat result;

	OtherData() : sync(0), ready(0), result(0) {}

	void run() {
		sync.down();
		Sleep(100);
		result = 201;
		ready.up();
	}
};

BEGIN_TEST(SyncTest, OS) {
	os::ThreadGroup group;
	OtherData data;

	os::Thread::spawn(util::memberVoidFn(&data, &OtherData::run), group);
	Sleep(100);
	data.sync.up();
	data.ready.down();
	CHECK_EQ(data.result, 201);

	group.join();
} END_TEST

class InlineOrder {
public:
	InlineOrder(int v) : v(v), next(null) {}
	InlineOrder *next;

	int v;

	inline bool operator <(const InlineOrder &o) const {
		return v < o.v;
	}
};

BEGIN_TEST(WaitTest, OS) {
	os::SortedInlineList<InlineOrder> l;

	InlineOrder a(10);
	InlineOrder b(20);
	InlineOrder c(30);
	InlineOrder d(40);

	l.push(&b);
	l.push(&d);
	l.push(&a);
	l.push(&c);

	CHECK_EQ(l.pop()->v, 10);
	CHECK_EQ(l.pop()->v, 20);
	CHECK_EQ(l.pop()->v, 30);
	CHECK_EQ(l.pop()->v, 40);
	CHECK(l.pop() == null);

	os::UThread::sleep(100);
} END_TEST
