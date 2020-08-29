#pragma once

#include "Root.h"
#include "Utils/Bitmask.h"

namespace storm {

	/**
	 * Set of callbacks for walking the heap.
	 *
	 * Note: in general, it is not safe to access any memory managed by the GC inside of
	 * any of these callbacks.
	 */
	class Walker {
	public:
		// Default constructor.
		Walker() : flags(fObjects) {}

		// Flags for the walker.
		enum Flags {
			// Walk objects (default).
			fObjects = 0x01,

			// Walk exact roots.
			fExactRoots = 0x02,

			// Walk ambiguous roots.
			fAmbiguousRoots = 0x04,

			// Make sure all location dependencies indicate that all objects have moved.
			fClearWatch = 0x10,
		};

		// What this walker wishes to do.
		Flags flags;

		// Called before traversal is started at a point where objects are known to not move
		// anymore. Thus, this allows storing GC pointers off the GC heap.
		virtual void prepare() {}

		// Called before scanning a root. Lets the implementation decide whether or not to consider
		// the root in question.
		virtual bool checkRoot(GcRoot *root) { return true; }

		// Called once for each formatted object on the heap. It is safe to inspect 'inspect' from
		// the GC heap, but nothing else.
		virtual void fixed(void *inspect) {}

		// Called once for each formatted object on the heap known to contain a vtable. It is safe
		// to inspect 'inspect' from the GC heap, but nothing else.
		virtual void object(RootObject *inspect) {}

		// Called once for each array or weak array. It is safe to inspect 'inspect' from the GC
		// heap, but nothing else.
		virtual void array(void *inspect) {}

		// Called once for each code allocation. It is safe to inspect 'inspect' from the GC heap,
		// but nothing else.
		virtual void code(void *inspect) {}

		// Called once for each exact pointer in a root.
		virtual void exactPointer(void **ptr) {}

		// Called once for each inexact pointer in a root, if requested.
		virtual void ambiguousPointer(void **ptr) {}
	};

	BITMASK_OPERATORS(Walker::Flags);

	/**
	 * Traverse all references inside scanned objects and call 'exactPointer' (or 'ambiguousPointer'
	 * if appropriate). Note: The 'type' pointer inside the GcType object is never traversed.
	 */
	class PtrWalker : public Walker {
	public:
		virtual void fixed(void *inspect);
		virtual void object(RootObject *inspect);
		virtual void array(void *inspect);
		virtual void code(void *inspect);

		// Called for each type-info pointer.
		virtual void header(GcType **header) {}
	};

}
