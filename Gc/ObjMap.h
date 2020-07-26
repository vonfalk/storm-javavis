#pragma once
#include "Gc.h"

namespace storm {

	/**
	 * Mapping between objects. Can be used to ask the GC to replace all references to one object
	 * with another.
	 */
	class RawObjMap {
	public:
		// Create.
		RawObjMap(Gc &gc);

		// Destroy.
		~RawObjMap();

		// Clear.
		void clear();

		// Count.
		nat count() const { return filled; }

		// Add an item.
		void put(void *from, void *to);

		// Items.
		struct Item {
			void *from;
			void *to;
		};

		// Get an item.
		Item operator[](nat pos) const { return data[pos]; }

		// Sort the underlying data for future lookups. Note: this is mainly intended for internal
		// use by the GC, as the ordering will otherwise be invalidated by the objects being moved.
		void sort();

		// Find items in a sorted map. Returns null if not found. Note: as with 'sort', this is
		// intended for internal use by the GC.
		void *find(void *ptr);

	private:
		RawObjMap(const RawObjMap &);
		RawObjMap &operator =(const RawObjMap &);

		// Gc used.
		Gc &gc;

		// Current root.
		Gc::Root *root;

		// Actual data.
		Item *data;

		// Array size.
		nat capacity;

		// Number of elements.
		nat filled;

		// Grow.
		void grow();

		struct Predicate;
	};

	/**
	 * Typed version of RawObjMap.
	 */
	template <class T>
	class ObjMap : public RawObjMap {
	public:
		ObjMap(Gc &gc) : RawObjMap(gc) {}

		struct Item {
			T *from;
			T *to;
		};

		Item operator[](nat pos) const {
			RawObjMap::Item r = RawObjMap::operator[](pos);
			Item i = { (T *)r.from, (T *)r.to };
			return i;
		}

		void put(T *from, T *to) {
			RawObjMap::put(from, to);
		}
	};

}
