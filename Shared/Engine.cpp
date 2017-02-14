#include "stdafx.h"
#include "Engine.h"

namespace storm {
	namespace runtime {

		static EngineFwd fwd;

		Type *cppType(Engine &e, Nat id) {
			return (*fwd.cppType)(e, id);
		}

		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list l) {
			return (*fwd.cppTemplateVa)(e, id, count, l);
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
		return (*runtime::fwd.getThread)(e, this);
	}

	void useFwd(const EngineFwd &fwd) {
		runtime::fwd = fwd;
	}

}


namespace os {

	ThreadData *currentThreadData() {
		return (*storm::runtime::fwd.getCurrentThreadData)();
	}

	void currentThreadData(ThreadData *data) {
		(*storm::runtime::fwd.setCurrentThreadData)(data);
	}

	UThreadState *currentUThreadState() {
		return (*storm::runtime::fwd.getCurrentUThreadState)();
	}

	void currentUThreadState(UThreadState *state) {
		(*storm::runtime::fwd.setCurrentUThreadState)(state);
	}

	void threadCreated() {
		(*storm::runtime::fwd.threadCreated)();
	}

	void threadTerminated() {
		(*storm::runtime::fwd.threadTerminated)();
	}

}
