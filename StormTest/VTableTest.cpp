#include "stdafx.h"
#include "Test/Lib/Test.h"

#include "Storm/Lib/Debug.h"
#include "Code/VTable.h"

Int CODECALL replaceOne(Dbg *me) {
	return 3;
}

Int CODECALL replaceTwo(Dbg *me) {
	return 4;
}

BEGIN_TEST(VTableTest) {
	Engine &engine = *gEngine;

	code::VTable table(Dbg::cppVTable());

	Auto<Dbg> test = CREATE(Dbg, engine);
	table.setTo(test.borrow());
	CHECK_EQ(test->returnOne(), 1);
	CHECK_EQ(test->returnTwo(), 2);
	table.set(address(&Dbg::returnOne), address(&replaceOne));
	table.set(address(&Dbg::returnTwo), address(&replaceTwo));
	CHECK_EQ(test->returnOne(), 3);
	CHECK_EQ(test->returnTwo(), 4);

} END_TEST
