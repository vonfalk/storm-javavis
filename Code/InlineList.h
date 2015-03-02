#pragma once

namespace code {

	/**
	 * A singly linked list that is placed inline in the
	 * data structure. Supports basic push and pop operations,
	 * not much more.
	 *
	 * T must have a T *next member that is initialized to null.
	 * Therefore, each element can not reside in more than one
	 * list at once. This is asserted in debug builds.
	 */
	template <class T>
	class InlineList : NoCopy {
	public:
		// Create an empty list.
		InlineList() : head(null), tail(null) {}

		// Empty the list.
		~InlineList() {
			while (head) {
				T *t = head;
				head = head->next;
				t->next = null;
			}
			head = tail = null;
		}

		// Push an element to the end of the list.
		void push(T *e) {
			assert(e->next == null, L"Can not an element into more than one list.");
			if (tail)
				tail->next = e;
			else
				head = e;
			tail = e;
		}

		// Pop an element from the beginning of the list. Returns null if empty.
		T *pop() {
			if (!head)
				return null;

			T *r = head;
			head = r->next;
			r->next = null;

			if (head == null)
				tail = null;
			return r;
		}

		// Empty?
		bool empty() const {
			return head == null;
		}

		// Any?
		bool any() const {
			return head != null;
		}

	private:
		// Head and tail pointers.
		T *head, *tail;
	};

}
