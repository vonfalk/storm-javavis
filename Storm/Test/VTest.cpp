#include "stdafx.h"
#include "VTest.h"

namespace storm {

	VTest::VTest(Type *t) : Object(t) {}
	VTest::VTest(Engine &e) : Object(type(e)) {}

	VTest::~VTest() {
	}

	Int VTest::returnOne() { return 1; }
	Int VTest::returnTwo() { return 2; }

}
