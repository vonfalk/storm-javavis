#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Block.h"
#include "Generation.h"
#include "Scanner.h"
#include "Arena.h"

namespace storm {
	namespace smm {

		/**
		 * Class representing the state of a scan cycle for some generation.
		 *
		 * This class essentially implements a queue of objects, stored in memory provided by some
		 * generation, that have been recently been moved to a new generation but have not yet been
		 * scanned themselves. Basically, this class keeps track of the set of gray objects during a
		 * scan in a tri-color garbage collecting scheme.
		 */
		class ScanState {
		public:
			// Create a new scan state. Scanning objects in 'from', moving them to 'to'.
			ScanState(ArenaTicket &ticket, const Generation::State &from, Generation *to);

			// Destroy.
			~ScanState();

			// Scanner that can be used to move relevant objects to a scan state.
			class Move;

			// Move an object from its current location into the new generation. The old object is
			// replaced with a forwarding object, and the address to the new object is returned.
			void *move(void *obj);

			// Scan all recently copied objects and recursively move reachable objects to the 'to' generation.
			void scanNew();

			// Scan all weak objects. This will mark the weak objects as scanned, and will only be possible once.
			template <class Scanner>
			typename Scanner::Result scanWeak(typename Scanner::Source &source) {
				typename Scanner::Result error;
				while (weak.scanStep<Scanner>(source, error)) {
					if (error != typename Scanner::Result())
						return error;
				}
				return error;
			}

		private:
			// No copying!
			ScanState(const ScanState &o);
			ScanState &operator =(const ScanState &o);

			// Source generation.
			const Generation::State &sourceGen;

			/**
			 * A queue of objects that may be scanned at a later point.
			 */
			struct Queue {
				// Target generation, where we allocate memory from.
				Generation *target;

				// Arena ticket.
				ArenaTicket &ticket;

				// First block containing objects that have been copied but not yet scanned. Objects
				// from 'committed' up to 'reserved' should be scanned.
				Block *head;

				// Last block containing copied objects. This is also where new objects are copied to.
				Block *tail;

				// Create.
				Queue(Generation *target, ArenaTicket &ticket)
					: target(target), ticket(ticket), head(null), tail(null) {}

				// Finish.
				~Queue();

				// Allocate a new block from 'sourceGen' and store it in 'tail'. Make sure we have
				// at least 'minSize' bytes free in the 'tail' block.
				void newBlock(size_t minSize);

				// Perform one step (= one contiguous range of addresses) of scanning new
				// objects. Returns 'true' if anything was done, or 'false' if there is nothing more
				// to do.
				// TODO: We need error handling somehow!
				template <class Scanner>
				bool scanStep(typename Scanner::Source &source, typename Scanner::Result &error) {
					error = typename Scanner::Result();

					Block *b = head;
					if (!b)
						return false;

					size_t from = b->committed();
					size_t to = b->reserved();
					if (from >= to)
						return false;

					// TODO: Handle errors!
					error = fmt::Scan<Scanner>::objects(source,
														b->mem(from + fmt::headerSize),
														b->mem(to + fmt::headerSize));
					if (error != typename Scanner::Result())
						return false;

					b->committed(to);

					// Make sure that the objects we scanned are marked as new objects, ie. we moved
					// things to these blocks during the GC cycle.
					ticket.objectsMovedTo(b->mem(from), b->mem(to));

					// Are we done scanning this block?
					if (to == b->size) {
						Block *next = b->next();
						if (next) {
							head = next;
						} else {
							head = null;
							tail = null;
						}

						target->done(ticket, b);
					}

					return head != null;
				}
			};

			// Queue for regular objects. These are scanned during the copying.
			Queue target;

			// Queue for objects with weak references. These are scanned once at the end of the process.
			Queue weak;
		};


		/**
		 * Scanner class that moves objects into a ScanState.
		 *
		 * In order to avoid re-scanning objects we've already touched, we make sure to update all
		 * scanned references immediately.
		 */
		class ScanState::Move {
		public:
			typedef int Result;
			typedef ScanState Source;

			ScanState &state;
			Arena &arena;
			byte srcGen;

			Move(Source &source)
				: state(source), arena(source.sourceGen.arena()), srcGen(source.sourceGen.identifier()) {}

			inline ScanOption object(void *, void *) const {
				return scanAll;
			}

			inline bool fix1(void *ptr) {
				return arena.memGeneration(ptr) == srcGen;
			}

			inline Result fix2(void **ptr) {
				void *obj = *ptr;
				void *end = (char *)fmt::skip(obj) - fmt::headerSize;

				// Don't touch pinned objects!
				if (state.sourceGen.isPinned(obj, end))
					return 0;

				// We don't need to copy forwarders again. Note: Will update the pointer if
				// necessary!
				if (fmt::isFwd(obj, ptr))
					return 0;

				// TODO: Forward any errors!
				*ptr = state.move(obj);
				return 0;
			}

			SCAN_FIX_HEADER
		};

	}
}

#endif
