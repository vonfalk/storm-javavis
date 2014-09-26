#pragma once

#include "Object.h"

#include <iterator>

namespace util {

	// Defines a set implemented as a doubly-linked list within its elements.
	// The elements are required to contain "next" and "prev" pointers of the
	// same type as the template parameter to this class.
	// This object is designed to keep track of a number of objects, and has
	// therefore no support for multiple keys with the same "key". The key
	// used in this set is the object pointer itself.
	// The "prev" and "next" members are assumed to be initialized to zero
	// at startup. Use the "SetMember" class to get this automatically.
	// The iterators in the set are designed to manage removal of the current
	// item. Eg, iterator a = foo.begin(); foo.erase(*a); a++; is legal. Insertions
	// during iterations will however fail in some cases.
	template <class T>
	class InlineSet : NoCopy {
	public:
		InlineSet();
		~InlineSet();

		// clear contents
		void clear();

		// Add an element to the set. It is assumed that the element has not previously been added to another set.
		void insert(T *item);

		// Remove an element from the set.
		void erase(T *item);

		// See if "item" is contained within this set. This operation is slow (linear time).
		bool contains(T *item) const;

		// Is the set empty?
		inline bool empty() { return first == null && last == null; }

		// Any elements in the set?
		inline bool any() { return !empty(); }

		// How many elements?
		inline nat size() { return count; }

		// Iterators...
		class iterator {
		private:
			friend class InlineSet<T>;
			iterator(T *at) : at(at), next(null) { if (at) next = at->next; }
			T *at;
			T *next;
		public:
			inline T *operator *() const { return at; }
			inline T *operator ->() const { return at; }
			inline iterator &operator ++() { at = next; if (next) next = next->next; return *this; }
			inline iterator operator ++(int) { iterator t = *this; at = next; if (next) next = next->next; return t; }
			inline bool operator ==(const iterator &other) const { return at == other.at; }
			inline bool operator !=(const iterator &other) const { return at != other.at; }

			typedef std::forward_iterator_tag iterator_category;
			typedef nat difference_type;
			typedef T *value_type;
			typedef T *pointer;
			typedef T &reference;
		};

		inline iterator begin() { return iterator(first); }
		iterator end() { return iterator(null); }
	private:
		T *first;
		T *last;
		nat count;

		// ensure integrity of this set, used for debugging
		void validate() const;
	};


	//////////////////////////////////////////////////////////////////////////
	// A simple class you can derive your data from to get a protected,
	// correctly initialized prev and next member in your class.
	//////////////////////////////////////////////////////////////////////////

	template <class T>
	class SetMember {
	public:
		SetMember() : prev(null), next(null) {}
		~SetMember() {
			// make sure we're not currently inside a set!
			assert(prev == null);
			assert(next == null);
		}
	private:
		friend class InlineSet<T>;
		T *prev, *next;
	};


	//////////////////////////////////////////////////////////////////////////
	// Implementation
	//////////////////////////////////////////////////////////////////////////

	template <class T>
	InlineSet<T>::InlineSet() : first(null), last(null), count(0) {}

	template <class T>
	InlineSet<T>::~InlineSet() {
		clear();
	}

	template <class T>
	void InlineSet<T>::clear() {
		T *current = first;
		while (current) {
			T *next = current->next;
			current->prev = null;
			current->next = null;

			current = next;
		}
		first = last = null;
		count = 0;
	}

	template <class T>
	void InlineSet<T>::insert(T *item) {
		++count;

		assert(item->prev == null);
		assert(item->next == null);

		item->prev = last;
		item->next = null;

		if (last) last->next = item;
		if (first == null) first = item;
		last = item;
	}

	template <class T>
	void InlineSet<T>::erase(T *item) {
		assert(count > 0);
		--count;

		if (item == first) first = item->next;
		if (item == last) last = item->prev;

		if (item->prev) item->prev->next = item->next;
		if (item->next) item->next->prev = item->prev;

		item->prev = item->next = null;
	}

	template <class T>
	bool InlineSet<T>::contains(T *item) const {
		for (T *current = first; current != null; current = current->next) {
			if (current == item) return true;
		}
		return false;
	}

	template <class T>
	void InlineSet<T>::validate() const {
		T *current = first;
		T *prev = null;
		while (current) {
			assert(prev == current->prev);
			prev = current;
			current = current->next;
		}
		assert(last == prev);
	}
}