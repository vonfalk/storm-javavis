#include "stdafx.h"
#include "Engine.h"
#include "OS/Sync.h"

namespace storm {

	// Shared function pointers.
	static EngineFwdShared fwd = { 0 };

	// Unique function pointers. One for each observed engine-id.
	static EngineFwdUnique *unique = null;
	static Nat uniqueCount = 0;
	static Nat uniqueFilled = 0;
	static os::Lock uniqueLock;

	// Empty (ie. all bytes = 0)?
	bool memempty(const void *src, size_t count) {
		const byte *s = (const byte *)src;
		bool zero = true;
		for (size_t i = 0; i < count; i++)
			zero &= s[i] == 0;
		return zero;
	}

	void Engine::attach(const EngineFwdShared &shared, const EngineFwdUnique &unique) {
		if (memempty(&fwd, sizeof(EngineFwdShared))) {
			fwd = shared;
		} else {
			assert(memcmp(&fwd, &shared, sizeof(EngineFwdShared)) == 0,
				L"Using two different implementations of Engines with the same instance of a shared library!");
		}

		{
			os::Lock::L z(uniqueLock);
			if (storm::uniqueCount < id) {
				EngineFwdUnique *n = new EngineFwdUnique[id];
				memcpy(n, storm::unique, uniqueCount*sizeof(EngineFwdUnique));

				EngineFwdUnique *old = storm::unique;
				atomicCAS(storm::unique, old, n);
				delete []old;
				storm::uniqueCount = id;
			}

			storm::uniqueFilled++;
		}

		storm::unique[id] = unique;
	}

	void Engine::detach() {
		os::Lock::L z(uniqueLock);

		if (--uniqueFilled == 0) {
			// Last one. Delete the unique array as no one needs it anymore.
			delete []unique;
			uniqueCount = 0;
		}
	}


	namespace runtime {

		Type *cppType(Engine &e, Nat id) {
			return (*unique[e.identifier()].cppType)(e, id);
		}

		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list l) {
			return (*unique[e.identifier()].cppTemplateVa)(e, id, count, l);
		}

		const Handle &typeHandle(Type *t) {
			return (*fwd.typeHandle)(t);
		}

		const Handle &voidHandle(Engine &e) {
			return (*fwd.voidHandle)(e);
		}

		Type *typeOf(const RootObject *o) {
			return (*fwd.typeOf)(o);
		}

		Str *typeName(Type *t) {
			return (*fwd.typeName)(t);
		}

		const GcType *gcTypeOf(const void *alloc) {
			return (*fwd.gcTypeOf)(alloc);
		}

		bool isA(const RootObject *a, const Type *b) {
			return (*fwd.isA)(a, b);
		}

		Engine &allocEngine(const RootObject *o) {
			return (*fwd.allocEngine)(o);
		}

		void *allocObject(size_t size, Type *type) {
			return (*fwd.allocObject)(size, type);
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return (*fwd.allocArray)(e, type, count);
		}

		void *allocWeakArray(Engine &e, size_t count) {
			return (*fwd.allocWeakArray)(e, count);
		}

		void *allocCode(Engine &e, size_t code, size_t refs) {
			return (*fwd.allocCode)(e, code, refs);
		}

		size_t codeSize(const void *code) {
			return (*fwd.codeSize)(code);
		}

		GcCode *codeRefs(void *code) {
			return (*fwd.codeRefs)(code);
		}

		void setVTable(RootObject *object) {
			(*fwd.setVTable)(object);
		}

		os::ThreadGroup &threadGroup(Engine &e) {
			return (*fwd.threadGroup)(e);
		}

		GcWatch *createWatch(Engine &e) {
			return (*fwd.createWatch)(e);
		}

		void attachThread(Engine &e) {
			return (*fwd.attachThread)(e);
		}

		void detachThread(Engine &e, const os::Thread &thread) {
			return (*fwd.detachThread)(e, thread);
		}

		void reattachThread(Engine &e, const os::Thread &thread) {
			return (*fwd.reattachThread)(e, thread);
		}

		void postStdRequest(Engine &e, StdRequest *request) {
			(*fwd.postStdRequest)(e, request);
		}

		RootObject *cloneObject(RootObject *obj) {
			return (*fwd.cloneObject)(obj);
		}

		RootObject *cloneObjectEnv(RootObject *obj, CloneEnv *env) {
			return (*fwd.cloneObjectEnv)(obj, env);
		}

	}

	Thread *DeclThread::thread(Engine &e) const {
		return (*unique[e.identifier()].getThread)(e, this);
	}

}


namespace os {

	ThreadData *currentThreadData() {
		return (*storm::fwd.getCurrentThreadData)();
	}

	void currentThreadData(ThreadData *data) {
		(*storm::fwd.setCurrentThreadData)(data);
	}

	UThreadState *currentUThreadState() {
		return (*storm::fwd.getCurrentUThreadState)();
	}

	void currentUThreadState(UThreadState *state) {
		(*storm::fwd.setCurrentUThreadState)(state);
	}

	void threadCreated() {
		(*storm::fwd.threadCreated)();
	}

	void threadTerminated() {
		(*storm::fwd.threadTerminated)();
	}

}
