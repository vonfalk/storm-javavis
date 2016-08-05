#include "stdafx.h"
#include "Runtime.h"
#include "Gc.h"
#include "Type.h"
#include "Engine.h"

namespace storm {
	namespace runtime {

		/**
		 * Implements the functions declared in 'Core/Runtime.h' for the compiler.
		 */

		Type *cppType(Engine &e, Nat id) {
			return e.cppType(id);
		}

		Type *cppTemplate(Engine &e, Nat id, Nat count, ...) {
			TODO(L"Implement me!");
			return null;
		}

		const Handle &typeHandle(Type *t) {
			return t->handle();
		}

		Type *typeOf(const Object *o) {
			return Gc::typeOf(o)->type;
		}

		Engine &allocEngine(const Object *o) {
			return typeOf(o)->engine;
		}

		void *allocObject(size_t size, Type *type) {
			assert(size <= type->gcType->stride,
				L"Invalid type description found! " + ::toS(size) + L" vs " + ::toS(type->gcType->stride));
			return type->engine.gc.alloc(type->gcType);
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return e.gc.allocArray(type, count);
		}

		os::ThreadGroup &threadGroup(Engine &e) {
			return e.threadGroup;
		}

		void attachThread(Engine &e) {
			e.gc.attachThread();
		}

		void detachThread(Engine &e, const os::Thread &thread) {
			e.gc.detachThread(thread);
		}

		void reattachThread(Engine &e, const os::Thread &thread) {
			e.gc.reattachThread(thread);
		}

	}
}
