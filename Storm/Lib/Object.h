#pragma once

namespace storm {

	class Type;

	/**
	 * The root object that all non-value objects inherit
	 * from. This class contains the logic for the central
	 * reference counting mechanism among other things.
	 * These are designed to be manipulated through pointer,
	 * and is therefore not copyable using regular C++ methods.
	 *
	 * Rules for the ref-counting:
	 * When returning Object*s, the caller has the responsibility
	 * to release one reference. Ie, the caller has ownership of one reference.
	 * Function parameters are also the caller's responisibility.
	 */
	class Object : NoCopy {
	public:
		// Initialize object to 1 reference.
		Object(Type *type);

		virtual ~Object();

		// The type of this object.
		Type *const type;

		// Add reference.
		inline void addRef() {
			atomicIncrement(refs);
		}

		// Release reference.
		inline void release() {
			if (this)
				if (atomicDecrement(refs) == 0)
					delete this;
		}

	private:
		// Current number of references.
		nat refs;
	};


	// Release (sets to null as well!)
	inline void release(Object *&o) {
		o->release();
		o = null;
	}

	// Release collection.
	template <class T>
	void releaseVec(T &v) {
		for (T::iterator i = v.begin(); i != v.end(); ++i)
			release(*i);
		v.clear();
	}

	// Release map.
	template <class T>
	void releaseMap(T &v) {
		for (T::iterator i = v.begin(); i != v.end(); ++i)
			release(i->second);
		v.clear();
	}

}
