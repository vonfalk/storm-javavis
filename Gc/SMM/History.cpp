#include "stdafx.h"
#include "History.h"

#if STORM_GC == STORM_GC_SMM

#include "ArenaTicket.h"

namespace storm {
	namespace smm {

		// Query is the one we're looking at, 'epoch' is where we're at now. Ie. 'query' <= 'epoch'.
		static bool inHistory(nat queryLow, nat queryHigh, nat epochLow, nat epochHigh) {
			if (epochLow - queryLow > historyLength) {
				return false;
			} else {
				// The low part is close enough, but did we wrap?
				if (epochLow < queryLow)
					return epochHigh == queryHigh + 1;
				else
					return epochHigh == queryHigh;
			}
		}


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

		wostream &operator <<(wostream &to, const HistorySummary &o) {
			return to << o.summary;
		}


		/**
		 * History set.
		 */

		HistorySet::HistorySet() {}

		bool HistorySet::query(const AddrSet<historyBytes> &watch, const History &ref, nat queryLow, nat queryHigh) const {
			// Read the epoch from the history object. By reading 'low' before 'high', we ensure
			// that we will get a too large number if we are interrupted between the two reads. This
			// is fine, as we will then look too far back in the history, which is more likely to
			// say that an object moved, which is fine according to our semantics.
			nat epochLow = atomicRead(ref.epochLow);
			nat epochHigh = atomicRead(ref.epochHigh);

			// If we are too far behind, we're done. The HistorySummary handles the rest of the threading issues.
			if (!inHistory(queryLow, queryHigh, epochLow, epochHigh))
				return preHistory.intersects(watch);

			// Otherwise, we can check the history!
			bool moved = history[queryLow % historyLength].intersects(watch);

			// Re-check so that we read the right element. If 'epochLow' changed, we need to
			// re-check if we did read valid data.
			nat newEpochLow = atomicRead(ref.epochLow);
			if (newEpochLow != epochLow) {
				epochLow = newEpochLow;
				epochHigh = atomicRead(ref.epochHigh);

				// If it is no longer in history, we read a value that might have turned stale
				// during or read. Therefore, we fall back to pre-history.
				if (!inHistory(queryLow, queryHigh, epochLow, epochHigh))
					return preHistory.intersects(watch);
			}

			return moved;
		}

		bool HistorySet::query(const void *watch, const History &ref, nat queryLow, nat queryHigh) const {
			// Read the epoch from the history object. By reading 'low' before 'high', we ensure
			// that we will get a too large number if we are interrupted between the two reads. This
			// is fine, as we will then look too far back in the history, which is more likely to
			// say that an object moved, which is fine according to our semantics.
			nat epochLow = atomicRead(ref.epochLow);
			nat epochHigh = atomicRead(ref.epochHigh);

			// If we are too far behind, we're done. The HistorySummary handles the rest of the threading issues.
			if (!inHistory(queryLow, queryHigh, epochLow, epochHigh))
				return preHistory.has(watch);

			// Otherwise, we can check the history!
			bool moved = history[queryLow % historyLength].has(watch);

			// Re-check so that we read the right element. If 'epochLow' changed, we need to
			// re-check if we did read valid data.
			nat newEpochLow = atomicRead(ref.epochLow);
			if (newEpochLow != epochLow) {
				epochLow = newEpochLow;
				epochHigh = atomicRead(ref.epochHigh);

				// If it is no longer in history, we read a value that might have turned stale
				// during or read. Therefore, we fall back to pre-history.
				if (!inHistory(queryLow, queryHigh, epochLow, epochHigh))
					return preHistory.has(watch);
			}

			return moved;
		}

		void HistorySet::step(nat newEpoch) {
			history[newEpoch % historyLength].clear();
		}

		void HistorySet::add(size_t begin, size_t end) {
			for (size_t i = 0; i < historyLength; i++)
				history[i].add(begin, end);

			preHistory.add(begin, end);
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

			from.step(epochLow);
			to.step(epochLow);
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

		bool AddrWatch::check() const {
			if (empty())
				return false;

			return history.from.query(watching, history, epochLow, epochHigh);
		}

		bool AddrWatch::check(const void *addr) const {
			if (!check())
				return false;

			// Additionally, we check if 'addr' has been written to recently.
			return history.to.query(addr, history, epochLow, epochHigh);
		}

		bool AddrWatch::empty() const {
			return (epochLow == 0) & (epochHigh == 0);
		}

	}
}

#endif
