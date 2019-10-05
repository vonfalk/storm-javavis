#include "stdafx.h"
#include "History.h"

#if STORM_GC == STORM_GC_SMM

#include "ArenaTicket.h"

namespace storm {
	namespace smm {

		/**
		 * Address summary.
		 */

		HistorySummary::HistorySummary() : summary(), version(0) {}

		void HistorySummary::clear() {
			summary = AddrSet<historyBytes>();
			bump();
		}

		void HistorySummary::add(size_t from, size_t to) {
			if (!summary.covers(from, to))
				summary = summary.resize(from, to);
			summary.add(from, to);
			bump();
		}


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

			// history[epochLow % historyLength] = ticket.reservedSet<AddrSummary>();
			history[epochLow % historyLength].clear();
		}

		void History::add(ArenaTicket &ticket, size_t from, size_t to) {
			for (nat i = 0; i < historyLength; i++) {
				history[i].add(from, to);
			}

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
			watching = Summary();
			epochLow = 0;
			epochHigh = 0;
		}

		// Epoch is the one we're looking at, 'hist' is where we're at now. Ie. 'epoch' <= 'hist'.
		static bool inHistory(nat epochLow, nat epochHigh, nat histLow, nat histHigh) {
			if (histLow - epochLow > historyLength) {
				return false;
			} else {
				// The low part is close enough, but did we wrap?
				if (histLow < epochLow)
					return histHigh == epochHigh + 1;
				else
					return histHigh == epochHigh;
			}
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

			// If we are too far behind, we're done now. The HistorySummary handles the rest.
			if (!inHistory(epochLow, epochHigh, histLow, histHigh))
				return history.preHistory.intersects(watching);

			// Otherwise, we should check the history.
			bool moved = history.history[epochLow % historyLength].intersects(watching);

			// Re-check so that we read the right element. If 'histLow' changed, we need to re-check
			// if we did read valid data.
			nat newHistLow = atomicRead(history.epochLow);
			if (newHistLow != histLow) {
				histLow = newHistLow;
				histHigh = atomicRead(history.epochHigh);

				// If it is no longer in history, we might have read a value that turned stale
				// during our read. Therefore, we need to fall back to pre-history.
				if (!inHistory(epochLow, epochHigh, histLow, histHigh))
					return history.preHistory.intersects(watching);
			}

			return moved;
		}

		bool AddrWatch::empty() const {
			return (epochLow == 0) & (epochHigh == 0);
		}

	}
}

#endif
