#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Gc/Format.h"
#include "FixedQueue.h"
#include "Scanner.h"

namespace storm {
	namespace smm {

		template <class Filter, size_t qSize>
		struct WalkScanner {
			typedef int Result;
			typedef WalkScanner Source;

			FixedQueue<fmt::Obj *, qSize> &q;
			Filter &filter;

			WalkScanner(FixedQueue<fmt::Obj *, qSize> &q, Filter &filter) : q(q), filter(filter) {}

			WalkScanner(WalkScanner &self) : q(self.q), filter(self.filter) {}

			inline bool fix1(void *ptr) {
				return filter(ptr);
			}

			inline Result fix2(void **ptr) {
				fmt::Obj *o = fmt::fromClient(*ptr);
				if (fmt::objIsSpecial(o) || fmt::objIsMarked(o))
					return 0;

				fmt::objSetMark(o);
				// TODO: We might not want to push all objects, only those fulfilling some predicate.
				return q.push(o) ? 0 : 1;
			}

			SCAN_FIX_HEADER
		};

		/**
		 * Walk object without using too much space on the stack or the heap. Each call instance has
		 * a queue of a fixed maximum size. Whenever the queue is full, the current object is added
		 * to the stack and some other objects are processed. Then, the object that caused the
		 * overflow is examined once more to find the remaining pointers.
		 *
		 * 'filter' is called to exclude certain pointers that should not be scanned. This implementation
		 * will be given a client pointer that is possibly not exactly a client pointer, but it has to
		 * reside inside the object.
		 * 'apply' is called for each marked object in the region, when the algorithm does not need
		 * to examine the object anymore.
		 */
		template <class Filter, class Apply, size_t qSize>
		void objWalk(fmt::Obj *start, Filter &filter, Apply &apply) {
			FixedQueue<fmt::Obj *, qSize> queue;
			typedef WalkScanner<Filter, qSize> Scanner;
			Scanner scanner(queue, filter);

			// Nothing needs to be done.
			if (fmt::objIsMarked(start))
				return;
			fmt::objSetMark(start);

			fmt::Obj *at = start;
			while (at) {
				fmt::Obj *next = null;

				// Scan this object.
				void *client = fmt::toClient(at);
				if (fmt::Scan<Scanner>::objects(scanner, client, fmt::skip(client))) {
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

				// Notify the caller.
				apply(at);

				at = next;
			}
		}

	}
}

#endif
