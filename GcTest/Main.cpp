#include "stdafx.h"

struct Dummy {
	Dummy *next;
	size_t data[9];
};

static GcType dummyType = {
	GcType::tFixed,
	null,
	null,
	10 * sizeof(void *),
	1,
	{ 0 }
};

void run(Gc &gc) {
	volatile Dummy *d[1000];
	PVAR((void *)d);

	for (Nat i = 0; i < 1000; i++) {
		d[i] = (Dummy *)gc.alloc(&dummyType);
		// PVAR(d);
	}

	gc.collect();
}


/**
 * Simple GC tests that can be used during the creation of a new GC so that large parts of the
 * compiler does not need to be rebiult so often during development.
 */
int main() {
	int z;
	os::Thread::setStackBase(&z);

	Gc gc(10*1024*1024, 1000);
	gc.attachThread();

	run(gc);

	return 0;
}
