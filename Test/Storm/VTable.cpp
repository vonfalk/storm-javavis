#include "stdafx.h"
#include "Compiler/VTableCpp.h"
#include "Compiler/Debug.h"
#include "Compiler/Package.h"
#include "Compiler/TypeCtor.h"
#include "Code/X86/Arena.h"

class VTableTest : public RootObject {
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

#if defined(GCC)
#define NO_OPTIMIZE __attribute__((optimize("O0")))
#else
#define NO_OPTIMIZE
#endif

static int NO_OPTIMIZE check(const VTableTest &v) {
	// NOTE: We may eventually have to disable optimizations for this function for tests to work. If
	// the compiler inlines this function, it will see that it knows the type of 'v', and that it
	// does not have to use the vtable at all.
	return v.replace();
}

BEGIN_TEST(VTableCppTest, Storm) {
	Engine &e = gEngine();

	VTableTest a(10);
	VTableTest b(20);

	VTableCpp *tab = VTableCpp::copy(e, vtable::from(&a));
	CHECK_LT(tab->count(), nat(10));
	CHECK_GT(tab->count(), nat(2));

	nat slot = vtable::fnSlot(address(&VTableTest::replace));
	CHECK_NEQ(slot, vtable::invalid);

	tab->set(slot, address(&replaced));

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
	sub->add(new (pkg) TypeDefaultCtor(sub));
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
	return fn->directRef().address() != fn->ref().address();
}

static Int callFn(Object *o, Function *fn) {
	typedef Int (*Ptr)(Object *);
	Ptr p = (Ptr)fn->ref().address();
	return (*p)(o);
}

BEGIN_TEST(VTableCppTest2, Storm) {
	Engine &e = gEngine();

	Type *extend = debug::Extend::stormType(e);
	Package *pkg = as<Package>(extend->parent());
	Function *value = as<Function>(extend->find(S("value"), Value(extend), Scope()));
	VERIFY(value);
	VERIFY(pkg);

	// Should not use vtable calls now.
	CHECK_EQ(value->ref().address(), value->directRef().address());


	// Add our own instantiation...
	Type *sub1 = addSubclass(pkg, extend, S("value"), address(&extendReplace));
	Function *value1 = as<Function>(sub1->find(S("value"), Value(sub1), Scope()));

	// Now, we should use vtable calls on the base class!
	CHECK(usesVTable(value));
	CHECK(!usesVTable(value1));


	// Add another subclass, subling to sub1.
	Type *sub2 = addSubclass(pkg, extend, S("value"), address(&extendReplace2));
	Function *value2 = as<Function>(sub2->find(S("value"), Value(sub2), Scope()));

	// VTable call on base class and first level derived.
	CHECK(usesVTable(value));
	CHECK(!usesVTable(value1));
	CHECK(!usesVTable(value2));

	// Add another subclass from sub1.
	Type *sub3 = addSubclass(pkg, sub1, S("value"), address(&extendReplace2));
	Function *value3 = as<Function>(sub3->find(S("value"), Value(sub3), Scope()));

	// VTable call on base class and first level derived.
	CHECK(usesVTable(value));
	CHECK(usesVTable(value1));
	CHECK(!usesVTable(value2));
	CHECK(!usesVTable(value3));

	// Instantiate a class and see if it is actually working!
	debug::Extend *o = new (e) debug::Extend(1);
	CHECK_EQ(o->value(), 1);
	CHECK_EQ(callFn(o, value), 1);
	sub1->vtable()->insert(o);
	CHECK_EQ(o->value(), 20);
	CHECK_EQ(callFn(o, value), 20);
	CHECK_EQ(callFn(o, value1), 20);
	sub2->vtable()->insert(o);
	CHECK_EQ(o->value(), 40);
	CHECK_EQ(callFn(o, value), 40);
	CHECK_EQ(callFn(o, value1), 40);
	CHECK_EQ(callFn(o, value2), 40);

	// Create actual objects!
	debug::Extend *p = (debug::Extend *)alloc(extend);
	CHECK_EQ(p->value(), 1);
	CHECK_EQ(callFn(p, value), 1);
	p = (debug::Extend *)alloc(sub1);
	CHECK_EQ(p->value(), 20);
	CHECK_EQ(callFn(p, value), 20);
	CHECK_EQ(callFn(p, value1), 20);
	p = (debug::Extend *)alloc(sub2);
	CHECK_EQ(p->value(), 40);
	CHECK_EQ(callFn(p, value), 40);
	CHECK_EQ(callFn(p, value1), 40);
	CHECK_EQ(callFn(p, value2), 40);
} END_TEST

BEGIN_TEST(VTableCppTest3, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	VERIFY(pkg);

	Type *sub1 = addSubclass(pkg, base);
	Type *sub2 = addSubclass(pkg, sub1);
	Type *sub3 = addSubclass(pkg, sub2);

	Function *f1 = addFn(sub1, S("val"), address(&extendReplace));
	Function *f2 = addFn(sub2, S("val"), address(&extendReplace2));
	Function *f3 = addFn(sub3, S("val"), address(&extendReplace3));

	CHECK(usesVTable(f1));
	CHECK(usesVTable(f2));
	CHECK(!usesVTable(f3));

	// Instantiate a class and make sure everything works as intended!
	debug::Extend *o = new (e) debug::Extend(1);
	sub1->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 20);
	sub2->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 40);
	CHECK_EQ(callFn(o, f2), 40);
	sub3->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 60);
	CHECK_EQ(callFn(o, f2), 60);
	CHECK_EQ(callFn(o, f3), 60);
} END_TEST

BEGIN_TEST(VTableCppTest4, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	VERIFY(pkg);

	Type *sub1 = addSubclass(pkg, base);
	Type *sub2 = addSubclass(pkg, sub1);
	Type *sub3 = addSubclass(pkg, sub2);

	Function *f3 = addFn(sub3, S("val"), address(&extendReplace3));
	Function *f2 = addFn(sub2, S("val"), address(&extendReplace2));
	Function *f1 = addFn(sub1, S("val"), address(&extendReplace));

	CHECK(usesVTable(f1));
	CHECK(usesVTable(f2));
	CHECK(!usesVTable(f3));

	// Instantiate a class and make sure everything works as intended!
	debug::Extend *o = new (e) debug::Extend(1);
	sub1->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 20);
	sub2->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 40);
	CHECK_EQ(callFn(o, f2), 40);
	sub3->vtable()->insert(o);
	CHECK_EQ(callFn(o, f1), 60);
	CHECK_EQ(callFn(o, f2), 60);
	CHECK_EQ(callFn(o, f3), 60);
} END_TEST

// Try create a few pure Storm functions which will need VTable entries and see that it works.
BEGIN_TEST(VTableStormTest, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	VERIFY(pkg);

	// Note: *not* overriding 'value' in debug::Extend.
	Type *sub1 = addSubclass(pkg, base, S("val"), address(&extendReplace));
	Function *val1 = as<Function>(sub1->find(S("val"), Value(sub1), Scope()));

	CHECK(!usesVTable(val1));

	Type *sub2 = addSubclass(pkg, sub1, S("val"), address(&extendReplace2));
	Function *val2 = as<Function>(sub2->find(S("val"), Value(sub2), Scope()));

	CHECK(!usesVTable(val2));
	CHECK(usesVTable(val1));

	TODO(L"Instantiate a class and make sure everything works as intended!");
} END_TEST


// Create a class hierarchy, remove the middle class, split it into two and verify that everything
// looks right.
BEGIN_TEST(VTableSplit, Storm) {
	Engine &e = gEngine();

	Type *base = debug::Extend::stormType(e);
	Package *pkg = as<Package>(base->parent());
	Function *value = as<Function>(base->find(S("value"), Value(base), Scope()));
	VERIFY(pkg);
	VERIFY(value);

	// Create subclasses a, b and c.
	Type *a = addSubclass(pkg, base);
	Type *b = addSubclass(pkg, a);
	Type *c = addSubclass(pkg, b);

	// Add both 'value' and 'val' to them. Note: this violates the assumption that functions with
	// different names have different implementation, which is made by the vtable subsystem.
	Function *aValue = addFn(a, S("value"), address(&extendReplace));
	Function *bValue = addFn(b, S("value"), address(&extendReplace2));
	Function *cValue = addFn(c, S("value"), address(&extendReplace3));

	Function *aVal = addFn(a, S("val"), address(&extendReplace));
	Function *bVal = addFn(b, S("val"), address(&extendReplace2));
	Function *cVal = addFn(c, S("val"), address(&extendReplace3));

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

// Make sure that classes implemented only in C++ behave correctly as well.
// Earlier, this could be observed with the code::Arena class.
BEGIN_TEST(VTableCppOnly, Storm) {
	Engine &e = gEngine();

	// Create an arena (always the X86 arena for simplicity).
	code::Arena *arena = new (e) code::x86::Arena();

	// Find the type of the base class.
	Type *base = StormInfo<code::Arena>::type(e);

	// Try to call the 'firstParamLoc' function, and see which function is actually called.
	Array<Value> *params = new (e) Array<Value>(2, Value());
	params->at(0) = Value(base);
	params->at(1) = Value(StormInfo<Nat>::type(e));
	Function *call = as<Function>(base->find(S("firstParamLoc"), params, Scope()));

	Nat param = 0;
	os::FnCall<code::Operand, 2> fnParams = os::fnCall().add(arena).add(param);

	// The base-class implementation asserts, so it is easy to see which version was actually called.
	CHECK_RUNS(fnParams.call(call->pointer(), true));

} END_TEST;
