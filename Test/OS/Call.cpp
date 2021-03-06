#include "stdafx.h"
#include "Tracker.h"
#include "OS/FnCall.h"

using namespace os;

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

struct HugeType {
	int v1, v2, v3, v4, v5, v6;
	HugeType(int v1, int v2, int v3, int v4, int v5, int v6) : v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6) {}
	bool operator ==(const HugeType &o) const { return v1 == o.v1 && v2 == o.v2 && v3 == o.v3 && v4 == o.v4 && v5 == o.v5 && v6 == o.v6; }
	bool operator !=(const HugeType &o) const { return !(*this == o); }
};
wostream &operator <<(wostream &to, const HugeType &o) {
	return to << L"{ " << o.v1 << L", " << o.v2 << L", " << o.v3 << L", " << o.v4 << L", " << o.v5 << L", " << o.v6 << L" }";
}

struct SmallComplexType {
	int v;
	SmallComplexType(int v) : v(v) {}
	bool operator ==(const SmallComplexType &o) const { return v == o.v; }
	bool operator !=(const SmallComplexType &o) const { return !(*this == o); }

	// Use a copy constructor to enforce explicit copies.
	SmallComplexType(const SmallComplexType &o) : v(o.v) {}
};
wostream &operator <<(wostream &to, const SmallComplexType &o) {
	return to << L"{ " << o.v << L" }";
}

static int testFnBool(int p1, int p2) {
	return (p1 + p2) == 20;
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

static HugeType testType4(int p1, int p2) {
	return HugeType(p1, p2, p1 + p2, p1 - p2, p1 * 2, p2 * 2);
}

static SmallComplexType testType5(int p1, int p2) {
	return SmallComplexType(p1 + p2);
}

struct Dummy {
	LargeType data;

	Dummy(LargeType d) : data(d) {}

	LargeType CODECALL large() {
		assert(data.v1 == 2);
		assert(data.v2 == 3);
		assert(data.v3 == 4);
		assert(data.v4 == 5);
		return data;
	}

	virtual LargeType CODECALL virtualLarge() {
		assert(data.v1 == 2);
		assert(data.v2 == 3);
		assert(data.v3 == 4);
		assert(data.v4 == 5);
		return data;
	}

	void CODECALL voidMember(int a, int b) {
		assert((size_t)this == 10);
		assert(a == 20);
		assert(b == 30);
	}
};


static int sum = 0;
static void testVoid(int p1, int p2) {
	sum = p1 + p2;
}

BEGIN_TEST(FnCallTest, OS) {
	int p1 = 1, p2 = 2;

	{
		FnCall<int> p = fnCall().add(p1).add(p2);
		CHECK_EQ(p.call(address(&testFn1), false), 3);
	}

	{
		FnCall<int64> p = fnCall().add(p1).add(p2);
		CHECK_EQ(p.call(address(&testFn2), false), 0x100000003LL);
	}

	{
		FnCall<SmallType> p = fnCall().add(p1).add(p2);
		CHECK_EQ(p.call(address(&testType1), false), SmallType(3));
	}

	{
		FnCall<MediumType> p = fnCall().add(p1).add(p2);
		CHECK_EQ(p.call(address(&testType2), false), MediumType(1, 2));
	}

	{
		FnCall<LargeType> p = fnCall().add(p1).add(p2);
		CHECK_EQ(p.call(address(&testType3), false), LargeType(1, 2, 3, -1));
	}

	{
		sum = 0;
		FnCall<void> p = fnCall().add(p1).add(p2);
		p.call(address(&testVoid), false);
		CHECK_EQ(sum, 3);
	}

	{
		Dummy dummy(LargeType(2, 3, 4, 5));
		Dummy *pDummy = &dummy;
		FnCall<LargeType> p = fnCall().add(pDummy);
		CHECK_EQ(p.call(address(&Dummy::large), true), LargeType(2, 3, 4, 5));
	}

	{
		Dummy dummy(LargeType(2, 3, 4, 5));
		Dummy *pDummy = &dummy;
		FnCall<LargeType> p = fnCall().add(pDummy);
		CHECK_EQ(p.call(address(&Dummy::virtualLarge), true), LargeType(2, 3, 4, 5));
	}

	{
		void *a = (void *)10;
		int b = 20;
		int c = 30;
		FnCall<void> p = fnCall().add(a).add(b).add(c);
		CHECK_RUNS(p.call(address(&Dummy::voidMember), true));
	}

	{
		// This should be equivalent to the one above. We're just storing the 'this' pointer separately.
		void *a = (void *)10;
		int b = 20;
		int c = 30;
		FnCall<void> p = fnCall().add(b).add(c);
		CHECK_RUNS(p.callRaw(address(&Dummy::voidMember), true, a, null));
	}

} END_TEST


void trackerFn(Tracker t) {
	assert(t.data == 10);
}

Tracker trackerError(Tracker t) {
	throw UserError(L"ERROR");
}

int64 variedFn(int a, int b, int64 c) {
	return a + b + c;
}

int shortFn(byte a, int b) {
	return a + b;
}

BEGIN_TEST(FunctionParamTest, OS) {

	Tracker::clear();
	{
		Tracker t(10);
		FnCall<void> p = fnCall().add(t);
		p.call(address(&trackerFn), false);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		Tracker t(10);
		FnCall<Tracker> p = fnCall().add(t);
		CHECK_ERROR(p.call(address(&trackerError), false), UserError);
	}
	CHECK(Tracker::clear());

	{
		int a = 0x1, b = 0x20;
		int64 c = 0x100000000;
		FnCall<int64> p = fnCall().add(a).add(b).add(c);
		CHECK_EQ(p.call(address(&variedFn), false), 0x100000021);
	}

	{
		byte a = 0x10;
		int b = 0x1010;
		FnCall<int> p = fnCall().add(a).add(b);
		CHECK_EQ(p.call(address(&shortFn), false), 0x1020);
	}

} END_TEST


static Tracker returnTracker() {
	return Tracker(22);
}

BEGIN_TEST(FunctionReturnTest, OS) {
	Tracker::clear();
	{
		FnCall<Tracker> p = fnCall();
		Tracker t = p.call(address(&returnTracker), false);
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

BEGIN_TEST(FunctionFloatTest, OS) {

	{
		int i = 100;
		FnCall<float> p = fnCall().add(i);
		CHECK_EQ(p.call(address(&returnFloat), false), 100.0f);
	}

	{
		float f = 100.0f;
		FnCall<int> p = fnCall().add(f);
		CHECK_EQ(p.call(address(&takeFloat), false), 100);
	}

	{
		int i = 100;
		FnCall<double> p = fnCall().add(i);
		CHECK_EQ(p.call(address(&returnDouble), false), 100.0);
	}

	{
		double d = 100.0f;
		FnCall<int> p = fnCall().add(d);
		CHECK_EQ(p.call(address(&takeDouble), false), 100);
	}

} END_TEST

LargeType &refFn(LargeType *t) {
	return *t;
}

LargeType *ptrFn(LargeType *t) {
	return t;
}

BEGIN_TEST(FunctionRefPtrTest, OS) {
	LargeType t(1, 2, 3, 4);
	LargeType *ptrT = &t;

	FnCall<LargeType *> c = fnCall().add(ptrT);
	CHECK_EQ(c.call(address(&refFn), false), ptrT);
	CHECK_EQ(c.call(address(&ptrFn), false), ptrT);
} END_TEST
