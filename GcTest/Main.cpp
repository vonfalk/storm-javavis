#include "stdafx.h"
#include "Utils/Timer.h"

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

// Build a linked list of a few elements.
__declspec(noinline) Dummy *makeList(Gc &gc, size_t count) {
	Dummy *first = (Dummy *)gc.alloc(&dummyType);
	first->data[0] = 0;

	Dummy *current = first;
	for (size_t i = 1; i < count; i++) {
		current->next = (Dummy *)gc.alloc(&dummyType);
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

void run(Gc &gc) {
	Dummy *longlived = makeList(gc, 100);

	for (size_t t = 0; t < 1000; t++) {
		Dummy *d;

		for (size_t i = 0; i < 20; i++) {
			d = makeList(gc, 100);
		}

		{
			// util::Timer t(L"gc");
			gc.collect();
		}

		verifyList(d, 100);
		verifyList(longlived, 100);
	}
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

	gc.collect();
	gc.dbg_dump();

	return 0;
}
