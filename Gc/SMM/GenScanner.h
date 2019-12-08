#pragma once

#if STORM_GC == STORM_GC_SMM

#include "GenSet.h"
#include "ArenaTicket.h"
#include "Scanner.h"

namespace storm {
	namespace smm {

		/**
		 * Scan, creating a GenSet that summarizes all pointers while using another pair of scanners.
		 *
		 * Note: We must make sure to record the contents of all pointers *after* the other scanner
		 * has been executed completely, as it is allowed to change the contents of the pointers!
		 */
		template <class Scanner>
		struct GenScanner {
			typedef typename Scanner::Result Result;

			struct Source {
				ArenaTicket &ticket;
				typename Scanner::Source &source;
				GenSet result;

				Source(ArenaTicket &ticket, typename Scanner::Source &source)
					: ticket(ticket), source(source) {}
			};

			ArenaTicket &ticket;
			ScanOption scan;
			Scanner scanner;
			GenSet &result;

			GenScanner(Source &source)
				: ticket(source.ticket),
				  scanner(source.source),
				  result(source.result),
				  scan(scanAll) {}

			inline ScanOption object(void *start, void *end) {
				scan = scanner.object(start, end);

				// We need to scan all pointers.
				return scanAll;
			}

			inline bool fix1(void *ptr) {
				if (scan == scanAll)
					if (scanner.fix1(ptr))
						return true;

				result.add(ticket.safeIdentifier(ptr));
				return false;
			}

			inline Result fix2(void **ptr) {
				Result r = scanner.fix2(ptr);
				result.add(ticket.safeIdentifier(*ptr));
				return r;
			}

			inline bool fixHeader1(GcType *header) {
				if (scan != scanNone)
					if (scanner.fixHeader1(header))
						return true;

				result.add(ticket.safeIdentifier(header));
				return false;
			}

			inline Result fixHeader2(GcType **header) {
				Result r = scanner.fixHeader2(header);
				result.add(ticket.safeIdentifier(*header));
				return r;
			}

		};

		/**
		 * Special version of the template above that only produces a summary.
		 */
		template <>
		struct GenScanner<void> {
			typedef int Result;

			struct Source {
				ArenaTicket &ticket;
				GenSet result;

				Source(ArenaTicket &ticket) : ticket(ticket) {}
			};

			ArenaTicket &ticket;
			GenSet &result;

			GenScanner(Source &source) : ticket(source.ticket), result(source.result) {}

			inline ScanOption object(void *, void *) const {
				return scanAll;
			}

			inline bool fix1(void *ptr) {
				result.add(ticket.safeIdentifier(ptr));
				return false;
			}

			inline Result fix2(void **ptr) {
				return 0;
			}

			SCAN_FIX_HEADER
		};

	}
}

#endif
