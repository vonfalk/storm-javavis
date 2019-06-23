#include "stdafx.h"
#include "History.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * History.
		 */

		History::History() {}



		/**
		 * Watch.
		 */

		AddrWatch::AddrWatch() {
			clear();
		}

		void AddrWatch::add(const void *addr) {
			TODO(L"Implement me!");
		}

		void AddrWatch::clear() {
			watching = AddrSummary();
			epoch = 0;
		}

		bool AddrWatch::check(const History &history) const {
			TODO(L"Implement me!");
			return true;
		}

	}
}

#endif
