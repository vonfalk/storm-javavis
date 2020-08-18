#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Root.h"
#include "SpilledRegs.h"
#include "Utils/Templates.h"
#include <csetjmp>

namespace storm {
	namespace smm {

		/**
		 * There are two types of ticket to the arena: ArenaTicket and LockTicket. Both of them make
		 * sure that the arena lock is held while the ticket exists. The difference is that an
		 * ArenaTicket collects more information than the LockTicket. This information is essential
		 * for proper garbage collections, but may be unnecessary if a garbage collection is known
		 * not to happen. Therefore, LockTicket only holds the arena lock, and little
		 * more. Furthermore, ArenaTicket may trigger finalizers upon destruction while LockTicket
		 * does not. Having an ArenaTicket implies also having a LockTicket.
		 *
		 * Initialization of these classes are a bit special. Therefore, it can not be constructed
		 * as a regular class. Rather, use the 'enter' function provided in the Arena class.
		 */
		class LockTicket {
		private:
			friend class Arena;
			friend class ArenaTicket;

			// Create.
			LockTicket(Arena &arena);
			LockTicket();
			LockTicket(const LockTicket &o);
			LockTicket &operator =(const LockTicket &o);

		public:
			// Destroy.
			~LockTicket();

		protected:
			// Associated arena.
			Arena &owner;

			// Are we holding the lock?
			bool locked;

			// Lock and unlock the arena. This includes stopping threads etc.
			void lock();
			void unlock();
		};


		/**
		 * We assume that ArenaTicket instances reside on the stack at the last
		 * location where references to garbage collected memory may be stored by the client
		 * program. This means that when an ArenaTicket is held, no client code may be called.
		 */
		class ArenaTicket : public LockTicket {
		private:
			friend class Arena;

			// Create.
			ArenaTicket(Arena &arena);
			ArenaTicket();
			ArenaTicket(const ArenaTicket &o);
			ArenaTicket &operator =(const ArenaTicket &o);

		public:
			// Destroy.
			~ArenaTicket();

			// Collected state of the stack when we entered the arena, so that we can easily scan
			// parts of the CPU state along with the rest of the stack.
			SpilledRegs regs;

			// Tell the ticket that we're currently performing garbage collection, and shall deny
			// suggested requests for collection.
			void gcRunning() { gc = true; }

			// Make sure that all threads that may interact with garbage collected memory are not
			// running. Threads are started again when the ticket is destroyed.
			void stopThreads();

			// Start any stopped threads. Threads will automatically be re-started as soon as the
			// ticket is destroyed anyway, but an earlier start can sometimes be desired.
			void startThreads();

			// Tell the ArenaTicket that a generation desires to be collected. This will trigger a
			// collection whenever the system has completed its current action (e.g. when the
			// ArenaTicket is being released).
			void scheduleCollection(Generation *gen);

			// Tell the ArenaTicket that we would like to collect the indicated generation at this
			// moment. The Ticket will immediately collect the desired generation as long as no
			// other generation is being collected. Otherwise, nothing is done.
			bool suggestCollection(Generation *gen);

			// Memory protection.
			inline void watchWrites(Chunk chunk) const {
				owner.alloc.watchWrites(chunk);
			}
			inline void stopWatchWrites(Chunk chunk) const {
				owner.alloc.stopWatchWrites(chunk);
			}
			inline bool anyWrites(Chunk chunk) const {
				return owner.alloc.anyWrites(chunk);
			}

			// Note that objects have been moved from the specified range.
			inline void objectsMovedFrom(const void *from, size_t size) {
				objectsMoved = true;
				owner.history.addFrom(*this, size_t(from), size_t(from) + size);
			}
			inline void objectsMovedFrom(const void *from, const void *to) {
				objectsMoved = true;
				owner.history.addFrom(*this, size_t(from), size_t(to));
			}

			// Note that objects have been moved to the specified range.
			inline void objectsMovedTo(const void *to, size_t size) {
				objectsMoved = true;
				owner.history.addTo(*this, size_t(to), size_t(to) + size);
			}
			inline void objectsMovedTo(const void *from, const void *to) {
				objectsMoved = true;
				owner.history.addTo(*this, size_t(from), size_t(to));
			}

			// Get the static object pool. This is considered being a part of all generations, since
			// it is assumed to contain a very small number of objects.
			inline Nonmoving &nonmoving() const {
				return owner.nonmoving();
			}

			// Get the finalizer object in the arena.
			inline FinalizerPool &finalizerPool() const {
				return *owner.finalizers;
			}

			// Get an addrset describing the full range currently reserved by the VM backend.
			template <class AddrSet>
			AddrSet reservedSet() const {
				return owner.alloc.addrSet<AddrSet>();
			}

			// Get the identifier for a memory address managed, or 0xFF if none exists.
			inline byte safeIdentifier(void *address) const {
				return owner.alloc.safeIdentifier(address);
			}

			// Scan only stacks using the given scanner.
			template <class Scanner>
			typename Scanner::Result scanStacks(typename Scanner::Source &source);

			// Scan inexact roots (e.g. stacks) using the given scanner.
			template <class Scanner>
			typename Scanner::Result scanInexactRoots(typename Scanner::Source &source);

			// Scan exact roots.
			template <class Scanner>
			typename Scanner::Result scanExactRoots(typename Scanner::Source &source);

			// Scan all blocks in all generations that may refer to a block in any generation
			// indicated by 'current'. We don't scan the generations indicated in 'current', as
			// those usually need to be handled with more care, and we will try to only scan blocks
			// referring to 'current'.
			template <class Scanner>
			typename Scanner::Result scanGenerations(typename Scanner::Source &source, GenSet current);

			// Scan all blocks, just as above. However, indicate that we're just about done with
			// garbage collection, and that we might want to take the opportunity to raise memory
			// barriers now to avoid scanning in the future.
			template <class Scanner>
			typename Scanner::Result scanGenerationsFinal(typename Scanner::Source &source, GenSet current);

			// Scan all formatted objects. Includes 'generations' but also other pools not involved in generations.
			template <class Scanner>
			typename Scanner::Result scanObjects(typename Scanner::Source &source);

		private:
			// Did any generation trigger a garbage collection?
			GenSet triggered;

			// Are we collecting garbage somewhere?
			bool gc;

			// Have we stopped threads?
			bool threads;

			// Did we move any objects?
			bool objectsMoved;

			// Called to perform any scheduled tasks, such as collecting certain generations.
			// Could be called in the destructor, but I'm not comfortable doing that much work there...
			void finalize();
		};



		/**
		 * Enter an arena. Note: Actually members of Arena, but they are belonging here since they
		 * are highly coupled with the behaviour of the ticket class above!
		 */

		template <class R, class M>
		typename IfNotVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &)) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			R r = (memberOf.*fn)(t);
			t.finalize();
			return r;
		}
		template <class R, class M>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &)) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			(memberOf.*fn)(t);
			t.finalize();
		}
		template <class R, class M, class P>
		typename IfNotVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			R r = (memberOf.*fn)(t, p);
			t.finalize();
			return r;
		}
		template <class R, class M, class P>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			(memberOf.*fn)(t, p);
			t.finalize();
		}
		template <class R, class M, class P, class Q>
		typename IfNotVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			R r = (memberOf.*fn)(t, p, q);
			t.finalize();
			return r;
		}
		template <class R, class M, class P, class Q>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q) {
			ArenaTicket t(*this);
			spillRegs(&t.regs);
			(memberOf.*fn)(t, p, q);
			t.finalize();
		}

		/**
		 * Lock the arena.
		 */
		template <class R, class M>
		R Arena::lock(M &memberOf, R (M::*fn)(LockTicket &)) {
			LockTicket t(*this);
			return (memberOf.*fn)(t);
		}

		template <class R, class M, class P>
		R Arena::lock(M &memberOf, R (M::*fn)(LockTicket &, P), P p) {
			LockTicket t(*this);
			return (memberOf.*fn)(t, p);
		}

		template <class R, class M, class P, class Q>
		R Arena::lock(M &memberOf, R (M::*fn)(LockTicket &, P, Q), P p, Q q) {
			LockTicket t(*this);
			return (memberOf.*fn)(t, p, q);
		}

	}
}

#endif
