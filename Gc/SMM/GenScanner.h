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
		template <class Predicate, class Scanner>
		struct GenScanner {
			typedef typename Scanner::Result Result;

			struct Pred {
				const Predicate &original;

				Pred(const Predicate &original) : original(original) {}

				bool operator ()(void *start, void *end) const {
					forward = original(start, end);
					return true;
				}

				mutable bool forward;
			};

			struct Source {
				ArenaTicket &ticket;
				typename Scanner::Source &source;
				Pred predicate;
				GenSet result;

				Source(ArenaTicket &ticket, const Predicate &predicate, typename Scanner::Source &source)
					: ticket(ticket), predicate(predicate), source(source) {}
			};

			ArenaTicket &ticket;
			Pred &predicate;
			Scanner scanner;
			GenSet &result;

			GenScanner(Source &source)
				: ticket(source.ticket),
				  predicate(source.predicate),
				  scanner(source.source),
				  result(source.result) {}

			inline bool fix1(void *ptr) {
				if (predicate.forward)
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
				if (predicate.forward)
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
		struct GenScanner<void, void> {
			typedef int Result;

			struct Source {
				ArenaTicket &ticket;
				GenSet result;

				Source(ArenaTicket &ticket) : ticket(ticket) {}
			};

			ArenaTicket &ticket;
			GenSet &result;

			GenScanner(Source &source) : ticket(source.ticket), result(source.result) {}

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
