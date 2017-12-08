#include "stdafx.h"
#include "Gc.h"
#include "VTableCpp.h" // We need to know how VTables are stored.
#include "Type.h" // For debugging heap corruptions.
#include "Core/Str.h"  // For debugging heap corruptions.
#include "Code/Refs.h"
#include "Utils/Memory.h"
#include "Utils/Bitwise.h"

#if defined(STORM_GC_MPS)

/**
 * MPS version.
 */

// If enabled, add and verify data after each object allocated in the Gc-heap in order to find
// memory-corruption bugs.
#define MPS_CHECK_MEMORY 0

// Use debug pools in MPS (behaves slightly differently from the standard and may not trigger errors).
#define MPS_DEBUG_POOL 0

// Number of words that shall be verified before and after each allocation.
#define MPS_CHECK_WORDS 32

// Data to check against. TODO: Put the data tightly around allocations. Currently, there may be a
// gap of up to 'headerSize' bytes before the barrier actually starts. This could be solved by
// setting MPS_CHECK_WORDS to 1 if larger values do not show the sought after bug.
#define MPS_HEADER_DATA 0xBB
#define MPS_MIDDLE_DATA 0xCC
#define MPS_FOOTER_DATA 0xAA

// Check the heap after this many allocations.
#define MPS_CHECK_INTERVAL 100000


#if MPS_CHECK_MEMORY
#define MPS_CHECK_BYTES (sizeof(void *)*MPS_CHECK_WORDS)
#define MPS_VERIFY_OBJECT(object) checkObject(object)
#define MPS_VERIFY_SIZE(object) checkSize(object);
#define MPS_INIT_OBJECT(object, size) initObject((MpsObj *)(object), size);
#define SLOW_DEBUG // additional asserts in the GC code.
#else
#define MPS_CHECK_BYTES 0
#define MPS_VERIFY_OBJECT(object)
#define MPS_VERIFY_SIZE(object)
#define MPS_INIT_OBJECT(object, size)
#endif

namespace storm {

	/**
	 * In MPS, we allocate objects with an extra pointer just before the object. This points to a
	 * MpsObj union which contains the information required for scanning. If the least significant
	 * bit of the header is set, this allocation is a code allocation, and the header pointer
	 * indicates the size of the code portion of the allocation (always word-aligned, so the lsb is
	 * discarded). After the code portion, the code allocation contains data about pointers in the
	 * allocation in the form of a GcCode struct.
	 *
	 * Objects are always allocated in multiples of the alignment, as that is required by the MPS
	 * and it makes it easier to reason about how forwarders and padding behave.
	 *
	 * Convention: Base pointers are always passed as MpsObj * while client pointers are always
	 * passed as either void * or mps_addr_t.
	 */


	static const size_t wordSize = sizeof(void *);

#if MPS_CHECK_MEMORY
	static const size_t headerSize = wordSize * (3 + MPS_CHECK_WORDS);

	static inline size_t align(size_t data) {
		nat a = nextPowerOfTwo(headerSize);
		return (data + a - 1) & ~(a - 1);
	}
#else
	static const size_t headerSize = wordSize;

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + wordSize - 1) & ~(wordSize - 1);
	}
#endif

	static const size_t arrayHeaderSize = wordSize * 2;

	static inline size_t wordAlign(size_t data) {
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
	 * Padding object (0 words).
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
	 * Static allocated headers for the MPS-specific types. MPS-specific types will have their
	 * headers point here.
	 */
	static const size_t headerPad0 = mpsPad0;
	static const size_t headerPad = mpsPad;
	static const size_t headerFwd1 = mpsFwd1;
	static const size_t headerFwd = mpsFwd;

	/**
	 * Weak object data. Note that these members are tagged with a 1 in the lowest bit.
	 */
	struct MpsWeakHead {
		size_t count;
		size_t splatted;
	};

	/**
	 * Object on the heap (for convenience). Note that the 'header' is not inside objects when
	 * visible to Storm.
	 */
	struct MpsObj {
		union {
			// Size of the code stored here (if tagged with a 1 in the least significant bit). This
			// excludes the size of the metadata.
			size_t size;

			// The header of this object (if tagged with a 0 in the least significant bit).
			const MpsHeader *header;
		};

#if MPS_CHECK_MEMORY
		// Size of this object in case the header is destroyed. This includes the size of the header and any barriers.
		size_t totalSize;

		// Allocation number for this object.
		size_t allocId;

		// Padding before the object.
		size_t barrier[MPS_CHECK_WORDS];
#endif

		// This is at offset 0 as far as C++ and Storm knows:
		union {
			// If header->type == tArray, this is the number of elements in the array.
			size_t count;

			// If header->type == tWeakArray, this is valid. Use 'weakCount' and 'weakSplat' to avoid
			// destroying the tag bit.
			MpsWeakHead weak;

			// Special MPS objects.
			MpsPad0 pad0;
			MpsPad pad;
			MpsFwd1 fwd1;
			MpsFwd fwd;
		};
	};

#if MPS_CHECK_MEMORY

	// Allocation id.
	static nat allocId = 0;

	// Dump information about an object.
	static String objInfo(const MpsObj *o) {
		std::wostringstream to;
		to << L"Object " << (void *)o << L", header: " << (void *)o->header;
		to << L", size " << o->totalSize << L", id: " << o->allocId << L" (of " << allocId << L")";
		return to.str();
	}

	// Check object barriers and size.
	static void checkObject(const MpsObj *o);

	// Check size for the object.
	static void checkSize(const MpsObj *o);

	// Initialize object.
	static void initObject(MpsObj *o, size_t size);

#endif

	// Return the size of a weak array.
	static inline size_t weakCount(const MpsObj *o) {
		return o->weak.count >> 1;
	}

	// Splat an additional reference in a weak array.
	static inline void weakSplat(MpsObj *o) {
		o->weak.splatted = (o->weak.splatted + 0x10) | 0x01;
	}

	// Set the header of the object.
	static inline void setHeader(mps_addr_t o, const void *to) {
		((MpsObj *)o)->header = (MpsHeader *)to;
	}

	/**
	 * Note: in the scanning functions below, we shall not touch any other garbage collected memory,
	 * nor consume too much stack.
	 */

	// Convert to/from client pointers.
	static inline MpsObj *fromClient(mps_addr_t o) {
		o = (byte *)o - headerSize;
		return (MpsObj *)o;
	}
	static inline const MpsObj *fromClient(const void *o) {
		o = (const byte *)o - headerSize;
		return (const MpsObj *)o;
	}
	static inline mps_addr_t toClient(MpsObj *o) {
		// Note: Any of the objects in the union will work.
		return &o->count;
	}
	static inline const void *toClient(const MpsObj *o) {
		// Note: Any of the objects in the union will work.
		return &o->count;
	}

	// Compute the size of an object given its header.
	static inline size_t mpsSizeObj(const GcType *type) {
		return align(headerSize
					+ type->stride
					+ MPS_CHECK_BYTES);
	}
	static inline size_t mpsSizeArray(const GcType *type, size_t count) {
		return align(headerSize
					+ arrayHeaderSize
					+ type->stride*count
					+ MPS_CHECK_BYTES);
	}

	// See if an object is a code object.
#define IS_CODE(obj) (((obj)->size & 0x1) != 0)
#define CODE_SIZE(obj) ((obj)->size & ~size_t(0x1))

	// Compute the size required for 'n' refs.
	static inline size_t mpsSizeRefs(size_t refs) {
		return sizeof(GcCode) - sizeof(GcCodeRef) + sizeof(GcCodeRef)*refs;
	}

	// Compute the size needed for an object containing code of the specified size and 'n' references.
	static inline size_t mpsSizeCode(size_t code, size_t refs) {
		return align(headerSize
					+ code
					+ MPS_CHECK_BYTES
					+ mpsSizeRefs(refs)
					+ MPS_CHECK_BYTES);
	}

	// Get a pointer to the references inside a code allocation.
	static inline GcCode *mpsRefsCode(MpsObj *obj) {
		size_t code = CODE_SIZE(obj);
		void *p = toClient(obj);
		p = (byte *)p + code + MPS_CHECK_BYTES;
		return (GcCode *)p;
	}
	static inline const GcCode *mpsRefsCode(const MpsObj *obj) {
		size_t code = CODE_SIZE(obj);
		const void *p = toClient(obj);
		p = (const byte *)p + code + MPS_CHECK_BYTES;
		return (const GcCode *)p;
	}

	// Size of an object.
	static inline size_t mpsSize(const MpsObj *o) {
		if (IS_CODE(o)) {
			size_t code = CODE_SIZE(o);
			return mpsSizeCode(code, mpsRefsCode(o)->refCount);
		}

		const MpsHeader *h = o->header;

		switch (h->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
			return mpsSizeObj(&h->obj);
		case GcType::tArray:
			return mpsSizeArray(&h->obj, o->count);
		case GcType::tWeakArray:
			return mpsSizeArray(&h->obj, weakCount(o));
		case mpsPad0:
			return headerSize;
		case mpsPad:
			return headerSize + o->pad.size;
		case mpsFwd1:
			return headerSize + sizeof(MpsFwd1);
		case mpsFwd:
			return headerSize + o->fwd.size;
		default:
			dbg_assert(false, L"Unknown object found!");
			return 0;
		}
	}

	// Skip objects. Figure out the size of an object, and return a pointer to after the end of it
	// (including the next object's header, it seems).
	static mps_addr_t mpsSkip(mps_addr_t at) {
		MPS_VERIFY_OBJECT(fromClient(at));
		return (byte *)at + mpsSize(fromClient(at));
	}

	// Helper for interpreting and scanning a GcType block.
#define FIX_HEADER(header)									\
	do {													\
		mps_addr_t *d = (mps_addr_t *)&((header).type);		\
		mps_res_t r = MPS_FIX12(ss, d);						\
		if (r != MPS_RES_OK)								\
			return r;										\
	} while (false)

	// # of bytes before the vtable the allocation actually starts.
	static const nat vtableOffset = VTableCpp::vtableAllocOffset();

	// Helper for interpreting and scanning a vtable.
	// Note: we can pass the interior pointer to MPS_FIX1
	// Note: we assume vtable pointers are at offset 0
#define FIX_VTABLE(base)						\
	do {										\
		mps_addr_t d = *(mps_addr_t *)(base);	\
		if (d && MPS_FIX1(ss, d)) {				\
			d = (byte *)d - vtableOffset;		\
			mps_res_t r = MPS_FIX2(ss, &d);		\
			if (r != MPS_RES_OK)				\
				return r;						\
			d = (byte *)d + vtableOffset;		\
			*(mps_addr_t *)(base) = d;			\
		}										\
	} while (false)

	// Helper for interpreting and scanning a block of data.
#define FIX_GCTYPE(header, start, base)								\
	for (nat _i = (start); _i < (header)->obj.count; _i++) {		\
		size_t offset = (header)->obj.offset[_i];					\
		mps_addr_t *data = (mps_addr_t *)((byte *)(base) + offset);	\
		mps_res_t r = MPS_FIX12(ss, data);							\
		if (r != MPS_RES_OK)										\
			return r;												\
	}

	// Scan objects. If a MPS_FIX returns something other than MPS_RES_OK, return that code as
	// quickly as possible.
	static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = mpsSkip(at)) {
				MpsObj *o = fromClient(at);
				MPS_VERIFY_OBJECT(o);

				if (IS_CODE(o)) {
					// Scan code.
					GcCode *c = mpsRefsCode(o);

					// Scan our self-pointer to ensure that this object will be scanned after it has
					// been moved.
					mps_res_t r = MPS_FIX12(ss, &c->reserved);
					if (r != MPS_RES_OK)
						return r;
#ifdef SLOW_DEBUG
					dbg_assert(c->reserved == at, L"Invalid self-pointer!");
#endif

					for (size_t i = 0; i < c->refCount; i++) {
						GcCodeRef &ref = c->refs[i];
#ifdef SLOW_DEBUG
						dbg_assert(ref.offset < CODE_SIZE(o), L"Code offset is out of bounds!");
#endif
						// Only some kind of references needs to be scanned.
						if (ref.kind & 0x01) {
							r = MPS_FIX12(ss, &ref.pointer);
							if (r != MPS_RES_OK)
								return r;
						}
					}

					// Update the pointers in the code blob as well.
					code::updatePtrs(at, c);
				} else {
					// Scan regular objects.
					const MpsHeader *h = o->header;
					mps_addr_t pos = at;

					switch (h->type) {
					case GcType::tFixedObj:
						FIX_VTABLE(pos);
						// Fall thru.
					case GcType::tFixed:
						// PLN(L"OBJECT");
						FIX_HEADER(h->obj);
						FIX_GCTYPE(h, 0, pos);
						break;
					case GcType::tType: {
						FIX_VTABLE(pos);
						size_t offset = h->obj.offset[0];
						GcType **data = (GcType **)((byte *)pos + offset);
						FIX_HEADER(h->obj);
						if (*data) {
							// NOTE: We can probably get away without scanning this one.
							FIX_HEADER(**data);
						}
						FIX_GCTYPE(h, 1, pos);
						break;
					}
					case GcType::tArray:
						FIX_HEADER(h->obj);
						// Skip the header.
						pos = (byte *)pos + arrayHeaderSize;
						for (size_t i = 0; i < o->count; i++, pos = (byte *)pos + h->obj.stride) {
							FIX_GCTYPE(h, 0, pos);
						}
						break;
					case GcType::tWeakArray:
						FIX_HEADER(h->obj);
						// Skip the header.
						pos = (byte *)pos + arrayHeaderSize;
						for (size_t i = 0; i < weakCount(o); i++, pos = (byte *)pos + h->obj.stride) {
							for (nat j = 0; j < h->obj.count; j++) {
								size_t offset = h->obj.offset[j];
								mps_addr_t *data = (mps_addr_t *)((byte *)pos + offset);
								if (MPS_FIX1(ss, *data)) {
									mps_res_t r = MPS_FIX2(ss, data);
									if (r != MPS_RES_OK)
										return r;
									// Splatted?
									if (*data == null)
										weakSplat(o);
								}
							}
						}
						break;
					}
				}
			}
		} MPS_SCAN_END(ss);
		return MPS_RES_OK;
	}

	// Create a forwarding object at 'at' referring to 'to'. These must be recognized by mpsSize, mpsSkip and mpsScan.
	static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
		MpsObj *o = fromClient(at);
		MPS_VERIFY_OBJECT(o);

		// Create the forwarding object.
		size_t size = mpsSize(o);
#ifdef SLOW_DEBUG
		dbg_assert(size >= headerSize + sizeof(MpsFwd1), L"Not enough space for a fwd object!");
#endif
		if (size <= headerSize + sizeof(MpsFwd1)) {
			setHeader(o, &headerFwd1);
			o->fwd1.to = to;
		} else {
			setHeader(o, &headerFwd);
			o->fwd.to = to;
			o->fwd.size = size - headerSize;
		}

		// Make sure our object is still valid.
		MPS_VERIFY_OBJECT(o);
	}

	// See if object at 'at' is a forwarder, and if so, where it points to.
	static mps_addr_t mpsIsFwd(mps_addr_t at) {
		// Convert to base pointers:
		MpsObj *o = fromClient(at);
		MPS_VERIFY_OBJECT(o);

		if (IS_CODE(o))
			return null;

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
#ifdef SLOW_DEBUG
		dbg_assert(size >= headerSize, L"Too small header!");
#endif

		if (size <= headerSize) {
			setHeader(at, &headerPad0);
		} else {
			setHeader(at, &headerPad);
			MpsObj *o = (MpsObj *)at;
			o->pad.size = size - headerSize;
		}

		MPS_INIT_OBJECT((MpsObj *)at, size);
		MPS_VERIFY_OBJECT((MpsObj *)at);
	}


	/**
	 * Thread description.
	 */
	struct GcThread {
		// # of times attached.
		nat attachCount;

		// # of allocations since last check for finalization messages.
		nat lastFinalization;

		// MPS thread description.
		mps_thr_t thread;

		// Allocation point.
		mps_ap_t ap;

		// Root for this thread.
		mps_root_t root;

		// All threads running on this thread.
		const os::InlineSet<os::UThreadStack> *stacks;

#if MPS_CHECK_MEMORY
		// How many allocations ago did we validate memory?
		nat lastCheck;
#endif
	};

	/**
	 * Platform specific scanning of threads.
	 *
	 * We can not use everything MPS provides since we're implementing our own UThreads on top of
	 * kernel threads. We need to scan those stacks as well, otherwise we would be in big
	 * trouble. Sadly, this breaks some assumptions that MPS makes about how the stack behaves, so
	 * we have to do some fairly ugly stuff...
	 *
	 * TODO: Move some of these to os::UThread to centralize the knowledge about threading on the
	 * current platform.
	 */
#if defined(X86) || defined(X64)

	// Stack dummy used to recognize when the MPS wants to scan an entire stack. This is the largest
	// possible address (word aligned). Ie. 0xFF...F0
	static void *const stackDummy = (void *)((size_t)-1 & ~(wordSize - 1));

	// Get the read lower bound for the stack.
	static void *mpsStackLimit(GcThread *thread);

	// Note: We're checking all word-aligned positions as we need to make sure we're scanning
	// the return addresses into functions (which are also in this pool). MPS currently scans
	// EIP as well, which is good as the currently executing function might otherwise be moved.
	static mps_res_t mpsScanThread(mps_ss_t ss, void *base, void *limit, void *closure) {
		GcThread *thread = (GcThread *)closure;
		void **from = (void **)base;
		void **to = (void **)limit;

		if (limit == stackDummy) {
			// We shall scan the entire stack! This is a bit special, as we will more or less
			// completely ignore what MPS told us and figure it out ourselves.

			// Decrease the scanned size to zero. We update it again later.
			mps_decrease_scanned(ss, (char *)limit - (char *)base);

			// Remember the total number of bytes scanned.
			size_t bytesScanned = 0;

			// We scan all UThreads on this thread, if one of them is the currently running thread
			// their 'desc' is null. In that case we delay scanning that thread until after the
			// other threads. If we do not find an 'active' thread, we're in the middle of a thread
			// switch, which means that we can scan all threads as if they were sleeping.

			// Aside from that, we need to be aware that UThreads may be executed by another thread
			// during detours. The UThreads will always be located inside the stacks set on the
			// thread where they are intended to run. They can not be moved around while they
			// contain anything useful since it is not possible to move the UThreads between sets
			// atomically. Instead, the UThreadStack objects participating in a thread switch are
			// marked with a 'detourActive' != 0. Any such UThread shall be ignored during stack
			// scanning since they are considered to belong to another thread. The UThread is
			// instead associated with the proper thread by following the pointer inside the
			// 'detourTo' member.

			// Remember we did not find a running stack.
			to = null;

			// Scan all UThreads.
			MPS_SCAN_BEGIN(ss) {
				// Examine all UThreads running on this thread.
				os::InlineSet<os::UThreadStack>::iterator i = thread->stacks->begin();
				for (nat id = 0; i != thread->stacks->end(); ++i, id++) {
					// If this thread is used as a detour thread, do not scan it at all.
					const os::UThreadStack *first = *i;
					if (first->detourActive)
						continue;

					// Examine the main stack and all detours for this thread.
					for (const os::UThreadStack *stack = first; stack; stack = stack->detourTo) {
						// Is this thread being initialized? During initialization, a stack does not
						// contain sensible data. For example, 'desc' is probably null even if this
						// stack is not the currently running stack. If 'stackLimit' is also null we
						// will not only be confused, but we will also crash.
						if (stack->initializing)
							continue;

						// Is this the main stack of this thread?
						if (!stack->desc) {
							// We should not find two of these for any given thread.
							dbg_assert(to == null, L"We found two potential main stacks.");

							// This is the main stack! Scan that later.
							to = (void **)stack->stackLimit;
							continue;
						}

						// All is well, scan it!
						void **low = (void **)stack->desc->low;
						void **high = (void **)stack->stackLimit;

						bytesScanned += (char *)high - (char *)low;
						for (void **at = low; at < high; at++) {
							mps_res_t r = MPS_FIX12(ss, at);
							if (r != MPS_RES_OK)
								return r;
						}
					}
				}

				// If we are right in the middle of a thread switch, we will fail to find a main
				// stack. This means we have already scanned all stacks, and thus we do not need to
				// do anything more.
				if (to == null) {
#ifdef DEBUG
					// To see if this ever happens, and if it is handled correctly. This is *really*
					// rare, as we have to hit a window of ~6 machine instructions when pausing another thread.
					PLN(L"------------ We found an UThread in the process of switching! ------------");
#endif
				} else {
					bytesScanned += (char *)to - (char *)from;
					for (void **at = from; at < to; at++) {
						mps_res_t r = MPS_FIX12(ss, at);
						if (r != MPS_RES_OK)
							return r;
					}
				}
			} MPS_SCAN_END(ss);

			// Finally, write the new size back to MPS.
			mps_increase_scanned(ss, bytesScanned);
		} else {
			MPS_SCAN_BEGIN(ss) {
				for (void **at = from; at < to; at++) {
					mps_res_t r = MPS_FIX12(ss, at);
					if (r != MPS_RES_OK)
						return r;
				}
			} MPS_SCAN_END(ss);
		}

		return MPS_RES_OK;
	}

#else
#error "Implement stack scanning for your machine!"
#endif

	static mps_res_t mpsScanArray(mps_ss_t ss, void *base, size_t count) {
		MPS_SCAN_BEGIN(ss) {
			void **data = (void **)base;
			for (nat i = 0; i < count; i++) {
				mps_res_t r = MPS_FIX12(ss, &data[i]);
				if (r != MPS_RES_OK)
					return r;
			}
		} MPS_SCAN_END(ss);
		return MPS_RES_OK;
	}


	// Check return codes from MPS.
	static void check(mps_res_t result, const wchar_t *msg) {
		if (result != MPS_RES_OK)
			throw GcError(msg);
	}

	/**
	 * Parameters about the generations to create.
	 * TODO: Tweak these!
	 */
	static mps_gen_param_s generationParams[] = {
		{ 2*1024*1024, 0.85 }, // Nursery generation, 2MB
		{ 4*1024*1024, 0.45 }, // Intermediate generation, 4MB
		{ 8*1024*1024, 0.10 }, // Long-lived generation (for types, code and other things), 8MB
	};

	Gc::Gc(size_t arenaSize, nat finalizationInterval) : finalizationInterval(finalizationInterval) {
		// We work under these assumptions.
		assert(wordSize == sizeof(size_t), L"Invalid word-size");
		assert(wordSize == sizeof(void *), L"Invalid word-size");
		assert(wordSize == sizeof(MpsFwd1), L"Invalid size of MpsFwd1");
		assert(headerSize == OFFSET_OF(MpsObj, count), L"Invalid header size.");
		assert(vtableOffset >= sizeof(void *), L"Invalid vtable offset (initialization failed?)");

		mps_lib_assert_fail_install(&mps_assert_fail);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenaSize);
			check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), L"Failed to create GC arena.");
		} MPS_ARGS_END(args);

		// Note: we can use mps_arena_pause_time_set(arena, 0.0) to enforce minimal pause
		// times. Default is 0.100 s, this should be configurable from Storm somehow.

		check(mps_chain_create(&chain, arena, ARRAY_COUNT(generationParams), generationParams),
			L"Failed to set up generations.");

		MPS_ARGS_BEGIN(args) {
#if MPS_CHECK_MEMORY
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, nextPowerOfTwo(headerSize));
#else
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize); // Default alignment.
#endif
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
#if MPS_DEBUG_POOL
			check(mps_pool_create_k(&pool, arena, mps_class_ams_debug(), args), L"Failed to create a GC pool.");
#else
			check(mps_pool_create_k(&pool, arena, mps_class_amc(), args), L"Failed to create a GC pool.");
#endif
		} MPS_ARGS_END(args);

		// GcType-objects are stored in a manually managed pool. We prefer this to just malloc()ing
		// them since we can remove all of them easily whenever the Gc object is destroyed.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_MEAN_SIZE, sizeof(GcType) + 10*sizeof(size_t));
			MPS_ARGS_ADD(args, MPS_KEY_ALIGN, wordSize);
			MPS_ARGS_ADD(args, MPS_KEY_SPARE, 0.50); // Low spare, as these objects are seldom allocated/deallocated.
			check(mps_pool_create_k(&gcTypePool, arena, mps_class_mvff(), args), L"Failed to create a pool for types.");
		} MPS_ARGS_END(args);

		// Types are stored in a separate non-moving pool. This is to get around the limitation that
		// no remote references (ie. references stored outside objects) are allowed by the MPS. In
		// our GcType, however, we want to store a reference to the actual type so we can access it
		// by reflection or the as<> operation. If Type-objects are allowed to move, then when one
		// object scans and updates its GcType to point to the new position, which confuses MPS when
		// it finds that other objects suddenly lost their reference to an object it thought they
		// had. This is solved by putting Type objects in an AMS pool, which does not move
		// objects. Therefore, our approach with a (nearly) constant remote reference works in this
		// case, with full GC on types.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			// Store types in the last generation, as they are very long-lived.
			MPS_ARGS_ADD(args, MPS_KEY_GEN, ARRAY_COUNT(generationParams) - 1);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			MPS_ARGS_ADD(args, MPS_KEY_AMS_SUPPORT_AMBIGUOUS, false);
			check(mps_pool_create_k(&typePool, arena, mps_class_ams(), args), L"Failed to create a GC pool for types.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&typeAllocPoint, typePool, mps_args_none), L"Failed to create type allocation point.");

		// Weak references are stored in a separate pool which supports weak references.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			// Store weak tables in the second generation, as they are usually quite long-lived.
			MPS_ARGS_ADD(args, MPS_KEY_GEN, 2);
			check(mps_pool_create_k(&weakPool, arena, mps_class_awl(), args), L"Failed to create a weak GC pool.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_RANK, mps_rank_weak());
			check(mps_ap_create_k(&weakAllocPoint, weakPool, args), L"Failed to create weak allocation point.");
		} MPS_ARGS_END(args);

		// Code allocations.
		MPS_ARGS_BEGIN(args) {
			// TODO: Make code live in its own chain, as code allocations follow very different
			// patterns compared to other data.
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			check(mps_pool_create_k(&codePool, arena, mps_class_amc(), args), L"Failed to create a code GC pool.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&codeAllocPoint, codePool, mps_args_none), L"Failed to create code allocation point.");

#ifdef MPS_USE_IO_POOL
		// Buffer allocations.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_GEN, 2);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
#if MPS_USE_IO_POOL == 1
			check(mps_pool_create_k(&ioPool, arena, mps_class_lo(), args), L"Failed to create GC pool for buffers.");
#elif MPS_USE_IO_POOL == 2
			check(mps_pool_create_k(&ioPool, arena, mps_class_amcz(), args), L"Failed to create GC pool for buffers.");
#endif
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&ioAllocPoint, ioPool, mps_args_none), L"Failed to create buffer allocation point.");
#endif

		// We want to receive finalization messages.
		mps_message_type_enable(arena, mps_message_type_finalization());

		// Initialize.
		runningFinalizers = 0;
		ignoreFreeType = false;
	}

	Gc::~Gc() {
		if (arena)
			destroy();
	}

	void Gc::destroy() {
		// Destroy all remaining threads (if any).
		{
			util::Lock::L z(threadLock);
			for (ThreadMap::iterator i = threads.begin(); i != threads.end(); ++i) {
				detach(i->second);
				delete i->second;
			}
			threads.clear();
		}

		// Collect the entire arena. We have no roots attached, so all objects with finalizers
		// should be found. Leaves the arena in a parked state, so no garbage collections will start
		// after this one.
		mps_arena_collect(arena);

		// See if any finalizers needs to be executed. Here, we should ignore freeing any GcTypes,
		// as the finalization order for the last types are unknown.
		ignoreFreeType = true;
		checkFinalizersLocked();

		// Destroy the global allocation points.
		mps_ap_destroy(typeAllocPoint);
		mps_ap_destroy(weakAllocPoint);
		mps_ap_destroy(codeAllocPoint);
#ifdef MPS_USE_IO_POOL
		mps_ap_destroy(ioAllocPoint);
#endif

		// Clear these now, otherwise the destructor will crash badly after we've removed the backing storage.
		freeTypes.clear();

		// Destroy pools.
		mps_pool_destroy(pool);
		mps_pool_destroy(weakPool);
		mps_pool_destroy(typePool);
		mps_pool_destroy(codePool);
#ifdef MPS_USE_IO_POOL
		mps_pool_destroy(ioPool);
#endif
		// The type pool has to be destroyed last, as any formatted object (not code) might reference things in here.
		mps_pool_destroy(gcTypePool);

		// Destroy format and chains.
		mps_fmt_destroy(format);
		mps_chain_destroy(chain);
		mps_arena_destroy(arena);
		arena = null;
	}

	void Gc::collect() {
		mps_arena_collect(arena);
		mps_arena_release(arena);
		// mps_arena_step(arena, 10.0, 1);
		checkFinalizers();
	}

	bool Gc::collect(nat time) {
		TODO(L"Better value for 'multiplier' here?");
		// mps_bool_t != bool...
		return mps_arena_step(arena, time / 1000.0, 1) ? true : false;
	}

	void Gc::attachThread() {
		os::Thread thread = os::Thread::current();
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			// Already attached, increase the attached count.
			i->second->attachCount++;
		} else {
			// New thread.
		    // Note: we may leak memory here if a check() fails. This is rare enough
			// for it not to be a problem.
			GcThread *desc = new GcThread;
			threads.insert(make_pair(thread.id(), desc));

			// Register the thread with MPS.
			attach(desc, thread);
		}
	}

	void Gc::reattachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i != threads.end()) {
			i->second->attachCount++;
		} else {
			assert(false, L"Trying to re-attach a new thread!");
		}
	}

	void Gc::detachThread(const os::Thread &thread) {
		util::Lock::L z(threadLock);

		ThreadMap::iterator i = threads.find(thread.id());
		if (i == threads.end())
			return;

		if (i->second->attachCount > 1) {
			// Wait a bit.
			i->second->attachCount--;
		} else {
			// Detach the thread now.
			GcThread *desc = i->second;
			threads.erase(i);

			// Detach from MPS.
			detach(desc);
			delete desc;
		}
	}

	void Gc::attach(GcThread *desc, const os::Thread &thread) {
		desc->attachCount = 1;
		desc->lastFinalization = 0;

#if MPS_CHECK_MEMORY
		desc->lastCheck = 0;
#endif

		// Register the thread with MPS.
		check(mps_thread_reg(&desc->thread, arena), L"Failed registering a thread with the gc.");

		// Find all stacks on this os-thread.
		desc->stacks = &thread.stacks();

		// Register the thread's root. Note that we're fooling MPS on where the stack starts, as
		// we will discover this ourselves later in a platform-specific manner depending on
		// which UThread is currently running.
		check(mps_root_create_thread_scanned(&desc->root,
												arena,
												mps_rank_ambig(),
												(mps_rm_t)0,
												desc->thread,
												&mpsScanThread,
												desc,
												stackDummy),
			L"Failed creating thread root.");

		// Create an allocation point for the thread.
		check(mps_ap_create_k(&desc->ap, pool, mps_args_none), L"Failed to create an allocoation point.");
	}

	void Gc::detach(GcThread *desc) {
		mps_ap_destroy(desc->ap);
		mps_root_destroy(desc->root);
		mps_thread_dereg(desc->thread);
	}

	// Thread-local variables for remembering the current thread's allocation point. We need some
	// integrity checking to support the (rare) case of one thread allocating from different
	// Engine:s. We do this by remembering which Gc-instance the saved ap is valid for.
	static THREAD Gc *currentInfoOwner = null;
	static THREAD GcThread *currentInfo = null;

	mps_ap_t &Gc::currentAllocPoint() {
		GcThread *info = null;

		// Check if everything is as we left it. This should be fast as it is called for every allocation!
		// Note: the 'currentInfoOwner' check should be sufficient.
		// Note: we will currently not detect a thread that has been detached too early.
		if (currentInfo != null && currentInfoOwner == this) {
			info = currentInfo;
		} else {
			// Either first time or allocations from another Gc in this thread since last time.
			// This is expected to happen rarely, so it is ok to be a bit slow here.
			util::Lock::L z(threadLock);
			os::Thread thread = os::Thread::current();
			ThreadMap::const_iterator i = threads.find(thread.id());
			if (i == threads.end())
				throw GcError(L"Trying to allocate memory from a thread not registered with the GC.");

			currentInfo = info = i->second;
			currentInfoOwner = this;
		}

		// Shall we run any finalizers right now?
		if (++info->lastFinalization >= finalizationInterval) {
			info->lastFinalization = 0;
			checkFinalizers();
		}

#if MPS_CHECK_MEMORY
		// Shall we validate memory now?
		if (++info->lastCheck >= MPS_CHECK_INTERVAL) {
			info->lastCheck = 0;
			checkMemory();
		}
#endif

		return info->ap;
	}

	void *Gc::alloc(const GcType *type) {
		// Types are special.
		if (type->kind == GcType::tType)
			return allocTypeObj(type);

		assert(type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj, L"Wrong type for calling alloc().");

		size_t size = mpsSizeObj(type);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory (alloc).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Initialize any consistency-checking data.
			MPS_INIT_OBJECT((MpsObj *)memory, size);

		} while (!mps_commit(ap, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *Gc::allocTypeObj(const GcType *type) {
		assert(type->kind == GcType::tType, L"Wrong type for calling allocTypeObj().");
		return allocStatic(type);
	}

	void *Gc::allocStatic(const GcType *type) {
		assert(type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj
			|| type->kind == GcType::tType, L"Wrong type for calling allocStatic().");
		// Since we're sharing one allocation point, take the lock for it.
		util::Lock::L z(typeAllocLock);

		size_t size = mpsSizeObj(type);
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, typeAllocPoint, size), L"Out of memory (alloc type).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Initialize any consistency-checking data.
			MPS_INIT_OBJECT((MpsObj *)memory, size);

		} while (!mps_commit(typeAllocPoint, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	GcArray<Byte> *Gc::allocBuffer(size_t count) {
#ifdef MPS_USE_IO_POOL
		const GcType *type = &byteArrayType;
		size_t size = mpsSizeArray(type, count);

		util::Lock::L z(ioAllocLock);

		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ioAllocPoint, size), L"Out of memory (alloc buffer).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Set size.
			((MpsObj *)memory)->count = count;
			// 4: Initialize any consistency-checking data.
			MPS_INIT_OBJECT((MpsObj *)memory, size);

		} while (!mps_commit(ioAllocPoint, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, return memory.
		void *result = (byte *)memory + headerSize;
		return (GcArray<Byte> *)result;
#else
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
#endif
	}

	void *Gc::allocArray(const GcType *type, size_t elements) {
		assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

		size_t size = mpsSizeArray(type, elements);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory (alloc array).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Set size.
			((MpsObj *)memory)->count = elements;
			// 4: Initialize any consistency-checking data.
			MPS_INIT_OBJECT((MpsObj *)memory, size);

		} while (!mps_commit(ap, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *Gc::allocWeakArray(size_t elements) {
		if (elements == 0)
			return null;

		util::Lock::L z(weakAllocLock);

		const GcType *type = &weakArrayType;
		size_t size = mpsSizeArray(type, elements);
		mps_ap_t &ap = weakAllocPoint;
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Set size (tagged).
			((MpsObj *)memory)->weak.count = (elements << 1) | 0x1;
			((MpsObj *)memory)->weak.splatted = 0x1;
			// 4: Initialize any consistency-checking data.
			MPS_INIT_OBJECT((MpsObj *)memory, size);

		} while (!mps_commit(ap, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, and return the allocated memory.
		return (byte *)memory + headerSize;
	}

	size_t Gc::typeSize(size_t entries) {
		return sizeof(MpsType) + entries*sizeof(size_t) - sizeof(size_t);
	}

	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = typeSize(entries);
		MpsType *t;// = (GcType *)malloc(s);
		check(mps_alloc((mps_addr_t *)&t, gcTypePool, s), L"Failed to allocate type info.");
		memset(t, 0, s);
		new (t) MpsType();
		t->type.kind = kind;
		t->type.type = type;
		t->type.stride = stride;
		t->type.count = entries;
		return &t->type;
	}

	GcType *Gc::allocType(const GcType *src) {
		size_t s = typeSize(src->count);
		MpsType *t;
		check(mps_alloc((mps_addr_t *)&t, gcTypePool, s), L"Failed to allocate type info.");
		new (t) MpsType();

		memcpy(&t->type, src, s);
		return &t->type;
	}

	struct MarkData {
		// Format to look for.
		mps_fmt_t fmt;
	};

	void Gc::markType(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t) {
		MarkData *data = (MarkData *)p;

		// Does this address belong to the correct format?
		if (fmt != data->fmt)
			return;

		// Mark the type.
		GcType *t = (GcType *)Gc::typeOf(addr);
		if (!t)
			return;

		MpsType *base = BASE_PTR(MpsType, t, type);
		base->reachable = true;

		// If this is a GcType::tType-object, also mark that type!
		if (t->kind == GcType::tType) {
			size_t offset = t->offset[0];
			GcType *t2 = *(GcType **)((byte *)addr + offset);
			if (t2) {
				MpsType *base2 = BASE_PTR(MpsType, t2, type);
				base2->reachable = true;
			}
		}
	}

	void Gc::freeType(GcType *t) {
		if (ignoreFreeType)
			return;
		if (!t)
			return;

		// It is possible we can not yet reclaim 't'. Put it in the free set instead.
		MpsType *base = BASE_PTR(MpsType, t, type);
		freeTypes.insert(base);

		// Shall we try to reclaim all freed types? This is expensive, so only do it rarely!
		if (freeTypes.count() > 100) {
			// Ensure no GC:s in flight.
			mps_arena_park(arena);

			// Mark all as unseen.
			os::InlineSet<MpsType>::iterator end = freeTypes.end();
			for (os::InlineSet<MpsType>::iterator i = freeTypes.begin(); i != end; ++i) {
				i->reachable = false;
			}

			// Iterate through the heap and mark all reachable nodes.
			MarkData d = { format };
			mps_arena_formatted_objects_walk(arena, &markType, &d, 0);

			// Release the GC once more.
			mps_arena_release(arena);

			// Remove all unseen nodes.
			size_t removed = 0;
			for (os::InlineSet<MpsType>::iterator i = freeTypes.begin(); i != end; ++i) {
				if (!i->reachable) {
					MpsType *e = *i;
					freeTypes.erase(e);

					size_t s = typeSize(e->type.count);
					mps_free(gcTypePool, e, s);
					removed++;
				}
			}
		}
	}

	const GcType *Gc::typeOf(const void *mem) {
		const MpsObj *o = fromClient(mem);
		if (IS_CODE(o))
			return null;
		else
			return &o->header->obj;
	}

	void Gc::switchType(void *mem, const GcType *type) {
		assert(typeOf(mem)->stride == type->stride, L"Can not change size of allocations.");
		assert(typeOf(mem)->kind == type->kind, L"Can not change kind of allocations.");

		// Seems reasonable. Switch headers!
		void *t = (byte *)mem - headerSize;
		setHeader(t, type);
	}

	void Gc::checkFinalizers() {
		if (atomicCAS(runningFinalizers, 0, 1) != 0) {
			// Someone else is already checking finalizers.
			return;
		}

		try {
			checkFinalizersLocked();
		} catch (...) {
			// Clear the flag.
			atomicWrite(runningFinalizers, 0);
			throw;
		}

		// Clear the flag.
		atomicWrite(runningFinalizers, 0);
	}

	void Gc::checkFinalizersLocked() {
		mps_message_t message;
		while (mps_message_get(&message, arena, mps_message_type_finalization())) {
			mps_addr_t obj;
			mps_message_finalization_ref(&obj, arena, message);
			mps_message_discard(arena, message);

			finalizeObject(obj);
		}
	}

	void Gc::finalizeObject(void *obj) {
		const GcType *t = typeOf(obj);

		if (!t) {
			// A code allocation. Call the finalizer over in the Code lib.
			code::finalize(obj);
		} else if (t->finalizer) {
			// An object might not yet be initialized...
			if ((t->kind != GcType::tFixedObj) || (vtable::from((RootObject *)obj) != null)) {
				typedef void (*Fn)(void *);
				Fn fn = (Fn)t->finalizer;
				(*fn)(obj);
			}
		}

		// NOTE: It is not a good idea to replace finalized objects with padding as we might have
		// finalization cycles, which means that one object depends on another object. If we were to
		// convert the objects into padding, then the vtables would be destroyed.

		// Replace the object with padding, as it is not neccessary to scan it anymore.
		// obj = (char *)obj - headerSize;
		// size_t size = mpsSize(obj);
		// mpsMakePad(obj, size);
	}

	void *Gc::allocCode(size_t code, size_t refs) {
		code = wordAlign(code);

		size_t size = mpsSizeCode(code, refs);
		dbg_assert(size > headerSize, L"Can not allocate zero-sized chunks of code!");

		mps_addr_t memory;
		util::Lock::L z(codeAllocLock);

		do {
			check(mps_reserve(&memory, codeAllocPoint, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the size.
			MpsObj *obj = (MpsObj *)memory;
			obj->size = code | 0x1;
			// 3: Set self pointer and # of refs.
			GcCode *codeRefs = mpsRefsCode(obj);
			codeRefs->reserved = (byte *)memory + headerSize;
			void *refPtr = codeRefs;
			*(size_t *)refPtr = refs;
			// 4: Init checking data.
			MPS_INIT_OBJECT(obj, size);

		} while (!mps_commit(codeAllocPoint, memory, size));

		MPS_VERIFY_SIZE((MpsObj *)memory);

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		// Register for finalization if the backend asks us to.
		if (code::needFinalization())
			mps_finalize(arena, &result);

		return result;
	}

	size_t Gc::codeSize(const void *alloc) {
		const MpsObj *o = fromClient(alloc);
		if (IS_CODE(o)) {
			return CODE_SIZE(o);
		} else {
			dbg_assert(false, L"Attempting to get the size of a non-code block.");
			return 0;
		}
	}

	GcCode *Gc::codeRefs(void *alloc) {
		MpsObj *o = fromClient(alloc);
		return mpsRefsCode(o);
	}

	struct WalkData {
		mps_fmt_t fmt;
		Gc::WalkCb fn;
		void *data;
	};

	static void walkFn(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t s) {
		WalkData *d = (WalkData *)p;
		if (fmt != d->fmt)
			return;

		const GcType *type = Gc::typeOf(addr);
		if (!type)
			return;

		switch (type->kind) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
			// Objects, let them through!
			(*d->fn)((RootObject *)addr, d->data);
			break;
		}
	}

	void Gc::walkObjects(WalkCb fn, void *data) {
		mps_arena_park(arena);

		WalkData d = {
			format,
			fn,
			data
		};
		mps_arena_formatted_objects_walk(arena, &walkFn, &d, 0);
		mps_arena_release(arena);
	}


	struct Gc::Root {
		mps_root_t root;
	};

	Gc::Root *Gc::createRoot(void *data, size_t count) {
		return createRoot(data, count, false);
	}

	Gc::Root *Gc::createRoot(void *data, size_t count, bool ambig) {
		Root *r = new Root;
		try {
			check(mps_root_create(&r->root,
									arena,
									ambig ? mps_rank_ambig() : mps_rank_exact(),
									(mps_rm_t)0,
									&mpsScanArray,
									data,
									count),
				L"Failed to create a root.");
		} catch (...) {
			delete r;
			throw;
		}
		return r;
	}

	void Gc::destroyRoot(Root *r) {
		if (r) {
			mps_root_destroy(r->root);
			delete r;
		}
	}

#if MPS_CHECK_MEMORY

	static void checkBarrier(const MpsObj *obj, const byte *start, nat count, byte pattern, const wchar *type) {
		size_t first = MPS_CHECK_BYTES, last = 0;

		for (size_t i = 0; i < MPS_CHECK_BYTES; i++) {
			if (start[i] != pattern) {
				first = min(first, i);
				last = max(last, i);
			}
		}

		dbg_assert(first > last, objInfo(obj)
				+ L" has an invaild " + type + L" barrier in bytes " + ::toS(first) + L" to " + ::toS(last));
	}

	static void checkHeader(const MpsObj *obj) {
		checkBarrier(obj, (byte *)obj->barrier, MPS_CHECK_BYTES, MPS_HEADER_DATA, L"header");
	}

	// Assumes there is a footer.
	static void checkFooter(const MpsObj *obj) {
		size_t size = obj->totalSize;
		checkBarrier(obj, (const byte *)obj + size - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_FOOTER_DATA, L"footer");

		if (IS_CODE(obj)) {
			const GcCode *c = mpsRefsCode(obj);
			checkBarrier(obj, (const byte *)c - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_MIDDLE_DATA, L"middle");

			// NOTE: We can not actually check this here, as objects are checked before any FIX
			// operations have been done.
			// dbg_assert(c->reserved != toClient(obj), L"Invalid self-pointer in code segment.");
		}
	}

	static bool hasFooter(const MpsObj *obj) {
		if (IS_CODE(obj))
			return true;

		switch (obj->header->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
		case GcType::tArray:
		case GcType::tWeakArray:
			return true;
		}
		return false;
	}

	static void checkSize(const MpsObj *obj) {
		size_t computed = mpsSize(obj);
		size_t expected = obj->totalSize;
		dbg_assert(computed == expected,
				objInfo(obj) + L": Size does not match. Expected " + ::toS(expected)
				+ L", but computed " + ::toS(computed));
	}

	static void checkObject(const MpsObj *obj) {
		checkHeader(obj);
		checkSize(obj);
		if (hasFooter(obj))
			checkFooter(obj);
	}

	static void initObject(MpsObj *obj, size_t size) {
		// if (obj->allocId == 1) DebugBreak();
		obj->totalSize = size;
		obj->allocId = allocId++;
		memset(obj->barrier, MPS_HEADER_DATA, MPS_CHECK_BYTES);
		if (hasFooter(obj))
			memset((byte *)obj + size - MPS_CHECK_BYTES, MPS_FOOTER_DATA, MPS_CHECK_BYTES);
		if (IS_CODE(obj))
			memset((byte *)mpsRefsCode(obj) - MPS_CHECK_BYTES, MPS_MIDDLE_DATA, MPS_CHECK_BYTES);
	}

	void Gc::checkPoolMemory(const void *object, bool recursive) {
		object = (const byte *)object - headerSize;
		const MpsObj *obj = (const MpsObj *)object;
		const MpsHeader *header = obj->header;

		checkHeader(obj);

		if (IS_CODE(obj)) {
			checkSize(obj);
			if (hasFooter(obj))
				checkFooter(obj);
			return;
		} else {
			mps_pool_t headerPool = null;
			if (mps_addr_pool(&headerPool, arena, (void *)header)) {
				dbg_assert(headerPool == gcTypePool, objInfo(obj)
						+ toHex(object) + L" has an invalid header: " + toHex(header));
			} else {
				// We do support statically allocated object descriptions.
				dbg_assert(readable(header), objInfo(obj)
						+ L" has an invalid header: " + toHex(header));
			}
		}

		checkSize(obj);
		if (hasFooter(obj))
			checkFooter(obj);

		switch (header->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
			if (recursive) {
				// Check pointers as well.
				for (nat i = 0; i < header->obj.count; i++) {
					size_t offset = header->obj.offset[i] + headerSize;
					mps_addr_t *data = (mps_addr_t *)((const byte *)obj + offset);
					checkMemory(*data, false);
				}
			}
			break;
		case GcType::tArray:
		case GcType::tWeakArray:
			break;
		case mpsPad0:
		case mpsPad:
		case mpsFwd1:
		case mpsFwd:
			// No validation to do.
			break;
		default:
			dbg_assert(false, objInfo(obj) + L" has an unknown GcType!");
		}
	}

	// Called by MPS for every object on the heap. This function may not call the MPS, nor access
	// other memory managed by the MPS (except for non-protecting pools such as our GcType-pool).
	static void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t) {
		Gc *me = (Gc *)p;

		if (p == me->pool || p == me->typePool || p == me->codePool || p == me->weakPool)
			me->checkPoolMemory(addr, true);
#ifdef MPS_USE_IO_POOL
		if (p == me->ioPool)
			me->checkPoolMemory(addr, true);
#endif
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		mps_addr_t obj = (mps_addr_t)object;
		mps_pool_t p;
		mps_addr_pool(&p, arena, obj);

		if (p == pool || p == typePool || p == codePool || p == weakPool)
			checkPoolMemory(obj, recursive);
#ifdef MPS_USE_IO_POOL
		if (p == ioPool)
			checkPoolMemory(obj, recursive);
#endif
	}

	void Gc::checkMemory() {
		mps_pool_check_fenceposts(pool);
		mps_pool_check_free_space(pool);
		mps_arena_park(arena);
		mps_arena_formatted_objects_walk(arena, &checkObject, this, 0);
		mps_arena_release(arena);
	}

	void Gc::checkMemoryCollect() {
		mps_pool_check_fenceposts(pool);
		mps_pool_check_free_space(pool);

		mps_arena_collect(arena);
		mps_arena_formatted_objects_walk(arena, &checkObject, this, 0);
		mps_arena_release(arena);
	}

#else
	void Gc::checkPoolMemory(const void *object, bool recursive) {
		// Nothing to do.
	}

	void Gc::checkMemory(const void *, bool) {
		// Nothing to do.
	}

	void Gc::checkMemory() {
		// Nothing to do.
	}

	void Gc::checkMemoryCollect() {
		// Nothing to do.
	}
#endif

	void Gc::checkMemory(const void *ptr) {
		checkMemory(ptr, true);
	}

	class MpsGcWatch : public GcWatch {
	public:
		MpsGcWatch(Gc &gc) : gc(gc) {
			mps_ld_reset(&ld, gc.arena);
		}

		MpsGcWatch(Gc &gc, const mps_ld_s &ld) : gc(gc), ld(ld) {}

		virtual void add(const void *addr) {
			mps_ld_add(&ld, gc.arena, (mps_addr_t)addr);
		}

		virtual void remove(const void *addr) {
			// Not supported by the MPS. Ignore it, as it will 'just' generate false positives, not
			// break anything.
		}

		virtual void clear() {
			mps_ld_reset(&ld, gc.arena);
		}

		virtual bool moved(const void *addr) {
			return mps_ld_isstale(&ld, gc.arena, (mps_addr_t)addr) ? true : false;
		}

		virtual GcWatch *clone() const {
			return new (gc.alloc(&type)) MpsGcWatch(gc, ld);
		}

		static const GcType type;

	private:
		Gc &gc;
		mps_ld_s ld;
	};

	const GcType MpsGcWatch::type = {
		GcType::tFixed,
		null,
		null,
		sizeof(MpsGcWatch),
		0,
		{},
	};

	GcWatch *Gc::createWatch() {
		return new (alloc(&MpsGcWatch::type)) MpsGcWatch(*this);
	}
}

#elif defined(STORM_GC_MALLOC)

namespace storm {

	// Word-align something.
	static inline size_t align(size_t data) {
		return (data + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
	}

	static const size_t headerSizeWords = 4;

	Gc::Gc(size_t initial, nat finalizationInterval) : finalizationInterval(finalizationInterval) {}

	Gc::~Gc() {}

	void Gc::destroy() {}

	void Gc::collect() {}

	bool Gc::collect(nat time) {
		return false;
	}

	void Gc::attachThread() {}

	void Gc::reattachThread(const os::Thread &thread) {}

	void Gc::detachThread(const os::Thread &thread) {}

	void *Gc::alloc(const GcType *type) {
		size_t size = align(type->stride) + headerSizeWords*sizeof(size_t);
		void *mem = malloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;
		void *start = (size_t *)mem + headerSizeWords;
		assert(typeOf(start) == type);
		return start;
	}

	void *Gc::allocStatic(const GcType *type) {
		return alloc(type);
	}

	GcArray<Byte> *Gc::allocBuffer(size_t count) {
		return allocArray(&byteArrayType, count);
	}

	void *Gc::allocArray(const GcType *type, size_t count) {
		size_t size = align(type->stride)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = malloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = type;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = count;
		return start;
	}

	void *Gc::allocWeakArray(size_t count) {
		size_t size = sizeof(void*)*count + 2*sizeof(size_t) + headerSizeWords*sizeof(size_t);
		void *mem = malloc(size);
		memset(mem, 0, size);
		memset(mem, 0xFF, headerSizeWords*sizeof(size_t));

		*(const GcType **)mem = &weakArrayType;

		void *start = (size_t *)mem + headerSizeWords;
		*(size_t *)start = (count << 1) | 1;
		return start;
	}

	static size_t typeSize(size_t entries) {
		return sizeof(GcType) + entries*sizeof(size_t) - sizeof(size_t);
	}

	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = typeSize(entries);
		GcType *t = (GcType *)malloc(s);
		memset(t, 0, s);
		t->kind = kind;
		t->type = type;
		t->stride = stride;
		t->count = entries;
		return t;
	}

	GcType *Gc::allocType(const GcType *src) {
		size_t s = typeSize(src->count);
		GcType *t = (GcType *)malloc(s);
		memcpy(t, src, s);
		return t;
	}

	void Gc::freeType(GcType *type) {
		free(type);
	}

	const GcType *Gc::typeOf(const void *mem) {
		const GcType **data = (const GcType **)mem;

		for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
			byte *addr = (byte *)data - i - 1;
			if (*addr != 0xFF) {
				PLN(L"Wrote before the object: " << mem << L" offset: -" << (i+1));
				DebugBreak();
			}
		}


		return *(data - headerSizeWords);
	}

	void Gc::switchType(void *mem, const GcType *to) {
		const GcType **data = (const GcType **)mem;
		*(data - headerSizeWords) = to;
	}

	void *Gc::allocCode(size_t code, size_t refs) {
		static memory::Manager mgr;

		code = align(code);
		size_t size = code + sizeof(GcCode) + refs*sizeof(GcCodeRef) - sizeof(GcCodeRef) + headerSizeWords*sizeof(size_t);
		void *mem = mgr.allocate(size);
		memset(mem, 0, size);
		memset((size_t *)mem + 1, 0xFF, (headerSizeWords - 1)*sizeof(size_t));

		*(size_t *)mem = code;

		void *start = (void **)mem + headerSizeWords;
		void *refPtr = (byte *)start + code;
		*(size_t *)refPtr = refs;

		return start;
	}

	size_t Gc::codeSize(const void *alloc) {
		const size_t *d = (const size_t *)alloc;
		return *(d - headerSizeWords);
	}

	GcCode *Gc::codeRefs(void *alloc) {
		void *p = ((byte *)alloc) + align(codeSize(alloc));
		return (GcCode *)p;
	}

	void Gc::walkObjects(WalkCb fn, void *param) {
		// Nothing to do...
	}

	Gc::Root *Gc::createRoot(void *data, size_t count) {
		// No roots here!
		return null;
	}

	void Gc::destroyRoot(Root *root) {}

	class MallocWatch : public GcWatch {
	public:
		virtual void add(const void *addr) {}
		virtual void remove(const void *addr) {}
		virtual void clear() {}
		virtual bool moved(const void *addr) { return false; }
		virtual GcWatch *clone() const {
			return new MallocWatch();
		}
	};

	GcWatch *Gc::createWatch() {
		return new MallocWatch();
	}

	void Gc::checkMemory() {}

	void Gc::checkMemory(const void *object) {
		checkMemory(object, true);
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		const GcType **data = (const GcType **)object;

		for (nat i = 0; i < (headerSizeWords-1)*sizeof(size_t); i++) {
			byte *addr = (byte *)data - i - 1;
			if (*addr != 0xFF) {
				PLN(L"Wrote before the object: " << object << L" offset: -" << (i+1));
				DebugBreak();
			}
		}
	}

	void Gc::checkMemoryCollect() {}

}

#else

#error "Unsupported or unknown GC chosen. See Storm.h for details."

#endif


namespace storm {

	/**
	 * Shared stuff.
	 */

	const GcType Gc::weakArrayType = {
		GcType::tWeakArray,
		null,
		null,
		sizeof(void *),
		1,
		{}
	};

	/**
	 * Simple linked list to test gc.
	 */
	struct GcLink {
	    nat value;
		GcLink *next;
	};

	static const GcType linkType = {
		GcType::tFixed,
		null,
		null,
		sizeof(GcLink),
		1,
		{ OFFSET_OF(GcLink, next) }
	};

	bool Gc::test(nat times) {
		bool ok = true;

		for (nat j = 0; j < times && ok; j++) {
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

			at = first;
			for (nat i = 0; i < 10000; i++) {
				if (!at) {
					PLN("Premature end at " << i);
					ok = false;
					break;
				}

				if (at->value != i) {
					PLN("Failed: " << at->value << " should be " << i);
					ok = false;
				}

				at = at->next;
			}
			// collect();
		}

		return ok;
	}

}
