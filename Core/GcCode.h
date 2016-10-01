#pragma once

namespace storm {

	/**
	 * Describes a reference inside the generated code.
	 *
	 * Note: in general, reading null (in whatever size is specified) is treated as null even if
	 * using a relative addressing scheme. This is so it is possible to fill in 'null' first, then
	 * mark the reference as alive, to finally fill in the correct value of the
	 * reference. Otherwise, you may end up with stale references as the Gc may move memory at any
	 * time.
	 */
	struct GcCodeRef {
		// Kind of reference. Ie. how do we read or write this reference to/from memory?
		enum Kind {
			// Disabled for now.
			disabled,

			// Raw pointer. Reads 4/8 bytes from 'offset' and treats those as a pointer.
			rawPtr,

			// Offset pointer to a GC:d object. Reads 4/8 bytes from 'offset' and treats those as a
			// pointer relative (offset + sizeof(size_t)), ie. the address just after the pointer.
			relativePtr,

			// A relative non-gc pointer to somewhere. This is updated as the object moves, but is
			// never scanned.
			relative,

			// An absolute pointer to somewhere within this code section. This is updated whenever
			// the object moves, but is never scanned as it does not point to the start.
			inside,

			// TODO: More to come! eg. relative and so on.
		};

		// Offset inside the code where this reference is located. Does not need to be aligned, that
		// depends entirely on the underlying machine we're working with.
		nat offset;

		// Reference type.
		Kind kind;
	};


	/**
	 * Describes allocated code.
	 *
	 * NOTE: It is _not_ possible to store pointers to GcCode objects anywhere except for the stack,
	 * as these are really pointers into an object.
	 *
	 * TODO: Expose to Storm? This is only needed when writing the lowest parts of code-generating
	 * backends, which is only really neccessary when porting Storm to a new platform, so this has a
	 * very low priority.
	 */
	struct GcCode {
		// Number of entries. Only changed on allocation.
		const size_t refCount;

		// References in here.
		GcCodeRef refs[1];
	};

}