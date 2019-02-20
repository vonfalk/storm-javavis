#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * A queue with a fixed maximum size, so that we may allocate it on the stack or in other
		 * fixed sized structures.
		 *
		 * Note: We assume T is a simple type, such as a pointer. We don't bother properly creating
		 * and destroying objects. We simply overwrite them.
		 */
		template <class T, size_t size>
		class FixedQueue {
		public:
			FixedQueue() : head(0), tail(0) {}

			bool any() const { return head != tail; }
			bool empty() const { return head == tail; }
			bool full() const {
				return head + 1 == tail
					|| (head + 1 == size && tail == 0);
			}

			const T &top() const {
				return data[tail];
			}

			void pop() {
				if (empty())
					return;
				if (++tail == size)
					tail = 0;
			}

			// Returns 'false' if the queue was full.
			bool push(const T &elem) {
				if (full())
					return false;
				data[head++] = elem;
				if (head == size)
					head = 0;
				return true;
			}

		private:
			T data[size];
			size_t head;
			size_t tail;
		};

	}
}

#endif
