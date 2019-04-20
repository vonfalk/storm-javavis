#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Block.h"
#include "Generation.h"
#include "Scanner.h"

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
			ScanState(Generation *from, Generation *to);

			// Destroy.
			~ScanState();

			// Scanner that can be used to move relevant objects to a scan state.
			class Move;

			// Move an object from its current location into the new generation. The old object is
			// replaced with a forwarding object.
			void move(void *obj);

		private:
			// No copying!
			ScanState(const ScanState &o);
			ScanState &operator =(const ScanState &o);

			// Source generation.
			Generation *sourceGen;

			// The generation where we shall store objects.
			Generation *targetGen;

			// First block containing objects that have been copied but not yet scanned. Objects
			// from 'committed' up to 'reserved' should be scanned.
			Block *targetHead;

			// Last block containing copied objects. This is also where new objects are copied to.
			Block *targetTail;

			// Allocate a new block for us to use, having a minimum of 'size' bytes free. This block
			// is allocated at the end of the target queue as 'targetTail'.
			void newBlock(size_t minSize);
		};


		/**
		 * Scanner class that moves objects into a ScanState.
		 */
		class ScanState::Move {
		public:
			typedef int Result;
			typedef ScanState Source;

			ScanState &state;
			Arena &arena;
			byte srcGen;

			Move(Source &source) : state(source), arena(source.sourceGen->owner), srcGen(source.sourceGen->identifier) {}

			inline bool fix1(void *ptr) {
				return arena.memGeneration(ptr) == srcGen;
			}

			inline Result fix2(void **ptr) {
				void *obj = *ptr;
				void *end = (char *)fmt::skip(obj) - fmt::headerSize;

				// Don't touch pinned objects!
				if (state.sourceGen->isPinned(obj, end))
					return 0;

				// TODO: Forward any errors!
				state.move(obj);
				return 0;
			}

			SCAN_FIX_HEADER
		};

	}
}

#endif
