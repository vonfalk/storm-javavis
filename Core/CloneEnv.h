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
	T *clone(T *obj, CloneEnv *env) {
		TODO(L"Look at the dynamic type as well!");
		T *t = new (obj) T(*obj);
		t->deepCopy(env);
		return t;
	}


	/**
	 * Make a cloned variand of a variable in C++. Useful when implementing deepCopy()
	 */
	template <class T>
	void cloned(T *&obj, CloneEnv *env) {
		obj = clone(obj, env);
	}

	template <class T>
	void cloned(T &value, CloneEnv *env) {
		value.deepCopy(env);
	}

	inline void cloned(Bool v, CloneEnv *e) {}
	inline void cloned(Byte v, CloneEnv *e) {}
	inline void cloned(Int v, CloneEnv *e) {}
	inline void cloned(Nat v, CloneEnv *e) {}
	inline void cloned(Long v, CloneEnv *e) {}
	inline void cloned(Word v, CloneEnv *e) {}
	inline void cloned(Float v, CloneEnv *e) {}

}
