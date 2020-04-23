#include "stdafx.h"
#include "PinnedSet.h"
#include "Compiler/Engine.h"

namespace storm {


	PinnedSet::PinnedSet() : data(null), root(null) {}

	PinnedSet::PinnedSet(const PinnedSet &o) : data(null), root(null) {
		if (o.data) {
			reserve(o.data->count);

			memcpy(data, o.data, dataSize(o.data->count));
		}
	}

	PinnedSet::~PinnedSet() {
		clear();
	}

	void PinnedSet::clear() {
		if (root) {
			engine().gc.destroyRoot(root);
			root = null;
		}

		if (data) {
			free(data);
			data = null;
		}
	}

	void PinnedSet::reserve(size_t n) {
		Data *newData = (Data *)malloc(dataSize(n));
		Gc::Root *newRoot = engine().gc.createRoot(newData, dataSize(n) / sizeof(void *), true);

		newData->count = n;
		memset(newData, 0, dataSize(n));

		if (data) {
			newData->filled = data->filled;
			for (size_t i = 0; i < min(data->count, n); i++) {
				newData->v[i] = data->v[i];
			}
		}

		std::swap(newData, data);
		std::swap(newRoot, root);

		if (newRoot)
			engine().gc.destroyRoot(newRoot);
		if (newData)
			free(newData);
	}

	void PinnedSet::add(void *ptr) {
		if (!ptr)
			return;

		if (!data)
			reserve(16);
		if (data->filled >= data->count)
			reserve(data->count * 2);

		// Note: We place non-null pointers at the end. Since 0 is the lowest number we have in the
		// array, it is natural that it appears in the beginning of the array as it is sorted.
		data->filled++;
		data->v[data->count - data->filled] = ptr;
		data->sorted = false;
	}

	struct PtrCompare {
		bool operator() (void *a, void *b) const {
			return size_t(a) < size_t(b);
		}
	};

	Bool PinnedSet::has(void *query) {
		if (!data->sorted) {
			std::sort(data->v, data->v + data->count, PtrCompare());
		}

		// TODO!

		return false;
	}

	size_t PinnedSet::dataSize(size_t n) {
		return sizeof(Data) + sizeof(void *) * (max(n, size_t(1)) - 1);
	}
}
