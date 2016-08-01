#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	class Thread;

	/**
	 * Root object for threaded objects. This object acts like 'Object' when the object is
	 * associated with a statically named thread. In C++, this object inherits from Object so that
	 * we do not have to duplicate a lot of code, however this is hidden in Storm and the as<>
	 * operator. Otherwise, it would be possible to call any member function of 'Object' in the
	 * wrong threading context by simply casting to 'Object' first.
	 *
	 * TODO: Should this object have a different public interface compared to Object? For example,
	 * should the 'equals' exist here? Clone (implemented as a no-op)?
	 */
	class TObject : public STORM_HIDDEN(Object) {
		STORM_CLASS;
	public:
		// Create an object that should live on 'thread'.
		STORM_CTOR TObject(Thread *thread);

		// Copy the object.
		STORM_CTOR TObject(TObject *o);

		// The thread we should be running on.
		Thread *thread;

		/**
		 * Overrides for the parts different from Object:
		 */

		// Equality check. This may be called from the 'wrong' thread, so this should not be
		// overloadable in Storm.
		Bool STORM_FN equals(Object *o) const;

		// Hash function. May be called from the 'wrong' thread, so it should not be overloadable in
		// Storm.
		Nat STORM_FN hash() const;

		/**
		 * Re-implementation of the public interface, as the Object base class is hidden from Storm.
		 */

		// Convert to string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Convenience class for declaring threaded objects on a specific thread in C++ without having
	 * to do anything in the constructor. Do not assume this class will be present in the
	 * inheritance chain in C++. This class is not exposed to the type system, which means that it
	 * may be erased from vtables and the like. Therefore: do not assume anything other than that
	 * the constructor is executed.
	 */
	template <class T>
	class ObjectOn : public TObject {
	public:
		// Create and run on the thread specified by T.
		ObjectOn() : TObject(T::thread(engine())) {}

		// Copy constructor may be convenient.
		ObjectOn(ObjectOn<T> *o) : TObject(o) {}
	};

}


// Usually, you want the Compiler thread as well.
#include "Thread.h"
