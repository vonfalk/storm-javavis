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

			// Store pointer into another object (eg. this object). 'param' is the offset used, so
			// 'ptr' - 'param' is seen by the Gc, and has to point to the start of an object.
			offsetPtr,

			// TODO: More to come! eg. relative and so on.
		};

		// Offset inside the code where this reference is located. Does not need to be aligned, that
		// depends entirely on the underlying machine we're working with.
		nat offset;

		// Parameter to the kind. Its interpretation depends on 'kind'.
		nat param;

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
