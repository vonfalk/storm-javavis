#include "stdafx.h"
#include "Test/Test.h"

class RefCont : public Content {
public:
	RefCont(const RefSource &source, bool &alive);
	~RefCont();

	Reference ref;
	bool &alive;
};

RefCont::RefCont(const RefSource &source, bool &alive) : Content(source.arena), ref(source, *this), alive(alive) {}

RefCont::~RefCont() {
	alive = false;
}

BEGIN_TEST(TestDestroyCircular) {
	Arena arena;

	bool aAlive = true;
	bool bAlive = true;

	RefSource *a = new RefSource(arena, L"A");
	RefSource *b = new RefSource(arena, L"B");

	a->set(new RefCont(*b, bAlive));
	b->set(new RefCont(*a, aAlive));

	delete a;
	CHECK(bAlive);
	CHECK(aAlive);

	delete b;
	CHECK(!bAlive);
	CHECK(!aAlive);

} END_TEST
