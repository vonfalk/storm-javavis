#include "stdafx.h"
#include "Map.h"

namespace storm {

	MapBase::MapBase(const Handle &key, const Handle &value) :
		keyHandle(key), valHandle(value),
		size(0), capacity(0),
		info(null), key(null), val(null) {

		TODO(L"Check if the key has hash() and equals() before continuing!");
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
			alloc(4);
		} else {
			nat oldCapacity = capacity;
			nat oldSize = size;
			Info *oldInfo = info; info = null;
			byte *oldKey = key; key = null;
			byte *oldVal = val; val = null;

			// Keep to a multiple of 2.
			alloc(capacity * 2);

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

		lastFree = 0;
	}

	void MapBase::insert(const void *key, const void *val, nat hash) {
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
				into = freeSlot();
				insert.status = info[into].status;
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

		assert(info[into] == Info::free, L"Internal error, trying to overwrite a slot!");
		info[into] = insert;
		(*keyHandle.create)(keyPtr(into), key);
		(*valHandle.create)(valPtr(into), val);
		size++;
	}

	nat MapBase::primarySlot(nat hash) {
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
