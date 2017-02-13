#include "stdafx.h"
#include "Runtime.h"

#ifndef STORM_COMPILER

namespace storm {
	namespace runtime {

		Type *cppType(Engine &e, Nat id) {
			return null;
		}

		Type *cppTemplate(Engine &e, Nat id, Nat count, ...) {
			return null;
		}

		const Handle &typeHandle(Type *t) {
			return *(const Handle *)null;
		}

		const Handle &voidHandle(Engine &e) {
			return *(const Handle *)null;
		}

		Type *typeOf(const RootObject *o) {
			return null;
		}

		Str *typeName(Type *t) {
			return null;
		}

		const GcType *gcTypeOf(const void *alloc) {
			return null;
		}

		bool isA(const RootObject *a, const Type *b) {
			return false;
		}

		Engine &allocEngine(const RootObject *o) {
			return *(Engine *)null;
		}

		void *allocObject(size_t size, Type *type) {
			return null;
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return null;
		}

		void *allocWeakArray(Engine &e, size_t count) {
			return null;
		}

		void *allocCode(Engine &e, size_t code, size_t refs) {
			return null;
		}

		size_t codeSize(const void *code) {
			return 0;
		}

		GcCode *codeRefs(void *code) {
			return 0;
		}

		void setVTable(RootObject *object) {}

		os::ThreadGroup &threadGroup(Engine &e) {
			return *(os::ThreadGroup *)null;
		}

		GcWatch *createWatch(Engine &e) {
			return null;
		}

		void attachThread(Engine &e) {}

		void detachThread(Engine &e, const os::Thread &thread) {}

		void reattachThread(Engine &e, const os::Thread &thread) {}

		void postStdRequest(Engine &e, StdRequest *request) {}

		RootObject *cloneObject(RootObject *obj) {
			return null;
		}

		RootObject *cloneObjectEnv(RootObject *obj, CloneEnv *env) {
			return null;
		}

	}

	Thread *DeclThread::thread(Engine &e) const {
		return null;
	}

}

namespace os {

	ThreadData *currentThreadData() {
		return null;
	}

	void currentThreadData(ThreadData *data) {}

	UThreadState *currentUThreadState() {
		return null;
	}

	void currentUThreadState(UThreadState *state) {}

	void threadCreated() {}

	void threadTerminated() {}

}

#endif
