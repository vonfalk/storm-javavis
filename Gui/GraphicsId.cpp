#include "stdafx.h"
#include "GraphicsId.h"

namespace gui {

	const Nat full = 0xFFFFFFFF;
	const Nat natBits = CHAR_BIT * sizeof(Nat);

	IdMgr::IdMgr() : data(null), count(0) {}

	IdMgr::~IdMgr() {
		delete []data;
	}

	Nat IdMgr::allocate() {
		if (!data) {
			// Initial allocation.
			count = 4;
			data = new Nat[count];
			memset(data, 0, sizeof(Nat) * count);
		}

		for (Nat i = 0; i < count; i++) {
			if (data[i] != full)
				return allocate(data[i]) + i*natBits + 1;
		}

		Nat oldCount = count;
		grow();
		return allocate(data[oldCount]) + oldCount*natBits + 1;
	}

	void IdMgr::free(Nat id) {
		if (id == 0)
			return;

		id--;

		Nat mask = Nat(1) << (id % natBits);
		data[id / natBits] &= ~mask;
	}

	void IdMgr::alloc(Nat &in) {
		// Note: This could probably be made more efficient, but it is not really worth it. This is not hot code.
		for (Nat i = 0; i < natBits; i++) {
			Nat mask = Nat(1) << i;
			if ((in & mask) == 0) {
				in |= mask;
				return i;
			}
		}
	}

	void IdMgr::grow() {
		// This rate should be more than enough.
		Nat newCount = count + 4;
		Nat *newData = new Nat[newCount];

		// This is a bit wasteful, but does not matter, as we're dealing with very small amounts of data.
		memset(newData, 0, sizeof(Nat) * newCount);
		memcpy(newData, data, sizeof(Nat) * count);

		delete[] data;
		count = newCount;
		data = newData;
	}

}
