#include "mps.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma optimize("", off)
#pragma GCC optimize ("O0")

typedef unsigned char byte;

enum Types {
	pad0,
	padN,
	fwd1,
	fwdN,
	data,
};

struct Object;

struct PadN {
	size_t size;
};

struct Fwd1 {
	void *to;
};

struct FwdN {
	void *to;
	size_t size;
};

struct Data {
	Object *ptr;
};

struct Object {
	size_t type;
	union {
		PadN padN;
		Fwd1 fwd1;
		FwdN fwdN;
		Data data;
	};
};

static size_t mpsSize(mps_addr_t at) {
	Object *o = (Object *)at;
	switch (o->type) {
	case pad0:
		return sizeof(size_t);
	case padN:
		return o->padN.size;
	case fwd1:
		return sizeof(size_t) + sizeof(Fwd1);
	case fwdN:
		return o->fwdN.size;
	case data:
		return sizeof(Object);
	default:
		printf("Invalid object\n");
		exit(1);
	}
}

static mps_addr_t mpsSkip(mps_addr_t at) {
	return (byte *)at + mpsSize(at);
}

static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
	size_t size = mpsSize(at);
	Object *o = (Object *)at;
	if (size <= sizeof(size_t) + sizeof(Fwd1)) {
		o->type = fwd1;
		o->fwd1.to = to;
	} else {
		o->type = fwdN;
		o->fwdN.to = to;
		o->fwdN.size = size;
	}
}

static mps_addr_t mpsIsFwd(mps_addr_t at) {
	Object *o = (Object *)at;
	if (o->type == fwd1)
		return o->fwd1.to;
	else if (o->type == fwdN)
		return o->fwdN.to;
	else
		return NULL;
}

static void mpsMakePad(mps_addr_t at, size_t size) {
	Object *o = (Object *)at;
	if (size <= sizeof(size_t)) {
		o->type = pad0;
	} else {
		o->type = padN;
		o->padN.size = size;
	}
}

static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
	mps_res_t r = MPS_RES_OK;
	MPS_SCAN_BEGIN(ss) {
		for (void *at = base; at != limit; at = mpsSkip(at)) {
			Object *o = (Object *)at;
			switch (o->type) {
			case pad0:
			case padN:
			case fwd1:
			case fwdN:
				break;
			case data:
				MPS_FIX12(ss, (void **)&o->data.ptr);
				break;
			default:
				printf("Invalid object (scan)\n");
				exit(1);
			}
		}
	} MPS_SCAN_END(ss);
	return r;
}

static mps_res_t mpsScanThread(mps_ss_t ss, void *base, void *limit, void *closure) {
	(void)closure;
	void **begin = (void **)base;
	void **end = (void **)limit;

	MPS_SCAN_BEGIN(ss) {
		for (void **at = begin; at < end; at++) {
			// printf("Scan %p (%p)\n", at, *at);
			mps_res_t r = MPS_FIX12(ss, at);
			if (r != MPS_RES_OK)
				return r;
		}
	} MPS_SCAN_END(ss);

	return MPS_RES_OK;
}

static void check(mps_res_t result, const char *msg) {
	if (result != MPS_RES_OK) {
		printf("ERROR: %s\n", msg);
		exit(1);
	}
}

static Object *alloc(mps_arena_t &arena, mps_ap_t &ap) {
	mps_addr_t memory;
	Object *r;
	do {
		check(mps_reserve(&memory, ap, sizeof(Object)), "Out of memory");
		memset(memory, 0, sizeof(Object));
		r = (Object *)memory;
		r->type = data;
	} while (!mps_commit(ap, memory, sizeof(Object)));

	mps_finalize(arena, &memory);

	return r;
}

static void collect(mps_arena_t &arena) {
	mps_arena_collect(arena);
	mps_arena_release(arena);

	// Check for finalizers.
	mps_message_t message;
	while (mps_message_get(&message, arena, mps_message_type_finalization())) {
		mps_addr_t obj;
		mps_message_finalization_ref(&obj, arena, message);
		mps_message_discard(arena, message);

		printf("Finalize %p\n", obj);
	}
}

static void print(Object *a, Object *b) {
	printf(" m: %p -> %p\n n: %p -> %p\n", a, a->data.ptr, b, b->data.ptr);
}

static void run(mps_arena_t &arena, mps_ap_t &moving, mps_ap_t &nonmoving) {
	Object *m = alloc(arena, moving);
	m->data.ptr = alloc(arena, moving);

	Object *n = alloc(arena, nonmoving);
	n->data.ptr = m->data.ptr;

	// Adding a reference from the AMC-pool to the AMS-pool fixes the issue.
	// m->data.ptr->data.ptr = n;

	printf("Before:\n");
	print(m, n);

	printf("Collecting...\n"); // This clears some registers, which is important to unpin the indirect object.
	collect(arena);

	printf("After:\n");
	print(m, n);

	printf("Collecting...\n"); // This clears some registers, which is important to unpin the indirect object.
	collect(arena);

	printf("After:\n");
	print(m, n);
}

mps_gen_param_s generationParams[] = {
	{ 2 * 1024, 0.9 },
	{ 8 * 1024, 0.5 },
	{ 16 * 1024, 0.1 },
};

int main(int argc, const char **argv) {
	mps_arena_t arena;
	mps_chain_t chain;
	mps_fmt_t format;

	mps_pool_t moving;
	mps_pool_t nonmoving;

	mps_ap_t movingAp;
	mps_ap_t nonmovingAp;

	mps_thr_t thread;
	mps_root_t root;

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, 1024*1024);
		check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), "arena");
	} MPS_ARGS_END(args);

	mps_chain_create(&chain, arena, 3, generationParams);

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, sizeof(void *));
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
		check(mps_pool_create_k(&moving, arena, mps_class_amc(), args), "Failed to create a GC pool.");
	} MPS_ARGS_END(args);

	MPS_ARGS_BEGIN(args) {
		MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
		MPS_ARGS_ADD(args, MPS_KEY_GEN, 2);
		MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
		MPS_ARGS_ADD(args, MPS_KEY_AMS_SUPPORT_AMBIGUOUS, false);
		check(mps_pool_create_k(&nonmoving, arena, mps_class_ams(), args), "Failed to create a GC pool for non-moving types.");
	} MPS_ARGS_END(args);

	check(mps_ap_create_k(&movingAp, moving, mps_args_none), "Failed to create AP.");
	check(mps_ap_create_k(&nonmovingAp, nonmoving, mps_args_none), "Failed to create AP.");

	check(mps_thread_reg(&thread, arena), "Failed to register thread.");
	check(mps_root_create_thread_scanned(&root, arena, mps_rank_ambig(), (mps_rm_t)0, thread, &mpsScanThread, NULL, &argv), "Failed to create root.");

	mps_message_type_enable(arena, mps_message_type_finalization());

	run(arena, movingAp, nonmovingAp);

	printf("Done!\n");

	return 0;
}
