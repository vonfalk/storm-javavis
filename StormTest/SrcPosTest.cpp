#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/SrcPos.h"

using namespace storm;

BEGIN_TEST(SrcPosTest) {
	Auto<Url> testData = dbgRootUrl(*gEngine);
	testData = testData->push(L"TestData");
	Auto<Url> testFile = testData->push(L"SrcPos.txt");

	CHECK_EQ(SrcPos(testFile, 10).lineCol(), LineCol(0, 10));
	CHECK_EQ(SrcPos(testFile, 16).lineCol(), LineCol(0, 16));
	CHECK_EQ(SrcPos(testFile, 18).lineCol(), LineCol(1,  0));
	CHECK_EQ(SrcPos(testFile, 28).lineCol(), LineCol(1, 10));
	CHECK_EQ(SrcPos(testFile, 50).lineCol(), LineCol(2, 10));
	CHECK_EQ(SrcPos(testFile, 73).lineCol(), LineCol(3, 10));

} END_TEST
