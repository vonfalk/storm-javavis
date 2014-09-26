#pragma once

#include <set>

namespace util {

	template<class T>
	class UniqueId {
	public:
		UniqueId();
		~UniqueId();

		int allocate(T object);
		void allocateAt(int id, T object);
		void remove(int id);
		void change(int newId, int oldId);
		T find(int id);
		bool hasItem(int id);
	private:
		CMap<int, int, T, T &> assigned;

		int lastId;
	};

	template<class T>
	UniqueId<T>::UniqueId() {
		lastId = 0;
	}

	template<class T>
	UniqueId<T>::~UniqueId() {}

	template<class T>
	int UniqueId<T>::allocate(T object) {
		while (hasItem(lastId)) { lastId++; }
		assigned[lastId] = object;
		return lastId++;
	}

	template<class T>
	void UniqueId<T>::allocateAt(int id, T object) {
		ASSERT(!hasItem(id));
		assigned[id] = object;
	}

	template<class T>
	void UniqueId<T>::remove(int id) {
		assigned.RemoveKey(id);
	}

	template<class T>
	void UniqueId<T>::change(int newId, int oldId) {
		T item;
		if (assigned.Lookup(oldId, item)) {
			assigned.RemoveKey(oldId);
			assigned[newId] = item;
		} else {
			ASSERT(FALSE);
		}
	}

	template<class T>
	T UniqueId<T>::find(int id) {
		T found;
		if (assigned.Lookup(id, found)) return found;
		return T();
	}

	template<class T>
	bool UniqueId<T>::hasItem(int id) {
		T t;
		return assigned.Lookup(id, t) == TRUE;
	}
};