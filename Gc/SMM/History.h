#pragma once

#if STORM_GC == STORM_GC_SMM

#include "AddrSet.h"

namespace storm {
	namespace smm {

		class AddrWatch;
		class ArenaTicket;

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
			inline void add(ArenaTicket &lock, void *from, void *to) { add(lock, size_t(from), size_t(to)); }

		private:
			History(const History &o);
			History &operator =(const History &o);

			// All history elements.
			AddrSummary history[historySize];

			// Pre-history, for accesses further behind the current epoch than 'historySize'.
			AddrSummary preHistory;

			// Current epoch. We're manually creating a 64-bit numbers using two 32-bit numbers in
			// order to ensure sufficient atomicity for 32-bit systems.
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

			// Set of addresses to be watched. This set contains the addresses of the objects before
			// they were potentially moved.
			AddrSummary watching;

			// The earliest epoch the pointers in 'watching' are referring to.
			nat epochLow;
			nat epochHigh;

			// Is this instance empty?
			bool empty() const;
		};

	}
}

#endif
