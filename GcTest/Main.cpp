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

struct Globals {
	TypeStore *store;
	GcWeakArray<Finalizable> *weak;
};

Globals globals;

size_t globalWeakPos = 0;

void CODECALL finalizer(Finalizable *fn) {
	PLN(L"Finalizing object: " << fn << L", " << fn->data);

	// Check so that it is *not* present in the weak array!
	if (!globals.weak)
		return;

	PLN(L"Global is at " << globals.weak);
	for (size_t i = 0; i < globals.weak->count(); i++) {
		Finalizable *v = globals.weak->v[i];
		PLN(i << L" - " << v << L" - " << (v ? v->data : 0));
		dbg_assert(v != fn, L"ERROR! We should not find ourselves in the global array when we're being finalized!");
	}
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
NOINLINE Dummy *makeList(Gc &gc, size_t count) {
	Dummy *first = (Dummy *)gc.alloc(globals.store->dummy);
	first->data[0] = 0;

	Dummy *current = first;
	for (size_t i = 1; i < count; i++) {
		current->next = (Dummy *)gc.alloc(globals.store->dummy);
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

NOINLINE Dummy *makeLists(Gc &gc) {
	Dummy *d;
	for (size_t i = 0; i < 20; i++) {
		d = makeList(gc, 100);
	}

	return d;
}

NOINLINE void makeFinalizable(Gc &gc, size_t id) {
	// Make an object with a finalizer, just for fun!
	Finalizable *f = (Finalizable *)gc.alloc(&finalizeType);
	globals.weak->v[globalWeakPos++] = f;
	f->data = id;
}

NOINLINE void lists(Gc &gc) {
	Dummy *longlived = makeList(gc, 100);

	for (size_t t = 0; t < 1000; t++) {
		// Make an object with a finalizer from time to time.
		if (t % 100 == 0)
			makeFinalizable(gc, t);

		// We put 'makeLists' inside another function, otherwise it seems we keep store->dummy on the stack.
		Dummy *d = makeLists(gc);

		{
			// util::Timer t(L"gc");
			// gc.collect();
		}

		verifyList(d, 100);
		verifyList(longlived, 100);
	}
}

NOINLINE void createGlobals(Gc &gc) {
	globals.store = (TypeStore *)gc.alloc(&storeType);
	globals.store->dummy = gc.allocType(GcType::tFixed, null, sizeof(Dummy), 1);
	globals.store->dummy->offset[0] = 0;

	globals.weak = (GcWeakArray<Finalizable> *)gc.allocWeakArray(30);
}

NOINLINE void run(Gc &gc) {
	createGlobals(gc);

	// Move the allocation to another location before we allocate 'longlived' inside
	// 'lists'. Otherwise, it will likely not move!
	gc.collect();
	// gc.dbg_dump();

	lists(gc);
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

	Gc::Root *r = gc.createRoot(&globals, sizeof(globals));
	{
		util::Timer t(L"test");
		run(gc);
	}

	{
		util::Timer t(L"gc");
		gc.collect();
	}

	globals.weak = null;
	gc.dbg_dump();
	gc.destroyRoot(r);

	return 0;
}
