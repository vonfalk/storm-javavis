#pragma once
#include "Utils/Memory.h"
#include "Utils/Bitwise.h"
#include "Utils/Templates.h"
#include "Core/GcType.h"
#include "Core/GcCode.h"
#include "Scan.h"
#include "Code.h"
#include "VTable.h"

namespace storm {

	/**
	 * Implementation of an object format that garbage collectors may use. Supports storing regular
	 * objects (described by a GcType), code objects, forward objects, and padding objects with one
	 * pointer overhead. Also supports padding around object to verify that no memory is being
	 * overwritten unintentionally.
	 *
	 * Everything required is implemented in this header, so that the compiler is likely to inline
	 * as much as possible.
	 *
	 * One pointer stored directly in front of an object stores the type of the object. This always
	 * refers to a fmt::Type struct that describes the kind of object. This may point to one of
	 * several pre-allocated Type instances when the object is a forwarding object or a padding
	 * object.
	 *
	 * To differentiate between base-pointers (pointers to the beginning of the actual allocation)
	 * and client pointers (seen by the rest of Storm), we always pass base pointers as fmt::Obj *
	 * and client pointers as void *. Furthermore, functions taking fmt::Obj * generally start with
	 * 'objXxx' while the corresponding functions for client pointers do not start with 'obj'.
	 *
	 * Objects are always at least word-aligned. GcType objects, or other objects containing
	 * metadata, need to be aligned at an 8 byte boundary.
	 *
	 * The function 'init' checks the assumptions made by this object format with assertions.
	 */
	namespace fmt {
		/**
		 * Configuration. If FMT_CHECK_MEMORY is defined, we will pad each allocation to make sure
		 * that nothing is overwritten by the client program.
		 *
		 * If FMT_CHECK_MEMORY is nonzero, that many words of padding will be appended before and
		 * after each allocation.
		 */
#define FMT_CHECK_MEMORY 0

		// Data to check against when memory checking is enabled. TODO: Put the data tightly around
		// allocations. Currently, there may be a gap of up to 'headerSize' bytes before the barrier
		// actually starts. This could be solved by setting FMT_CHECK_MEMORY to 1 if larger values do
		// not show the sought after bug.
#define FMT_HEADER_DATA 0xBB
#define FMT_MIDDLE_DATA 0xCC
#define FMT_FOOTER_DATA 0xAA

		// Make sure the alignment of GcType is at least 8.
#if GCTYPE_ALIGNMENT < 8
#error "Too narrow alignment for GcType!"
#endif

		// The size of a word on this machine.
		static const size_t wordSize = sizeof(void *);

		// Alignment required for object header objects, e.g. GcType objects.
		static const size_t headerAlign = wordSize;

#if FMT_CHECK_MEMORY
		// Make a back-up of some important data before the allocations if we're using padding.
		static const size_t headerSize = wordSize * (3 + FMT_CHECK_MEMORY);

		// Align the size of an allocation.
		static inline size_t alignAlloc(size_t data) {
			Nat a = nextPowerOfTwo(headerSize);
			return (data + a - 1) & ~(a - 1);
		}
#else
		static const size_t headerSize = wordSize;

		// Align the size of an allocation.
		static inline size_t alignAlloc(size_t data) {
			return (data + headerSize - 1) & ~(headerSize - 1);
		}
#endif

		// Align to a word.
		static inline size_t wordAlign(size_t data) {
			return (data + wordSize - 1) & ~(wordSize - 1);
		}

		// An array contains 2 words before the actual allocation.
		static const size_t arrayHeaderSize = OFFSET_OF(GcArray<void *>, v[0]);

		// Extra data types for forwarding and padding.
		enum Types {
			// Padding object (0 words long).
			pad0 = 0x100,

			// Padding object (>= 1 word long).
			pad,

			// Forwarding object (1 word long).
			fwd1,

			// Forwarding object (>= 2 words long).
			fwd
		};

		/**
		 * Padding object.
		 */
		struct Pad0 {};

		/**
		 * Padding object (>= 1 word).
		 */
		struct Pad {
			size_t size;
		};

		/**
		 * Forwarding object (1 word).
		 */
		struct Fwd1 {
			// Note: This may or may not be a client pointer. That is up to the GC implementation to
			// decide.
			void *to;
		};

		/**
		 * Forwarding object (>= 2 words).
		 */
		struct Fwd {
			// Note: This may or may not be a client pointer. That is up to the GC implementation to
			// decide.
			void *to;
			size_t size;
		};

		/**
		 * Pre-allocated headers with custom types.
		 */
		struct ALIGN_AS(GCTYPE_ALIGNMENT) InternalHeader {
			size_t type;
		};


		/**
		 * Union for easy access.
		 */
		union Header {
			// Read the type.
			size_t type;

			// Always valid, equivalent to "type".
			InternalHeader internal;

			// Only valid if the type is a type known to GcType.
			GcType obj;
		};


		/**
		 * Static allocated headers for our types.
		 */
		static const InternalHeader headerPad0 = { pad0 };
		static const InternalHeader headerPad = { pad };
		static const InternalHeader headerFwd1 = { fwd1 };
		static const InternalHeader headerFwd = { fwd };

		/**
		 * Array object data.
		 */
		struct ArrayHeader {
			size_t count;
			// Note: We ignore 'filled'.
			size_t filled;
		};

		/**
		 * Weak object data. Note: Members are tagged with a 1 in their lowest bit.
		 */
		struct WeakHeader {
			size_t count;
			size_t splatted;
		};

		// Get the count from a weak object.
		static inline size_t weakCount(const WeakHeader *weak) {
			return weak->count >> 1;
		}

		// Add another splatted object to the weak header.
		static inline void weakSplat(WeakHeader *weak) {
			weak->splatted = (weak->splatted + 0x10) | 0x1;
		}


		/**
		 * An object on the heap, for convenience. Note that client pointers point to the union that
		 * is the last member of the object.
		 */
		struct Obj {
			// Object information. Describes the kind of object, and indirectly, its size. The three
			// least significant bits also contain information on the state of the object as follows:
			//
			// ??0: This is a regular allocation. The remaining bits are a pointer to a Header object
			//      containing information about this object. Depending on the contents of the Header,
			//      one of the members in the union below may be valid.
			//
			// ??1: This is a code allocation. The rest of the field denotes the size of the allocation,
			//      excluding the metadata. The size of the metadata is stored in the metadata itself,
			//      which is stored directly after the code. The size is rounded up to at least 4 bytes
			//      so the actual size is computed by masking out the three least significant bits and
			//      dividing by two. This allows storing code segments the size of about half of
			//      the available address space, which should be enough.
			//
			// ?X?: Indicates whether or not this object is finalized. Since pointers to finalized
			//      objects might linger in weak sets and the like, we need the ability to inform other
			//      parts of the system that an object is actually finalized.
			//
			// X??: One bit of information reserved by the collector. The collector may use this to
			//      mark objects in any way it pleases. The object format is not concerned about the
			//      use of this information.
			//
			// Use the objIsXxx functions to extract information from this member.
			size_t info;

#if FMT_CHECK_MEMORY
			// Size of this object in case the header is destroyed. This includes the size of the
			// header and any barriers.
			size_t totalSize;

			// Allocation number for this object, so that we can break on a certain allocation.
			size_t allocId;

			// Padding before the object.
			size_t barrier[FMT_CHECK_MEMORY];
#endif

			// This is at offset 0 as far as Storm is concerned.
			union {
				ArrayHeader array;
				WeakHeader weak;
				Pad0 pad0;
				Pad pad;
				Fwd1 fwd1;
				Fwd fwd;
			};
		};


#if FMT_CHECK_MEMORY
#define FMT_CHECK_BYTES (wordSize*FMT_CHECK_MEMORY)
#define FMT_CHECK_OBJ(o) checkObj(o)
#define FMT_CHECK_SIZE(o) checkSize(o)
#define FMT_INIT_PAD(o, size) initObjPad(o, size)
#define SLOW_DEBUG
		static void checkObj(const Obj *obj);
		static void checkSize(const Obj *obj);
		static void initObjPad(Obj *obj, size_t size);
		static size_t currentAlloc = 0;
#else
#define FMT_CHECK_BYTES 0
#define FMT_CHECK_OBJ(o)
#define FMT_CHECK_SIZE(o)
#define FMT_INIT_PAD(o, size)
#endif


		/**
		 * Convert to/from client pointers.
		 */
		static inline Obj *fromClient(void *o) {
			o = (byte *)o - headerSize;
			return (Obj *)o;
		}
		static inline const Obj *fromClient(const void *o) {
			o = (const byte *)o - headerSize;
			return (Obj *)o;
		}

		static inline void *toClient(Obj *o) {
			return &o->array;
		}
		static inline const void *toClient(const Obj *o) {
			return &o->array;
		}


		/**
		 * Extract/set information inside Obj.
		 */

		// Is this a code allocation?
		static inline bool objIsCode(const Obj *obj) {
			return (obj->info & size_t(0x1)) != 0;
		}

		// Get the size of the code allocation. Assumes 'objIsCode' is true.
		static inline size_t objCodeSize(const Obj *obj) {
			return (obj->info & ~size_t(0x7)) >> 1;
		}

		// Get the header of this allocation. Assumes 'objIsCode' returned false.
		static inline const Header *objHeader(const Obj *obj) {
			return (const Header *)(obj->info & ~size_t(0x7));
		}

		// Get the header of this allocation. Assumes 'objIsCode' returned false.
		static inline Header *objHeader(Obj *obj) {
			return (Header *)(obj->info & ~size_t(0x7));
		}

		// Check if this object is finalized. Works for both code- and regular allocations.
		static inline bool objIsFinalized(const Obj *obj) {
			return (obj->info & size_t(0x2)) != 0;
		}
		static inline bool isFinalized(const void *obj) {
			return objIsFinalized(fromClient(obj));
		}

		// Set the header to indicate a code allocation of 'codeSize' bytes. 'codeSize' is assumed
		// to be rounded up to 'wordSize' (or at least 4).
		static inline void objSetCode(Obj *obj, size_t codeSize) {
			obj->info = (codeSize << 1) | size_t(0x1);
		}

		// Set the header to indicate a regular allocation described by 'header'. Assumes 'header'
		// is aligned to 'headerAlign' and compatible with Header (e.g. a single size_t, CppType, ...).
		static inline void objSetHeader(Obj *obj, const GcType *header) {
			dbg_assert((size_t(header) & 0x7) == 0x0, L"All header descriptions need to be aligned to at least 8 bytes!");
			obj->info = size_t(header);
		}
		static inline void objSetHeader(Obj *obj, const InternalHeader *header) {
			dbg_assert((size_t(header) & 0x7) == 0x0, L"All header descriptions need to be aligned to at least 8 bytes!");
			obj->info = size_t(header);
		}

		// Replace the header of this allocation. Assumes 'objIsCode' returned false. Preserves any other flags.
		static inline void objReplaceHeader(Obj *obj, const GcType *newHeader) {
			dbg_assert((size_t(newHeader) & 0x7) == 0x0, L"All header descriptions need to be aligned to at least 8 bytes!");

			// Note: We want to do this in one instruction to not mess up any use of the flag in the
			// GC.  We could do with an XOR operation, but since we will only be replacing an
			// object's header very rarely, we use CAS instead, since that will more likely be
			// correct.
			size_t old, replace;
			do {
				old = atomicRead(obj->info);
				replace = old;
				replace &= size_t(0x7);
				replace |= size_t(newHeader);
			} while (atomicCAS(obj->info, old, replace) != old);
		}

		// Mark this allocation as finalized. Assumes it is not a code allocation.
		static inline void objSetFinalized(Obj *obj) {
			// Make sure to do this in one instruction to not mess up the GC flag, if it is used.
			atomicOr(obj->info, size_t(0x2));
		}

		// Get the GC mark.
		static inline bool objIsMarked(const Obj *obj) {
			return (obj->info & size_t(0x4)) != 0x0;
		}

		// Set the GC flag.
		static inline void objSetMark(Obj *obj) {
			atomicOr(obj->info, size_t(0x4));
		}

		// Clear the GC flag.
		static inline void objClearMark(Obj *obj) {
			atomicAnd(obj->info, ~size_t(0x4));
		}

		// Compute the size of an object given its header.
		static inline size_t sizeObj(const GcType *type) {
			return alignAlloc(headerSize + type->stride + FMT_CHECK_BYTES);
		}

		// Compute the size of an array.
		static inline size_t sizeArray(const GcType *type, size_t count) {
			return alignAlloc(headerSize + arrayHeaderSize + type->stride*count + FMT_CHECK_BYTES);
		}

		// Compute the size required for 'n' refs in a code allocation.
		static inline size_t sizeRefs(size_t refs) {
			return sizeof(GcCode) - sizeof(GcCodeRef) + sizeof(GcCodeRef)*refs;
		}

		// Compute the size required for an object containing code of the specified size (word
		// aligned) and 'n' references.
		static inline size_t sizeCode(size_t code, size_t refs) {
			return alignAlloc(headerSize + code + FMT_CHECK_BYTES + sizeRefs(refs) + FMT_CHECK_BYTES);
		}

		// Get a pointer to the references inside a code allocation (stored immediately after the code itself).
		static inline GcCode *refsCode(Obj *obj) {
			size_t code = objCodeSize(obj);
			void *p = toClient(obj);
			p = (byte *)p + code + FMT_CHECK_BYTES;
			return (GcCode *)p;
		}
		static inline const GcCode *refsCode(const Obj *obj) {
			size_t code = objCodeSize(obj);
			const void *p = toClient(obj);
			p = (const byte *)p + code + FMT_CHECK_BYTES;
			return (const GcCode *)p;
		}



		/**
		 * Functions for high-level information about object instances.
		 */

		// Size of an object (including the header, so that we can skip objects using this size).
		static inline size_t objSize(const Obj *o) {
			if (objIsCode(o)) {
				size_t code = objCodeSize(o);
				return sizeCode(code, refsCode(o)->refCount);
			}

			const Header *h = objHeader(o);
			switch (h->type) {
			case GcType::tFixed:
			case GcType::tFixedObj:
			case GcType::tType:
				return sizeObj(&h->obj);
			case GcType::tArray:
				return sizeArray(&h->obj, o->array.count);
			case GcType::tWeakArray:
				return sizeArray(&h->obj, weakCount(&o->weak));
			case pad0:
				return headerSize;
			case pad:
				return headerSize + o->pad.size;
			case fwd1:
				return headerSize + sizeof(Fwd1);
			case fwd:
				return headerSize + o->fwd.size;
			default:
				// Most likely, memory was corrupted somehow.
				dbg_assert(false, L"Unknown object found!");
				return 0;
			}
		}
		static inline size_t size(const void *at) {
			return objSize(fromClient(at));
		}

		// Skip an object; return a pointer to whatever is directly after the current object.
		static inline Obj *objSkip(Obj *obj) {
			FMT_CHECK_OBJ(obj);
			void *next = (byte *)obj + objSize(obj);
			return (Obj *)next;
		}
		static inline void *skip(void *at) {
			return (byte *)at + objSize(fromClient(at));
		}


		/**
		 * Create various object instances.
		 */

		// Create a padding object. 'at' will be initialized.
		static inline void objMakePad(Obj *at, size_t size) {
#ifdef SLOW_DEBUG
			dbg_assert(size >= headerSize, L"Too small padding size specified!");
#endif
			if (size <= headerSize) {
				objSetHeader(at, &headerPad0);
			} else {
				objSetHeader(at, &headerPad);
				at->pad.size = size - headerSize;
			}

			FMT_INIT_PAD(at, size);
			FMT_CHECK_OBJ(at);
		}

		// Create a padding object using client pointers. Note: 'size' includes the hidden header.
		static inline void makePad(void *at, size_t size) {
			objMakePad(fromClient(at), size);
		}

		// Check if an object is a padding object.
		static inline bool objIsPad(Obj *at) {
			switch (objHeader(at)->type) {
			case pad0:
			case pad:
				return true;
			default:
				return false;
			}
		}

		// Check if an object is a padding object with client pointers.
		static inline bool isPad(void *at) {
			return objIsPad(fromClient(at));
		}

		// Make the object 'at' into a forwarding object to the specified address, assuming we know
		// its size from earlier. The size is the size returned from 'objSize'.
		static inline void objMakeFwd(Obj *o, size_t size, void *to) {
			FMT_CHECK_OBJ(o);
#ifdef SLOW_DEBUG
			dbg_assert(size >= headerSize + sizeof(Fwd1), L"Not enough space for a fwd object!");
#endif
			if (size <= headerSize + sizeof(Fwd1)) {
				objSetHeader(o, &headerFwd1);
				o->fwd1.to = to;
			} else {
				objSetHeader(o, &headerFwd);
				o->fwd.to = to;
				o->fwd.size = size - headerSize;
			}

			FMT_INIT_PAD(o, size0);
			FMT_CHECK_OBJ(o);
		}

		// Make the object 'at' into a forwarding object to the specified address.
		static inline void objMakeFwd(Obj *o, void *to) {
			FMT_CHECK_OBJ(o);

			size_t size = objSize(o);
#ifdef SLOW_DEBUG
			dbg_assert(size >= headerSize + sizeof(Fwd1), L"Not enough space for a fwd object!");
#endif
			if (size <= headerSize + sizeof(Fwd1)) {
				objSetHeader(o, &headerFwd1);
				o->fwd1.to = to;
			} else {
				objSetHeader(o, &headerFwd);
				o->fwd.to = to;
				o->fwd.size = size - headerSize;
			}

			FMT_INIT_PAD(o, size);
			FMT_CHECK_OBJ(o);
		}

		// Make a forwarding object using client pointers.
		static inline void makeFwd(void *obj, void *to) {
			objMakeFwd(fromClient(obj), to);
		}

		// Is the object a forwarder? If so, to where?
		static inline void *objIsFwd(const Obj *o) {
			FMT_CHECK_OBJ(o);

			if (objIsCode(o))
				return null;

			switch (objHeader(o)->type) {
			case fwd1:
				return o->fwd1.to;
			case fwd:
				return o->fwd.to;
			default:
				return null;
			}
		}

		// Version that allows detecting pointers to 'null'.
		static inline bool objIsFwd(const Obj *o, void **out) {
			FMT_CHECK_OBJ(o);

			if (objIsCode(o))
				return null;

			switch (objHeader(o)->type) {
			case fwd1:
				*out = o->fwd1.to;
				return true;
			case fwd:
				*out = o->fwd.to;
				return true;
			default:
				return false;
			}
		}

		// Is the object a forwarder? Using client pointers.
		static inline void *isFwd(const void *o) {
			return objIsFwd(fromClient(o));
		}

		// Version that allows detecting pointers to 'null'.
		static inline bool isFwd(const void *o, void **out) {
			return objIsFwd(fromClient(o), out);
		}

		// Is this a special object (ie. a forwarder or a padding object)?
		static inline bool objIsSpecial(const Obj *o) {
			FMT_CHECK_OBJ(o);
			if (objIsCode(o))
				return false;

			switch (objHeader(o)->type) {
			case pad0:
			case pad:
			case fwd1:
			case fwd:
				return true;
			default:
				return false;
			}
		}

		// Is this a special object (ie. a forwarder or a padding object)?
		static inline bool isSpecial(const void *o) {
			return objIsSpecial(fromClient(o));
		}


		/**
		 * Create and initialize whole allocations.
		 *
		 * These functions take pointers to allocated memory (i.e. raw pointers, not client
		 * pointers) and returns the client pointer for the allocation.
		 *
		 * These functions assume you have previously the size of the allocation using a suitable
		 * function.
		 */

		// Initialize a regular object.
		static inline void *initObj(void *memory, const GcType *type, size_t size) {
			// 1: Clear all memory to zero, so that we don't have any pointers confusing the GC.
			memset(memory, 0, size);
			// 2: Set the header.
			Obj *o = (Obj *)memory;
			objSetHeader(o, type);
			// 3: Intialize any padding we need.
			FMT_INIT_PAD(o, size);
			FMT_CHECK_SIZE(o);

			return toClient(o);
		}

		// Initialize an array.
		static inline void *initArray(void *memory, const GcType *type, size_t size, size_t elements) {
			// 1: Clear all memory to zero, so that we don't have any pointers confusing the GC.
			memset(memory, 0, size);
			// 2: Set the header.
			Obj *o = (Obj *)memory;
			objSetHeader(o, type);
			// 3: Set the size.
			o->array.count = elements;
			// 4: Intialize any padding we need.
			FMT_INIT_PAD(o, size);
			FMT_CHECK_SIZE(o);

			return toClient(o);
		}

		// Initialize a weak array.
		static inline void *initWeakArray(void *memory, const GcType *type, size_t size, size_t elements) {
			// 1: Clear all memory to zero, so that we don't have any pointers confusing the GC.
			memset(memory, 0, size);
			// 2: Set the header.
			Obj *o = (Obj *)memory;
			objSetHeader(o, type);
			// 3: Set the size and splat count (tagged).
			o->weak.count = (elements << 1) | 0x1;
			o->weak.splatted = 0x1;
			// 4: Intialize any padding we need.
			FMT_INIT_PAD(o, size);
			FMT_CHECK_SIZE(o);

			return toClient(o);
		}

		// Initialize a code allocation.
		static inline void *initCode(void *memory, size_t size, size_t code, size_t refs) {
			// 1: Clear all memory to zero, so that we don't have any pointers confusing the GC.
			memset(memory, 0, size);
			// 2: Set the size.
			Obj *o = (Obj *)memory;
			objSetCode(o, code);
			// 3: Set the self pointer, and number of references.
			GcCode *codeRefs = refsCode(o);
			codeRefs->reserved = toClient(o);
			void *refPtr = codeRefs;
			*(size_t *)refPtr = refs;
			// Initialize any padding we need.
			FMT_INIT_PAD(o, size);
			FMT_CHECK_SIZE(o);

			return toClient(o);
		}

		/**
		 * Validation.
		 */

		// Check our assumptions.
		static void init() {
			assert(wordSize == sizeof(size_t), L"Invalid word-size");
			assert(wordSize == sizeof(void *), L"Invalid word-size");
			assert(wordSize == sizeof(Fwd1), L"Invalid size of MpsFwd1");
			assert(headerSize == OFFSET_OF(Obj, array), L"Invalid header size.");
		}

#if FMT_CHECK_MEMORY
		static String objInfo(const Obj *o) {
			std::wostringstream to;
			to << L"Object " << (void *)o << L", header: " << (void *)objHeader(o);
			to << L", size " << o->totalSize << L", id: " << o->allocId << L" (of " << currentAlloc << L")";
			return to.str();
		}

		static void checkBarrier(const Obj *obj, const byte *start, nat count, byte pattern, const wchar *type) {
			size_t first = FMT_CHECK_BYTES, last = 0;

			for (size_t i = 0; i < FMT_CHECK_BYTES; i++) {
				if (start[i] != pattern) {
					first = min(first, i);
					last = max(last, i);
				}
			}

			dbg_assert(first > last, objInfo(obj)
					+ L" has an invaild " + type + L" barrier in bytes " + ::toS(first) + L" to " + ::toS(last));
		}

		static void checkHeader(const Obj *obj) {
			checkBarrier(obj, (byte *)obj->barrier, FMT_CHECK_BYTES, FMT_HEADER_DATA, L"header");
		}

		// Assumes there is a footer.
		static void checkFooter(const Obj *obj) {
			size_t size = obj->totalSize;
			checkBarrier(obj, (const byte *)obj + size - FMT_CHECK_BYTES, FMT_CHECK_BYTES, FMT_FOOTER_DATA, L"footer");

			if (objIsCode(obj)) {
				const GcCode *c = refsCode(obj);
				checkBarrier(obj, (const byte *)c - FMT_CHECK_BYTES, FMT_CHECK_BYTES, FMT_MIDDLE_DATA, L"middle");

				// NOTE: We can not actually check this here, as objects are checked before any FIX
				// operations have been done.
				// dbg_assert(c->reserved != toClient(obj), L"Invalid self-pointer in code segment.");
			}
		}

		static bool hasFooter(const Obj *obj) {
			if (objIsCode(obj))
				return true;

			switch (objHeader(obj)->type) {
			case GcType::tFixed:
			case GcType::tFixedObj:
			case GcType::tType:
			case GcType::tArray:
			case GcType::tWeakArray:
				return true;
			}
			return false;
		}

		static void checkSize(const Obj *obj) {
			size_t computed = objSize(obj);
			size_t expected = obj->totalSize;
			dbg_assert(computed == expected,
					objInfo(obj) + L": Size does not match. Expected " + ::toS(expected) +
					L", but computed " + ::toS(computed));
		}

		static void checkObj(const Obj *obj) {
			checkHeader(obj);
			checkSize(obj);
			if (hasFooter(obj))
				checkFooter(obj);
		}

		static void initObjPad(Obj *obj, size_t size) {
			obj->totalSize = size;
			obj->allocId = currentAlloc++;
			memset(obj->barrier, FMT_HEADER_DATA, FMT_CHECK_BYTES);
			if (hasFooter(obj))
				memset((byte *)obj + size - FMT_CHECK_BYTES, FMT_FOOTER_DATA, FMT_CHECK_BYTES);
			if (objIsCode(obj))
				memset((byte *)refsCode(obj) - FMT_CHECK_BYTES, FMT_MIDDLE_DATA, FMT_CHECK_BYTES);
		}
#endif


		/**
		 * Scanning of objects with the standard layout.
		 */
		template <class Scanner>
		struct Scan {
		private:
			typedef typename Scanner::Result Result;
			typedef typename Scanner::Source Source;

			// Helper functions. The public interface is below.
			static inline Result fix12(Scanner &s, void **ptr) {
				if (s.fix1(*ptr))
					return s.fix2(ptr);
				return Result();
			}

			static inline Result fixHeader(Scanner &s, Obj *obj, Header *header) {
				if (s.fixHeader1(&header->obj)) {
					GcType *t = &header->obj;
					Result r = s.fixHeader2(&t);
					objReplaceHeader(obj, t);
					return r;
				}

				return Result();
			}

			struct ScanAll {
				inline bool operator() (void *, void *) const { return true; }
			};

			// Helper for interpreting and scanning a vtable.
			// We assume vtables are at offset 0.
#define FMT_FIX_VTABLE(base)								\
			do {											\
				void *d = *(void **)(base);					\
				if (s.fix1(d)) {							\
					d = (byte *)d - vtable::allocOffset();	\
					r = s.fix2(&d);							\
					if (r != Result())						\
						return r;							\
					d = (byte *)d + vtable::allocOffset();	\
					*(void **)(base) = d;					\
				}											\
			} while (false)

			// Helper for interpreting and scanning a block of data described by a GcType.
#define FMT_FIX_GCTYPE(header, start, base)								\
			do {														\
				for (size_t _i = (start); _i < (header)->obj.count; _i++) { \
					size_t offset = (header)->obj.offset[_i];			\
					void **data = (void **)((byte *)(base) + offset);	\
					r = fix12(s, data);									\
					if (r != Result())									\
						return r;										\
				}														\
			} while (false)

		public:
			// Scan a set of objects that are stored back-to-back. Assumes the entire region
			// [base,limit) is filled entirely with objects.
			static Result objects(Source &source, void *base, void *limit) {
				return objectsIf<ScanAll>(ScanAll(), source, base, limit);
			}


			// Scan a set of objects that are stored back-to-back. Only scans objects where the
			// predicate returns "true".
			template <class Predicate>
			static Result objectsIf(const Predicate &predicate, Source &source, void *base, void *limit) {
				Scanner s(source);
				Result r;
				void *next = base;
				for (void *at = base; at < limit; at = next) {
					Obj *o = fromClient(at);
					FMT_CHECK_OBJ(o);

					next = fmt::skip(at);

					// Note: This call will be optimized away entirely if 'predicate' is an object
					// that always returns true, as is the case with 'ScanAll'.
					if (!predicate(at, (byte *)next - fmt::headerSize))
						continue;

					if (objIsCode(o)) {
						// Scan the code segment.
						GcCode *c = refsCode(o);

						// Scan our self-pointer to make sure that this object will be scanned
						// whenever it is moved.
						r = fix12(s, &c->reserved);
						if (r != Result())
							return r;

#ifdef SLOW_DEBUG
						dbg_assert(c->reserved == at, L"Invalid self-pointer!");
#endif

						for (size_t i = 0; i < c->refCount; i++) {
							GcCodeRef &ref = c->refs[i];
#ifdef SLOW_DEBUG
							dbg_assert(ref.offset < objCodeSize(o), L"Code offset is out of bounds!");
#endif
							// Only some kind of references need to be scanned.
							if (ref.kind & 0x01) {
								r = fix12(s, &ref.pointer);
								if (r != Result())
									return r;
							}
						}

						// Update the pointers in the code blob as well.
						code::updatePtrs(at, c);
					} else {
						// Scan the regular object.
						Header *h = objHeader(o);
						void *tmp = at;

						switch (h->type) {
						case GcType::tFixedObj:
							FMT_FIX_VTABLE(tmp);
							// Fall thru.
						case GcType::tFixed:
							r = fixHeader(s, o, h);
							if (r != Result())
								return r;
							FMT_FIX_GCTYPE(h, 0, tmp);
							break;
						case GcType::tType: {
							FMT_FIX_VTABLE(tmp);
							r = fixHeader(s, o, h);
							if (r != Result())
								return r;

							GcType **data = (GcType **)((byte *)tmp + h->obj.offset[0]);
							if (s.fixHeader1(*data)) {
								r = s.fixHeader2(data);
								if (r != Result())
									return r;
							}

							FMT_FIX_GCTYPE(h, 1, tmp);
							break;
						}
						case GcType::tArray: {
							r = fixHeader(s, o, h);
							if (r != Result())
								return r;

							tmp = (byte *)tmp + arrayHeaderSize;
							size_t stride = h->obj.stride;
							size_t count = o->array.count;
							for (size_t i = 0; i < count; i++, tmp = (byte *)tmp + stride) {
								FMT_FIX_GCTYPE(h, 0, tmp);
							}
							break;
						}
						case GcType::tWeakArray: {
							r = fixHeader(s, o, h);
							if (r != Result())
								return r;

							tmp = (byte *)tmp + arrayHeaderSize;
							size_t stride = h->obj.stride;
							size_t count = weakCount(&o->weak);
							for (size_t i = 0; i < count; i++, tmp = (byte *)tmp + stride) {
								for (size_t j = 0; j < h->obj.count; j++) {
									size_t offset = h->obj.offset[j];
									void **data = (void **)((byte *)tmp + offset);
									if (s.fix1(*data)) {
										r = s.fix2(data);
										if (r != Result())
											return r;
										// Splatted?
										if (*data == null)
											weakSplat(&o->weak);
									}
								}
							}
							break;
						}
#ifdef SLOW_DEBUG
						case pad0:
						case pad:
						case fwd1:
						case fwd:
							break;
						default:
							dbg_assert(false, L"Unknown object type scanned!");
							break;
#endif
						}
					}
				}

				return Result();
			}
		};
	}

}
