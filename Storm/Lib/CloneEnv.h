#pragma once
#include "Object.h"
#include "Auto.h"
#include "Utils/Templates.h"

namespace storm {
	STORM_PKG(core);

	// Implemented in TypeCtor.h
	class CloneEnv;
	Object *CODECALL cloneObjectEnv(Object *o, CloneEnv *e);

	/**
	 * Keeps track of objects already cloned.
	 */
	class CloneEnv : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR CloneEnv();

		// Destroy
		~CloneEnv();

		// If 'o' was cloned before, get the clone of it. Otherwise returns null.
		Object *STORM_FN cloned(Par<Object> o);

		// Tell us that 'o' is cloned into 'to'
		void STORM_FN cloned(Par<Object> o, Par<Object> to);

	private:
		typedef hash_map<Object *, Object *> ObjMap;
		ObjMap objs;

	};


	/**
	 * Clone objects the C++ way! Takes either a value type or Auto<T>.
	 */
	template <class T>
	void clone(T &v, Par<CloneEnv> env) {
		v.deepCopy(env);
	}

	// Assumes we have ownership of the pointer...
	template <class T>
	void clone(T *v, Par<CloneEnv> env) {
		T *tmp = (T *)cloneObjectEnv(v, env.borrow());
		swap(tmp, v);
		v->release();
	}

	// Built-in types do not need anything special.
	template <>
	inline void clone(Int &v, Par<CloneEnv> env) {}
	template <>
	inline void clone(Nat &v, Par<CloneEnv> env) {}
	template <>
	inline void clone(Bool &v, Par<CloneEnv> env) {}

	/**
	 * Clone not inplace.
	 */
	template <class T>
	T cloned(const T &v, Par<CloneEnv> env) {
		T r = v;
		clone(r);
		return r;
	}

}
