#pragma once

#if STORM_GC == STORM_GC_SMM

#include "ArenaTicket.h"
#include "Generation.h"
#include "Thread.h"
#include "Gc/Exception.h"

namespace storm {
	namespace smm {

		/**
		 * Implementation of template members from ArenaTicket. These are separate, as they need
		 * some headers that would otherwise create cyclic dependencies.
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

			// We also need to scan the active exceptions.
			r = scanExceptions<Scanner>(source);
			if (r != typename Scanner::Result())
				return r;

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
		typename Scanner::Result ArenaTicket::scanGenerations(typename Scanner::Source &source, GenSet current) {
			typename Scanner::Result r = typename Scanner::Result();

			for (size_t i = 0; i < owner.generations.size(); i++) {
				Generation *gen = owner.generations[i];

				// Don't scan the current generations, we want to handle those with more care.
				if (current.has(gen->identifier))
					continue;

				// Scan it, instructing the generation to only scan references to the current generation.
				r = gen->scan<Scanner>(*this, current, source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}

		template <class Scanner>
		typename Scanner::Result ArenaTicket::scanGenerationsFinal(typename Scanner::Source &source, GenSet current) {
			typename Scanner::Result r = typename Scanner::Result();

			for (size_t i = 0; i < owner.generations.size(); i++) {
				Generation *gen = owner.generations[i];

				// Don't scan the current generations, we want to handle those with more care.
				if (current.has(gen->identifier))
					continue;

				// Scan it, instructing the generation to only scan references to the current generation.
				r = gen->scanFinal<Scanner>(*this, current, source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}


	}
}

#endif
