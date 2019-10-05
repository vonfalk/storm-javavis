#pragma once

#if STORM_GC == STORM_GC_SMM

#include "AddrSet.h"

namespace storm {
	namespace smm {

		class AddrWatch;
		class ArenaTicket;

		/**
		 * Small wrapper around an AddrSummary object to ensure that sufficient atomicity is provided.
		 *
		 * More specifically, the AddrWatch class needs to see if a particular range of adresses are
		 * valid without acquiring the arena lock (that would congest the lock
		 * needlessly). Additionally, the GC may modify the AddrSet at any time. However, the
		 * problem is vastly simplified by the fact that we assume that only the GC will modify the
		 * contained set when holding the arena lock, and while threads are stopped. This means that
		 * the implementation can assume that all changes to the contained data are atomic with
		 * regards to any threads reading the data. The fact that the AddrSet is a statically sized
		 * data structure also helps greatly, since modifying it during a read will not cause a
		 * crash, only incorrect results.
		 *
		 * The solution is fairly simple. We add a version number to the data, and make sure to
		 * update the version whenever the data is changed. As we assume that no other thread is
		 * accessing the data at that point, it does not really matter that it is difficult to do
		 * this atomically. When reading data, we simply make sure to first read the version, read
		 * the actual data, and then check the version once more. If it changed, that means we
		 * should retry.
		 */
		class HistorySummary {
		public:
			// Create.
			HistorySummary();

			// Clear contents.
			void clear();

			// Add an address range to the summary, resizing the summary to fit the addresses.
			void add(size_t from, size_t to);

			// Check if this summary intersects with another summary. Safe to call without stopping
			// other threads.
			template <class Set>
			bool intersects(const Set &other) const {
				bool result;
				size_t old;
				do {
					// Read version.
					old = atomicRead(version);

					// Check intersections.
					result = summary.intersects(other);

					// Read again, and check if it is the same.
				} while (old != atomicRead(version));

				return result;
			}


		private:
			// The actual summary.
			AddrSet<historyBytes> summary;

			// Version of the data. It doesn't matter that this might wrap, as long as it does not
			// wrap around during a single GC cycle. A pointer type means that a number of objects
			// equal to the bytes in the system need to be copied for that to happen, which is not
			// possible since each object is at least 4 bytes.
			size_t version;

			// Bump the version.
			inline void bump() {
				// Note: Atomics are actually overkill. A barrier would suffice.
				atomicIncrement(version);
			}
		};


		/**
		 * Class that stores object movement history, so that we can implement the ability to ask
		 * whether or not certain objects have been moved or not.
		 *
		 * This is done by keeping track of a number of pointer summaries that indicate the areas of
		 * moved objects, along with an integer that is updated each time we move some objects (the
		 * epoch). The integer thus gives a measurement of time that allows us to talk about events
		 * in the past.
		 *
		 * Note: The epoch is represented by a 64-bit number to avoid handling the number wrapping
		 * around (we can represent timestamps in ms resolution with 64-bit numbers for the
		 * foreseeable future, so it should be enough). Furthermore, epoch 0 does not exists, so
		 * that it can be used as a value representing "not initialized".
		 *
		 * Since location dependencies are fairly common, we want to be able to extract this
		 * information from the history class without acquiring any locks.
		 */
		class History {
			friend class AddrWatch;
		public:
			// Create.
			History();

			// Note the beginning of a new epoch.
			void step(ArenaTicket &lock);

			// Add a range of addresses to the current epoch.
			void add(ArenaTicket &lock, size_t from, size_t to);
			inline void add(ArenaTicket &lock, const void *from, const void *to) { add(lock, size_t(from), size_t(to)); }

		private:
			History(const History &o);
			History &operator =(const History &o);

			// All history elements.
			HistorySummary history[historyLength];

			// Pre-history, for accesses further behind the current epoch than 'historyLength'.
			HistorySummary preHistory;

			// Current epoch. We're manually creating a 64-bit numbers using two 32-bit numbers in
			// order to ensure sufficient atomicity on 32-bit systems.
			nat epochLow;
			nat epochHigh;
		};


		/**
		 * Watch for changes in a set of objects.
		 *
		 * Accessable without acquiring the arena lock.
		 */
		class AddrWatch {
		public:
			// Create.
			AddrWatch(const History &history);

			// Add an address to watch.
			void add(const void *addr);

			// Remove all addresses, essentially re-creating the object.
			void clear();

			// Check if any of the addresses have changed. Possibly gives false positives.
			bool check() const;

		private:
			// Reference to the history instance we're associated with.
			const History &history;

			// Size of our summary. Currently the same size as the history, but could benefit from being smaller.
			typedef AddrSet<historyBytes> Summary;

			// Set of addresses to be watched. This set contains the addresses of the objects before
			// they were potentially moved.
			Summary watching;

			// The earliest epoch the pointers in 'watching' are referring to.
			nat epochLow;
			nat epochHigh;

			// Is this instance empty?
			bool empty() const;
		};

	}
}

#endif
