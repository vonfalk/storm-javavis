#include "stdafx.h"
#include "Arena.h"

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"
#include "Generation.h"
#include "Root.h"
#include "Block.h"
#include "Thread.h"
#include "Nonmoving.h"
#include "ArenaTicket.h"
#include "FinalizerPool.h"

namespace storm {
	namespace smm {

		// TODO: Make the nursery generation size customizable.
		Arena::Arena(size_t initialSize, const size_t *genSize, size_t generationCount)
			: alloc(initialSize), entries(0), rampAttempts(0) {

			// Check assumptions of the formatting code.
			fmt::init();

			// Check our limit on generations. If this is broken, things will go very wrong.
			assert(generationCount >= 2, L"Must have at least two generations.");
			byte limit = GenSet::maxGen - firstFreeIdentifier;
			assert(generationCount < limit, L"Must have fewer than " + ::toS(limit) + L" generations.");

			// Create the static allocations.
			nonmovingAllocs = new Nonmoving(*this);

			// Create the finalizer pool.
			finalizers = new FinalizerPool(*this);

			// Allocate the individual generations.
			byte genId = firstFreeIdentifier;
			generations = vector<Generation *>(generationCount + 1, null);
			for (size_t i = 0; i < generationCount; i++) {
				assert(genId != finalizerIdentifier);

				size_t genSz = roundUp(genSize[i], alloc.pageSize);
				generations[i] = new Generation(*this, genSz, genId++);
			}

			// Duplicate the last generation.
			{
				size_t genSz = roundUp(genSize[generationCount - 1], alloc.pageSize);
				generations[generationCount] = new Generation(*this, genSz, genId++);
			}

			// Connect the generations.
			for (size_t i = 0; i < generationCount; i++)
				generations[i]->next = generations[i + 1];

			// Make the next-to-last generation collect into the duplicated one, just as the last
			// one. Otherwise garbage will get out of sync.
			generations[generationCount - 2]->next = generations[generationCount];


			// TODO: We most likely want to connect the generations in a more interesting pattern,
			// Additionally, it is worth investigating whether it is useful to have parallel "lanes"
			// for objects of certain types or not. E.g. objects with finalizers.
		}

		Arena::~Arena() {
			destroy();
		}

		void Arena::destroy() {
			// Note: We might call 'destroy' multiple times, since it is called from the destructor
			// but also possibly earlier than that.

			exactRoots.clear();
			inexactRoots.clear();

			// Note: This causes 'finalizers' to execute all pending finalizers as well!
			delete finalizers;
			finalizers = null;

			{
				FinalizerContext context;
				// Look through the memory for additional finalizers that need to be executed.
				for (size_t i = 0; i < generations.size(); i++)
					generations[i]->runAllFinalizers(context);
				if (nonmovingAllocs)
					nonmovingAllocs->runAllFinalizers(context);
			}

			for (size_t i = 0; i < generations.size(); i++)
				delete generations[i];
			generations.clear();

			delete nonmovingAllocs;
			nonmovingAllocs = null;
		}

		Chunk Arena::allocChunk(size_t size, byte identifier) {
			util::Lock::L z(arenaLock);

			return alloc.alloc(size, identifier);
		}

		Chunk Arena::allocChunk(size_t min, size_t preferred, byte identifier) {
			util::Lock::L z(arenaLock);

			return alloc.alloc(min, preferred, identifier);
		}

		void Arena::freeChunk(Chunk chunk) {
			util::Lock::L z(arenaLock);
			alloc.free(chunk);
		}

		Thread *Arena::attachThread() {
			Thread *t = new Thread(*this);
			{
				util::Lock::L z(arenaLock);
				threads.insert(t);
			}
			return t;
		}

		void Arena::detachThread(Thread *thread) {
			util::Lock::L z(arenaLock);
			threads.erase(thread);
		}

		void Arena::addExact(Root &root) {
			util::Lock::L z(arenaLock);
			exactRoots.insert(&root);
		}

		void Arena::removeExact(Root &root) {
			util::Lock::L z(arenaLock);
			exactRoots.erase(&root);
		}

		void Arena::addInexact(Root &root) {
			util::Lock::L z(arenaLock);
			inexactRoots.insert(&root);
		}

		void Arena::removeInexact(Root &root) {
			util::Lock::L z(arenaLock);
			inexactRoots.erase(&root);
		}

		void Arena::collect() {
			enter(*this, &Arena::collectI);
		}

		void Arena::collectI(ArenaTicket &entry) {
			// The client program asked us for a full collection, causing all generations to be
			// collected (not simultaneously at the moment, we might want to do that in order to
			// reduce memory usage as much as possible without requiring multiple calls to 'collect').
			// Note: We're not collecting the last generation (the duplicate one), since that generation
			// do not have anything to collect into. Instead we swap the last two generations when
			// we are done so that it is collected the next time.
			for (size_t i = generations.size() - 1; i > 0; i--) {
				generations[i - 1]->collect(entry);
			}

			swapLastGens();
		}

		// Collect the specified generation, but first see if there is enough free space in the
		// following generations. Any generations in 'ignore' are assumed to have been collected
		// already and are ignored.
		// Doing this prevents objects from an early generation to be moved multiple steps through
		// the generations during a single GC cycle, making it age unnecessarily quickly, which
		// causes extra work for the GC in the long run.
		static void collectRecursive(ArenaTicket &entry, Generation *gen, GenSet ignore) {
			Generation *next = gen->next;
			if (!next)
				return;

			if (gen->currentUsed() > next->currentGrace()) {
				if (ignore.has(next->identifier))
					return;

				// The data from 'gen' might not fit. We need to collect the next one as well!
				collectRecursive(entry, next, ignore);
			}

			// Now, we can collect this one.
			gen->collect(entry);
		}

		GenSet Arena::collectI(ArenaTicket &entry, GenSet collect) {
			size_t last = generations.size() - 1;

			// If we're collecting one of the last generations, collect the other one as
			// well. Otherwise, we'll get out of sync with the swapping of the last generations!
			bool swapLast = collect.has(generations[last - 1]->identifier)
				|| collect.has(generations[last - 2]->identifier);

			if (swapLast) {
				collect.add(generations[last - 1]->identifier);
				collect.add(generations[last - 2]->identifier);
			}

			for (size_t i = last; i > 0; i--) {
				Generation *gen = generations[i - 1];
				if (collect.has(gen->identifier)) {
					collectRecursive(entry, gen, collect);
				}
			}

			if (swapLast)
				swapLastGens();

			return collect;
		}

		void Arena::swapLastGens() {
			size_t gens = generations.size();
			swap(generations[gens - 1], generations[gens - 2]);

			// Fix pointers. Assumes that we only need to look at the second-to-last one.
			generations[gens - 1]->next = null;
			generations[gens - 2]->next = generations[gens - 1];
			generations[gens - 3]->next = generations[gens - 1];
		}

		MemorySummary Arena::summary() {
			util::Lock::L z(arenaLock);

			MemorySummary summary;
			alloc.fillSummary(summary);

			for (size_t i = 0; i < generations.size(); i++) {
				generations[i]->fillSummary(summary);
			}

			nonmovingAllocs->fillSummary(summary);
			finalizers->fillSummary(summary);

			return summary;
		}

		void Arena::startRamp() {
			atomicIncrement(rampAttempts);
		}

		void Arena::endRamp() {
			if (atomicDecrement(rampAttempts) == 1) {
				// Reset and read.
				size_t collect = atomicAnd(rampBlockedCollections, 0);

				// Collect the generations that wished to be collected.
				GenSet gens;
				for (byte i = 0; collect; i++, collect >>= 1) {
					if (collect & 0x1)
						gens.add(i);
				}

				enter(*this, &Arena::collectI, gens);
			}
		}

		void Arena::dbg_verify() {
			util::Lock::L z(arenaLock);

			// Check so that the only pointer to gen-2 is from gen-3.
			size_t gens = generations.size();
			for (size_t i = 0; i < gens; i++)
				if (i != gens - 3)
					assert(generations[i]->next != generations[gens - 2], L"Only generation -3 may point to generation -2!");

			// Verify the generations!
			for (size_t i = 0; i < gens; i++)
				generations[i]->dbg_verify();

			nonmovingAllocs->dbg_verify();

			// TODO: Check finalizer pool as well?
		}

		void Arena::dbg_dump() {
			util::Lock::L z(arenaLock);

			PLN(summary());

			for (size_t i = 0; i < generations.size(); i++) {
				PLN(L"Generation " << (i + 1) << L":");
				Indent z(util::debugStream());
				generations[i]->dbg_dump();
			}

			nonmovingAllocs->dbg_dump();

			// TODO: Dump finalizer pool as well?
		}

	}
}

#endif
