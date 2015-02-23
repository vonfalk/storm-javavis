#include "stdafx.h"
#include "Test/Test.h"
#include "Code/StackTrace.h"

namespace test {
	void bar() {
		PLN(format(code::stackTrace()));
	}
}

struct Foo {};

StackTrace bar(nat depth) {
	if (depth == 0)
		return code::stackTrace();
	else
		return bar(depth - 1);
}

BEGIN_TEST_(TestTrace) {

	StackTrace depth1 = bar(10);
	StackTrace depth2 = bar(20);

	CHECK_EQ(depth2.count(), depth1.count() + 10);

	PLN(format(depth1));
	test::bar();

} END_TEST
