#include "stdafx.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

#include "FinalizerPool.h"

namespace storm {
	namespace smm {

		ArenaTicket::ArenaTicket(Arena &owner) : owner(owner), locked(false) {
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

			owner.entries--;
			owner.lock.unlock();
			locked = false;
		}

		void ArenaTicket::requestCollection(Generation *gen) {
			triggered.add(gen->identifier);
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
		}


	}
}

#endif
