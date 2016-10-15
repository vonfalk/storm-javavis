#include "stdafx.h"
#include "Gc.h"
#include "Type.h" // For debugging heap corruptions.
#include "Core/Str.h"  // For debugging heap corruptions.
#include "Utils/Memory.h"

#ifdef STORM_GC_MPS

/**
 * MPS version.
 */

// If enabled, add and verify data after each object allocated in the Gc-heap in order to find
// memory-corruption bugs.
#define MPS_CHECK_MEMORY 0

// Number of words that shall be verified.
#define MPS_CHECK_WORDS 16

// Data to check against.
#define MPS_CHECK_DATA 0xDD

// Check the heap after this many allocations.
#define MPS_CHECK_INTERVAL 100


#if MPS_CHECK_MEMORY
#define MPS_CHECK_BYTES (sizeof(void *)*MPS_CHECK_WORDS)
#define MPS_CHECK_INIT(base, size)										\
	memset(((byte *)(base)) + (size) - MPS_CHECK_BYTES, MPS_CHECK_DATA, MPS_CHECK_BYTES)
#else
#define MPS_CHECK_BYTES 0
#define MPS_CHECK_INIT(base, size)
#endif


namespace storm {

	/**
	 * In MPS, we allocate objects with an extra pointer just before the object. This points to a
	 * MpsObj union which contains the information required for scanning. We also make sure to
	 * allocate objects that are a whole number of words, to make it easier to reason about what
	 * needs to be done in the special forwarders and padding types.
	 */


	static const size_t wordSize = sizeof(void *);
	static const size_t headerSize = wordSize;
	static const size_t arrayHeaderSize = wordSize * 2;

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
		const MpsHeader *header;

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

	// Return the size of a weak array.
	static inline size_t weakCount(MpsObj *o) {
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

	// Size of object.
	static inline size_t mpsSize(mps_addr_t o) {
		MpsObj *obj = (MpsObj *)o;
		const MpsHeader *h = obj->header;
		size_t s = 0;

		switch (h->type) {
		case GcType::tFixed:
		case GcType::tType:
			s = h->obj.stride + MPS_CHECK_BYTES;
			break;
		case GcType::tArray:
			s = arrayHeaderSize + h->obj.stride*obj->count + MPS_CHECK_BYTES;
			break;
		case GcType::tWeakArray:
			s = arrayHeaderSize + h->obj.stride*weakCount(obj) + MPS_CHECK_BYTES;
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

	// Skip objects. Figure out the size of an object, and return a pointer to after the end of it
	// (including the next object's header, it seems).
	static mps_addr_t mpsSkip(mps_addr_t at) {
		return (byte *)at + mpsSize((byte *)at - headerSize);
	}

	// Helper for interpreting and scanning a GcType block.
#define FIX_HEADER(header)									\
	{														\
		mps_addr_t *d = (mps_addr_t *)&((header).type);		\
		mps_res_t r = MPS_FIX12(ss, d);						\
		if (r != MPS_RES_OK)								\
			return r;										\
	}

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
					FIX_HEADER(h->obj);
					FIX_GCTYPE(h, 0, pos);
					break;
				case GcType::tType: {
					size_t offset = h->obj.offset[0];
					GcType **data = (GcType **)((byte *)pos + offset);
					FIX_HEADER(h->obj);
					if (*data) {
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

	/**
	 * Code scanning. Note: if the size is marked with a 1 in the highest position ('codeMask'
	 * below), the type of the allocation is special: the size should be treated as one of the
	 * mpsXxx variants in the MpsTypes enum above.
	 */

	static const size_t codeMask = size_t(1) << (sizeof(size_t)*CHAR_BIT - 1);

	static const size_t mpsCodePad0 = mpsPad0 | codeMask;
	static const size_t mpsCodePad = mpsPad | codeMask;
	static const size_t mpsCodeFwd1 = mpsFwd1 | codeMask;
	static const size_t mpsCodeFwd = mpsFwd | codeMask;

	/**
	 * Describes the layout of code allocations for easy access.
	 */
	struct MpsCode {
		// Header or size of code chunk. If equal to one of MpsCodeType, then members of the union
		// are valid.
		size_t header;

		// Read only if header indicates they are valid!
		union {
			MpsPad0 pad0;
			MpsPad pad;
			MpsFwd1 fwd1;
			MpsFwd fwd;
		};
	};

	static size_t mpsSizeCode(mps_addr_t at) {
		MpsCode *c = (MpsCode *)at;
		switch (c->header) {
		case mpsCodePad0:
			return wordSize;
		case mpsCodePad:
			return wordSize + c->pad.size;
		case mpsCodeFwd1:
			return wordSize * 2;
		case mpsCodeFwd:
			return wordSize + c->fwd.size;
		default:
			// Regular code.
			at = ((byte *)at) + c->header + wordSize;
			// 'at' now points at 'refCount' in 'GcCode'.
			// Compute total size:
			return c->header + wordSize + sizeof(GcCode) + (*(size_t *)at)*sizeof(GcCodeRef) - sizeof(GcCodeRef);
		}
	}

	static mps_addr_t mpsSkipCode(mps_addr_t at) {
		return (byte *)at + mpsSizeCode((byte *)at - wordSize);
	}

	static mps_res_t mpsScanCode(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		// Convert from client pointers to 'real' pointers:
		base = (byte *)base - headerSize;
		limit = (byte *)limit - headerSize;

		mps_res_t result = MPS_RES_OK;

		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = (byte *)at + mpsSizeCode(at)) {
				MpsCode *h = (MpsCode *)at;

				// Special kind of allocation?
				if (h->header & codeMask)
					continue;

				byte *code = (byte *)at + wordSize;
				GcCode *c = (GcCode *)(code + h->header);
				size_t tmp;

				for (nat i = 0; i < c->refCount; i++) {
					GcCodeRef &ref = c->refs[i];
					void *offset = code + ref.offset;

					switch (ref.kind) {
					case GcCodeRef::disabled:
						break;
					case GcCodeRef::rawPtr:
						result = MPS_FIX12(ss, (mps_addr_t *)offset);
						break;
					case GcCodeRef::relativePtr:
						tmp = *(size_t *)offset;
						if (tmp) {
							tmp += size_t(offset) + sizeof(size_t);
							result = MPS_FIX12(ss, (mps_addr_t *)&tmp);
							*(size_t *)offset = tmp - size_t(offset) - sizeof(size_t);
						}
						break;
					case GcCodeRef::relative:
					case GcCodeRef::inside:
						// No need to show these to the gc.
						break;
					default:
						assert(false, L"Unknown reference type found in a code block.");
						break;
					}

					// On error, return asap!
					if (result != MPS_RES_OK)
						return result;
				}
			}
		} MPS_SCAN_END(ss);

		return result;
	}

	static void mpsMakeFwdCode(mps_addr_t at, mps_addr_t to) {
		size_t delta = size_t(to) - size_t(at);

		// Convert to base pointers:
		at = (byte *)at - wordSize;

		size_t size = mpsSizeCode(at);
		MpsCode *c = (MpsCode *)at;

		// See if we need to modify pointers in the new object.
		if ((c->header & codeMask) == 0) {
			GcCode *gc = (GcCode *)((byte *)at + wordSize + c->header);

			for (nat i = 0; i < gc->refCount; i++) {
				GcCodeRef &ref = gc->refs[i];
				void *offset = (byte *)to + ref.offset;

				switch (ref.kind) {
				case GcCodeRef::disabled:
				case GcCodeRef::rawPtr:
					// No need to do anything here.
					break;
				case GcCodeRef::relativePtr:
				case GcCodeRef::relative:
					// Pointers to something outside of us relative something else needs to be
					// updated.
					*(size_t *)offset -= delta;
					break;
				case GcCodeRef::inside:
					*(size_t *)offset += delta;
					break;
				default:
					assert(false, L"Unknown reference type found in a code block.");
					break;
				}
			}
		}

		if (size <= 2 * wordSize) {
			c->header = mpsCodeFwd1;
			c->fwd1.to = to;
		} else {
			c->header = mpsCodeFwd;
			c->fwd.to = to;
			c->fwd.size = size - wordSize;
		}
	}

	static mps_addr_t mpsIsFwdCode(mps_addr_t at) {
		at = (byte *)at - wordSize;

		MpsCode *h = (MpsCode *)at;
		switch (h->header) {
		case mpsCodeFwd1:
			return h->fwd1.to;
		case mpsCodeFwd:
			return h->fwd.to;
		default:
			return null;
		}
	}

	static void mpsMakePadCode(mps_addr_t at, size_t size) {
		MpsCode *to = (MpsCode *)at;
		if (size <= wordSize) {
			to->header = mpsCodePad0;
		} else {
			to->header = mpsCodePad;
			to->pad.size = size - headerSize;
		}
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
		const util::InlineSet<os::UThreadStack> *stacks;

#if defined(WINDOWS)
		struct TIB {
			void *sehFrame;
			void *stackBase;
			void *stackLimit;
		};

		// Location of the TIB for this thread.
		TIB *tib;
#endif

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
#if defined(X86) && defined(WINDOWS)

	// Set to 0xFFFFFF so that MPS will not discard this thread when we switch stacks. Note, it must
	// be properly word-aligned.
	static void * const stackDummy = (void *)((size_t)-1 & ~(wordSize - 1));

	static void mpsAttach(GcThread *thread) {
		GcThread::TIB *tmp;
		__asm {
			// At offset 0x18, the linear address of the TIB is stored.
			mov eax, fs:[0x18];
			mov tmp, eax;
		}
		thread->tib = tmp;
	}

	// Note: We're checking all word-aligned positions as we need to make sure we're scanning
	// the return addresses into functions (which are also in this pool). MPS currently scans
	// EIP as well, which is good as the currently executing function might otherwise be moved.
	static mps_res_t mpsScanThread(mps_ss_t ss, void *base, void *limit, void *closure) {
		GcThread *thread = (GcThread *)closure;
		void **from = (void **)base;
		void **to = (void **)limit;

		if (limit == stackDummy) {
			// Read the current stack base from TIB and replace the dummy.
			to = (void **)thread->tib->stackBase;

			// We shall scan the entire stack! This is a bit special, as we will more or less
			// completely ignore what MPS told us and figure it out ourselves.

			// Decrease the scanned size to zero. We update it again later.
			mps_decrease_scanned(ss, (char *)limit - (char *)base);

			// We scan all UThreads on this os thread, and watch out if 'to' is equal to a UThread
			// we scanned. If that is the case, we're in the middle of a thread switch. That means
			// that whatever is in 'from' and 'to' are not consistent, so we shall ignore
			// them. However, that means that all UThreads have proper states saved to them, which
			// we can scan anyway.
			size_t bytesScanned = 0;
			bool ignoreMainStack = false;

			// Scan all UThreads.
			MPS_SCAN_BEGIN(ss) {
				util::InlineSet<os::UThreadStack>::iterator i = thread->stacks->begin();
				for (nat id = 0; i != thread->stacks->end(); ++i, id++) {
					os::UThreadStack::Desc *desc = i->desc;
					if (!desc)
						continue;

					bytesScanned += (char *)desc->high - (char *)desc->low;
					for (void **at = (void **)desc->low; at < (void **)desc->high; at++) {
						mps_res_t r = MPS_FIX12(ss, at);
						if (r != MPS_RES_OK)
							return r;
					}

					// Ignore the main stack later?
					ignoreMainStack |= desc->high == to;
				}

				// Scan the main stack if we need to.
				if (ignoreMainStack) {
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
	static void check(mps_res_t result, const wchar *msg) {
		if (result != MPS_RES_OK)
			throw GcError(msg);
	}

	/**
	 * Parameters about the generations to create.
	 * TODO: Tweak these!
	 */
	static mps_gen_param_s generationParams[] = {
		{ 1024, 0.85 }, // Nursery generation, 1MB
		{ 2048, 0.45 }, // Intermediate generation, 2MB
		{ 4096, 0.10 }, // Long-lived generation (for types, code and other things), 4MB
	};

	Gc::Gc(size_t arenaSize, nat finalizationInterval) : finalizationInterval(finalizationInterval) {
		// We work under this assumption.
		assert(wordSize == sizeof(size_t), L"Invalid word-size");
		assert(wordSize == sizeof(void *), L"Invalid word-size");
		assert(wordSize == sizeof(MpsFwd1), L"Invalid size of MpsFwd1");
		assert(wordSize == headerSize, L"Too large header");

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenaSize);
			check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), L"Failed to create GC arena.");
		} MPS_ARGS_END(args);

		// Note: we can use mps_arena_pause_time_set(arena, 0.0) to enforce minimal pause
		// times. Default is 0.100 s, this should be configurable from Storm somehow.

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
			MPS_ARGS_ADD(args, MPS_KEY_AMS_SUPPORT_AMBIGUOUS, FALSE);
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
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_HEADER_SIZE, wordSize);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, &mpsScanCode);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, &mpsSkipCode);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, &mpsMakeFwdCode);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, &mpsIsFwdCode);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, &mpsMakePadCode);
			check(mps_fmt_create_k(&codeFormat, arena, args), L"Failed to create code format.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			// TODO: Make code live in its own chain, as code allocations follow very different
			// patterns compared to other data.
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, codeFormat);
			check(mps_pool_create_k(&codePool, arena, mps_class_amc(), args), L"Failed to create a code GC pool.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&codeAllocPoint, codePool, mps_args_none), L"Failed to create code allocation point.");

		// We want to receive finalization messages.
		mps_message_type_enable(arena, mps_message_type_finalization());

		// Initialize.
		runningFinalizers = 0;
		ignoreFreeType = false;
	}

	Gc::~Gc() {
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

		// Destroy pools.
		mps_pool_destroy(pool);
		mps_pool_destroy(weakPool);
		mps_pool_destroy(typePool);
		mps_pool_destroy(codePool);
		// The type pool has to be destroyed last, as any formatted object (not code) might reference things in here.
		mps_pool_destroy(gcTypePool);

		// Destroy format and chains.
		mps_fmt_destroy(codeFormat);
		mps_fmt_destroy(format);
		mps_chain_destroy(chain);
		mps_arena_destroy(arena);
	}

	void Gc::collect() {
		mps_arena_collect(arena);
		mps_arena_release(arena);
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
			WARNING(L"Trying to re-attach a new thread!");
			assert(false);
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

		// Fill in anything else the MPS needs to scan.
		mpsAttach(desc);

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

		assert(type->kind == GcType::tFixed, L"Wrong type for calling alloc().");

		size_t size = align(type->stride + headerSize) + MPS_CHECK_BYTES;
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Initialize any consistency-checking data.
			MPS_CHECK_INIT(memory, size);

		} while (!mps_commit(ap, memory, size));

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *Gc::allocTypeObj(const GcType *type) {
		assert(type->kind == GcType::tType, L"Wrong type for calling allocTypeObj().");

		// Since we're sharing one allocation point, take the lock for it.
		util::Lock::L z(typeAllocLock);

		size_t size = align(type->stride + headerSize) + MPS_CHECK_BYTES;
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, typeAllocPoint, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Initialize any consistency-checking data.
			MPS_CHECK_INIT(memory, size);

		} while (!mps_commit(typeAllocPoint, memory, size));

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + headerSize;

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *Gc::allocArray(const GcType *type, size_t elements) {
		assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

		size_t size = align(type->stride*elements + headerSize + arrayHeaderSize) + MPS_CHECK_BYTES;
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the header.
			setHeader(memory, type);
			// 3: Set size.
			((MpsObj *)memory)->count = elements;
			// 4: Initialize any consistency-checking data.
			MPS_CHECK_INIT(memory, size);

		} while (!mps_commit(ap, memory, size));

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
		size_t size = align(type->stride*elements + headerSize + arrayHeaderSize) + MPS_CHECK_BYTES;
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
			MPS_CHECK_INIT(memory, size);

		} while (!mps_commit(ap, memory, size));

		// Exclude our header, and return the allocated memory.
		return (byte *)memory + headerSize;
	}

	static size_t typeSize(size_t entries) {
		return sizeof(GcType) + entries*sizeof(size_t) - sizeof(size_t);
	}

	GcType *Gc::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t s = typeSize(entries);
		GcType *t;
		check(mps_alloc((mps_addr_t *)&t, gcTypePool, s), L"Failed to allocate type info.");
		memset(t, 0, s);
		t->kind = kind;
		t->type = type;
		t->stride = stride;
		t->count = entries;
		return t;
	}

	GcType *Gc::allocType(const GcType *src) {
		size_t s = typeSize(src->count);
		GcType *t;
		check(mps_alloc((mps_addr_t *)&t, gcTypePool, s), L"Failed to allocate type info.");
		memcpy(t, src, s);
		return t;
	}

	void Gc::freeType(GcType *t) {
		if (ignoreFreeType)
			return;

		size_t s = sizeof(GcType) + t->count*sizeof(size_t) - sizeof(size_t);
		mps_free(gcTypePool, t, s);
	}

	const GcType *Gc::typeOf(const void *mem) {
		const void *t = (byte *)mem - headerSize;
		const MpsObj *o = (const MpsObj *)t;
		return &(o->header->obj);
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

		// Should always hold, but better safe than sorry!
		if (t->finalizer) {
			typedef void (*Fn)(void *);
			Fn fn = (Fn)t->finalizer;
			(*fn)(obj);
		}

		// Replace the object with padding, as it is not neccessary to scan it anymore.
		obj = (char *)obj - headerSize;
		size_t size = mpsSize(obj);
		mpsMakePad(obj, size);
	}

	void *Gc::allocCode(size_t code, size_t refs) {
		code = align(code);
		size_t size = code + sizeof(GcCode) + refs*sizeof(GcCodeRef) - sizeof(GcCodeRef) + wordSize;
		mps_addr_t memory;

		util::Lock::L z(codeAllocLock);

		do {
			check(mps_reserve(&memory, codeAllocPoint, size), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, size);
			// 2: Set the size.
			*(size_t *)memory = code;
			// 3: Set # of refs.
			void *refPtr = ((byte *)memory) + code + wordSize;
			*(size_t *)refPtr = refs;

		} while (!mps_commit(codeAllocPoint, memory, size));

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + wordSize;

		return result;
	}

	size_t Gc::codeSize(const void *alloc) {
		const size_t *d = (const size_t *)alloc;
		return d[-1];
	}

	GcCode *Gc::codeRefs(void *alloc) {
		void *p = ((byte *)alloc) + align(codeSize(alloc));
		return (GcCode *)p;
	}

	struct Gc::Root {
		mps_root_t root;
	};

	Gc::Root *Gc::createRoot(void *data, size_t count) {
		Root *r = new Root;
		try {
			check(mps_root_create(&r->root,
									arena,
									mps_rank_exact(),
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
	struct CheckData {
		// Pools to check.
		mps_pool_t pools[2];

		// Pool where type info objects should reside.
		mps_pool_t descPool;

		// Arena in which all pools reside.
		mps_arena_t arena;

		// Pointer to the first object which failed the check (if any).
		MpsObj *failed;

		// First and last byte which failed the check.
		size_t failedFirst, failedLast;

		// Did the description check fail?
		bool invalidDesc;
	};

	static void checkMem(MpsObj *obj, size_t size, CheckData *out) {
		if (size > 10*1024*1024) {
			WARNING(L"Large object found at " << (void *)obj << ", " << (void *)size << " bytes!");
			DebugBreak();
		}

		size_t first = MPS_CHECK_BYTES, last = 0;

		byte *start = (byte *)obj;
		start += size - MPS_CHECK_BYTES;
		for (size_t i = 0; i < MPS_CHECK_BYTES; i++) {
			if (start[i] != MPS_CHECK_DATA) {
				first = min(first, i);
				last = max(last, i);
			}
		}

		if (first <= last) {
			out->failed = obj;
			out->failedFirst = first;
			out->failedLast = last;
		}
	}

	// Called by MPS for every object on the heap. This function may not call the MPS, nor access
	// other memory managed by the MPS (except for non-protecting pools such as our GcType-pool).
	static void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t) {
		addr = (byte *)addr - headerSize;
		CheckData *data = (CheckData *)p;

		bool found = false;
		for (nat i = 0; i < ARRAY_COUNT(data->pools); i++)
			found |= data->pools[i] == pool;

		if (!found)
			return;

		// Only care about the first one.
		if (data->failed)
			return;

		MpsObj *obj = (MpsObj *)addr;

		mps_pool_t headerPool;
		if (mps_addr_pool(&headerPool, data->arena, (void *)obj->header)) {
			if (headerPool != data->descPool) {
				data->failed = obj;
				data->invalidDesc = true;
			}
		}

		switch (obj->header->type) {
		case GcType::tFixed:
		case GcType::tType:
		case GcType::tArray:
		case GcType::tWeakArray:
			checkMem(obj, mpsSize(obj), data);
			break;
		case mpsPad0:
		case mpsPad:
		case mpsFwd1:
		case mpsFwd:
			// No validation to do. These do not contain data we can verify.
			break;
		default:
			data->failed = obj;
			break;
		}
	}

	void Gc::checkMemory() {
		CheckData data = {
			{ pool, typePool },
			gcTypePool,
			arena,
			null, 0, 0,
		};
		mps_arena_formatted_objects_walk(arena, &checkObject, &data, 0);

		if (data.failed) {
			PLN(L"--- HEAP CHECK FAILED ---");
			if (data.invalidDesc) {
				PLN(L"The object at " << (void *)data.failed << L" has an invalid description.");
			}
			PLN(L"The object at " << (void *)data.failed << L" wrote "
				<< data.failedFirst << L" to " << data.failedLast << L" bytes after its end.");

			switch (data.failed->header->type) {
			case GcType::tFixed:
			case GcType::tType:
				if (Type *t = data.failed->header->obj.type) {
					if (Str *name = t->name) {
						PLN(L"It is an object of type: " << name->c_str());
						break;
					} else {
						PLN(L"It is an object of type: " << (void *)t << L" (no name found).");
					}
				} else {
					PLN(L"It is an object, but it has no type.");
				}
				break;
			case GcType::tArray:
				PLN(L"It is an array.");
				break;
			case GcType::tWeakArray:
				PLN(L"It is a weak array.");
				break;
			default:
				PLN(L"The object is of unknown type.");
				break;
			}

			// DebugBreak();
		}
	}
#else
	void Gc::checkMemory() {
		// Nothing to do.
	}
#endif

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
		}

		return ok;
	}

}
