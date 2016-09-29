#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(OperandTest, Code) {
	Operand op = ptrConst(Offset(-2));
	CHECK_EQ(op.constant(), Word(-2));
} END_TEST
