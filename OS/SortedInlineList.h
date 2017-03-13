#pragma once

namespace os {

	/**
	 * A singly linked list that is placed inline in the data structure. Supports basic push and pop
	 * operations and not much more.
	 *
	 * T must have a T *next member that is initialized to null. Therefore, each element can not
	 * reside in more than one list at once. This is asserted in debug builds. Aside from this,
	 * elements should be comparable using <.
	 */
	template <class T>
	class SortedInlineList : NoCopy {
	public:
		// Create an empty list.
		SortedInlineList() : head(null) {}

		// Empty the list.
		~SortedInlineList() {
			while (head) {
				T *t = head;
				head = head->next;
				t->next = null;
			}
			head = null;
		}

		// Push an element to its location in the list.
		void push(T *elem) {
			assert(elem->next == null, L"Can not push an element into more than one list.");

			T **to = &head;
			while (*to && **to < *elem)
				to = &((*to)->next);

			elem->next = *to;
			*to = elem;
		}

		// Pop an element from the beginning of the list.
		T *pop() {
			if (!head)
				return null;

			T *r = head;
			head = r->next;
			r->next = null;
			return r;
		}

		// Peek the topmost element.
		T *peek() {
			return head;
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
		// Head.
		T *head;
	};

}
