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

	VTableCpp *tab = VTableCpp::copy(e, vtable::from(&a));
	CHECK_LT(tab->count(), nat(5));

	nat slot = vtable::fnSlot(address(&VTableTest::replace));
	CHECK_NEQ(slot, vtable::invalid);

	tab->set(slot, &replaced);

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
	// Note: we can not easily perform a super-call here.
	return 20;
}

static int CODECALL extendReplace2(debug::Extend *me) {
	// Note: we can not easily perform a super-call here.
	return 40;
}

static int CODECALL extendReplace3(debug::Extend *me) {
	// Note: we can not easily perform a super-call here.
	return 60;
}

static Type *addSubclass(Package *pkg, Type *base) {
	Type *sub = new (pkg) Type(pkg->anonName(), typeClass);
	pkg->add(sub);
	sub->setSuper(base);
	return sub;
}

static Type *addSubclass(Package *pkg, Type *base, const wchar *name, const void *fn) {
	Type *sub = addSubclass(pkg, base);

	Array<Value> *params = new (pkg) Array<Value>(1, Value(sub));
	sub->add(nativeFunction(pkg->engine(), Value(StormInfo<Int>::type(pkg->engine())), name, params, fn));

	return sub;
}

static Function *addFn(Type *to, const wchar *name, const void *fn) {
	Array<Value> *params = new (to) Array<Value>(1, Value(to));
	Function *f = nativeFunction(to->engine, Value(StormInfo<Int>::type(to->engine)), name, params, fn);
	to->add(f);
	return f;
}

static bool usesVTable(Function *fn) {
	return fn->directRef()->address() != fn->ref()->address();
}

BEGIN_TEST(VTableCppTest2, Storm) {
	Engine &e = gEngine();

	Type *extend = debug::Extend::stormType(e);
	Package *pkg = as<Package>(extend->parent());
	Function *value = as<Function>(extend->find(L"value", Value(extend)));
	VERIFY(value);
	VERIFY(pkg);

	// Should not use vtable calls now.
	CHECK_EQ(value->ref()->address(), value->directRef()->address());


	// Add our own instantiation...
	Type *sub1 = addSubclass(pkg, extend, L"value", &extendReplace);
	Function *value1 = as<Function>(sub1->find(L"value", Value(sub1)));

	// Now, we should use vtable calls on the base class!
	CHECK(usesVTable(value));
	CHECK(!usesVTable(value1));


	// Add another subclass, subling to sub1.
	Type *sub2 = addSubclass(pkg, extend, L"value", &extendReplace2);
	Function *value2 = as<Function>(sub2->find(L"value", Value(sub2)));

	// VTable call on base class and first level derived.
	CHECK(usesVTable(value));
	CHECK(!usesVTable(value1));
	CHECK(!usesVTable(value2));

	// Add another subclass from sub1.
	Type *sub3 = addSubclass(pkg, sub1, L"value", &extendReplace2);
	Function *value3 = as<Function>(sub3->find(L"value", Value(sub3)));

	// VTable call on base class and first level derived.
	CHECK(usesVTable(value));
	CHECK(usesVTable(value1));
	CHECK(!usesVTable(value2));
	CHECK(!usesVTable(value3));

	TODO(L"Instantiate a class and make sure everything works as intended!");
} END_TEST

BEGIN_TEST(VTableCppTest3, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	VERIFY(pkg);

	Type *sub1 = addSubclass(pkg, base);
	Type *sub2 = addSubclass(pkg, sub1);
	Type *sub3 = addSubclass(pkg, sub2);

	Function *f3 = addFn(sub3, L"val", &extendReplace);
	Function *f2 = addFn(sub2, L"val", &extendReplace2);
	Function *f1 = addFn(sub1, L"val", &extendReplace3);

	CHECK(usesVTable(f1));
	CHECK(usesVTable(f2));
	CHECK(!usesVTable(f3));

	TODO(L"Instantiate a class and make sure everything works as intended!");
} END_TEST

// Try create a few pure Storm functions which will need VTable entries and see that it works.
BEGIN_TEST(VTableStormTest, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	VERIFY(pkg);

	// Note: *not* overriding 'value' in debug::Extend.
	Type *sub1 = addSubclass(pkg, base, L"val", &extendReplace);
	Function *val1 = as<Function>(sub1->find(L"val", Value(sub1)));

	CHECK(!usesVTable(val1));

	Type *sub2 = addSubclass(pkg, sub1, L"val", &extendReplace2);
	Function *val2 = as<Function>(sub2->find(L"val", Value(sub2)));

	CHECK(!usesVTable(val2));
	CHECK(usesVTable(val1));

	TODO(L"Instantiate a class and make sure everything works as intended!");
} END_TEST


// Create a class hierarchy, remove the middle class, split it into two and verify that everything
// looks right.
BEGIN_TEST_(VTableSplit, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	Function *value = as<Function>(base->find(L"value", Value(base)));
	VERIFY(pkg);
	VERIFY(value);

	// Create subclasses a, b and c.
	Type *a = addSubclass(pkg, base);
	Type *b = addSubclass(pkg, a);
	Type *c = addSubclass(pkg, b);

	// Add both 'value' and 'val' to them. Note: this violates the assumption that functions with
	// different names have different implementation, which is made by the vtable subsystem.
	Function *aValue = addFn(a, L"value", &extendReplace);
	Function *bValue = addFn(b, L"value", &extendReplace2);
	Function *cValue = addFn(c, L"value", &extendReplace3);

	Function *aVal = addFn(a, L"val", &extendReplace);
	Function *bVal = addFn(b, L"val", &extendReplace2);
	Function *cVal = addFn(c, L"val", &extendReplace3);

	// See so that everything is as we expect.
	CHECK(usesVTable(value));
	CHECK(usesVTable(aValue));
	CHECK(usesVTable(bValue));
	CHECK(!usesVTable(cValue));

	CHECK(usesVTable(aVal));
	CHECK(usesVTable(bVal));
	CHECK(!usesVTable(cVal));

	// Now, make 'b' into its own hierarchy.
	b->setSuper(null);

	// And see everything has been updated as we would expect.
	CHECK(usesVTable(value));
	CHECK(!usesVTable(aValue));
	CHECK(usesVTable(bValue));
	CHECK(!usesVTable(cValue));

	CHECK(!usesVTable(aVal));
	CHECK(usesVTable(bVal));
	CHECK(!usesVTable(cVal));

	// Note: we should check if functions have VTable slots as well.

	TODO(L"Make sure to update the vtable slots of classes which do not contain the function in question as well!");
} END_TEST
