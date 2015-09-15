#pragma once
#include "Object.h"
#include "Handle.h"
#include "Types.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Map for use in Storm and in C++.
	 *
	 * Note: actors are currently supported by disallowing custom hash functions to be associated
	 * with actors and always using their address as the value for hashing.
	 *
	 * The implementation is inspired from the hashmap implementation found in Lua. All keys and
	 * values are stored as a flat array. Chaining is done by maintaining pointers in each of the
	 * slots of the array. An entry is said to be in its primary position if it is in the location
	 * computed by the hash function. This implementation maintains the invariant that for each hash
	 * value, the element contained is either the element in the primary position, or that hash does
	 * not exist in the hash map.
	 */

	// This function is implemented in MapTemplate.cpp.
	// Look up a specific array type (create it if it is not already created).
	Type *mapType(Engine &e, const ValueData &key, const ValueData &value);


	/**
	 * The base class that is used in Storm, use the derived class in C++.
	 */
	class MapBase : public Object {
		STORM_CLASS;
	public:
		// Empty map.
		MapBase(const Handle &key, const Handle &value);

		// Copy another map.
		MapBase(Par<MapBase> other);

		// Dtor.
		~MapBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// # of contained elements.
		inline Nat STORM_FN count() const { return size; }

		// Any elements.
		Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		Bool STORM_FN empty() const { return count() == 0; }

		// Clear.
		void STORM_FN clear();

		// Key and value handles.
		const Handle &keyHandle;
		const Handle &valHandle;

	private:
		// # of contained elements.
		nat size;

		// Capacity.
		nat capacity;

		// Information about a slot.
		struct Info {
			// Used? Part of a chain?
			// - if unused: 'free'
			// - if used and at the end of a chain: 'end'
			// - if used and inside of a chain: <index of the next element>
			nat status;

			// Status codes (at the end of the nat interval to not interfere).
			static const nat free = -1;
			static const nat end = -2;

			// Cached hash value.
			nat hash;
		};

		// Allocated memory. This is split into three regions: info, key, value. Each of these are
		// 'capacity' elements large.
		Info *info;
		byte *key;
		byte *val;

		// Get the locations for keys or values.
		inline void *keyPtr(nat id) { return key + (id * keyHandle.size); }
		inline void *valPtr(nat id) { return val + (id * valHandle.size); }

		// Allocate data for a specific size. Assumes 'info', 'key' and 'val' are null.
		void alloc(nat capacity);

		// Grow to fit at least one more element.
		void grow();

		// Insert a node, given its hash is known (eg. when re-hashing).
		void insert(const void *key, const void *val, nat hash);

		// Compute the primary slot of data, given its hash.
		nat primarySlot(nat hash);

		// Find a free slot. Always succeeds as long as size != capacity.
		nat freeSlot();

		// Last seen free slot in this table. Used by 'freeSlot'.
		nat lastFree;

	};


	/**
	 * C++ version.
	 */
	template <class K, class V>
	class Map : public ArrayBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return mapType(e, value<T>(e)); }

		// Empty map.
		Map() : ArrayBase(storm::handle<K>(), storm::handle<V>()) { setVTable(this); }

		// Copy map.
		Map(Par<Map<K, V>> o) : ArrayBase(o) { setVTable(this); }

	};

}
