#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Root.h"
#include "Generation.h"
#include "Thread.h"
#include "SpilledRegs.h"
#include "Utils/Templates.h"
#include <csetjmp>

namespace storm {
	namespace smm {

		/**
		 * Operations on the arena that requires holding the arena lock and collecting some
		 * information that allows garbage collection to occur.
		 *
		 * Initialization of the ArenaTicket class is a bit special. Therefore, it can not be
		 * constructed as a regular class. Rather, use the 'enter' function provided below.
		 *
		 * Furthermore, we assume that ArenaTicket instances reside on the stack at the last
		 * location where references to garbage collected memory may be stored by the client
		 * program. This means that when an ArenaTicket is held, no client code may be called.
		 */
		class ArenaTicket {
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

			// Tell the ArenaTicket that a generation desires to be collected. This will trigger a
			// collection whenever the system has completed its current action (e.g. when the
			// ArenaTicket is being released).
			void scheduleCollection(Generation *gen);

			// Tell the ArenaTicket that we would like to collect the indicated generation at this
			// moment. The Ticket will immediately collect the desired generation as long as no
			// other generation is being collected. Otherwise, nothing is done.
			bool suggestCollection(Generation *gen);

			// Get the finalizer object in the arena.
			inline FinalizerPool &finalizerPool() const {
				return *owner.finalizers;
			}

			// Scan inexact roots (e.g. stacks) using the given scanner.
			template <class Scanner>
			typename Scanner::Result scanInexactRoots(typename Scanner::Source &source);

			// Scan exact roots.
			template <class Scanner>
			typename Scanner::Result scanExactRoots(typename Scanner::Source &source);

			// Scan all blocks in all generations that may refer to a block in the given
			// generation.
			template <class Scanner>
			typename Scanner::Result scanGenerations(typename Scanner::Source &source, Generation *curr);

			template <class Predicate, class Scanner>
			typename Scanner::Result scanGenerations(const Predicate &predicate,
													typename Scanner::Source &source,
													Generation *curr);

		private:
			// Associated arena.
			Arena &owner;

			// Did any generation trigger a garbage collection?
			GenSet triggered;

			// Are we holding the lock?
			bool locked;

			// Are we collecting garbage somewhere?
			bool gc;

			// Have we stopped threads?
			bool threads;

			// Lock and unlock the arena. This includes stopping threads etc.
			void lock();
			void unlock();

			// Called to perform any scheduled tasks, such as collecting certain generations.
			// Could be called in the destructor, but I'm not comfortable doing that much work there...
			void finalize();
		};


		/**
		 * Implementation of template members.
		 */

		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanInexactRoots(typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			InlineSet<Thread> &threads = owner.threads;
			for (InlineSet<Thread>::iterator i = threads.begin(); i != threads.end(); ++i) {
				r = i->scan<Scanner>(source, *this);
				if (r != typename Scanner::Result())
					return r;
			}

			InlineSet<Root> &roots = owner.inexactRoots;
			for (InlineSet<Root>::iterator i = roots.begin(); i != roots.end(); ++i) {
				r = i->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanExactRoots(typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			InlineSet<Root> &roots = owner.exactRoots;
			for (InlineSet<Root>::iterator i = roots.begin(); i != roots.end(); ++i) {
				r = i->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanGenerations(typename Scanner::Source &source, Generation *current) {
			return scanGenerations<fmt::ScanAll, Scanner>(fmt::ScanAll(), source, current);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result ArenaTicket::scanGenerations(const Predicate &predicate,
															typename Scanner::Source &source,
															Generation *current) {
			typename Scanner::Result r = typename Scanner::Result();
			GenSet scanFor;
			scanFor.add(current->identifier);

			for (size_t i = 0; i < owner.generations.size(); i++) {
				Generation *gen = owner.generations[i];

				// Don't scan the current generation, we want to handle that with more care.
				if (gen == current)
					continue;

				// Scan it, instructing the generation to only scan references to the current generation.
				r = gen->scan<Predicate, Scanner>(predicate, scanFor, source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}


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


	}
}

#endif
