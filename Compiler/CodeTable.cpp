#include "stdafx.h"
#include "CodeTable.h"
#include "Engine.h"
#include <algorithm>

namespace storm {

	// Comparision used when sorting pointers.
	struct CodeCompare {
		inline bool operator ()(const void *pa, const void *pb) const {
			size_t a = size_t(pa);
			size_t b = size_t(pb);

			// In order to sort '0' last, subtract one from each.
			a--; b--;
			return a < b;
		}
	};

	// Does the code segment 'code' contain 'ptr'?
	static bool contains(const void *code, void *ptr) {
		size_t start = size_t(code);
		size_t end = size_t(start + Gc::codeSize(code));
		size_t test = size_t(ptr);

		return start <= test && test < end;
	}


	CodeTable::CodeTable(Engine &e) : e(e), root(null), count(0), sorted(false) {
		memset(&data, 0, sizeof(Data));
	}

	CodeTable::~CodeTable() {
		clear();
	}

	void CodeTable::clear() {
		data.table = null;
		data.watch = null;

		if (root)
			Gc::destroyRoot(root);
		root = null;
	}

	void CodeTable::add(void *code) {
		// Make sure that 'ptr' is somewhere inside the Gc arena.
		if (!e.gc.isCodeAlloc(code)) {
			assert(false, L"Code not managed by the GC!");
			return;
		}

		util::Lock::L z(lock);

		ensure(count + 1);

		// Insert the new allocation. Don't care if it is a duplicate yet. We'll remove duplicates
		// in the next sort operation.
		data.table->v[count++] = code;
		sorted = false;
	}

	void *CodeTable::find(void *ptr) {
		// Check if 'ptr' was likely allocated as code in the GC.
		if (!e.gc.isCodeAlloc(ptr))
			return null;

		util::Lock::L z(lock);

		if (!data.table)
			return null;

		void **begin = data.table->v;
		void **end = data.table->v + count;

		if (sorted) {
			// The array could be sorted! Try to find the code using a binary search.
			void **found = std::lower_bound(begin, end, ptr, CodeCompare());
			if (found != end && contains(*found, ptr))
				return *found;
			if (found != begin && contains(found[-1], ptr))
				return found[-1];

			// If the watch object indicates that nothing has changed, then we can be sure that we
			// do not contain 'ptr'.
			if (!data.watch->moved())
				return null;
		}

		// Sort the table while we're looking for 'ptr'. We are using heapsort, since it lets us
		// remove duplicates and search the elements easily while sorting. Furthermore, it will not
		// hang or remove elements if pointers are updated during sorting.
		std::make_heap(begin, end, CodeCompare());

		void *prev = null;
		void *found = null;

		while (begin != end) {
			std::pop_heap(begin, end--, CodeCompare());
			void *&now = *end;

			if (!now)
				continue;

			if (contains(now, ptr))
				found = now;

			// Duplicate?
			if (now == prev)
				now = null;
		}

		// Update 'count' and compact the data.
		compact(data.table);
		sorted = true;

		// TODO: We should probably shrink the array from time to time as well. Considering that
		// code is not deallocated very often, we're probably fine.

		return found;
	}

	void CodeTable::ensure(Nat count) {
		Nat old = 0;
		if (data.table)
			old = data.table->count();

		if (count <= old)
			return;

		if (!root)
			root = e.gc.createRoot(&data, sizeof(Data)/sizeof(void *));
		if (!data.watch)
			data.watch = e.gc.createWatch();

		Nat newCount = max(old * 2, count);
		newCount = max(Nat(16), newCount);

		GcWeakArray<void> *oldTable = data.table;
		data.table = runtime::allocWeakArray<void>(e, newCount);
		data.watch->clear();

		if (oldTable) {
			// Count any elements that have become null since we last checked while we're copying
			// data anyway.
			compact(oldTable);
		}
	}

	void CodeTable::compact(GcWeakArray<void> *src) {
		data.watch->clear();
		count = 0;
		for (Nat i = 0; i < src->count(); i++) {
			void *elem = src->v[i];
			if (elem) {
				data.table->v[count++] = elem;
				data.watch->add(elem);
			}
		}
	}

}
