#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(ArrayTest) {
	Engine &e = *gEngine;

	DbgVal::clear();

	// Auto<Array<DbgVal> > array = new (ArrayBase::stormType(e)) Array<DbgVal>();
	Auto<Array<DbgVal>> array = CREATE(Array<DbgVal>, e);

	for (nat i = 0; i < 30; i++)
		array->push(DbgVal(i));

	CHECK_EQ(array->count(), 30);

	for (nat i = 0; i < 30; i++) {
		CHECK_EQ(array->at(i).get(), i);
		array->at(i) = DbgVal(i + 2);
	}

	for (nat i = 0; i < 30; i++) {
		CHECK_EQ(array->at(i).get(), i + 2);
	}

	// Try the iterator as well!
	{
		nat count = 2;
		for (Array<DbgVal>::Iter i = array->begin(); i != array->end(); ++i) {
			CHECK_EQ(i->get(), count++);
		}
	}

	array->clear();

	CHECK(DbgVal::clear());


	// Reverse it as well!
	for (nat i = 0; i < 10; i++)
		array->push(DbgVal(i));

	array->reverse();

	for (nat i = 0; i < 10; i++)
		CHECK_EQ(array->at(i).get(), 9 - i);

	array->clear();

	CHECK(DbgVal::clear());

} END_TEST


BEGIN_TEST(StormArrayTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testArray"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.testValArray"), 250);
	CHECK_EQ(runFn<Int>(L"test.bs.testIntArray"), 95);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitArray"), 1337);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitAutoArray"), 1234);
	CHECK_EQ(runFn<Int>(L"test.bs.testAutoArray"), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testCastArray"), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.testIterator"), 15);
	CHECK_EQ(runFn<Int>(L"test.bs.testIteratorIndex"), 16);

	// Interoperability.
	Auto<Array<DbgVal>> r = runFn<Array<DbgVal>*>(L"test.bs.createValArray");
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i).get(), i);
	}

} END_TEST

