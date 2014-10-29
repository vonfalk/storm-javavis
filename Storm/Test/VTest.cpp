#include "stdafx.h"
#include "VTest.h"

namespace storm {

	VTest::VTest(Type *t) : Object(t) {}
	VTest::VTest(Engine &e) : Object(type(e)) {}

	VTest::~VTest() {
	}

	int VTest::returnOne() { return 1; }
	int VTest::returnTwo() { return 2; }

}
