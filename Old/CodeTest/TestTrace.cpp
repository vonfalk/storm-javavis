#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Utils/StackTrace.h"
#include "Code/FnLookup.h"

StackTrace foo() {
	PNN(""); // This prevents some optimization and gives better results on the test below.
	return stackTrace();
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

	CHECK(depth2.count() > depth1.count() + 5);

} END_TEST
