#include "stdafx.h"
#include "Test/Lib/Test.h"

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

class RefCont2 : public Content {
public:
	RefCont2(const RefSource &source, const RefSource &s2, bool &alive) :
		Content(source.arena), ref(source, *this), ref2(s2, *this), alive(alive) {}

	~RefCont2() {
		alive = false;
	}

	Reference ref, ref2;
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


BEGIN_TEST(TestDestroyComplex) {
	Arena arena;

	bool aAlive = true;
	bool bAlive = true;
	bool cAlive = true;
	bool dAlive = true;
	bool eAlive = true;

	RefSource *a = new RefSource(arena, L"A");
	RefSource *b = new RefSource(arena, L"B");
	RefSource *c = new RefSource(arena, L"C");
	RefSource *d = new RefSource(arena, L"D");
	RefSource *e = new RefSource(arena, L"E");

	a->set(new RefCont(*e, aAlive));
	b->set(new RefCont(*a, bAlive));
	c->set(new RefCont(*a, cAlive));
	d->set(new RefCont2(*b, *c, dAlive));
	e->set(new RefCont(*d, eAlive));

	delete e;
	CHECK(aAlive);
	CHECK(bAlive);
	CHECK(cAlive);
	CHECK(dAlive);
	CHECK(eAlive);

	delete c;
	CHECK(aAlive);
	CHECK(bAlive);
	CHECK(cAlive);
	CHECK(dAlive);
	CHECK(eAlive);

	delete b;
	CHECK(aAlive);
	CHECK(bAlive);
	CHECK(cAlive);
	CHECK(dAlive);
	CHECK(eAlive);

	delete a;
	CHECK(aAlive);
	CHECK(bAlive);
	CHECK(cAlive);
	CHECK(dAlive);
	CHECK(eAlive);

	delete d;
	CHECK(!aAlive);
	CHECK(!bAlive);
	CHECK(!cAlive);
	CHECK(!dAlive);
	CHECK(!eAlive);

} END_TEST
