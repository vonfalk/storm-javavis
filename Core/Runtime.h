#pragma once
#include "GcArray.h"
#include "GcType.h"

namespace storm {
	struct GcType;

	namespace runtime {

		/**
		 * This file declares the interface to the runtime that code in Core/ needs, but can not be
		 * implemented here, as the implementation differs depending on if Core/ is linked to the
		 * Compiler/ or a shared library. For shared libraries, include SharedLib.h from *one*
		 * source file, to get the implementation suitable for shared libraries (which just
		 * delegates calls to the compiler linked at runtime).
		 */

		// Get the Storm type description for a C++ type given its ID (used by the generated type information code).
		Type *cppType(Engine &e, Nat id);

		// Get a type handle for 'type'.
		const Handle &typeHandle(Type *t);

		// Get the type of an allocation.
		Type *typeOf(const Object *o);

		// Get the engine object for an allocation.
		Engine &allocEngine(const Object *o);

		// Allocate an object of the given type (size used for sanity checking).
		void *allocObject(size_t size, Type *type);

		// Allocate an array of objects.
		void *allocArray(Engine &e, const GcType *type, size_t count);

		template <class T>
		inline GcArray<T> *allocArray(Engine &e, const GcType *type, nat count) {
			return (GcArray<T> *)allocArray(e, type, count);
		}

		// Get the thread group to use for all threads.
		os::ThreadGroup &threadGroup(Engine &e);

		// Attach the current thread with the given engine.
		void attachThread(Engine &e);

		// Detach 'thread' from the engine.
		void detachThread(Engine &e, const os::Thread &thread);

		// Reattach a thread (ie. make it possible to call detach once more without anything bad happening).
		void reattachThread(Engine &e, const os::Thread &thread);

	}
}
