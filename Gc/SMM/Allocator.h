#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Block.h"
#include "Arena.h"

namespace storm {
	namespace smm {

		class Generation;

		/**
		 * A pending allocation returned from an allocator. Call 'commit' when the allocation is
		 * finished. If the allocation is not committed, subsequent allocations from the same
		 * allocator will overwrite the pending allocation.
		 */
		class PendingAlloc {
			friend class Allocator;
		public:
			PendingAlloc() : source(null) {}

			// Does this pending allocation contain any memory?
			operator bool() const {
				return source != null;
			}

			// Get the actual memory.
			inline void *mem() const {
				return source->mem(source->committed());
			}

			// Commit! This means that the allocated memory can be scanned properly.
			bool commit() {
				// Note: The ordering here is important. The GC may decrease 'committed' during a
				// collection. Since we read 'committed' first, we will correctly bail out both if
				// we were interrupted before or after reading 'committed'. We will always set
				// 'committed' equal to 'reserved' during a collection, so the comparison below will
				// take care of these cases correctly.
				size_t committed = source->committed();
				size_t reserved = source->reserved();

				// Shall we re-try the allocation?
				if (reserved <= committed) {
					if (release)
						release->unlock();
					return false;
				}

				// Attempt to apply the allocation. Note: We're using a CAS operation here to make
				// sure that nothing changed since we read 'committed' and 'reserved'. The CAS
				// operation only needs to detect if something happened between the point we read
				// 'committed' and this point. Anything happening before we read the two values will
				// be caught by the condition above.
				bool ok = source->committedCAS(committed, reserved);
				if (release)
					release->unlock();
				return true;
			}

		private:
			PendingAlloc(Block *source, size_t size, util::Lock *release = null) : source(source), release(release) {
				// Note: We only need this to be atomic wrt garbage collections, not other threads
				// using this object concurrently.
				source->reserved(source->committed() + size);
			}

			// The source of the allocation.
			Block *source;

			// Lock we need to release when the allocation is committed (if any).
			util::Lock *release;
		};


		/**
		 * An allocator is the interface the rest of the system uses to request memory from the GC
		 * in units smaller than whole blocks.
		 *
		 * This is one of the few classes that can be used safely without acquiring the GC lock, as
		 * it manages the needed synchronization internally.
		 *
		 * Each allocator instance have their own Block (nursery generation) used for allocations,
		 * so that the majority of allocations can be performed without acquiring any locks.
		 */
		class Allocator {
		public:
			// Create an allocator for a specific generation.
			Allocator(Generation &generation);

			// Destroy the allocator.
			~Allocator();

			// Allocate memory. The allocated memory needs to be committed later, otherwise it will
			// not be scanned and will be overwritten by subsequent allocations.
			inline PendingAlloc reserve(size_t size) {
				if (size >= largeLimit)
					return allocLarge(size);

				if (!source || source->remaining() < size) {
					fill(size);
					if (!source)
						return PendingAlloc();
				}

				return PendingAlloc(source, size);
			}

		private:
			// No copying!
			Allocator(const Allocator &o);
			Allocator &operator =(const Allocator &o);

			// Owner.
			Generation &owner;

			// Current block we're allocating from.
			Block *source;

			// Limit of what we consider a 'large' object.
			size_t largeLimit;

			// Fill the allocator with more memory from the arena!
			void fill(size_t desiredMin);
			void fillI(Arena::Entry &entry, size_t desiredMin);

			// Make a large allocation (in a higher-numbered generation).
			PendingAlloc allocLarge(size_t size);
		};

	}
}

#endif
