#pragma once
#include "Object.h"
#include "Map.h"
#include "Utils/Templates.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Remember objects copied during a clone.
	 *
	 * TODO: We can maybe increase performance by inlining the implementation from Map and ripping
	 * out any Handles.
	 */
	class CloneEnv : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR CloneEnv();

		// If 'o' was cloned before, get the clone of it. Otherwise returns null.
		Object *cloned(Object *o);

		// Tell us that 'o' is cloned into 'to'.
		void cloned(Object *o, Object *to);

	private:
		// Keep track of the cloned objects. Note: we're letting this map act as if it contained TObjects.
		Map<Object *, Object *> *data;
	};


	/**
	 * Clone an object.
	 */
	template <class T>
	T *clone(T *obj) {
		return (T *)runtime::cloneObject((RootObject *)obj);
	}

	template <class T>
	T *clone(T *obj, CloneEnv *env) {
		return (T *)runtime::cloneObjectEnv((RootObject *)obj, env);
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
