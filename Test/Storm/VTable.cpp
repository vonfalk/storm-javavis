#include "stdafx.h"
#include "Compiler/VTableCpp.h"

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
