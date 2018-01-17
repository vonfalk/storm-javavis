#include "stdafx.h"
#include "CodeTable.h"
#include <algorithm>

namespace code {

	/**
	 * The allocator.
	 */

	CodeTable::Allocator::Allocator() : first(null) {}

	CodeTable::Allocator::~Allocator() {
		for (size_t i = 0; i < chunks.size(); i++)
			delete []chunks[i];
	}

	CodeTable::Elem *CodeTable::Allocator::alloc() {
		if (!first)
			allocChunk();

		Elem *a = first;
		first = (Elem *)first->code;
		return a;
	}

	void CodeTable::Allocator::free(Elem *e) {
		memset(e, 0, sizeof(Elem));
		e->code = first;
		first = e;
	}

	void CodeTable::Allocator::alive(vector<Elem *> &out) {
		out.clear();

		for (size_t chunk = 0; chunk < chunks.size(); chunk++) {
			Elem *c = chunks[chunk];
			for (size_t i = 0; i < chunkSize; i++) {
				if (c[i].owner)
					out.push_back(&c[i]);
			}
		}
	}

	void CodeTable::Allocator::allocChunk() {
		Elem *chunk = new Elem[chunkSize];
		memset(chunk, 0, sizeof(Elem)*chunkSize);

		// Build the free-list.
		for (size_t i = 1; i < chunkSize; i++)
			chunk[i - 1].code = &chunk[i];
		chunk[chunkSize - 1].code = first;

		first = chunk;
		chunks.push_back(chunk);
	}


	/**
	 * The actual table.
	 */

	CodeTable::CodeTable() : sorted(0) {}

	CodeTable::~CodeTable() {}

	CodeTable::Handle CodeTable::add(void *code) {
		util::Lock::L z(lock);

		Elem *e = mem.alloc();
		e->code = code;
		e->owner = this;

		table.push_back(e);
		atomicWrite(sorted, 0);

		return e;
	}

	void CodeTable::update(Handle handle, void *code) {
		Elem *e = (Elem *)handle;
		e->code = code;
		atomicWrite(e->owner->sorted, 0);
	}

	void CodeTable::remove(Handle handle) {
		util::Lock::L z(lock);

		Elem *e = (Elem *)handle;
		mem.free(e);
		table.clear();
		atomicWrite(sorted, 0);
	}

	void *CodeTable::find(const void *addr) {
		util::Lock::L z(lock);
		Elem find = { (void *)addr, this };

		Iter begin = table.begin();
		Iter end = table.end();

		if (atomicRead(sorted)) {
			// Try to find using binary search.
			Iter found = std::lower_bound(begin, end, &find, CodeCompare());
			if (found != end && contains(*found, addr))
				return (*found)->code;
			if (found != begin && contains(*--found, addr))
				return (*found)->code;

			// If we did not find it. See if the array is still sorted.
			if (atomicRead(sorted))
				return null;
		}

		// The array is not sorted. Sort it while finding 'addr'.
		mem.alive(table);
		begin = table.begin();
		end = table.end();
		atomicWrite(sorted, 1);

		std::make_heap(begin, end, CodeCompare());

		void *result = null;
		while (begin != end) {
			std::pop_heap(begin, end--, CodeCompare());

			if (contains(*end, addr))
				result = (*end)->code;
		}

		return result;
	}

	bool CodeTable::contains(Elem *elem, const void *ptr) {
		size_t start = size_t(elem->code);
		size_t end = start + runtime::codeSize(elem->code);
		size_t test = size_t(ptr);

		return start <= test && test < end;
	}

}
