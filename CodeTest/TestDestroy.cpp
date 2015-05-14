#include "stdafx.h"
#include "Test/Test.h"

class RefCont : public Content {
public:
	RefCont(const RefSource &source, bool &alive) :
		Content(source.arena), ref(source, *this), alive(alive) {}

	~RefCont() {
		alive = false;
	}

	Reference ref;
	bool &alive;
};

class PtrCont : public Content {
public:
	PtrCont(Arena &arena, bool &alive) :
		Content(arena), alive(alive) {}

	~PtrCont() {
		alive = false;
	}

	bool &alive;
};

BEGIN_TEST(TestDestroy) {
	Arena arena;

	bool aAlive = true;
	bool bAlive = true;

	RefSource *a = new RefSource(arena, L"A");
	RefSource *b = new RefSource(arena, L"B");

	a->set(new RefCont(*b, aAlive));
	b->set(new PtrCont(arena, bAlive));

	delete a;
	CHECK(!aAlive);
	CHECK(bAlive);

	delete b;
	CHECK(!aAlive);
	CHECK(!bAlive);


} END_TEST

BEGIN_TEST(TestDestroyDelay) {
	Arena arena;

	bool aAlive = true;
	bool bAlive = true;

	RefSource *a = new RefSource(arena, L"A");
	RefSource *b = new RefSource(arena, L"B");

	b->set(new PtrCont(arena, bAlive));
	a->set(new RefCont(*b, aAlive));

	delete b;
	CHECK(aAlive);
	CHECK(bAlive);

	delete a;
	CHECK(!aAlive);
	CHECK(!bAlive);

} END_TEST

BEGIN_TEST(TestDestroyCircular) {
	Arena arena;

	bool aAlive = true;
	bool bAlive = true;

	RefSource *a = new RefSource(arena, L"A");
	RefSource *b = new RefSource(arena, L"B");

	a->set(new RefCont(*b, aAlive));
	b->set(new RefCont(*a, bAlive));

	delete a;
	CHECK(bAlive);
	CHECK(aAlive);

	delete b;
	CHECK(!bAlive);
	CHECK(!aAlive);

} END_TEST
