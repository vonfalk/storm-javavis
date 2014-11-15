#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Test/VTest.h"
#include "Code/VTable.h"

Int CODECALL replaceOne(VTest *me) {
	return 3;
}

Int CODECALL replaceTwo(VTest *me) {
	return 4;
}

BEGIN_TEST(VTableTest) {
	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);
	code::VTable table(VTest::cppVTable());

	Auto<VTest> test = CREATE(VTest, engine);
	table.setTo(test.borrow());
	CHECK_EQ(test->returnOne(), 1);
	CHECK_EQ(test->returnTwo(), 2);
	table.set(address(&VTest::returnOne), address(&replaceOne));
	table.set(address(&VTest::returnTwo), address(&replaceTwo));
	CHECK_EQ(test->returnOne(), 3);
	CHECK_EQ(test->returnTwo(), 4);

} END_TEST
