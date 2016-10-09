#pragma once
#include "RootObject.h"

namespace storm {
	STORM_PKG(core);

	class Thread;

	/**
	 * Root object for threaded objects. This object acts like 'Object' when the object is
	 * associated with a statically named thread.
	 */
	class TObject : public STORM_HIDDEN(RootObject) {
		STORM_CLASS;
	public:
		// Create an object that should live on 'thread'.
		STORM_CTOR TObject(Thread *thread);

		// Copy the object.
		STORM_CTOR TObject(TObject *o);

		// The thread we should be running on.
		Thread *thread;

		// Convert to string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;

		// Dummy deepCopy function which does nothing and is not exposed to Storm. Makes it easier
		// to write template code in C++.
		inline void deepCopy(CloneEnv *env) {}
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
