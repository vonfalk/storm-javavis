#pragma once
#include "Utils/Lock.h"
#include <vector>

namespace storm {

	/**
	 * A table that stores a set of pointers to Gc-allocated machine code. This implementation
	 * allows quick (O(log n) in most cases) lookup from a pointer within one of the code
	 * blocks. When registering a code block, using 'track', a handle is returned. This is so the Gc
	 * can update the table whenever the block has been moved, even though the CodeTable is
	 * allocated as a global.
	 *
	 * The internals of the table are protected by locks unless otherwise stated.
	 *
	 * This could be used to implement exception handling in the various code backends, but is not
	 * used otherwise.
	 */
	class CodeTable : NoCopy {
	public:
		// Create.
		CodeTable();

		// Destroy.
		~CodeTable();

		// A handle. This will always be pointer-sized so that it fits inside 'GcCodeRef::pointer'.
		typedef void *Handle;

		// Start tracking a new piece of machine code. Assumed to be allocated in a GC pool somewhere.
		Handle add(void *code);

		// Update the location of a previous handle. This function is *not* protected by locks as it
		// is expected to be called from inside the Gc scanning logic. All calls from outside Gc
		// scanning needs to be properly locked. 'code' is expected to be non-null.
		static void update(Handle handle, void *code);

		// Remove a code segment from the tracking mechanisms. Make sure so that 'update' for that
		// handle is not called again, as it may be reused shortly after.
		void remove(Handle handle);

		// Find a code segment from a pointer that is somewhere inside of it. Returns 'null' if
		// nothing was found.
		void *find(const void *addr);

	private:
		// An element in the table. These will never move, so a handle is simply a pointer to one of these.
		struct Elem {
			// Code allocation.
			void *code;

			// Owner.
			CodeTable *owner;
		};

		// Allocator.
		class Allocator {
		public:
			// Create.
			Allocator();

			// Destroy.
			~Allocator();

			// Allocate an object.
			Elem *alloc();

			// Deallocate an object.
			void free(Elem *e);

			// Fill 'out' with all elements that are allocated in some order.
			void alive(vector<Elem *> &out);

		private:
			enum {
				chunkSize = 10240
			};

			// Allocated chunks.
			vector<Elem *> chunks;

			// First element in the free list.
			Elem *first;

			// Allocate a new chunk.
			void allocChunk();
		};

		// Predicate for sorting.
		struct CodeCompare {
			inline bool operator() (const Elem *a, const Elem *b) const {
				return a->code < b->code;
			}
		};

		// Lock protecting the table.
		util::Lock lock;

		// Allocator for elements.
		Allocator mem;

		// Table of all active functions. Sorted when 'sorted' is true.
		typedef vector<Elem *>::iterator Iter;
		vector<Elem *> table;

		// Are the entries in the table sorted? (0 or 1, updated using atomics).
		size_t sorted;

		// Does the code in 'elem' contain 'ptr'?
		static bool contains(Elem *elem, const void *ptr);
	};

	// Get a global instance of the code table.
	CodeTable &codeTable();
}
