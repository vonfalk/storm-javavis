#include "stdafx.h"
#include "Compiler/VTableCpp.h"
#include "Compiler/Debug.h"
#include "Compiler/Package.h"

class VTableTest {
public:
	VTableTest(int v) : z(v) {}

	virtual int CODECALL replace() const {
		return z;
	}

	int z;
};

static int CODECALL replaced(VTableTest *me) {
	return me->z + 10;
}

static int check(const VTableTest &v) {
	// NOTE: We may eventually have to disable optimizations for this function for tests to work. If
	// the compiler inlines this function, it will see that it know the type of 'v', so it does not
	// have to use the vtable at all.
	return v.replace();
}

BEGIN_TEST(VTableCppTest, Storm) {
	Engine &e = gEngine();

	VTableTest a(10);
	VTableTest b(20);

	VTableCpp *tab = new (e) VTableCpp(vtable::from(&a));
	CHECK_LT(tab->count(), nat(5));

	nat slot = vtable::fnSlot(address(&VTableTest::replace));
	CHECK_NEQ(slot, vtable::invalid);

	tab->slot(slot) = &replaced;

	CHECK_EQ(a.replace(), 10);
	CHECK_EQ(b.replace(), 20);

	tab->insert(&a);

	CHECK_EQ(check(a), 20);
	CHECK_EQ(check(b), 20);

	tab->insert(&b);

	CHECK_EQ(check(a), 20);
	CHECK_EQ(check(b), 30);

} END_TEST

static int CODECALL extendReplace(debug::Extend *me) {
	// Note: we can not really perform a super-call here.
	return 20;
}

static int CODECALL extendReplace2(debug::Extend *me) {
	// Note: we can not really perform a super-call here.
	return 40;
}

static Type *addSubclass(Package *pkg, Type *base, const void *fn) {
	Type *sub = new (pkg) Type(pkg->anonName(), typeClass);
	pkg->add(sub);
	sub->setSuper(base);

	Array<Value> *params = new (pkg) Array<Value>(1, Value(sub));
	sub->add(nativeFunction(pkg->engine(), Value(StormInfo<Int>::type(pkg->engine())), L"value", params, fn));

	return sub;
}

BEGIN_TEST(VTableCppTest2, Storm) {
	Engine &e = gEngine();

	Type *extend = debug::Extend::stormType(e);
	Package *pkg = as<Package>(extend->parent());
	Function *value = as<Function>(extend->find(L"value", Value(extend)));
	CHECK(value);
	CHECK(pkg);
	if (!value)
		break;
	if (!pkg)
		break;

	// Should not use vtable calls now.
	CHECK_EQ(value->ref()->address(), value->directRef()->address());


	// Add our own instantiation...
	Type *sub1 = addSubclass(pkg, extend, &extendReplace);
	Function *value1 = as<Function>(sub1->find(L"value", Value(sub1)));

	// Now, we should use vtable calls on the base class!
	CHECK_NEQ(value->ref()->address(), value->directRef()->address());
	CHECK_EQ(value1->ref()->address(), value1->directRef()->address());


	// Add another subclass, subling to sub1.
	Type *sub2 = addSubclass(pkg, extend, &extendReplace2);
	Function *value2 = as<Function>(sub2->find(L"value", Value(sub2)));

	// VTable call on base class and first level derived.
	CHECK_NEQ(value->ref()->address(), value->directRef()->address());
	CHECK_EQ(value1->ref()->address(), value1->directRef()->address());
	CHECK_EQ(value2->ref()->address(), value2->directRef()->address());

	// Add another subclass from sub1.
	Type *sub3 = addSubclass(pkg, sub1, &extendReplace2);
	Function *value3 = as<Function>(sub3->find(L"value", Value(sub3)));

	// VTable call on base class and first level derived.
	CHECK_NEQ(value->ref()->address(), value->directRef()->address());
	CHECK_NEQ(value1->ref()->address(), value1->directRef()->address());
	CHECK_EQ(value2->ref()->address(), value2->directRef()->address());
	CHECK_EQ(value3->ref()->address(), value3->directRef()->address());


} END_TEST
