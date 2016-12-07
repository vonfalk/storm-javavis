#pragma once
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Array which can store POD types without pointers. This array starts pre-allocated with a
	 * number of bytes so that no allocations have to be made.
	 */
	template <class T, nat alloc>
	class PODArray {
	public:
		PODArray(Engine &e) : e(e) {
			data = pre;
			data->filled = 0;
		}

		// Add an element.
		void push(const T &e) {
			if (data->filled == data->count)
				grow();
			data->v[data->filled++] = e;
		}

		// Access elements.
		T &operator[] (nat id) {
			return data->v[id];
		}

		// Count.
		nat count() const {
			return data->filled;
		}

		// Clear (does not clear storage)
		void clear() {
			data->filled = 0;
		}

	private:
		PODArray(const PODArray &o);
		PODArray &operator =(const PODArray &o);

		// Engine.
		Engine &e;

		// Type when allocating gc types.
		static const GcType gcType;

		// Pre-allocated array.
		GcPreArray<T, alloc> pre;

		// Pointer to the current data.
		GcArray<T> *data;

		// Grow storage.
		void grow() {
			nat newCount = data->count * 2;
			GcArray<T> *d = runtime::allocArray<T>(e, &gcType, newCount);
			d->filled = data->filled;
			memcpy(d->v, data->v, sizeof(T) * data->filled);
			data = d;
		}
	};

	template <class T, nat alloc>
	const GcType PODArray<T, alloc>::gcType = {
		GcType::tArray,
		null,
		null,
		sizeof(T),
		0, {}
	};

}
