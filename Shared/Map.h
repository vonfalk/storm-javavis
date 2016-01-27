#pragma once
#include "Object.h"
#include "Handle.h"
#include "Types.h"
#include "Value.h"
#include "Utils/Exception.h"

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
	 * Exception thrown from the map.
	 */
	class MapError : public Exception {
	public:
		MapError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Map error: " + msg; }
	private:
		String msg;
	};


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

		// Shrink to fit the contained entries as tightly as possible (while maintaining our
		// power-of-two constraint).
		void STORM_FN shrink();

		// Count the number of collisions currently in the map. This is only intended as a way to
		// benchmark hash-functions, computed in linear time. Elements which end up in the same
		// bucket are counted as collisions, but the first element in each bucket is excluded from
		// the count.
		Nat STORM_FN collisions() const;

		// Find the single longest chain. Provided to benchmark, computed in linear time.
		Nat STORM_FN maxChain() const;

		// Key and value handles.
		const Handle &keyHandle;
		const Handle &valHandle;

		/**
		 * Low-level operations.
		 */

		// Put a value.
		void putRaw(const void *key, const void *value);

		// Has a value?
		bool hasRaw(const void *key);

		// Get a value. Throws if the value does not exist.
		void *getRaw(const void *key);

		// Get a value. Create it if it does not exist.
		void *getRaw(const void *key, const void *def);

		// Remove a value. Removing a non-existing key is a no-op.
		void removeRaw(const void *key);

		// Insert a value, creating it using the ctor-in 'fn'.
		typedef void (*CreateCtor)(void *to);
		void *CODECALL accessRaw(const void *key, CreateCtor fn);

		// String representation.
		Str *STORM_FN toS();

		/**
		 * Debug.
		 */

		// Print the current low-level layout.
		void dbg_print();

		/**
		 * Iterator. Not exposed to strom, but used internally to make the implementation easier.
		 * TODO: How does this fit with deepCopy?
		 */
		class Iter {
		public:
			// Pointing to the end.
			Iter();

			// Pointing to the first element.
			Iter(Par<MapBase> owner);

			// Compare
			bool operator ==(const Iter &o) const;
			bool operator !=(const Iter &o) const;

			// Increase.
			Iter &operator ++();
			Iter operator ++(int);

			// Raw get function.
			void *getRawKey() const;
			void *getRawVal() const;

			// Raw pre- and post increment.
			Iter &CODECALL preIncRaw();
			Iter CODECALL postIncRaw();

		private:
			// Map we're pointing to.
			Auto<MapBase> owner;

			// Index in the raw table we're pointing to.
			nat index;

			// At end?
			bool atEnd() const;
		};

		// Raw begin and end.
		Iter CODECALL beginRaw();
		Iter CODECALL endRaw();

	private:
		// Befriend the iterator.
		friend class Iter;

		// # of contained elements.
		nat size;

		// Minimum capacity.
		static const nat minCapacity = 4;

		// Capacity, has to be a power of two.
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

		// Do a re-hash to a specific size.
		void rehash(nat size);

		// Insert a node, given its hash is known (eg. when re-hashing). Assumes that no other node
		// with the same key exists in the map, and will therefore always insert the element.
		// Returns the slot inserted into.
		nat insert(const void *key, const void *val, nat hash);

		// Compute the insertion point of a new element. Does everything except copying the value
		// into the array. Do not skip copying the value, as the map will be left in an inconsistent
		// state.
		nat insert(const void *key, nat hash);

		// Find the current location of 'key', hiven 'hash'. Returns 'Info::free' if none exists.
		nat findSlot(const void *key, nat hash);

		// Compute the primary slot of data, given its hash.
		nat primarySlot(nat hash) const;

		// Find a free slot. Always succeeds as long as size != capacity.
		nat freeSlot();

		// Last seen free slot in this table. Used by 'freeSlot'.
		nat lastFree;

	};


	/**
	 * C++ version.
	 */
	template <class K, class V>
	class Map : public MapBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return mapType(e, value<K>(e), value<V>(e)); }

		// Empty map.
		Map() : MapBase(storm::handle<K>(), storm::handle<V>()) { setVTable(this); }

		// Copy map.
		Map(Par<Map<K, V>> o) : MapBase(o) { setVTable(this); }

		// Insert a value into the map, or update the existing.
		void put(const K &k, const V &v) {
			putRaw(&k, &v);
		}

		// Contains a key?
		bool has(const K &k) {
			return hasRaw(&k);
		}

		// Get a value from the map. Throws if the key is not found.
		V &get(const K &k) {
			V *r = (V *)getRaw(&k);
			return *r;
		}

		// Get a value from the map, creating it if it does not already exist.
		V &get(const K &k, const V &def) {
			V *r = (V *)getRaw(&k, &def);
			return *r;
		}

		// Remove a value.
		void remove(const K &k) {
			removeRaw(&k);
		}

		// Our iterator.
		class Iter : public MapBase::Iter {
		public:
			Iter() : MapBase::Iter() {}

			Iter(Par<Map<K, V>> owner) : MapBase::Iter(owner) {}

			std::pair<K, V&> operator *() const {
				return std::pair<K, V&>(*(K *)getRawKey(), *(V *)getRawVal());
			}

			// We can not provide the -> operator, so we have .key and .value instead.
			const K &key() const {
				return *(const K *)getRawKey();
			}

			V &val() const {
				return *(V *)getRawVal();
			}
		};

		// Access the iterator.
		Iter begin() {
			return Iter(this);
		}

		Iter end() {
			return Iter();
		}

	};

	/**
	 * Map type for use in the CREATE macro. CREATE(Map<T, U>), does not work well...
	 */
#define MAP(tKey, tVal) Map<tKey, tVal>
#define MAP_VP(tKey, tVal) Map<tKey, Auto<tVal> >
#define MAP_PP(tKey, tVal) Map<Auto<tKey>, Auto<tVal> >

}
