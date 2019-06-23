#pragma once

#if STORM_GC == STORM_GC_SMM

#include "AddrSet.h"

namespace storm {
	namespace smm {

		class AddrWatch;

		/**
		 * Class that stores object movement history, so that we can implement the ability to ask
		 * whether or not certain objects have been moved or not.
		 *
		 * This is done by keeping track of a number of pointer summaries that indicate the areas of
		 * moved objects, along with an integer that is updated each time we move some objects (the
		 * epoch). The integer thus gives a measurement of time that allows us to talk about events
		 * in the past.
		 *
		 * Since location dependencies are fairly common, we want to be able to extract this
		 * information from the history class without acquiring any locks.
		 */
		class History {
		public:
			// Create.
			History();

			// Note the beginning of a new epoch.
			void step();

			// Add a range of addresses to the current epoch.
			void add(size_t from, size_t to);
			inline void add(void *from, void *to) { add(size_t(from), size_t(to)); }

		private:
			History(const History &o);
			History &operator =(const History &o);

			// All history elements.
			AddrSummary history[historySize];

			// Pre-history, for accesses further behind the current epoch than 'historySize'.
			AddrSummary preHistory;
		};


		/**
		 * Watch for changes in a set of objects.
		 *
		 * Accessable without acquiring the arena lock.
		 */
		class AddrWatch {
		public:
			// Create.
			AddrWatch();

			// Add an address to watch.
			void add(const void *addr);

			// Remove all addresses, essentially re-creating the object.
			void clear();

			// Check if any of the addresses have changed. Possibly gives false positives.
			bool check(const History &history) const;

		private:
			// Set of addresses to be watched. This set contains the addresses of the objects before
			// they were potentially moved.
			AddrSummary watching;

			// The earliest epoch we're referring to.
			size_t epoch;
		};

	}
}

#endif
