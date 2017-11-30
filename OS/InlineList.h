#pragma once

namespace os {

	/**
	 * A singly linked list that is placed inline in the data structure. Supports basic push and pop
	 * operations, not much more.
	 *
	 * T must have a T *next member that is initialized to null. Therefore, each element can not
	 * reside in more than one list at once. This is asserted in debug builds.
	 *
	 * Note: When inserted into a list, we use the special value 'end' (0x1) instead of null so that
	 * we can detect that an element is inserted into a list even if it is the last element.
	 */
	template <class T>
	class InlineList : NoCopy {
		// Tag used instead of 'null' to indicate that a node is inside a list.
#define end ((T *)0x1)
	public:
		// Create an empty list.
		InlineList() : head(end), tail(end) {}

		// Empty the list.
		~InlineList() {
			while (head != end) {
				T *t = head;
				head = head->next;
				t->next = null;
			}
			head = tail = end;
		}

		// Push an element to the end of the list.
		void push(T *e) {
			assert(e->next == null, L"Can not push an element into more than one list.");
			e->next = end;
			if (tail != end)
				tail->next = e;
			else
				head = e;
			tail = e;
		}

		// Pop an element from the beginning of the list. Returns null if empty.
		T *pop() {
			if (head == end)
				return null;

			T *r = head;
			head = r->next;
			r->next = null;

			if (head == end)
				tail = end;

			return r;
		}

		// Empty?
		bool empty() const {
			return head == end;
		}

		// Any?
		bool any() const {
			return head != end;
		}

	private:
		// Head and tail pointers.
		T *head, *tail;
#undef end
	};

}
