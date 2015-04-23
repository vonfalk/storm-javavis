#include "stdafx.h"
#include "StormVTable.h"
#include "Function.h"
#include "VTable.h"
#include "Code/VTable.h"
#include <iomanip>

namespace storm {

	StormVTable::StormVTable(code::VTable *&update)
		: update(update), size(0), capacity(0), addrs(null), src(null) {}

	StormVTable::~StormVTable() {
		clear();
	}

	void StormVTable::ensure(nat to) {
		if (size >= to)
			return;

		if (capacity >= to) {
			size = to;
			return;
		}

		// Grow in somewhat reasonable steps!
		capacity = max(to, capacity + 10);

		// Copy 'addrs'
		void **n = new void *[capacity];
		zeroArray(n, capacity);
		if (addrs)
			for (nat i = 0; i < size; i++)
				n[i] = addrs[i];
		delete[] addrs;
		addrs = n;

		// Copy 'src'
		VTableSlot **z = new VTableSlot*[capacity];
		zeroArray(z, capacity);
		if (addrs)
			for (nat i = 0; i < size; i++)
				z[i] = src[i];
		delete[] src;
		src = z;

		size = to;
		if (update)
			update->extra(addrs);
	}

	void StormVTable::clear() {
		for (nat i = 0; i < size; i++)
			delete src[i];
		delete[] src;
		src = null;

		delete[] addrs;
		addrs = null;

		size = 0;
		capacity = 0;
		if (update)
			update->extra(addrs);
	}

	void StormVTable::clearRefs() {
		for (nat i = 0; i < size; i++)
			del(src[i]);
	}

	void StormVTable::slot(nat i, VTableSlot *update) {
		assert(i < size);
		delete src[i];
		src[i] = update;
		update->id.offset = i;
	}

	VTableSlot *StormVTable::slot(nat i) {
		assert(i < size);
		return src[i];
	}

	void StormVTable::addr(nat i, void *to) {
		assert(i < size);
		addrs[i] = to;
	}

	void *StormVTable::addr(nat i) {
		assert(i < size);
		return addrs[i];
	}


	nat StormVTable::emptySlot(nat from) const {
		for (nat i = from; i < size; i++)
			if (src[i] == null)
				return i;

		return count();
	}

	nat StormVTable::findAddr(void *v) const {
		for (nat i = 0; i < size; i++)
			if (addrs[i] == v)
				return i;

		return count();
	}

	nat StormVTable::findSlot(VTableSlot *v) const {
		for (nat i = 0; i < size; i++)
			if (src[i] == v)
				return i;

		return count();
	}

	nat StormVTable::findSlot(Function *v) const {
		for (nat i = 0; i < size; i++)
			if (src[i] != null && src[i]->fn == v)
				return i;

		return count();
	}

	void StormVTable::expand(nat at, nat count) {
		ensure(size + count);

		for (nat i = size; i > at + count; i--) {
			nat from = i - count - 1;
			nat to = i - 1;

			addrs[to] = addrs[from];
			src[to] = src[from];
			if (src[to])
				src[to]->id.offset = to;

			addrs[from] = null;
			src[from] = null;
		}
	}


	void StormVTable::contract(nat from, nat to) {
		for (nat i = from; i < to; i++)
			del(src[to]);

		nat move = to - from;
		for (nat i = to; i < size; i++) {
			addrs[i - move] = addrs[i];
			src[i - move] = src[i];
			if (src[i - move])
				src[i - move]->id.offset = i - move;

			addrs[i] = null;
			src[i] = null;
		}

		size -= move;
	}

	void StormVTable::dbg_dump() {
		wostream &to = std::wcout;

		for (nat i = 0; i < size; i++) {
			to << std::setw(3) << i << L": " << src[i] << endl;
		}
	}

}
