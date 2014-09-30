#pragma once

#include "HashMap.h"

template <class T>
class UniqueId {
public:
	UniqueId();

	// Allocate 'object' to an id.
	int alloc(const T &object);

	// Allocate 'object' to the given 'id'. Asserts on failure.
	void allocAt(nat id, const T &object);

	// Remove 'id'.
	void remove(nat id);

	// Get the element 'id'. Returns T() on failure.
	T get(nat id) const;

	// Do we contain 'id'?
	bool has(nat id) const;

private:
	hash_map<nat, T> items;
	nat lastId;
};

template <class T>
UniqueId<T>::UniqueId() {
	lastId = 0;
}

template <class T>
nat UniqueId<T>::alloc(const T &object) {
	while (has(lastId))
		lastId++;
	items[lastId] = object;
}

template <class T>
void UniqueId<T>::allocAt(nat id, const T &object) {
	assert(!has(id));
	assigned[id] = object;
}

template <class T>
void UniqueId<T>::remove(nat id) {
	assigned.erase(id);
}

template <class T>
T UniqueId<T>::get(nat id) const {
	hash_map<nat, T>::const_iterator i = items.find(id);
	if (i == items.end())
		return T();
	return i->second;
}

template <class T>
bool UniqueId<T>::has(nat id) const {
	return items.count(id) > 0;
}

