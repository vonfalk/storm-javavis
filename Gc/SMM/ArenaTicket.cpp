#include "stdafx.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		ArenaTicket::ArenaTicket(Arena &owner) : owner(owner) {
			owner.lock.lock();
			dbg_assert(owner.entries == 0, L"Recursive arena entry!");
			owner.entries++;
		}

		ArenaTicket::~ArenaTicket() {
			owner.entries--;
			owner.lock.unlock();
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
		}


	}
}

#endif
