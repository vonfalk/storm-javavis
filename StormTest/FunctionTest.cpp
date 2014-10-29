#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Function.h"

using namespace code;

struct SmallType {
	int v;
	SmallType(int v) : v(v) {}
	bool operator ==(const SmallType &o) const { return v == o.v; }
	bool operator !=(const SmallType &o) const { return !(*this == o); }
};
wostream &operator <<(wostream &to, const SmallType &o) {
	return to << L"{ " << o.v << L" }";
}

struct MediumType {
	int v1, v2;
	MediumType(int v1, int v2) : v1(v1), v2(v2) {}
	bool operator ==(const MediumType &o) const { return v1 == o.v1 && v2 == o.v2; }
	bool operator !=(const MediumType &o) const { return !(*this == o); }
};
wostream &operator <<(wostream &to, const MediumType &o) {
	return to << L"{ " << o.v1 << L", " << o.v2 << L" }";
}

struct LargeType {
	int v1, v2, v3, v4;
	LargeType(int v1, int v2, int v3, int v4) : v1(v1), v2(v2), v3(v3), v4(v4) {}
	bool operator ==(const LargeType &o) const { return v1 == o.v1 && v2 == o.v2 && v3 == o.v3 && v4 == o.v4; }
	bool operator !=(const LargeType &o) const { return !(*this == o); }
};
wostream &operator <<(wostream &to, const LargeType &o) {
	return to << L"{ " << o.v1 << L", " << o.v2 << L", " << o.v3 << L", " << o.v4 << L" }";
}

int testFn1(int p1, int p2) {
	return p1 + p2;
}

int64 testFn2(int p1, int p2) {
	return 0x100000000LL + p1 + p2;
}

SmallType testType1(int p1, int p2) {
	return SmallType(p1 + p2);
}

MediumType testType2(int p1, int p2) {
	return MediumType(p1, p2);
}

LargeType testType3(int p1, int p2) {
	return LargeType(p1, p2, p1 + p2, p1 - p2);
}

int sum = 0;
void testVoid(int p1, int p2) {
	sum = p1 + p2;
}

BEGIN_TEST(FunctionTest) {

	FnCall call;
	int p1 = 1, p2 = 2;
	call.param(p1);
	call.param(p2);
	CHECK_EQ(call.call<int>(&testFn1), 3);
	CHECK_EQ(call.call<int64>(&testFn2), 0x100000003LL);
	CHECK_EQ(call.call<SmallType>(&testType1), SmallType(3));
	CHECK_EQ(call.call<MediumType>(&testType2), MediumType(1, 2));
	CHECK_EQ(call.call<LargeType>(&testType3), LargeType(1, 2, 3, -1));
	sum = 0;
	call.callVoid(&testVoid);
	CHECK_EQ(sum, 3);

} END_TEST
