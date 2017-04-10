#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "mps.h"

// #include <Windows.h> // uncomment for 'Sleep' below.

#define assert(cond, msg)							\
	if (!(cond)) {									\
		printf("Assertion failed: %s\n", msg);		\
		exit(1);									\
	}

#define null NULL

const size_t wordSize = sizeof(void *);

mps_arena_t arena;
mps_chain_t chain;
mps_fmt_t format;
mps_pool_t pool;
mps_ap_t ap;
mps_thr_t thread;
mps_root_t threadRoot;

mps_pool_t loPool;
mps_ap_t loAp;

static void check(mps_res_t result, const char *msg) {
	if (result != MPS_RES_OK) {
		printf("MPS error: %s\n", msg);
		exit(1);
	}
}

static mps_gen_param_s generationParams[] = {
	{ 2*1024*1024, 0.85 },
	{ 4*1024*1024, 0.45 },
	{ 8*1024*1024, 0.10 },
};

enum ObjTag {
	object,
	fwd2,
	fwd,
	pad1,
	pad,
};

struct Object {
	size_t tag;

	size_t value;
	Object *next;
};

struct Fwd2 {
	size_t tag;
	mps_addr_t to;
};

struct Fwd {
	size_t tag;
	mps_addr_t to;
	size_t size;
};

struct Pad1 {
	size_t tag;
};

struct Pad {
	size_t tag;
	size_t size;
};

union MpsObj {
	size_t tag;
	Object obj;
	Fwd2 fwd2;
	Fwd fwd;
	Pad1 pad1;
	Pad pad;
};

static size_t mpsSize(mps_addr_t at) {
	MpsObj *o = (MpsObj *)at;
	switch (o->tag) {
	case object:
		return sizeof(Object);
	case fwd2:
		return sizeof(Fwd2);
	case fwd:
		return o->fwd.size;
	case pad1:
		return sizeof(Pad1);
	case pad:
		return o->pad.size;
	default:
		assert(false, "Invalid type");
	}
}

static mps_addr_t mpsSkip(mps_addr_t at) {
	return (char *)at + mpsSize(at);
}

static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
	MPS_SCAN_BEGIN(ss) {
		for (mps_addr_t at = base; at < limit; at = mpsSkip(at)) {
			MpsObj *o = (MpsObj *)at;
			mps_res_t result;
			mps_addr_t tmp;

			switch (o->tag) {
			case object:
				tmp = o->obj.next;
				result = MPS_FIX12(ss, &tmp);
				if (result != MPS_RES_OK)
					return result;
				o->obj.next = (Object *)tmp;
				break;
			}
		}
	} MPS_SCAN_END(ss);
	return MPS_RES_OK;
}

static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
	size_t size = mpsSize(at);

	MpsObj *o = (MpsObj *)at;
	if (size <= sizeof(Fwd2)) {
		assert(size == sizeof(Fwd2), "Too small object!");
		o->fwd2.tag = fwd2;
		o->fwd2.to = to;
	} else {
		o->fwd.tag = fwd;
		o->fwd.to = to;
		o->fwd.size = size;
	}
}

static mps_addr_t mpsIsFwd(mps_addr_t at) {
	MpsObj *o = (MpsObj *)at;
	switch (o->tag) {
	case fwd2:
		return o->fwd2.to;
	case fwd:
		return o->fwd.to;
	default:
		return null;
	}
}

static void mpsMakePad(mps_addr_t at, size_t size) {
	MpsObj *o = (MpsObj *)at;
	if (size <= sizeof(Pad1)) {
		assert(size == sizeof(Pad1), "Too small to pad!");
		o->pad1.tag = pad1;
	} else {
		o->pad.tag = pad;
		o->pad.size = size;
	}
}

Object *alloc(size_t value = 0, Object *next = null) {
	mps_addr_t memory;
	Object *r;
	do {
		check(mps_reserve(&memory, ap, sizeof(Object)), "Out of memory.");

		r = (Object *)memory;
		r->tag = object;
		r->value = value;
		r->next = next;
	} while (!mps_commit(ap, memory, sizeof(Object)));

	return r;
}

Object *allocLo(size_t value = 0, Object *next = null) {
	mps_addr_t memory;
	Object *r;
	do {
		check(mps_reserve(&memory, loAp, sizeof(Object)), "Out of memory.");

		r = (Object *)memory;
		r->tag = object;
		r->value = value;
		r->next = next;
	} while (!mps_commit(loAp, memory, sizeof(Object)));

	return r;
}

void collect() {
	mps_arena_collect(arena);
	mps_arena_release(arena);
}

// Allocate 'n' nodes in a linked list.
Object *createList(size_t n) {
	if (n == 0)
		return null;

	Object *first = alloc(0);
	Object *last = first;
	for (size_t i = 1; i < n; i++) {
		last->next = alloc(i);
		last = last->next;
	}

	return first;
}

// Verify a linked list.
void checkList(Object *l, size_t count) {
	for (size_t i = 0; i < count; i++) {
		if (!l) {
			printf("Verification failure. The list is too short (only %Iu elements).\n", i);
			return;
		}
		if (l->value != i)
			printf("Verification failure. Expected %Iu, got %Iu\n", i, l->value);
		l = l->next;
	}

	if (l)
		printf("Verification failure. The list is more than %Iu elements.\n", count);
}

// Allocate and verify quite a lot of lists to put pressure on the GC.
void listOps(size_t times) {
	size_t len = 10000;
	for (size_t i = 0; i < times; i++) {
		Object *l = createList(len);
		checkList(l, len);
	}
}

void smallAllocs() {
	const size_t count = 200;
	Object *objs[count];

	for (size_t i = 0; i < count; i++) {
		objs[i] = createList(i+2);
		// The line below causes very slow shutdown times.
		collect();
	}

	for (size_t i = 0; i < count; i++) {
		checkList(objs[i], i+2);
	}
}

void realMain() {
	// If doing something heavy before calling 'smallAllocs', the problem also disappears.
	// listOps(2000);

	smallAllocs();

	printf("Done with small allocations.\n");
	fflush(stdout);

	// This line causes an assertion from the MPS. Something like:
	// trace.c:382: MPS ASSERTION FAILED: trace->condemned > condemnedBefore
	// allocLo(0, 0);

	listOps(2000);

	printf("Committed memory: %Iu kB\n", mps_arena_committed(arena) / 1024);
	// Sleep(10000); // uncomment to see the total memory consumption clearly in Task Manager etc.
}

int main() {
	int cold;

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, 1024*1024);
		check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), "Failed to create arena.");
	} MPS_ARGS_END(args);

	check(mps_chain_create(&chain, arena, 3, generationParams), "Failed to create a chain.");

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_HEADER_SIZE, 0);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, &mpsScan);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, &mpsSkip);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, &mpsMakeFwd);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, &mpsIsFwd);
		MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, &mpsMakePad);
		check(mps_fmt_create_k(&format, arena, args), "Failed to create object format.");
	} MPS_ARGS_END(args);

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
		MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
		check(mps_pool_create_k(&pool, arena, mps_class_amc(), args), "Failed to create AMC pool.");
	} MPS_ARGS_END(args);

	check(mps_ap_create_k(&ap, pool, mps_args_none), "Failed to create allocation point.");

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
		MPS_ARGS_ADD(args, MPS_KEY_GEN, 1);
		MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
		check(mps_pool_create_k(&loPool, arena, mps_class_lo(), args), "Failed to create LO pool.");
	} MPS_ARGS_END(args);

	check(mps_ap_create_k(&loAp, loPool, mps_args_none), "Failed to creat LO allocation point.");

	check(mps_thread_reg(&thread, arena), "Failed to register thread");
	check(mps_root_create_thread(&threadRoot, arena, thread, &cold), "Failed to set up thread root.");

	printf("Initialization done.\n");
	fflush(stdout);

	realMain();

	printf("Shutting down...\n");
	fflush(stdout);

	mps_arena_collect(arena);
	mps_root_destroy(threadRoot);
	mps_thread_dereg(thread);
	mps_ap_destroy(ap);
	mps_pool_destroy(pool);
	mps_ap_destroy(loAp);
	mps_pool_destroy(loPool);
	mps_fmt_destroy(format);
	mps_chain_destroy(chain);
	mps_arena_destroy(arena);

	printf("Done!\n");

	return 0;
}
