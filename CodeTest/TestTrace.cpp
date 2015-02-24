#include "stdafx.h"
#include "Test/Test.h"
#include "Code/StackTrace.h"
#include "Code/FnLookup.h"

StackTrace foo() {
	PNN(""); // This prevents some optimization and gives better results on the test below.
	return code::stackTrace();
}

StackTrace bar(nat depth, bool exception) {
	if (depth == 0) {
		if (exception)
			throw UserError(L"Fail!");
		else
			return foo();
	} else {
		return bar(depth - 1, exception);
	}
}

BEGIN_TEST(TestTrace) {

	StackTrace depth1 = bar(10, false);
	StackTrace depth2 = bar(20, false);

	CHECK_EQ(depth2.count(), depth1.count() + 10);

} END_TEST
