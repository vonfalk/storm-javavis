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
	GcType *nonmoving;
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

struct Nonmoving {
	// Containing a pointer, so that we can see that it is updated correctly!
	TypeStore *store;
};

void CODECALL nonmovingFinalizer(Nonmoving *obj) {
	PLN(L"Finalizing nonmoving object: " << obj);
}

struct Globals {
	TypeStore *store;
	GcWeakArray<Finalizable> *weak;
	Nonmoving *nonmoving;

	// Location watcher for the global objects.
	GcWatch *watch;
};

Globals globals;

// Non-scanned global objects. Used to check if location dependencies are working properly.
Globals staleGlobals;

// Non-scanned reference to a nonmoving object. Refers to the same object as in 'globals'.
Nonmoving *nonmoving;

size_t globalWeakPos = 0;

void CODECALL finalizer(Finalizable *fn) {
	PLN(L"Finalizing object: " << fn << L", " << fn->data);

	// Check so that it is *not* present in the weak array!
	if (!globals.weak)
		return;

	PLN(L"Global is at " << globals.weak << L", splats: " << globals.weak->splatted());
	for (size_t i = 0; i < globals.weak->count(); i++) {
		Finalizable *v = globals.weak->v[i];
		PLN(i << L" - " << v << L" - " << (v ? v->data : 0));
#if STORM_GC == STORM_GC_SMM
		dbg_assert(v != fn, L"ERROR! We should not find ourselves in the global array when we're being finalized!");
#endif
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

void checkGlobals();

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

		checkGlobals();
	}
}

NOINLINE void createGlobals(Gc &gc) {
	globals.store = (TypeStore *)gc.alloc(&storeType);
	globals.store->dummy = gc.allocType(GcType::tFixed, null, sizeof(Dummy), 1);
	globals.store->dummy->offset[0] = 0;

	globals.store->nonmoving = gc.allocType(GcType::tFixed, null, sizeof(Nonmoving), 1);
	globals.store->nonmoving->offset[0] = 0;
	globals.store->nonmoving->finalizer = address(&nonmovingFinalizer);

	globals.weak = (GcWeakArray<Finalizable> *)gc.allocWeakArray(30);
	nonmoving = globals.nonmoving = (Nonmoving *)gc.allocStatic(globals.store->nonmoving);
	nonmoving->store = globals.store;

	// Static allocation that should be collected.
	gc.allocStatic(globals.store->nonmoving);

	// Gc watch.
	globals.watch = gc.createWatch();
	globals.watch->add(globals.store);
	globals.watch->add(globals.weak);
	globals.watch->add(globals.nonmoving);
	globals.watch->add(globals.watch);

	staleGlobals = globals;
}

// Keep track of the watch false positive rate.
size_t watchChecks = 0;
size_t watchPositives = 0;

NOINLINE void checkGlobals() {
	assert(nonmoving->store == globals.store, L"The nonmoving object does not seem to be scanned correctly!");

	assert(nonmoving == globals.nonmoving, L"The nonmoving object moved from " + ::toHex(nonmoving) + L" to "
		+ ::toHex(globals.nonmoving) + L"!");

	bool redoWatch = false;
	if (globals.store != staleGlobals.store) {
		redoWatch = true;
		assert(globals.watch->moved(globals.store), L"Watch failed to indicate movement of 'store' ("
			+ ::toHex(staleGlobals.store) + L" -> " + ::toHex(globals.store) + L")");
	}

	if (globals.weak != staleGlobals.weak) {
		redoWatch = true;
		assert(globals.watch->moved(globals.weak), L"Watch failed to indicate movement of 'weak' ("
			+ ::toHex(staleGlobals.weak) + L" -> " + ::toHex(globals.weak) + L")");
	}

	if (globals.watch != staleGlobals.watch) {
		redoWatch = true;
		assert(globals.watch->moved(globals.watch), L"Watch failed to indicate movement of 'watch' ("
			+ ::toHex(staleGlobals.watch) + L" -> " + ::toHex(globals.watch) + L")");
	}

	watchChecks++;
	if (globals.watch->moved() && !redoWatch)
		watchPositives++;

	if (redoWatch) {
		globals.watch->clear();
		globals.watch->add(globals.store);
		globals.watch->add(globals.weak);
		globals.watch->add(globals.nonmoving);
		globals.watch->add(globals.watch);

		staleGlobals = globals;
	}
}

struct OtherThread {
	Gc &gc;

	OtherThread(Gc &gc) : gc(gc) {}

	void startThread() {
		gc.attachThread();
	}

	void stopThread() {
		gc.detachThread(os::Thread::current());
	}

	void fn() {
		Dummy *l = makeList(gc, 10);

		os::UThread::sleep(100);

		verifyList(l, 10);
	}
};

NOINLINE void createThread(OtherThread &other, os::ThreadGroup &group) {
	os::Thread::spawn(util::memberVoidFn(&other, &OtherThread::fn), group);
}

NOINLINE void run(Gc &gc, OtherThread &other, os::ThreadGroup &group) {
	createGlobals(gc);
	createThread(other, group);

	// Move the allocation to another location before we allocate 'longlived' inside
	// 'lists'. Otherwise, it will likely not move!
	gc.collect();
	// gc.dbg_dump();

	lists(gc);
	checkGlobals();
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

	OtherThread other(gc);
	os::ThreadGroup threads(util::memberVoidFn(&other, &OtherThread::startThread),
							util::memberVoidFn(&other, &OtherThread::stopThread));

	Gc::Root *r = gc.createRoot(&globals, sizeof(globals));
	{
		util::Timer t(L"test");
		run(gc, other, threads);
	}

	threads.join();

	{
		util::Timer t(L"gc");
		gc.collect();
	}

	globals.weak = null;
	gc.dbg_dump();
	gc.destroyRoot(r);

	PLN(L"Watch false positive percentage: " << (float(watchPositives * 100) / float(watchChecks)));

	return 0;
}
