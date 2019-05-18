#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Generation.h"
#include "Thread.h"
#include "Utils/Templates.h"

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

			// Collected state of the stack when we entered the arena.
			std::jmp_buf buf;

			// Tell the ArenaTicket that a generation desires to be collected. This will trigger a
			// collection whenever the system has completed its current action (e.g. when the
			// ArenaTicket is being released).
			void requestCollection(Generation *gen);

			// Get the finalizer object in the arena.
			inline FinalizerPool &finalizerPool() const {
				return *owner.finalizers;
			}

			// Scan all stacks using the given scanner. This will scan inexact references.
			template <class Scanner>
			typename Scanner::Result scanStackRoots(typename Scanner::Source &source);

			// Scan all blocks in all generations that may refer to a block in the given
			// generation. The given generation is never scanned.
			template <class Scanner>
			typename Scanner::Result scanGenerations(typename Scanner::Source &source, Generation *curr);

		private:
			// Associated arena.
			Arena &owner;

			// Did any generation trigger a garbage collection?
			GenSet triggered;

			// Did we unlock the lock yet?
			bool unlocked;

			// Called to perform any scheduled tasks, such as collecting certain generations.
			// Could be called in the destructor, but I'm not comfortable doing that much work there...
			void finalize();
		};


		/**
		 * Implementation of template members.
		 */

		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanStackRoots(typename Scanner::Source &source) {
			typename Scanner::Result r;
			InlineSet<Thread> &threads = owner.threads;
			for (InlineSet<Thread>::iterator i = threads.begin(); i != threads.end(); ++i) {
				r = i->scan<Scanner>(source, *this);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}


		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanGenerations(typename Scanner::Source &source, Generation *current) {
			typename Scanner::Result r;
			GenSet scanFor;
			scanFor.add(current->identifier);

			for (size_t i = 0; i < owner.generations.size(); i++) {
				Generation *gen = owner.generations[i];

				// Don't scan the current generation, we want to handle that with more care.
				if (gen == current)
					continue;

				// Scan it, instructing the generation to only scan references to the current generation.
				r = gen->scan<Scanner>(scanFor, source);
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
			ArenaTicket e(*this);
			setjmp(e.buf);
			R r = (memberOf.*fn)(e);
			e.finalize();
			return r;
		}
		template <class R, class M>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &)) {
			ArenaTicket e(*this);
			setjmp(e.buf);
			(memberOf.*fn)(e);
			e.finalize();
		}
		template <class R, class M, class P>
		typename IfNotVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p) {
			ArenaTicket e(*this);
			setjmp(e.buf);
			R r = (memberOf.*fn)(e, p);
			e.finalize();
			return r;
		}
		template <class R, class M, class P>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p) {
			ArenaTicket e(*this);
			setjmp(e.buf);
			(memberOf.*fn)(e, p);
			e.finalize();
		}
		template <class R, class M, class P, class Q>
		typename IfNotVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q) {
			ArenaTicket e(*this);
			setjmp(e.buf);
			R r = (memberOf.*fn)(e, p, q);
			e.finalize();
			return r;
		}
		template <class R, class M, class P, class Q>
		typename IfVoid<R>::t Arena::enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q) {
			ArenaTicket e(*this);
			setjmp(e.buf);
			(memberOf.*fn)(e, p, q);
			e.finalize();
		}


	}
}

#endif
