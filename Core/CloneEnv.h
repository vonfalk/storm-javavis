#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Remember objects copied during a clone.
	 */
	class CloneEnv : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR CloneEnv();

		// TODO.
	};

	/**
	 * Crude and incomplete clone function for now. We want to look at the actual type of the object
	 * so we do not loose the dynamic type, as we're currently only looking at the static type.
	 */
	template <class T>
	T *clone(T *obj) {
		TODO(L"Look at the dynamic type as well!");
		return new (obj) T(obj);
	}


	/**
	 * Make a cloned variand of a variable in C++. Useful when implementing deepCopy()
	 */
	template <class T>
	void cloned(T *&obj, CloneEnv *env) {
		obj = clone(obj);
		obj->deepCopy(env);
	}

	template <class T>
	void cloned(T &value, CloneEnv *env) {
		value.deepCopy(env);
	}

}
