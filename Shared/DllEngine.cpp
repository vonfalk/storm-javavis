#include "stdafx.h"
#include "DllEngine.h"
#ifdef STORM_DLL

namespace storm {

	static const DllInterface *interface = null;

	void Engine::setup(const DllInterface *i) {
		interface = i;
	}

	Type *Engine::builtIn(nat id) {
		return (*interface->builtIn)(*this, interface->data, id);
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

#ifdef DEBUG
	void checkLive(void *ptr) {
		(*interface->checkLive)(ptr);
	}
#endif

}

#endif
