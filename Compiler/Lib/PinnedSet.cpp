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

	void PinnedSet::deepCopy(CloneEnv *) {}

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

	void PinnedSet::toS(StrBuf *to) const {
		sort();

		*to << S("{");
		bool first = true;
		for (size_t i = 0; i < data->count; i++) {
			if (!data->v[i])
				continue;

			if (!first)
				*to << S(", ");
			first = false;

			*to << S("0x") << hex(data->v[i]);
		}
		*to << S("}");
	}

	struct PtrCompare {
		bool operator() (const void *a, const void *b) const {
			return size_t(a) < size_t(b);
		}
	};

	void PinnedSet::put(void *ptr) {
		if (!ptr)
			return;

		if (!data)
			reserve(16);

		// If we're full, try sorting the array first. There might be duplicates we can remove.
		if (data->filled >= data->count && !data->sorted)
			sort();

		// Final fallback, resize.
		if (data->filled >= data->count)
			reserve(data->count * 2);

		// If the array is sorted, we can check if the address is already present.
		if (data->sorted) {
			void **begin = data->v;
			void **end = data->v + data->count;
			void **found = std::lower_bound(begin, end, ptr, PtrCompare());

			// Found it, we can return now!
			if (found != end && *found == ptr)
				return;
		}


		// Note: We place non-null pointers at the end. Since 0 is the lowest number we have in the
		// array, it is natural that it appears in the beginning of the array as it is sorted.
		data->filled++;
		data->v[data->count - data->filled] = ptr;
		data->sorted = false;
	}

	static size_t objSize(const void *object) {
		const GcType *type = Gc::typeOf(object);
		size_t size = type->stride;
		if (type->kind == GcType::tArray) {
			size *= ((const GcArray<Byte> *)object)->count;
			size += OFFSET_OF(GcArray<Byte>, v);
		} else if (type->kind == GcType::tWeakArray) {
			size *= ((const GcWeakArray<void *> *)object)->count();
			size += OFFSET_OF(GcArray<Byte>, v);
		}
		return size;
	}

	Bool PinnedSet::has(const void *query) {
		if (!query)
			return false;

		sort();

		void **begin = data->v;
		void **end = data->v + data->count;

		size_t size = objSize(query);
		void **found = std::lower_bound(begin, end, query, PtrCompare());

		if (found == end)
			return false;

		// Out of range?
		if (size_t(*found) >= size_t(query) + size)
			return false;

		return true;
	}

	Array<Nat> *PinnedSet::offsets(const void *query) {
		if (!query)
			return new (this) Array<Nat>();

		sort();

		void **begin = data->v;
		void **end = data->v + data->count;

		size_t size = objSize(query);
		Array<Nat> *result = new (this) Array<Nat>();

		// Loop until we're out of range and add all offsets.
		for (void **found = std::lower_bound(begin, end, query, PtrCompare());
			 found != end && size_t(*found) < size_t(query) + size;
			 found++) {

			size_t offset = size_t(*found) - size_t(query);
			*result << offset;
		}

		return result;
	}

	void PinnedSet::reserve(size_t n) {
		Data *newData = (Data *)malloc(dataSize(n));
		Gc::Root *newRoot = engine().gc.createRoot(newData, dataSize(n) / sizeof(void *), true);

		memset(newData, 0, dataSize(n));
		newData->count = n;

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

	void PinnedSet::sort() const {
		if (data->sorted)
			return;

		void **begin = data->v;
		void **end = data->v + data->count;

		std::sort(begin, end, PtrCompare());

		// Move duplicates to the beginning of the array, then we can zero them. We stop when we see a null.
		void **to = end - 1;
		for (void **at = end - 1; at != begin && *(at - 1) != null; at--) {
			if (*at != *(at - 1)) {
				*--to = *(at - 1);
			}
		}

		// Remember new (possibly smaller) size.
		data->filled = end - to;

		// Zero remaining
		while (to != begin)
			*--to = null;

		data->sorted = true;
	}

	size_t PinnedSet::dataSize(size_t n) {
		return sizeof(Data) + sizeof(void *) * (max(n, size_t(1)) - 1);
	}

	void addPinned(Type *rawPtr) {
		Engine &e = rawPtr->engine;
		Type *to = StormInfo<PinnedSet>::type(e);

		Array<Value> *raw = new (e) Array<Value>(2, Value(to));
		raw->at(1) = Value(rawPtr);
		to->add(nativeFunction(e, Value(StormInfo<Bool>::type(e)), S("has"), raw, address(&PinnedSet::has)));
		to->add(nativeFunction(e, Value(StormInfo<Array<Nat>>::type(e)), S("offsets"), raw, address(&PinnedSet::offsets)));

		Type *unknown = as<Type>(e.nameSet(parseSimpleName(e, S("core.lang.unknown.PTR_NOGC"))));
		if (!unknown)
			throw new (e) InternalError(S("Unable to find the type PTR_NOGC"));

		Array<Value> *un = new (e) Array<Value>(2, Value(to));
		un->at(1) = Value(unknown);
		to->add(nativeFunction(e, Value(), S("put"), un, address(&PinnedSet::put)));

		// This version is more restricted, but easier to use from Storm.
		to->add(nativeFunction(e, Value(), S("put"), raw, address(&PinnedSet::put)));
	}

}
