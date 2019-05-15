#include "stdafx.h"
#include "Utils/Timer.h"
#include "Utils/Platform.h"

struct Dummy {
	Dummy *next;
	size_t data[9];
};


// Indirect storage of types, so that the GcType instance may move.
struct TypeStore {
	GcType *dummy;
};

static GcType storeType = {
	GcType::tFixed,
	null,
	null,
	sizeof(TypeStore),
	1,
	{ 0 }
};

struct Finalizable {
	size_t data;
};

void CODECALL finalizer(Finalizable *fn) {
	PLN(L"Finalizing object: " << fn << L", " << fn->data);
}

static GcType finalizeType = {
	GcType::tFixed,
	null,
	address(&finalizer),
	sizeof(Finalizable),
	0,
	{ 0 }
};


// Build a linked list of a few elements.
NOINLINE Dummy *makeList(Gc &gc, TypeStore *store, size_t count) {
	Dummy *first = (Dummy *)gc.alloc(store->dummy);
	first->data[0] = 0;

	Dummy *current = first;
	for (size_t i = 1; i < count; i++) {
		current->next = (Dummy *)gc.alloc(store->dummy);
		current->next->data[0] = i;
		current = current->next;
	}
	return first;
}

// Verify the list.
void verifyList(Dummy *head, size_t count) {

	for (size_t i = 0; i < count; i++) {
		if (!head) {
			PLN(L"List ends early!");
			return;
		}

		if (head->data[0] != i) {
			PLN(L"Invalid element at " << head << L": " << head->data[0] << L", expected " << i);
			return;
		}

		head = head->next;
	}

	if (head)
		PLN(L"List has extra elements.");
}

NOINLINE Dummy *makeLists(Gc &gc, TypeStore *store) {
	Dummy *d;
	for (size_t i = 0; i < 20; i++) {
		d = makeList(gc, store, 100);
	}

	return d;
}

NOINLINE void makeFinalizable(Gc &gc, size_t id) {
	// Make an object with a finalizer, just for fun!
	Finalizable *f = (Finalizable *)gc.alloc(&finalizeType);
	f->data = id;
}

NOINLINE void lists(Gc &gc, TypeStore *store) {
	Dummy *longlived = makeList(gc, store, 100);

	for (size_t t = 0; t < 1000; t++) {
		// Make an object with a finalizer from time to time.
		if (t % 100 == 0)
			makeFinalizable(gc, t);

		// We put 'makeLists' inside another function, otherwise it seems we keep store->dummy on the stack.
		Dummy *d = makeLists(gc, store);

		{
			// util::Timer t(L"gc");
			// gc.collect();
		}

		verifyList(d, 100);
		verifyList(longlived, 100);
	}
}

NOINLINE TypeStore *createTypes(Gc &gc) {
	TypeStore *store = (TypeStore *)gc.alloc(&storeType);

	// Create some room after the first type, so that it may be moved!
	for (size_t i = 0; i < 1000; i++)
		gc.alloc(&storeType);

	store->dummy = gc.allocType(GcType::tFixed, null, sizeof(Dummy), 1);
	store->dummy->offset[0] = 0;
	return store;
}

NOINLINE void run(Gc &gc) {
	TypeStore *store = createTypes(gc);

	// Move the allocation to another location before we allocate 'longlived' inside
	// 'lists'. Otherwise, it will likely not move!
	gc.collect();
	// gc.dbg_dump();

	lists(gc, store);
}

/**
 * Simple GC tests that can be used during the creation of a new GC so that large parts of the
 * compiler does not need to be rebiult so often during development.
 */
int main(int argc, const char *argv[]) {
	int z;
	os::Thread::setStackBase(&z);

	Gc gc(100*1024*1024, 1000);
	gc.attachThread();

	{
		util::Timer t(L"test");
		run(gc);
	}

	{
		util::Timer t(L"gc");
		gc.collect();
	}
	gc.dbg_dump();

	return 0;
}
