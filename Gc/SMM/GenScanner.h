#pragma once

#if STORM_GC == STORM_GC_SMM

#include "GenSet.h"
#include "ArenaTicket.h"

namespace storm {
	namespace smm {

		/**
		 * Scan, creating a GenSet that summarizes all pointers while using another pair of scanners.
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
				result.add(ticket.safeIdentifier(ptr));

				if (predicate.forward)
					return scanner.fix1(ptr);
				else
					return false;
			}

			inline Result fix2(void **ptr) {
				return scanner.fix2(ptr);
			}

			inline bool fixHeader1(GcType *header) {
				result.add(ticket.safeIdentifier(header));

				if (predicate.forward)
					return scanner.fixHeader1(header);
				else
					return false;
			}

			inline Result fixHeader2(GcType **header) {
				return scanner.fixHeader2(header);
			}

		};

	}
}

#endif
