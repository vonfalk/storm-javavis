#include "stdafx.h"
#include "ObjMap.h"

namespace storm {

	RawObjMap::RawObjMap(Gc &gc) : gc(gc), root(null), data(null), capacity(0), filled(0) {}

	RawObjMap::~RawObjMap() {
		clear();
	}

	void RawObjMap::clear() {
		Gc::destroyRoot(root);
		root = null;
		delete []data;
		data = null;
		capacity = 0;
		filled = 0;
	}

	void RawObjMap::put(void *from, void *to) {
		if (capacity == filled)
			grow();

		Item i = { from, to };
		data[filled++] = i;
	}

	void RawObjMap::grow() {
		nat nCap = capacity * 2;
		if (capacity == 0)
			nCap = 8;

		Item *nData = new Item[nCap];
		memset(nData, 0, sizeof(Item)*nCap);
		Gc::Root *nRoot = gc.createRoot(nData, nCap*2);

		if (data) {
			for (nat i = 0; i < filled; i++)
				nData[i] = data[i];

			gc.destroyRoot(root);
			delete []data;
		}

		data = nData;
		capacity = nCap;
		root = nRoot;
	}

	struct RawObjMap::Predicate {
		bool operator()(const Item &a, const Item &b) const {
			return size_t(a.from) < size_t(b.from);
		}
		bool operator()(const Item &a, void *b) const {
			return size_t(a.from) < size_t(b);
		}
	};

	void RawObjMap::sort() {
		if (data)
			std::sort(data, data + filled, Predicate());
	}

	void *RawObjMap::find(void *obj) {
		Item *end = data + filled;
		Item *found = std::lower_bound(data, end, obj, Predicate());
		if (found == end)
			return null;

		if (found->from == obj)
			return found->to;
		else
			return null;
	}

}
