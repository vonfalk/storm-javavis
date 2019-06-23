#include "stdafx.h"
#include "ArenaTicket.h"

#if STORM_GC == STORM_GC_SMM

#include "Nonmoving.h"
#include "FinalizerPool.h"

namespace storm {
	namespace smm {

		LockTicket::LockTicket(Arena &owner) : owner(owner), locked(false) {
			lock();
		}

		LockTicket::~LockTicket() {
			unlock();
		}

		void LockTicket::lock() {
			owner.arenaLock.lock();
			dbg_assert(owner.entries == 0, L"Recursive arena entry!");
			owner.entries++;
			locked = true;
		}

		void LockTicket::unlock() {
			if (!locked)
				return;

			owner.entries--;
			owner.arenaLock.unlock();
			locked = false;
		}


		ArenaTicket::ArenaTicket(Arena &owner) : LockTicket(owner), gc(false), threads(false) {}

		ArenaTicket::~ArenaTicket() {
			startThreads();
			unlock();
		}

		void ArenaTicket::stopThreads() {
			if (threads)
				return;

			assert(false, L"We don't have the ability to stop threads yet!");

			threads = true;
		}

		void ArenaTicket::startThreads() {
			if (!threads)
				return;

			threads = false;
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
			startThreads();
			unlock();

			// Note: Since references from finalizers keep nonmoving objects alive, but not the
			// other way around, we want to run finalizers for nonmoving objects first.
			owner.nonmoving().runFinalizers();
			owner.finalizers->finalize();
		}


	}
}

#endif
