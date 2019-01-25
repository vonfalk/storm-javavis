#pragma once
#include "Utils/Memory.h"

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
	 * and client pointers as void *.
	 *
	 * Objects are always word-aligned.
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

		// The size of a word on this machine.
		static const size_t wordSize = sizeof(void *);

#if FMT_CHECK_MEMORY
		// Make a back-up of some important data before the allocations if we're using padding.
		static const size_t headerSize = wordSize * (3 + FMT_CHECK_MEMORY);

		// Align the size of an allocation.
		static inline size_t alignAlloc(size_t data) {
			Nat a = nextPowerOfTwo(headerSize);
			return (data + a - 1) & ~(a - 1);
		}
#endif
		static const size_t headerSize = wordSize;

		// Align the size of an allocation.
		static inline size_t alignAlloc(size_t data) {
			return (data + wordSize - 1) & ~(wordSize - 1);
		}
#else

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
		 * Union for easy access.
		 */
		union Header {
			// Read the type.
			size_t type;

			// Only valid if the type is a type known to GcType.
			GcType obj;
		};


		/**
		 * Static allocated headers for our types.
		 */
		static const size_t headerPad0 = pad0;
		static const size_t headerPad = pad;
		static const size_t headerFwd1 = fwd1;
		static const size_t headerFwd = fwd;

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

		/**
		 * An object on the heap, for convenience. Note that client pointers point to the union that
		 * is the last member of the object.
		 */
		struct Obj {
			// Object information. Contains the following data:
			//
			// 1: If the lowest bit is set, this is a code allocation, and the rest of this field
			// denotes the size of the allocation, excluding the metadata.
			//
			// 2: If the lowest bit is clear, this is a regular allocation. The remainder of this
			// field is a pointer to a Header object containing information about this object.
			//
			// 3: If the lowest bit is clear, then the second lowest bit (0x2) indicates whether
			// this object is finalized or not. We need to be able to inform other parts of the
			// system if certain allocations are finalized so that they may ignore finalized objects
			// (e.g. WeakSet) before we have collected them.
			//
			// Use IS_CODE, CODE_SIZE, OBJ_HEADER etc. to extract the information properly.
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


	}
}
