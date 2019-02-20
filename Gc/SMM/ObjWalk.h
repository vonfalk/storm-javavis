#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Gc/Format.h"
#include "FixedQueue.h"
#include "Scanner.h"

namespace storm {
	namespace smm {

		template <size_t qSize>
		struct WalkScanner {
			typedef int Result;
			typedef FixedQueue<fmt::Obj *, qSize> Source;

			FixedQueue<fmt::Obj *, qSize> &q;

			WalkScanner(Source &source) : q(source) {}

			inline bool fix1(void *ptr) {
				// TODO: We only want to examine objects managed by the arena we're operating in!
				// Otherwise we will crash badly.
				// TODO: We might want to do an early out here.
				return ptr != null;
			}

			inline Result fix2(void **ptr) {
				fmt::Obj *o = fmt::fromClient(*ptr);
				if (fmt::objIsFwd(o) || fmt::objIsMarked(o))
					return 0;

				fmt::objSetMark(o);
				// TODO: We might not want to push all objects, only those fulfilling some predicate.
				return q.push(o) ? 0 : 1;
			}

			// SCAN_FIX_HEADER
			// For now: Don't scan object formats. TODO: Fix!
			inline bool fixHeader1(GcType *) { return false; }
			inline Result fixHeader2(GcType **) { return 0; }
		};

		/**
		 * Walk object without using too much space on the stack or the heap. Each call instance has
		 * a queue of a fixed maximum size. Whenever the queue is full, the current object is added
		 * to the stack and some other objects are processed. Then, the object that caused the
		 * overflow is examined once more to find the remaining pointers.
		 */
		template <size_t qSize>
		void objWalk(fmt::Obj *start) {
			FixedQueue<fmt::Obj *, qSize> queue;

			// Nothing needs to be done.
			if (fmt::objIsMarked(start))
				return;

			fmt::Obj *at = start;
			while (at) {
				fmt::Obj *next = null;

				// Scan this object.
				void *client = fmt::toClient(at);
				if (fmt::Scan<WalkScanner<qSize>>::objects(queue, client, fmt::skip(client))) {
					// We failed to add an element to the queue. Add the current object and try to
					// scan some additional objects to free up space.
					next = queue.top();
					queue.pop();
					queue.push(at);
				} else if (queue.any()) {
					// Just pop the next element, if it exists.
					next = queue.top();
					queue.pop();
				}

				// TODO: notify that we found a new object.

				at = next;
			}
		}

	}
}

#endif
