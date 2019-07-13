#include "stdafx.h"
#include "History.h"

#if STORM_GC == STORM_GC_SMM

#include "ArenaTicket.h"

namespace storm {
	namespace smm {

		/**
		 * History.
		 */

		History::History() : epochLow(1), epochHigh(0) {}

		void History::step(ArenaTicket &ticket) {
			// This must be done atomically wrt. other threads. Generally, we will only call this
			// function when we need to stop threads anyway, so this is mostly a precaution.
			ticket.stopThreads();

			if (epochLow == std::numeric_limits<nat>::max()) {
				epochLow = 0;
				epochHigh++;
			} else {
				epochLow++;
			}

			// history[epochLow % historySize] = ticket.reservedSet<AddrSummary>();
			history[epochLow % historySize] = AddrSummary();
		}

		void History::add(ArenaTicket &ticket, size_t from, size_t to) {
			for (nat i = 0; i < historySize; i++) {
				if (!history[i].covers(from, to))
					history[i] = history[i].resize(from, to);
				history[i].add(from, to);
			}

			if (!preHistory.covers(from, to))
				preHistory = preHistory.resize(from, to);
			preHistory.add(from, to);
		}


		/**
		 * Watch.
		 */

		AddrWatch::AddrWatch(const History &history) : history(history) {
			clear();
		}

		void AddrWatch::add(const void *addr) {
			// First time: copy the current epoch to us. Having a too low epoch will only give false
			// positives, so that is fine.
			if (empty()) {
				// Retry until we get a consistent picture of the counter. Since 'step' is executed
				// without any other threads running, we know that the entire epoch variable will be
				// updated atomically as seen from us. Thereby, we can just read one part, then the
				// other and finally check the first part. If that changed, we know we need to
				// re-read the number to get a consistent picture. We start with 'high' since that
				// updates more seldom.

				do {
					epochHigh = atomicRead(history.epochHigh);
					epochLow = atomicRead(history.epochLow);
				} while (epochHigh != atomicRead(history.epochHigh));
			}


			if (!watching.covers(addr))
				watching = watching.resize(addr, (const byte *)addr + 1);
			watching.add(addr);
		}

		void AddrWatch::clear() {
			watching = AddrSummary();
			epochLow = 0;
			epochHigh = 0;
		}

		bool AddrWatch::check() const {
			if (empty())
				return false;

			// TODO: We could keep track of a second set of summaries inside History: one for "moved
			// from" (which we're doing now), and one "moved to". Then we could support checking
			// individual addresses as well by checking if the desired pointer has been moved
			// recently. This could reduce the rate of false positives slightly.

			// Read the epoch from the history object. By reading 'low' before 'high', we ensure
			// that we will get a too large number if we are interrupted between the two reads. This
			// is fine, as we will then look too far back in the history, which is more likely to
			// say that an object moved, which is fine according to our semantics.
			nat histLow = atomicRead(history.epochLow);
			nat histHigh = atomicRead(history.epochHigh);

			// If we're not behind at all, then we know that nothing has changed.
			if (histLow == epochLow && histHigh == epochHigh)
				return false;

			// Read the history entry from the proper location. If this is written to while we read
			// it, we will discard it later. This means we don't have to worry too much about
			// atomicity here...
			AddrSummary moved = history.history[histLow % historySize];

			// Re-read the epoch and see if whatever we just read is still valid.
			histLow = atomicRead(history.epochLow);
			histHigh = atomicRead(history.epochHigh);

			// See if we're too far behind. If we are, use the prehistory instead.
			if (histLow - epochLow > historySize) {
				moved = history.preHistory;
			} else {
				// The low part is close enough, but did we wrap?
				if (histLow < epochLow) {
					if (histHigh != epochHigh + 1)
						moved = history.preHistory;
				} else {
					if (histHigh != epochHigh)
						moved = history.preHistory;
				}
			}

			return watching.intersects(moved);
		}

		bool AddrWatch::empty() const {
			return (epochLow == 0) & (epochHigh == 0);
		}

	}
}

#endif
