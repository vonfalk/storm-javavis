#include "stdafx.h"
#include "Test/Test.h"
#include "Tracker.h"

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

static int testFn1(int p1, int p2) {
	return p1 + p2;
}

static int64 testFn2(int p1, int p2) {
	return 0x100000000LL + p1 + p2;
}

static SmallType testType1(int p1, int p2) {
	return SmallType(p1 + p2);
}

static MediumType testType2(int p1, int p2) {
	return MediumType(p1, p2);
}

static LargeType testType3(int p1, int p2) {
	return LargeType(p1, p2, p1 + p2, p1 - p2);
}

struct Dummy {
	LargeType data;

	LargeType CODECALL large() {
		return data;
	}
};

static int sum = 0;
static void testVoid(int p1, int p2) {
	sum = p1 + p2;
}

BEGIN_TEST(FnCallTest) {

	FnParams p;
	int p1 = 1, p2 = 2;
	p.add(p1);
	p.add(p2);
	CHECK_EQ(call<int>(&testFn1, false, p), 3);
	CHECK_EQ(call<int64>(&testFn2, false, p), 0x100000003LL);
	CHECK_EQ(call<SmallType>(&testType1, false, p), SmallType(3));
	CHECK_EQ(call<MediumType>(&testType2, false, p), MediumType(1, 2));
	CHECK_EQ(call<LargeType>(&testType3, false, p), LargeType(1, 2, 3, -1));
	sum = 0;
	call<void>(&testVoid, false, p);
	CHECK_EQ(sum, 3);

	Dummy dummy = { LargeType(2, 3, 4, 5) };
	FnParams q;
	q.add(&dummy);
	CHECK_EQ(call<LargeType>(address(&Dummy::large), true, q), LargeType(2, 3, 4, 5));

} END_TEST


void trackerFn(Tracker t) {
	assert(t.data == 10);
}

Tracker trackerError(Tracker t) {
	throw UserError(L"ERROR");
}

int64 variedFn(int a, int64 b, int c) {
	return a + b + c;
}

int shortFn(byte a, int b) {
	return a + b;
}

BEGIN_TEST(FunctionParamTest) {

	Tracker::clear();
	{
		FnParams p;
		Tracker t(10);
		p.add(t);
		call<void>(&trackerFn, false, p);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		FnParams p;
		Tracker t(10);
		p.add(t);
		CHECK_ERROR(call<Tracker>(&trackerError, false, p), UserError);
	}
	CHECK(Tracker::clear());

	{
		FnParams p;
		int a = 0x1, c = 0x20;
		int64 b = 0x100000000;
		p.add(a).add(b).add(c);
		CHECK_EQ(call<int64>(&variedFn, false, p), 0x100000021);
	}

	{
		FnParams p;
		byte a = 0x10;
		int b = 0x1010;
		p.add(a).add(b);
		CHECK_EQ(call<int>(&shortFn, false, p), 0x1020);
	}

} END_TEST


static Tracker returnTracker() {
	return Tracker(22);
}


BEGIN_TEST(FunctionReturnTest) {
	Tracker::clear();
	{
		Tracker t = call<Tracker>(&returnTracker, false);
		CHECK_EQ(t.data, 22);
	}
	CHECK(Tracker::clear());
} END_TEST


static float returnFloat(int i) {
	return float(i);
}

static int takeFloat(float f) {
	return int(f);
}

static double returnDouble(int i) {
	return double(i);
}

static int takeDouble(double d) {
	return int(d);
}

BEGIN_TEST(FunctionFloatTest) {

	{
		int p = 100;
		CHECK_EQ(call<float>(&returnFloat, false, FnParams().add(p)), 100.0f);
	}

	{
		float f = 100.0f;
		CHECK_EQ(call<int>(&takeFloat, false, FnParams().add(f)), 100);
	}

	{
		int p = 100;
		CHECK_EQ(call<double>(&returnDouble, false, FnParams().add(p)), 100.0);
	}

	{
		double d = 100.0f;
		CHECK_EQ(call<int>(&takeDouble, false, FnParams().add(d)), 100);
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
	FnParams par;
	par.add(&t);

	LargeType *f = &callRef<LargeType>(&refFn, false, par);
	CHECK_EQ(f, &t);
	CHECK_EQ(&callRef<LargeType>(&refFn, false, par), &t);
	CHECK_EQ(call<LargeType*>(&ptrFn, false, par), &t);

} END_TEST


BEGIN_TEST(FunctionCopyTest) {
	int a = 10;
	int b = 20;

	FnParams d;
	{
		FnParams c;
		c.add(a).add(b);
		d = c;
	}

	CHECK_EQ(call<int>(&testFn1, false, d), 30);
} END_TEST
