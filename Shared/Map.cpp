#include "stdafx.h"
#include "Map.h"

namespace storm {

	MapBase::MapBase(const Handle &key, const Handle &value) :
		keyHandle(key), valueHandle(value),
		size(0), capacity(0),
		info(null), key(null), value(null) {}

	MapBase::MapBase(Par<MapBase> o) :
		keyHandle(o->keyHandle), valueHandle(o->valueHandle),
		size(0), capacity(0),
		info(null), key(null), value(null) {
		assert(false, L"Not implemented yet!");
	}

	MapBase::~MapBase() {
		clear();
	}

	void MapBase::deepCopy(Par<CloneEnv> env) {
		assert(false, L"Not implemented yet!");
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
				(*valueHandle.destroy)(valuePtr(i));
			}
		}

		// If we get here, we destroyed all elements.
		delete []info;  info = null;
		delete []key;   key = null;
		delete []value; value = null;
	}

	void MapBase::grow() {
		if (capacity == 0) {
			// Create initial data.
			capacity = 4;
			info = new Info[capacity];
			key = new byte[capacity * keyHandle.size];
			value = new byte[capacity * valueHandle.size];
		} else {
			assert(false, L"Re-hashing is not implemented yet!");
		}
	}

}
