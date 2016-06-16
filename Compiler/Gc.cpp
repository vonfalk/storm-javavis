#include "stdafx.h"
#include "Gc.h"

#ifdef STORM_GC_MPS

/**
 * MPS version.
 */
namespace storm {

	/**
	 * In MPS, we allocate objects with an extra pointer just before the object. This points to a
	 * MpsObj union which contains the information required for scanning. We also make sure to
	 * allocate objects that are a whole number of words, to make it easier to reason about what
	 * needs to be done in the special forwarders and padding types.
	 */


	static const size_t wordSize = sizeof(void *);
	static const size_t headerSize = wordSize;

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + wordSize - 1) & ~(wordSize - 1);
	}


	/**
	 * Extra data types for forwarding and padding.
	 */
	enum MpsTypes {
		// Padding object (0 word).
		mpsPad0 = 0x100,

		// Padding object (multi-word).
		mpsPad,

		// Forwarding object (1 word).
		mpsFwd1,

		// Forwarding object (multi-word).
		mpsFwd,
	};

	/**
	 * Padding object (0 word).
	 */
	struct MpsPad0 {};

	/**
	 * Padding object (multi-word).
	 */
	struct MpsPad {
		size_t size;
	};

	/**
	 * Forwarding object (1 word).
	 */
	struct MpsFwd1 {
		void *to;
	};

	/**
	 * Forwarding object (multi-word).
	 */
	struct MpsFwd {
		void *to;
		size_t size;
	};


	/**
	 * Union for easy access.
	 */
	union MpsHeader {
		size_t type;

		// Only valid if type is one of the types known in GcType.
		GcType obj;
	};

	/**
	 * Static allocated headers for the mps-specific types.
	 */
	static const size_t headerPad0 = mpsPad0;
	static const size_t headerPad = mpsPad;
	static const size_t headerFwd1 = mpsFwd1;
	static const size_t headerFwd = mpsFwd;

	/**
	 * Object on the heap (for convenience). Note that the 'header' is not inside objects when
	 * visible to Storm.
	 */
	struct MpsObj {
		const MpsHeader *header;

		// This is at offset 0 as far as C++ and Storm knows:
		union {
			// If header->type == tArray, this is the number of elements in the array.
			size_t count;

			// Special MPS objects.
			MpsPad0 pad0;
			MpsPad pad;
			MpsFwd1 fwd1;
			MpsFwd fwd;
		};
	};

	/**
	 * Set the header of the object.
	 */
	static inline void setHeader(mps_addr_t o, const void *to) {
		((MpsObj *)o)->header = (MpsHeader *)to;
	}

	/**
	 * Note: in the scanning functions below, we shall not touch any other garbage collected memory,
	 * nor consume too much stack.
	 */

	// Size of object.
	static inline size_t mpsSize(mps_addr_t o) {
		MpsObj *obj = (MpsObj *)o;
		const MpsHeader *h = obj->header;
		size_t s = 0;

		switch (h->type) {
		case GcType::tFixed:
			s = h->obj.stride;
			break;
		case GcType::tArray:
			s = h->obj.stride * obj->count;
			break;
		case mpsPad0:
			s = 0;
			break;
		case mpsPad:
			s = obj->pad.size;
			break;
		case mpsFwd1:
			s = sizeof(MpsFwd1);
			break;
		case mpsFwd:
			s = obj->fwd.size;
			break;
		}

		// Account for header;
		s += headerSize;

		// Word-align the size.
		return align(s);
	}

	// Skip objects. Figure out the size of an object, and return a pointer to after the end of it.
	static mps_addr_t mpsSkip(mps_addr_t at) {
		return (byte *)at + mpsSize((byte *)at - headerSize);
	}


	// Helper for interpreting and scanning a block of data.
#define FIX_GCTYPE(header, base)									\
	for (nat _i = 0; _i < (header)->obj.count; _i++) {				\
		size_t offset = (header)->obj.offset[_i];					\
		mps_addr_t *data = (mps_addr_t *)((byte *)(base) + offset);	\
		mps_res_t r = MPS_FIX12(ss, data);							\
		if (r != MPS_RES_OK)										\
			return r;												\
	}

	// Scan objects. If a MPS_FIX returns something other than MPS_RES_OK, return that code as
	// quickly as possible.
	static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		// Convert from client pointers to 'real' pointers:
		base = (byte *)base - headerSize;
		limit = (byte *)limit - headerSize;

		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = (byte *)at + mpsSize(at)) {
				MpsObj *o = (MpsObj *)at;
				const MpsHeader *h = o->header;

				// Exclude header.
				mps_addr_t pos = (byte *)at + headerSize;

				switch (h->type) {
				case GcType::tFixed:
					FIX_GCTYPE(h, pos);
					break;
				case GcType::tArray:
					for (size_t i = 0; i < o->count; i++, pos = (byte *)pos + h->obj.stride) {
						FIX_GCTYPE(h, pos);
					}
					break;
				}
			}
		} MPS_SCAN_END(ss);
		return MPS_RES_OK;
	}

	// Create a forwarding object at 'at' referring to 'to'. These must be recognized by mpsSize, mpsSkip and mpsScan.
	static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
		// Convert to base pointers:
		at = (byte *)at - headerSize;

		size_t size = mpsSize(at) - headerSize;
		MpsObj *o = (MpsObj *)at;
		if (size <= sizeof(MpsFwd1)) {
			setHeader(o, &headerFwd1);
			o->fwd1.to = to;
		} else {
			setHeader(o, &headerFwd);
			o->fwd.to = to;
			o->fwd.size = size;
		}
	}

	// See if object at 'at' is a forwarder, and if so, where it points to.
	static mps_addr_t mpsIsFwd(mps_addr_t at) {
		// Convert to base pointers:
		at = (byte *)at - headerSize;

		MpsObj *o = (MpsObj *)at;
		switch (o->header->type) {
		case mpsFwd1:
			return o->fwd1.to;
		case mpsFwd:
			return o->fwd.to;
		default:
			return null;
		}
	}

	// Create a padding object. These must be recognized by mpsSize, mpsSkip and mpsScan.
	static void mpsMakePad(mps_addr_t at, size_t size) {
		if (size <= headerSize) {
			setHeader(at, &headerPad0);
		} else {
			setHeader(at, &headerPad);
			MpsObj *o = (MpsObj *)at;
			o->pad.size = size - headerSize;
		}
	}


	// Check return codes from MPS.
	static void check(mps_res_t result, const wchar *msg) {
		if (result != MPS_RES_OK)
			throw GcError(msg);
	}

	/**
	 * Parameters about the generations to create.
	 * TODO: Tweak these!
	 */
	static mps_gen_param_s generationParams[] = {
		{ 150, 0.85 },
		{ 170, 0.45 }
	};

	Gc::Gc(size_t arenaSize) {
		// We work under this assumption.
		assert(wordSize == sizeof(size_t), L"Invalid word-size");
		assert(wordSize == sizeof(void *), L"Invalid word-size");
		assert(wordSize == sizeof(MpsFwd1), L"Invalid size of MpsFwd1");
		assert(wordSize == headerSize, L"Too large header.");

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenaSize);
			check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), L"Failed to create GC arena.");
		} MPS_ARGS_END(args);

		check(mps_chain_create(&chain, arena, ARRAY_COUNT(generationParams), generationParams),
			L"Failed to set up generations.");

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize); // Default alignment.
			MPS_ARGS_ADD(args, MPS_KEY_FMT_HEADER_SIZE, headerSize);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, &mpsScan);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, &mpsSkip);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, &mpsMakeFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, &mpsIsFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, &mpsMakePad);
			check(mps_fmt_create_k(&format, arena, args), L"Failed to create object format.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			check(mps_pool_create_k(&pool, arena, mps_class_amc(), args), L"Failed to create a GC pool.");
		} MPS_ARGS_END(args);

		// TODO: Make this better! Register thread as a root.
		// Register the current thread with MPS.
		check(mps_thread_reg(&mainThread, arena), L"Failed to register thread.");

		// Create an allocation point for the current thread.
		check(mps_ap_create_k(&allocPoint, pool, mps_args_none), L"Failed to create allocation point");
	}

	Gc::~Gc() {
		// Park the arena, so nothing goes on during cleanup.
		mps_arena_park(arena);

		// TODO: We might want to run a full gc to get destructors properly called.

		// TODO: Fix threads
		mps_ap_destroy(allocPoint);
		mps_thread_dereg(mainThread);

		// Destroy pools.
		mps_pool_destroy(pool);
		mps_fmt_destroy(format);
		mps_chain_destroy(chain);
		mps_arena_destroy(arena);
	}

	void *Gc::alloc(const GcType *type) {
		assert(type->type == GcType::tFixed, L"Wrong type for calling alloc().");

		size_t size = align(type->stride + headerSize);
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, allocPoint, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);

		} while (!mps_commit(allocPoint, memory, size));

		// Exclude our header, and return the allocated memory.
		return (byte *)memory + headerSize;
	}

	void *Gc::allocArray(const GcType *type, size_t elements) {
		assert(type->type == GcType::tArray, L"Wrong type for calling allocArray().");

		if (elements == 0)
			return null;

		size_t size = align(type->stride*elements + headerSize);
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, allocPoint, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Set size.
			((MpsObj *)memory)->count = elements;

		} while (!mps_commit(allocPoint, memory, size));

		// Exclude our header, and return the allocated memory.
		return (byte *)memory + headerSize;
	}

	GcType *Gc::allocType(size_t entries) {
		size_t s = sizeof(GcType) + entries*sizeof(size_t) - sizeof(size_t);
		GcType *t = (GcType *)malloc(s);
		memset(t, 0, s);
		t->count = entries;
		return t;
	}

	void Gc::freeType(GcType *t) {
		free(t);
	}

}

#else

#error "Unsupported or unknown GC chosen. See Storm.h for details."

#endif


namespace storm {

	/**
	 * Shared stuff.
	 */


	/**
	 * Simple linked list to test gc.
	 */
	struct GcLink {
	    nat value;
		GcLink *next;
	};

	static const GcType linkType = {
		GcType::tFixed,
		sizeof(GcLink),
		1,
		{ OFFSET_OF(GcLink, next) }
	};

	void Gc::test() {
		int base;

		// Hack, as we do not yet register threads properly.
		mps_root_t root;
		check(mps_root_create_thread(&root, arena, mainThread, &base + 20), L"Failed to create thread's root.");


		for (nat j = 0; j < 100; j++) {
			GcLink *first = null;
			GcLink *at = null;
			for (nat i = 0; i < 10000; i++) {
				GcLink *l = (GcLink *)alloc(&linkType);
				l->value = i;

				if (at) {
					at->next = l;
					at = l;
				} else {
					first = at = l;
				}
			}

			PLN("Verifying it...");
			at = first;
			for (nat i = 0; i < 10000; i++) {
				if (!at) {
					PLN("Premature end at " << i);
					break;
				}

				if (at->value != i)
					PLN("Failed: " << at->value << " should be " << i);

				at = at->next;
			}

			PLN("Done!");
		}

		mps_root_destroy(root);
	}

}
