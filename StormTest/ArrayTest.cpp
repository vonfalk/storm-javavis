#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(ArrayTest) {
	Engine &e = *gEngine;

	DbgVal::clear();

	Auto<Array<DbgVal> > array = new (ArrayBase::type(e)) Array<DbgVal>();

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

	array->clear();

	CHECK(DbgVal::clear());

} END_TEST
