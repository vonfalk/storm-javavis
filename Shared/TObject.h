#pragma once
#include "Object.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core);

	class Thread;

	/**
	 * Root object for threaded objects. This object acts like 'Object'
	 * when the object is associated with a statically named thread (so there is
	 * no memory overhead for this). In C++, this object inherits from Object
	 * so that we do not have to duplicate a lot of code, however this is hidden
	 * in Storm and the as<> operator. Otherwise, it would be possible to call
	 * any member function of 'Object' in the wrong threading context by simply
	 * casting to 'Object' first.
	 *
	 * TODO: Should this object have a different public interface compared to Object? For example,
	 * should the 'equals' exist here? Clone (implemented as a no-op)?
	 */
	class TObject : public STORM_HIDDEN(Object) {
		STORM_CLASS;
	public:
		// Create an object that should live on 'thread'.
		STORM_CTOR TObject(Par<Thread> thread);

		// Copy the object (Maybe we should not support this?)
		STORM_CTOR TObject(Par<TObject> copy);

		// Dtor.
		~TObject();

		// This is more or less a dummy object to make the Object and TObject to be separate.

		// Equality check. This may be called from the 'wrong' thread, so this should not be
		// overloadable in Storm (even though it currently is, TODO). NOTE: since we're not
		// explicitly declaring this function as 'virtual', it should be 'final' in storm.
		Bool STORM_FN equals(Par<Object> o);

		// Hash function, may be called from the 'wrong' thread, so this should not be overloadable
		// in Storm (even though it currently is, TODO). NOTE: since we're not explicitly declaring
		// this function as 'virtual', it should be 'final' in storm.
		Nat STORM_FN hash();


		// The thread we should be running on.
		Auto<Thread> thread;

	protected:
		// Delete on the owning thread.
		virtual void deleteMe();
	};


	/**
	 * Convenience class for declaring threaded objects on a specific thread without having
	 * to declare anything in the constructor. Do not assume this class will be present
	 * in the inheritance chain in C++. We do not expose this type to the type system, which
	 * means that it may be erased from vtables and the like... Therefore: do not assume anything
	 * other than the constructor is executed when explicitly called!
	 */
	template <class T>
	class ObjectOn : public TObject {
	public:
		// Create and run on the thread specified by T.
		ObjectOn() : TObject(T::thread(engine())) {}

		// Copy ctor may be convenient.
		ObjectOn(Par<ObjectOn<T>> o) : TObject(o) {}
	};

}
