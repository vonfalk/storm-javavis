#pragma once

// Mechanics to automatically manage a shared resource and manage locking automatically.
// The workflow looks like this:
// Exclusive<Foo, Lock> exResource(myFoo, myFooLock);
// ...
// Resource<Foo, Lock::L> access(exResource);
// access->bar();
// ...
// By only returning Exclusive objects from a class, this ensures that the returned objects
// will always be properly locked when used.

template <class T, class Lock>
class Exclusive {
public:
	Exclusive(T *object, Lock &lock) : object(object), lock(lock) {};

private:
	template <class U, class Taker>
	friend class Resource;

	T *object;
	Lock &lock;
};

template <class T, class Taker>
class Resource : NoCopy {
public:
	template <class Lock>
	Resource(Exclusive<T, Lock> &e) : lockTaker(e.lock), object(e.object) {}

	T *operator->() { return object; }
private:
	Taker lockTaker;
	T *object;
};