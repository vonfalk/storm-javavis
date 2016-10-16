#include "stdafx.h"
#include "Core/Fn.h"

static Int addTwo(Int v, Int w) {
	return v + w + 2;
}

static Type *getType(Value v) {
	return v.type;
}

static Value toValue(Type *t) {
	return thisPtr(t);
}

static void voidFn(Int v) {}

BEGIN_TEST(FnFree, Core) {
	Engine &e = gEngine();

	storm::Fn<Int, Int, Int> *fn = fnPtr(e, &addTwo);
	CHECK_EQ(fn->call(2, 3), 7);

	Type *t = Value::stormType(e);

	storm::Fn<Type *, Value> *fn2 = fnPtr(e, &getType);
	CHECK_EQ(fn2->call(Value(t)), t);

	storm::Fn<Value, Type *> *fn3 = fnPtr(e, &toValue);
	CHECK_EQ(fn3->call(t), thisPtr(t));

	storm::Fn<void, Int> *fn4 = fnPtr(e, &voidFn);
	CHECK_RUNS(fn4->call(10));
} END_TEST
