#include "stdafx.h"
#include "Test/Test.h"
#include "Utils/Semaphore.h"
#include "Code/UThread.h"
#include "Tracker.h"

static void returnVoid(bool error) {
	if (error)
		throw UserError(L"ERROR");
}

static int returnInt(int v) {
	return v;
}

static int64 returnInt64(int64 v) {
	return v;
}

static float returnFloat(float v) {
	return v;
}

static double returnDouble(double v) {
	return v;
}

static Tracker returnTracker(int t) {
	if (t < 0)
		throw UserError(L"ERROR");
	return Tracker(t);
}

static int takeTracker(Tracker t) {
	if (t.data < 0)
		throw UserError(L"ERROR");
	return t.data;
}

static void checkVoid(int *c) {
	(*c)++;
}

static void checkInt(int *c, int v) {
	*c = v;
}

static void checkInt64(int *c, int64 v) {
	*c = int(v >> 30LL);
}

static void checkFloat(double *c, float v) {
	*c = v;
}

static void checkDouble(double *c, double v) {
	*c = v;
}

static void checkTracker(int *c, Tracker t) {
	*c = t.data;
}

static void error(int *c, const Exception &e) {
	if (e.what() == L"ERROR")
		*c = -1;
	else
		*c = -2;
}

static void errorFloat(double *c, const Exception &e) {
	if (e.what() == L"ERROR")
		*c = -1.0;
	else
		*c = -2.0;
}

BEGIN_TEST(UThreadResultTest) {

	int c = 0;
	{
		bool e = false;
		FnCall params; params.param(e);
		UThread::Result<void, int> rv = { &c, &checkVoid, &error };
		UThread::spawn(returnVoid, params, rv);
		UThread::leave();
		CHECK_EQ(c, 1);

		e = true;
		UThread::spawn(returnVoid, params, rv);
		UThread::leave();
		CHECK_EQ(c, -1);
	}

	{
		int v = 10;
		FnCall params; params.param(v);
		UThread::Result<int, int> ri = { &c, &checkInt, &error };
		UThread::spawn(returnInt, params, ri);
		UThread::leave();
		CHECK_EQ(c, 10);
	}

	{
		int64 v = 1024LL << 30LL;
		FnCall params; params.param(v);
		UThread::Result<int64, int> ri = { &c, &checkInt64, &error };
		UThread::spawn(returnInt64, params, ri);
		UThread::leave();
		CHECK_EQ(c, 1024);
	}

	{
		double r = 0.0;
		float v = 13.37f;
		FnCall params; params.param(v);
		UThread::Result<float, double> ri = { &r, &checkFloat, &errorFloat };
		UThread::spawn(returnFloat, params, ri);
		UThread::leave();
		CHECK_EQ(r, 13.37f);
	}

	{
		double r = 0.0;
		double v = 13.37;
		FnCall params; params.param(v);
		UThread::Result<double, double> ri = { &r, &checkDouble, &errorFloat };
		UThread::spawn(returnDouble, params, ri);
		UThread::leave();
		CHECK_EQ(r, 13.37);
	}

	Tracker::clear();
	{
		int r = 0;
		int t = 22;
		FnCall params; params.param(t);
		UThread::Result<Tracker, int> ri = { &r, &checkTracker, &error };
		UThread::spawn(returnTracker, params, ri);
		UThread::leave();
		CHECK_EQ(r, 22);

		t = -2;
		UThread::spawn(returnTracker, params, ri);
		UThread::leave();
		CHECK_EQ(r, -1);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		int r = 0;
		Tracker t(22);
		FnCall params; params.param(t);
		UThread::Result<int, int> ri = { &r, &checkInt, &error };
		UThread::spawn(takeTracker, params, ri);
		UThread::leave();
		CHECK_EQ(r, 22);

		t.data = -3;
		UThread::spawn(takeTracker, params, ri);
		UThread::leave();
		CHECK_EQ(r, -1);
	}
	CHECK(Tracker::clear());


} END_TEST

