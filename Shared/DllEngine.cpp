#include "stdafx.h"
#include "DllEngine.h"
#ifdef STORM_DLL
#include "BuiltIn.h"

namespace storm {

	static const DllInterface *interface = null;

	static void shutdownLibData(void *data) {
		LibData *d = (LibData *)data;
		d->shutdown();
	}

	static void destroyLibData(void *data) {
		LibData *d = (LibData *)data;
		delete d;
	}

	LibData::LibData() {}

	LibData::~LibData() {}

	void LibData::shutdown() {}

	DllInfo Engine::setup(const DllInterface *i, LibData *data) {
		interface = i;

#ifdef DEBUG
		void **d = (void **)i;
		for (nat z = 0; z < sizeof(*i) / sizeof(void *); z++) {
			assert(d[z] != null, L"Pointer at offset " + ::toS(z) + L" is null in DllInterface!");
		}
#endif

		DllInfo info = {
			&storm::builtIn(),
			data,
			&shutdownLibData,
			&destroyLibData,
		};
		return info;
	}

	Type *Engine::builtIn(nat id) {
		return (*interface->builtIn)(*this, interface->data, id);
	}

	LibData *Engine::data() {
		return (LibData *)(*interface->getData)(*this, interface->data);
	}

	Thread *Engine::thread(uintptr_t id, DeclThread::CreateFn fn) {
		return (*interface->getThread)(*this, id, fn);
	}

	void *cppVTable(nat id) {
		return (*interface->cppVTable)(interface->data, id);
	}

	/**
	 * Implementation of functions required by Object.
	 */

	void objectCreated(Object *o) {
		(*interface->objectCreated)(o);
	}

	void objectDestroyed(Object *o) {
		(*interface->objectDestroyed)(o);
	}

	void *allocObject(Type *t, size_t cppSize) {
		return (*interface->allocObject)(t, cppSize);
	}

	void freeObject(void *mem) {
		(*interface->freeObject)(mem);
	}

	Engine &engine(const Object *o) {
		return (*interface->engineFrom)(o);
	}

	bool objectIsA(const Object *o, const Type *t) {
		return (*interface->objectIsA)(o, t);
	}

	String typeIdentifier(const Type *t) {
		return (*interface->typeIdentifier)(t);
	}

	vector<ValueData> typeParams(const Type *t) {
		return (*interface->typeParams)(t);
	}

	void setVTable(Object *o) {
		return (*interface->setVTable)(o);
	}

	bool isClass(Type *t) {
		return (*interface->isClass)(t);
	}

	Object *cloneObjectEnv(Object *o, CloneEnv *env) {
		return (*interface->cloneObjectEnv)(o, env);
	}

	Type *intType(Engine &e) {
		return (*interface->intType)(e);
	}

	Type *natType(Engine &e) {
		return (*interface->natType)(e);
	}

	Type *longType(Engine &e) {
		return (*interface->longType)(e);
	}

	Type *wordType(Engine &e) {
		return (*interface->wordType)(e);
	}

	Type *byteType(Engine &e) {
		return (*interface->byteType)(e);
	}

	Type *floatType(Engine &e) {
		return (*interface->floatType)(e);
	}

	Type *boolType(Engine &e) {
		return (*interface->boolType)(e);
	}

	Type *arrayType(Engine &e, const ValueData &v) {
		return (*interface->arrayType)(e, v);
	}

	Type *mapType(Engine &e, const ValueData &k, const ValueData &v) {
		return (*interface->mapType)(e, k, v);
	}

	Type *futureType(Engine &e, const ValueData &v) {
		return (*interface->futureType)(e, v);
	}

	Type *fnPtrType(Engine &e, const vector<ValueData> &params) {
		return (*interface->fnPtrType)(e, params);
	}

	bool toSOverridden(const Object *o) {
		return (*interface->toSOverridden)(o);
	}

	const Handle &typeHandle(Type *t) {
		return (*interface->typeHandle)(t);
	}

	os::ThreadGroup &threadGroup(Engine &e) {
		return (*interface->threadGroup)(e);
	}

#ifdef DEBUG
	void checkLive(void *ptr) {
		(*interface->checkLive)(ptr);
	}
#endif

	Thread *DeclThread::thread(Engine &e) const {
		return e.thread((uintptr_t)&createFn, createFn);
	}
}

namespace os {
	using namespace storm;

	ThreadData *currentThreadData() {
		return (*interface->osFns.getThreadData)();
	}

	void currentThreadData(ThreadData *data) {
		(*interface->osFns.setThreadData)(data);
	}

	UThreadState *currentUThreadState() {
		return (*interface->osFns.getUThreadState)();
	}

	void currentUThreadState(UThreadState *state) {
		(*interface->osFns.setUThreadState)(state);
	}

	void threadCreated() {
		(*interface->osFns.threadCreated)();
	}

	void threadTerminated() {
		(*interface->osFns.threadTerminated)();
	}

}

#endif
