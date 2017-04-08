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
#define MPS_CHECK_MEMORY 1

// Use debug pools in MPS (behaves slightly differently from the standard and may not trigger errors).
#define MPS_DEBUG_POOL 0

// Number of words that shall be verified before and after each allocation.
#define MPS_CHECK_WORDS 32

// Data to check against.
#define MPS_HEADER_DATA 0xBB
#define MPS_FOOTER_DATA 0xAA

// Check the heap after this many allocations.
#define MPS_CHECK_INTERVAL 10000


#if MPS_CHECK_MEMORY
#define MPS_CHECK_BYTES (sizeof(void *)*MPS_CHECK_WORDS)
#define MPS_VERIFY_OBJECT(object) checkBarriers(object)
#define MPS_VERIFY_SIZE(object) checkSize(object)
#define MPS_INIT_OBJECT(object, size) initObject(object, size)
#define MPS_INIT_FWD_OBJECT(object, size, id) initFwdObject(object, size, id)

#define MPS_VERIFY_CODE(object) checkCode(object)
#define MPS_INIT_CODE(object, size, refs) initCode(object, size, refs)
#else
#define MPS_CHECK_BYTES 0
#define MPS_VERIFY_OBJECT(object)
#define MPS_VERIFY_SIZE(object)
#define MPS_INIT_OBJECT(object, size)
#define MPS_INIT_FWD_OBJECT(object, size, id)

#define MPS_VERIFY_CODE(object)
#define MPS_VERIFY_CODE_SIZE(object)
#define MPS_INIT_CODE(object, size)
#endif

namespace storm {

	/**
	 * In MPS, we allocate objects with an extra pointer just before the object. This points to a
	 * MpsObj union which contains the information required for scanning. We also make sure to
	 * allocate objects that are a whole number of words, to make it easier to reason about what
	 * needs to be done in the special forwarders and padding types.
	 */


	static const size_t wordSize = sizeof(void *);

#if MPS_CHECK_MEMORY
	static const size_t headerSize = wordSize * (3 + MPS_CHECK_WORDS);
	static const size_t codeHeaderSize = wordSize * (3 + MPS_CHECK_WORDS);

	static inline size_t align(size_t data) {
		nat a = nextPowerOfTwo(headerSize);
		return (data + a - 1) & ~(a - 1);
	}
#else
	static const size_t headerSize = wordSize;
	static const size_t codeHeaderSize = wordSize;

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

#if MPS_CHECK_MEMORY
		// Size of this object in case 'header' is destroyed. This excludes the size of the header.
		size_t size;

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
		to << L", size " << o->size << L", id: " << o->allocId << L" (of " << allocId << L")";
		return to.str();
	}

	// Check object barriers.
	static void checkBarriers(const MpsObj *o);

	// Check size for the object.
	static void checkSize(const MpsObj *o);

	// Initialize object.
	static void initObject(MpsObj *o, size_t size);
	static void initFwdObject(MpsObj *o, size_t size, size_t allocId);

#endif

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
		case GcType::tFixedObj:
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

		// Account for header.
		s += headerSize;

		// Word-align the size.
		return align(s);
	}

	// Skip objects. Figure out the size of an object, and return a pointer to after the end of it
	// (including the next object's header, it seems).
	static mps_addr_t mpsSkip(mps_addr_t at) {
#if MPS_CHECK_MEMORY
		mps_addr_t offset = (byte *)at - headerSize;
		MpsObj *o = (MpsObj *)offset;
		MPS_VERIFY_OBJECT(o);
		MPS_VERIFY_SIZE(o);
#endif
		return (byte *)at + mpsSize((byte *)at - headerSize);
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
		// Convert from client pointers to 'real' pointers:
		base = (byte *)base - headerSize;
		limit = (byte *)limit - headerSize;

		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = (byte *)at + mpsSize(at)) {
				MpsObj *o = (MpsObj *)at;
				MPS_VERIFY_SIZE(o);
				MPS_VERIFY_OBJECT(o);
				const MpsHeader *h = o->header;

				// Exclude header.
				mps_addr_t pos = (byte *)at + headerSize;

				switch (h->type) {
				case GcType::tFixedObj:
					FIX_VTABLE(pos);
					// Fall thru.
				case GcType::tFixed:
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
		} MPS_SCAN_END(ss);
		return MPS_RES_OK;
	}

	// Create a forwarding object at 'at' referring to 'to'. These must be recognized by mpsSize, mpsSkip and mpsScan.
	static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
		// Convert to base pointers:
		at = (byte *)at - headerSize;
		size_t size = mpsSize(at) - headerSize;
		MpsObj *o = (MpsObj *)at;
		MPS_VERIFY_SIZE(o);
		MPS_VERIFY_OBJECT(o);

		if (size <= sizeof(MpsFwd1)) {
			setHeader(o, &headerFwd1);
			o->fwd1.to = to;
		} else {
			setHeader(o, &headerFwd);
			o->fwd.to = to;
			o->fwd.size = size;
		}

		MPS_INIT_FWD_OBJECT(o, size + headerSize, o->allocId);
		MPS_VERIFY_SIZE(o);
		MPS_VERIFY_OBJECT(o);
	}

	// See if object at 'at' is a forwarder, and if so, where it points to.
	static mps_addr_t mpsIsFwd(mps_addr_t at) {
		// Convert to base pointers:
		at = (byte *)at - headerSize;
		MpsObj *o = (MpsObj *)at;
		MPS_VERIFY_SIZE(o);
		MPS_VERIFY_OBJECT(o);

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
		MPS_VERIFY_SIZE((MpsObj *)at);
		MPS_VERIFY_OBJECT((MpsObj *)at);
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

#if MPS_CHECK_MEMORY

		// Size of this block (including headers).
		size_t size;

		// Number of references.
		size_t numRefs;

		// Barrier.
		size_t barrier[MPS_CHECK_WORDS];
#endif

		// Read only if header indicates they are valid!
		union {
			MpsPad0 pad0;
			MpsPad pad;
			MpsFwd1 fwd1;
			MpsFwd fwd;
		};
	};

#if MPS_CHECK_MEMORY

	static String objInfo(const MpsCode *o) {
		std::wostringstream to;
		to << L"Code blob " << (void *)o << L", header: " << (void *)o->header;
		to << L", size " << o->size;
		return to.str();
	}

	// Check and initialize code objects.
	static void initCode(MpsCode *o, size_t size, size_t refs);
	static void checkCode(MpsCode *o);

#endif

	static inline GcCode *mpsRefsCode(MpsCode *src) {
		size_t codeSize = src->header;
		void *p = src;
		p = (byte *)p + codeHeaderSize + codeSize + MPS_CHECK_BYTES;
		return (GcCode *)p;
	}

	static inline size_t mpsSizeCode(size_t codeSize, size_t refCount) {
		return codeHeaderSize
			+ codeSize
			+ MPS_CHECK_BYTES
			+ sizeof(GcCode) - sizeof(GcCodeRef)
			+ sizeof(GcCodeRef)*refCount
			+ MPS_CHECK_BYTES;
	}

	static size_t mpsSizeCode(mps_addr_t at) {
		MpsCode *c = (MpsCode *)at;
		size_t r = 0;
		switch (c->header) {
		case mpsCodePad0:
			return align(codeHeaderSize);
		case mpsCodePad:
			return align(codeHeaderSize + c->pad.size);
		case mpsCodeFwd1:
			return align(codeHeaderSize + wordSize);
		case mpsCodeFwd:
			return align(codeHeaderSize + c->fwd.size);
		}

		dbg_assert((c->header & codeMask) == 0, L"Unknown special object found.");

		// Regular code.
		size_t codeSize = c->header;
		GcCode *g = mpsRefsCode(c);
		return align(mpsSizeCode(codeSize, g->refCount));
	}

	static mps_addr_t mpsSkipCode(mps_addr_t at) {
#if MPS_CHECK_MEMORY
		mps_addr_t offset = (byte *)at - codeHeaderSize;
		MpsCode *o = (MpsCode *)offset;
		MPS_VERIFY_CODE(o);
#endif
		return (byte *)at + mpsSizeCode((byte *)at - codeHeaderSize);
	}

	static mps_res_t mpsScanCode(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		// Convert from client pointers to 'real' pointers:
		base = (byte *)base - codeHeaderSize;
		limit = (byte *)limit - codeHeaderSize;

		mps_res_t result = MPS_RES_OK;

		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = (byte *)at + mpsSizeCode(at)) {
				MpsCode *h = (MpsCode *)at;
				MPS_VERIFY_CODE(h);

				// Special kind of allocation?
				if (h->header & codeMask)
					continue;

				// Scan all references in here.
				GcCode *refs = mpsRefsCode(h);
				for (size_t i = 0; i < refs->refCount; i++) {
					GcCodeRef &ref = refs->refs[i];

					switch (ref.kind) {
					case GcCodeRef::rawPtr:
					case GcCodeRef::relativePtr:
						// These are the only kinds that need to be scanned.
						result = MPS_FIX12(ss, &ref.pointer);
						if (result != MPS_RES_OK)
							return result;

						break;
					}
				}

				// Update the pointers in the code blob as well.
				code::updatePtrs((byte *)at + codeHeaderSize, refs);
			}
		} MPS_SCAN_END(ss);

		return result;
	}

	static void mpsMakeFwdCode(mps_addr_t at, mps_addr_t to) {
		// Convert to base pointers and extract information.
		at = (byte *)at - codeHeaderSize;

		size_t size = mpsSizeCode(at);
		MpsCode *c = (MpsCode *)at;
		MPS_VERIFY_CODE(c);

		// See if we need to modify pointers in the new object.
		if ((c->header & codeMask) == 0) {
			mps_addr_t dest = (byte *)to - codeHeaderSize;
			code::updatePtrs(to, mpsRefsCode((MpsCode *)dest));
		}

		// Create the forwarding object.
		if (size <= codeHeaderSize + wordSize) {
			c->header = mpsCodeFwd1;
			c->fwd1.to = to;
		} else {
			c->header = mpsCodeFwd;
			c->fwd.to = to;
			c->fwd.size = size - codeHeaderSize;
		}
		MPS_VERIFY_CODE(c);
	}

	static mps_addr_t mpsIsFwdCode(mps_addr_t at) {
		at = (byte *)at - codeHeaderSize;

		MpsCode *h = (MpsCode *)at;
		MPS_VERIFY_CODE(h);
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
		if (size <= codeHeaderSize) {
			to->header = mpsCodePad0;
		} else {
			to->header = mpsCodePad;
			to->pad.size = size - codeHeaderSize;
		}
		MPS_INIT_CODE(to, size, 0);
		MPS_VERIFY_CODE(to);
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
				os::InlineSet<os::UThreadStack>::iterator i = thread->stacks->begin();
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
#if MPS_CHECK_MEMORY
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, nextPowerOfTwo(codeHeaderSize));
#else
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize); // Default alignment.
#endif
			MPS_ARGS_ADD(args, MPS_KEY_FMT_HEADER_SIZE, codeHeaderSize);
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

		// Clear these now, otherwise the destructor will crash badly after we've removed the backing storage.
		freeTypes.clear();

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
		arena = null;
	}

	void Gc::collect() {
		mps_arena_collect(arena);
		mps_arena_release(arena);
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

		assert(type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj, L"Wrong type for calling alloc().");

		size_t size = headerSize + type->stride;
		size_t alignedSize = align(size + MPS_CHECK_BYTES);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, alignedSize), L"Out of memory (alloc).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, alignedSize);
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

		size_t size = type->stride + headerSize;
		size_t alignedSize = align(size + MPS_CHECK_BYTES);
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, typeAllocPoint, alignedSize), L"Out of memory (alloc type).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, alignedSize);
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

	void *Gc::allocArray(const GcType *type, size_t elements) {
		assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

		size_t size = type->stride*elements + headerSize + arrayHeaderSize;
		size_t alignedSize = align(size + MPS_CHECK_BYTES);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, alignedSize), L"Out of memory (alloc array).");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero, so that we do not have any rouge pointers confusing MPS.
			memset(memory, 0, alignedSize);
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
		size_t size = type->stride*elements + headerSize + arrayHeaderSize;
		size_t alignedSize = align(size + MPS_CHECK_BYTES);
		mps_ap_t &ap = weakAllocPoint;
		mps_addr_t memory;
		do {
			check(mps_reserve(&memory, ap, alignedSize), L"Out of memory.");

			// Make sure we can scan the newly allocated memory:
			// 1: Clear all to zero.
			memset(memory, 0, alignedSize);
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
		if (freeTypes.size() > 100) {
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
		if (code & codeMask)
			// Too large if we start polluting the 'codeMask' flag.
			return null;

		size_t size = align(mpsSizeCode(code, refs));
		dbg_assert(size > codeHeaderSize, L"Can not allocate zero-sized chunks of code!");

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
			void *refPtr = mpsRefsCode((MpsCode *)memory);
			*(size_t *)refPtr = refs;
			// 4: Init checking data.
			MPS_INIT_CODE((MpsCode *)memory, size, refs);
		} while (!mps_commit(codeAllocPoint, memory, size));

		// Exclude our header, and return the allocated memory.
		void *result = (byte *)memory + codeHeaderSize;

		return result;
	}

	size_t Gc::codeSize(const void *alloc) {
		alloc = (const byte *)alloc - codeHeaderSize;
		return *(const size_t *)alloc;
	}

	GcCode *Gc::codeRefs(void *alloc) {
		alloc = (byte *)alloc - codeHeaderSize;
		return mpsRefsCode((MpsCode *)alloc);
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

	static void checkBarrier(const MpsObj *obj, byte *start, nat count, byte pattern) {
		size_t first = MPS_CHECK_BYTES, last = 0;

		for (size_t i = 0; i < MPS_CHECK_BYTES; i++) {
			if (start[i] != pattern) {
				first = min(first, i);
				last = max(last, i);
			}
		}

		dbg_assert(first > last, objInfo(obj)
				+ L" has an invaild barrier in bytes " + ::toS(first) + L" to " + ::toS(last));
	}

	static void checkHeader(const MpsObj *obj) {
		checkBarrier(obj, (byte *)obj->barrier, MPS_CHECK_BYTES, MPS_HEADER_DATA);
	}

	static void checkFooter(const MpsObj *obj) {
		size_t size = obj->size;
		checkBarrier(obj, (byte *)obj + headerSize + size, MPS_CHECK_BYTES, MPS_FOOTER_DATA);
	}

	static bool hasFooter(const MpsObj *obj) {
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
		size_t size = mpsSize((mps_addr_t)obj);
		size_t expected = hasFooter(obj)
			? align(obj->size + headerSize + MPS_CHECK_BYTES)
			: align(obj->size + headerSize);
		dbg_assert(expected == size,
				objInfo(obj) + L": Size does not match. Got " + ::toS(size) + L", expected " + ::toS(expected));
	}

	static void checkBarriers(const MpsObj *obj) {
		checkHeader(obj);
		if (hasFooter(obj))
			checkFooter(obj);
	}

	static void initObject(MpsObj *obj, size_t size) {
		initFwdObject(obj, size, allocId++);
		// if (obj->allocId == 1) DebugBreak();
	}

	static void initFwdObject(MpsObj *obj, size_t size, size_t allocId) {
		obj->size = size - headerSize;
		obj->allocId = allocId;
		memset(obj->barrier, MPS_HEADER_DATA, MPS_CHECK_BYTES);
		if (hasFooter(obj)) {
			memset((byte *)obj + size, MPS_FOOTER_DATA, MPS_CHECK_BYTES);
		}
	}

	void Gc::checkMemory(const void *object, bool recursive) {
		object = (const byte *)object - headerSize;
		const MpsObj *obj = (const MpsObj *)object;
		const MpsHeader *header = obj->header;

		checkHeader(obj);

		mps_pool_t headerPool = null;
		if (mps_addr_pool(&headerPool, arena, (void *)header)) {
			dbg_assert(headerPool == gcTypePool, objInfo(obj)
					+ toHex(object) + L" has an invalid header: " + toHex(header));
		} else {
			// We do support statically allocated object descriptions.
			dbg_assert(readable(header), objInfo(obj)
					+ L" has an invalid header: " + toHex(header));
		}

		checkSize(obj);
		if (hasFooter(obj))
			checkFooter(obj);

		switch (header->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
			if (recursive) {
				// Check pointers as well.
				for (nat i = 0; i < header->obj.count; i++) {
					size_t offset = header->obj.offset[i] + headerSize;
					mps_addr_t *data = (mps_addr_t *)((const byte *)obj + offset);
					checkMemory(*data, false);
				}
			}
			break;
		case GcType::tType:
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

	static void checkBarrier(const MpsCode *obj, byte *start, nat count, byte pattern, const wchar *note) {
		size_t first = MPS_CHECK_BYTES, last = 0;

		for (size_t i = 0; i < MPS_CHECK_BYTES; i++) {
			if (start[i] != pattern) {
				first = min(first, i);
				last = max(last, i);
			}
		}

		dbg_assert(first > last, objInfo(obj)
				+ L" has an invaild " + note + L" barrier in bytes " + ::toS(first) + L" to " + ::toS(last));
	}

	static void checkHeader(const MpsCode *obj) {
		checkBarrier(obj, (byte *)obj->barrier, MPS_CHECK_BYTES, MPS_HEADER_DATA, L"first");
	}

	static void checkFooter(const MpsCode *obj) {
		size_t size = obj->size;
		GcCode *c = mpsRefsCode((MpsCode *)obj);
		// Barrier between actual code and references.
		checkBarrier(obj, (byte *)c - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_FOOTER_DATA, L"middle");
		// Final barrier.
		checkBarrier(obj, (byte *)obj + size - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_FOOTER_DATA, L"end");
	}

	static void checkCode(MpsCode *obj) {
		checkHeader(obj);

		// See so that the size is what we think it should be!
		size_t ref = mpsSizeCode(obj);
		dbg_assert(ref == obj->size, objInfo(obj) + L" has an invalid size. " + ::toS(ref) + L" vs " + ::toS(obj->size));

		if (obj->header & codeMask) {
			// Special object, nothing more to check.
			return;
		}

		checkFooter(obj);

		// Check references, so that none refers outside the code region.
		GcCode *c = mpsRefsCode(obj);
		for (size_t i = 0; i < c->refCount; i++) {
			dbg_assert(c->refs[i].offset < obj->header, L"Offset " + ::toS(i) + L" is out of bounds ("
					+ ::toS(c->refs[i].offset) + L" > " + ::toS(obj->header) + L").");
		}
	}

	static void initCode(MpsCode *obj, size_t size, size_t refs) {
		obj->size = size;
		obj->numRefs = refs;
		memset(obj->barrier, MPS_HEADER_DATA, MPS_CHECK_BYTES);

		if (obj->header & codeMask) {
		} else {
			GcCode *c = mpsRefsCode(obj);
			memset((byte *)c - MPS_CHECK_BYTES, MPS_FOOTER_DATA, MPS_CHECK_BYTES);
			memset((byte *)obj + size - MPS_CHECK_BYTES, MPS_FOOTER_DATA, MPS_CHECK_BYTES);
		}

		dbg_assert(size == mpsSizeCode(obj), L"Size invalid after creation: " + ::toS(size)
				+ L" vs " + ::toS(mpsSizeCode(obj)));
	}

	void Gc::checkCode(const void *code) {
		const void *objVoid = (byte *)code - codeHeaderSize;
		storm::checkCode((MpsCode *)objVoid);
	}

	// Called by MPS for every object on the heap. This function may not call the MPS, nor access
	// other memory managed by the MPS (except for non-protecting pools such as our GcType-pool).
	static void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t) {
		Gc *me = (Gc *)p;

		if (p == me->pool || p == me->typePool)
			me->checkMemory(addr);
		else if (p == me->codePool)
			me->checkCode(addr);
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
	void Gc::checkMemory(const void *, bool) {
		// Nothing to do.
	}

	void Gc::checkMemory() {
		// Nothing to do.
	}

	void Gc::checkCode() {
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
#include "Utils/MemoryManager.h"

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
		}

		return ok;
	}

}
