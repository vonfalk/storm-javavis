#include "stdafx.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

#include "Nonmoving.h"
#include "FinalizerPool.h"

namespace storm {
	namespace smm {

		ArenaTicket::ArenaTicket(Arena &owner) : owner(owner), locked(false), gc(false), threads(false) {
			lock();
		}

		ArenaTicket::~ArenaTicket() {
			unlock();
		}

		void ArenaTicket::lock() {
			owner.lock.lock();
			dbg_assert(owner.entries == 0, L"Recursive arena entry!");
			owner.entries++;
			locked = true;
		}

		void ArenaTicket::unlock() {
			if (!locked)
				return;

			if (threads) {
				TODO(L"Re-start threads!");
				threads = false;
			}

			owner.entries--;
			owner.lock.unlock();
			locked = false;
		}

		void ArenaTicket::stopThreads() {
			if (threads)
				return;

			assert(false, L"We don't have the ability to stop threads yet!");

			threads = true;
		}

		void ArenaTicket::scheduleCollection(Generation *gen) {
			triggered.add(gen->identifier);
		}

		bool ArenaTicket::suggestCollection(Generation *gen) {
			if (gc)
				return false;

			owner.collectI(*this, GenSet(gen->identifier));
			return true;
		}

		void ArenaTicket::finalize() {
			if (triggered.any()) {
				GenSet collected;
				GenSet old;

				do {
					// Don't collect the same generation twice!
					old = triggered;
					old.remove(collected);
					triggered.clear();

					if (old.any())
						collected.add(owner.collectI(*this, old));
				} while (old.any());
			}

			// Unlock and run finalizers!
			unlock();
			owner.finalizers->finalize();
			owner.nonmoving().runFinalizers();
		}


	}
}

#endif
