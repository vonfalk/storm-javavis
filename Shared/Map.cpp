#include "stdafx.h"
#include "Map.h"
#include <iomanip>

namespace storm {

	MapBase::MapBase(const Handle &key, const Handle &value) :
		keyHandle(key), valHandle(value),
		size(0), capacity(0),
		info(null), key(null), val(null) {

		TODO(L"Check if the key has hash() and equals() before continuing!");
		if (!key.hash)
			throw MapError(L"Hash function missing in the key type, can not continue.");

		if (!key.equals)
			throw MapError(L"Equals function missing in the key type, can not continue.");
	}

	MapBase::MapBase(Par<MapBase> o) :
		keyHandle(o->keyHandle), valHandle(o->valHandle),
		size(o->size), capacity(o->capacity), // Keeping the same capacity means we do not have to re-allocate.
		info(null), key(null), val(null) {

		alloc(capacity);

		for (nat i = 0; i < capacity; i++) {
			info[i] = o->info[i];
			if (info[i].status != Info::free) {
				(*keyHandle.create)(keyPtr(i), o->keyPtr(i));
				(*valHandle.create)(valPtr(i), o->valPtr(i));
			}
		}
	}

	MapBase::~MapBase() {
		clear();
	}

	void MapBase::deepCopy(Par<CloneEnv> env) {
		for (nat i = 0; i < capacity; i++) {
			if (info[i].status != Info::free) {
				(*keyHandle.deepCopy)(keyPtr(i), env.borrow());
				(*valHandle.deepCopy)(valPtr(i), env.borrow());
			}
		}
	}

	void MapBase::clear() {
		for (nat i = 0; i < capacity; i++) {
			if (info[i].status != Info::free) {
				// By doing this, we may loose chaining information if a destruction fails, but we
				// will not leak memory at least!
				// It _is_ possible to handle this gracefully as well, but it takes extra memory and
				// code complexity. It will perhaps be done in a later version of this implementation!
				info[i].status = Info::free;

				// We will leak if the key throws at the moment...
				(*keyHandle.destroy)(keyPtr(i));
				(*valHandle.destroy)(valPtr(i));
			}
		}

		// If we get here, we destroyed all elements.
		delete []info; info = null;
		delete []key; key = null;
		delete []val; val = null;
	}

	void MapBase::putRaw(const void *key, const void *value) {
		nat hash = (*keyHandle.hash)(key);

		nat old = findSlot(key, hash);
		if (old == Info::free) {
			insert(key, value, hash);
		} else {
			(*valHandle.destroy)(valPtr(old));
			(*valHandle.create)(valPtr(old), value);
		}
	}

	bool MapBase::hasRaw(const void *key) {
		nat hash = (*keyHandle.hash)(key);
		return findSlot(key, hash) != Info::free;
	}

	void *MapBase::getRaw(const void *key) {
		nat hash = (*keyHandle.hash)(key);
		nat slot = findSlot(key, hash);
		if (slot == Info::free) {
			TODO(L"Create one if possible..?");
			throw MapError(L"Key not in map.");
		}

		return valPtr(slot);
	}

	void *MapBase::getRaw(const void *key, const void *def) {
		nat hash = (*keyHandle.hash)(key);
		nat slot = findSlot(key, hash);

		if (slot == Info::free)
			slot = insert(key, def, hash);

		return valPtr(slot);
	}

	void MapBase::removeRaw(const void *key) {
		// Will break 'primarySlot' otherwise.
		if (capacity == 0)
			return;

		nat hash = (*keyHandle.hash)(key);
		nat slot = primarySlot(hash);

		if (info[slot].status == Info::free)
			return;

		nat prev = Info::free;
		do {
			if (info[slot].hash == hash && (*keyHandle.equals)(key, keyPtr(slot))) {
				// Is the node we're about to delete inside a chain?
				if (prev != Info::free) {
					// Unlink us from the chain.
					info[prev].status = info[slot].status;
				}

				nat next = info[slot].status;

				// Destroy the node.
				info[slot].status = Info::free;
				(*keyHandle.destroy)(keyPtr(slot));
				(*valHandle.destroy)(valPtr(slot));

				if (prev == Info::free && next != Info::end) {
					// The removed node was in the primary slot, and we need to move the next one
					// into our slot.
					(*keyHandle.create)(keyPtr(slot), keyPtr(next));
					(*valHandle.create)(valPtr(slot), valPtr(next));
					info[slot] = info[next];
					info[next].status = Info::free;
					(*keyHandle.destroy)(keyPtr(next));
					(*valHandle.destroy)(valPtr(next));
				}

				size--;
				return;
			}

			prev = slot;
			slot = info[slot].status;
		} while (slot != Info::end);
	}

	Str *MapBase::toS() {
		Auto<StrBuf> b = CREATE(StrBuf, this);
		b->add(L"{");

		bool first = true;
		for (nat i = 0; i < capacity; i++) {
			if (info[i].status == Info::free)
				continue;

			if (!first)
				b->add(L", ");
			first = false;

			keyHandle.output(keyPtr(i), b.borrow());
			b->add(L" -> ");
			valHandle.output(valPtr(i), b.borrow());
		}

		b->add(L"}");
		return b->toS();
	}

	void MapBase::dbg_print() {
		std::wcout << L"Map contents:" << endl;
		for (nat i = 0; i < capacity; i++) {
			std::wcout << std::setw(2) << i << L": ";
			if (info[i].status == Info::free) {
				std::wcout << L"free";
			} else if (info[i].status == Info::end) {
				std::wcout << toHex(info[i].hash) << L" end";
			} else {
				std::wcout << toHex(info[i].hash) << L" -> " << info[i].status;
			}

			if (info[i].status != Info::free) {
				std::wcout << "   ";
				Auto<StrBuf> b = CREATE(StrBuf, this);
				keyHandle.output(keyPtr(i), b.borrow());
				std::wcout << b;
			}
			std::wcout << endl;
		}
	}

	Nat MapBase::collisions() const {
		Nat c = 0;
		for (nat i = 0; i < capacity; i++) {
			if (info[i].status != Info::free && i == primarySlot(info[i].hash)) {
				for (nat at = i; info[i].status != Info::end; i++)
					c++;
			}
		}
		return c;
	}

	Nat MapBase::maxChain() const {
		Nat c = 0;
		for (nat i = 0; i < capacity; i++) {
			if (info[i].status != Info::free && i == primarySlot(info[i].hash)) {
				// Start of a chain, traverse from here!
				Nat len = 1;
				for (nat at = i; info[i].status != Info::end; i++)
					len++;
				c = max(c, len);
			}
		}
		return c;
	}

	void MapBase::alloc(nat cap) {
		assert(info == null);
		assert(key == null);
		assert(val == null);
		capacity = cap;
		size = 0;

		info = new Info[capacity];
		key = new byte[capacity * keyHandle.size];
		val = new byte[capacity * valHandle.size];

		for (nat i = 0; i < capacity; i++)
			info[i].status = Info::free;
	}

	void MapBase::grow() {
		if (capacity == 0) {
			// Initial table size.
			alloc(minCapacity);
		} else {
			// Keep to a multiple of 2.
			rehash(capacity * 2);
		}

		lastFree = 0;
	}

	void MapBase::shrink() {
		if (capacity == 0)
			return;

		nat to = minCapacity;
		while (size > to)
			to *= 2;

		rehash(to);
	}

	void MapBase::rehash(nat cap) {
		nat oldCapacity = capacity;
		nat oldSize = size;

		Info *oldInfo = info; info = null;
		byte *oldKey = key; key = null;
		byte *oldVal = val; val = null;

		alloc(cap);

		try {

			// Insert all elements once again.
			for (nat i = 0; i < oldCapacity; i++) {
				if (oldInfo[i].status != Info::free) {
					byte *key = oldKey + i*keyHandle.size;
					byte *val = oldVal + i*valHandle.size;
					insert(key, val, oldInfo[i].hash);
					(*keyHandle.destroy)(key);
					(*valHandle.destroy)(val);
				}
			}

			delete []oldInfo;
			delete []oldKey;
			delete []oldVal;
		} catch (...) {
			clear();

			// Restore old state.
			swap(capacity, oldCapacity);
			swap(size, oldSize);
			swap(info, oldInfo);
			swap(key, oldKey);
			swap(val, oldVal);
			throw;
		}
	}

	nat MapBase::insert(const void *key, const void *val, nat hash) {
		if (size == capacity)
			grow();

		Info insert = { Info::end, hash };
		nat into = primarySlot(hash);

		if (info[into].status != Info::free) {
			// Check if the contained element is in its primary position.
			nat from = primarySlot(info[into].hash);
			if (from == into) {
				// It is in its primary position. Attach ourselves to the chain.
				// TODO: We probably want to check each element in the chain for equality first!
				nat to = freeSlot();
				insert.status = info[into].status;
				info[into].status = to;
				into = to;
			} else {
				// It is not. Move it somewhere else.

				// Walk the list from the original position and find the node before the one we're
				// to move...
				while (info[from].status != into)
					from = info[from].status;

				// Redo linking.
				nat to = freeSlot();
				info[from].status = to;

				// Move the node itself. TODO: Exception safety?
				info[to] = info[into];
				(*keyHandle.create)(keyPtr(to), keyPtr(into));
				(*valHandle.create)(valPtr(to), valPtr(into));
				(*keyHandle.destroy)(keyPtr(into));
				(*valHandle.destroy)(valPtr(into));
				info[into].status = Info::free;
			}
		}

		assert(info[into].status == Info::free, L"Internal error, trying to overwrite a slot!");
		info[into] = insert;
		(*keyHandle.create)(keyPtr(into), key);
		(*valHandle.create)(valPtr(into), val);
		size++;

		return into;
	}

	nat MapBase::findSlot(const void *key, nat hash) {
		// Otherwise, primarySlot won't work.
		if (capacity == 0)
			return Info::free;

		nat slot = primarySlot(hash);

		if (info[slot].status == Info::free)
			return Info::free;

		do {
			if (info[slot].hash == hash && (*keyHandle.equals)(key, keyPtr(slot)))
					return slot;

			slot = info[slot].status;
		} while (slot != Info::end);

		return Info::free;
	}

	nat MapBase::primarySlot(nat hash) const {
		// TODO: Change to bitwise and by making capacity store log2 of capacity, and ensuring
		// capacity is always a power of two.
		return hash % capacity;
	}

	nat MapBase::freeSlot() {
		while (info[lastFree].status != Info::free)
			// TODO: if 'capacity' is a power of two, we can do this with a bitwise and.
			if (++lastFree >= capacity)
				lastFree = 0;
		return lastFree++;
	}

}
