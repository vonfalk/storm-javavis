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
	call.call<void>(&testVoid);
	CHECK_EQ(sum, 3);

} END_TEST


class Tracker {
public:
	Tracker(int data) : data(data) { copies = 0; }
	~Tracker() { copies--; }
	Tracker(const Tracker &o) : data(o.data) { copies++; }

	int data;

	static int copies;
};

int Tracker::copies = 0;

void trackerFn(Tracker t) {
	assert(t.data == 10);
}

int64 variedFn(int a, int64 b, int c) {
	return a + b + c;
}

int shortFn(byte a, int b) {
	return a + b;
}

BEGIN_TEST(FunctionParamTest) {

	{
		FnCall call;
		Tracker t(10);
		call.param(t);
		call.call<void>(&trackerFn);
		CHECK_EQ(Tracker::copies, 0);
	}

	{
		FnCall call;
		int a = 0x1, c = 0x20;
		int64 b = 0x100000000;
		call.param(a).param(b).param(c);
		CHECK_EQ(call.call<int64>(&variedFn), 0x100000021);
	}

	{
		FnCall call;
		byte a = 0x10;
		int b = 0x1010;
		call.param(a).param(b);
		CHECK_EQ(call.call<int>(&shortFn), 0x1020);
	}

} END_TEST


float returnFloat(int i) {
	return float(i);
}

int takeFloat(float f) {
	return int(f);
}

double returnDouble(int i) {
	return double(i);
}

int takeDouble(double d) {
	return int(d);
}

BEGIN_TEST(FunctionFloatTest) {

	{
		FnCall call;
		int p = 100;
		CHECK_EQ(call.param(p).call<float>(&returnFloat), 100.0f);
	}

	{
		FnCall call;
		float f = 100.0f;
		CHECK_EQ(call.param(f).call<int>(&takeFloat), 100);
	}

	{
		FnCall call;
		int p = 100;
		CHECK_EQ(call.param(p).call<double>(&returnDouble), 100.0);
	}

	{
		FnCall call;
		double d = 100.0f;
		CHECK_EQ(call.param(d).call<int>(&takeDouble), 100);
	}

} END_TEST

LargeType &refFn(LargeType *t) {
	return *t;
}

LargeType *ptrFn(LargeType *t) {
	return t;
}

BEGIN_TEST(FunctionRefPtrTest) {

	LargeType t(1, 2, 3, 4);
	FnCall call;
	call.param(&t);

	LargeType *f = &call.callRef<LargeType>(&refFn);
	CHECK_EQ(f, &t);
	CHECK_EQ(&call.callRef<LargeType>(&refFn), &t);
	CHECK_EQ(call.call<LargeType*>(&ptrFn), &t);

} END_TEST


BEGIN_TEST(FunctionCopyTest) {
	int a = 10;
	int b = 20;

	FnCall d;
	{
		FnCall c;
		c.param(a).param(b);
		d = c;
	}

	CHECK_EQ(d.call<int>(&testFn1), 30);
} END_TEST
