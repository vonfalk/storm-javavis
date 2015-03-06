#pragma once
#include "Auto.h"
#include "Lib/Types.h"
#include "Code/Size.h"
#include "Code/FnParams.h"

namespace storm {

	// When debugging, checks that objects are alive.
	void checkLive(void *obj);

	class Type;
	class Str;
	class Function;

	STORM_PKG(core);

	/**
	 * Since all classes inherited from Object follow this pattern:
	 * Foo *foo = new (Foo::type(engine)) Foo(10);
	 * Foo *foo = new (Foo::type(otherObject)) Foo(10);
	 * We can use the CREATE-macro instead:
	 * Foo *foo = CREATE(Foo, engine, 10);
	 * Foo *foo = CREATE(Foo, otherObject, 10);
	 */
#define CREATE(tName, eRef, ...) \
	new (tName::type(eRef)) tName(__VA_ARGS__)

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
	 * The cast of Auto<> steals ownership of this reference automatically,
	 * so Auto<T> t = fnCall() is correct.
	 *
	 * Function parameters are freed by the calling function. The Auto<> can
	 * fall back to a Par<> (which should be used for function parameters) and
	 * behaves correctly in that case. Otherwise, use Auto<>::borrow().
	 */
	class Object : public Printable {
		STORM_CLASS;
	public:
		// Initialize object to 1 reference.
		STORM_CTOR Object();

		virtual ~Object();

		// The type of this object.
		Type *const myType;

		// Get the size.
		static Size baseSize();

		// Get the engine somehow.
		Engine &engine() const;

		// This one is used to detect the presence of custom casting. Do not remove/rename!
		bool isA(Type *o) const;

		// Add reference.
		inline void CODECALL addRef() {
			// _ASSERT(_CrtCheckMemory());
			if (this) {
				checkLive(this);
				atomicIncrement(refs);
			}
		}

		// Release reference.
		inline void CODECALL release() {
			// _ASSERT(_CrtCheckMemory());
			if (this) {
				checkLive(this);
				if (atomicDecrement(refs) == 0)
					delete this;
			}
		}

		// Get a peek at the current refcount.
		inline nat dbg_refs() { return refs; }

		// Our specialized operator new(). Allocates memory for the Type provided,
		// regardless of the size here. Ie, the allocated size may be >= sizeof(obj).
		// If 'size' is 0, the check is ignored.
		static void *operator new (size_t size, Type *type);
		static void *operator new (size_t size, void *mem);

		// Matching delete.
		static void operator delete(void *ptr, Type *type);
		static void operator delete(void *ptr, void *mem);
		static void operator delete(void *ptr);

		// NOTE: these will simply assert, not possible to implement right now.
		static void *operator new[] (size_t size, Type *type);
		static void operator delete[](void *ptr);

		/**
		 * Members all objects are assumed to contain:
		 * These will probably be auto-generated by some part of the compiler.
		 */

		// To string.
		virtual Str *STORM_FN toS();

		// Compare for equality.
		virtual Bool STORM_FN equals(Par<Object> o);

		// Dump leaks.
		static void dumpLeaks();
	protected:
		virtual void output(wostream &to) const;

		static void *allocDumb(Engine &e, size_t size);

	private:
		Object(const Object &o);
		Object &operator =(const Object &o);

		// Current number of references.
		nat refs;
	};

	// Create an object using the supplied constructor.
	Object *createObj(Function *ctor, code::FnParams params);
	template <class T>
	inline T *create(Function *ctor, const code::FnParams &params) {
		return (T *)createObj(ctor, params);
	}

	// Create an object from Storm.
	void *CODECALL stormMalloc(Type *type);

	// Destroy an object from Storm.
	void CODECALL stormFree(void *mem);

	// Release (sets to null as well!)
	inline void release(Object *&o) {
		o->release();
		o = null;
	}

	// Release collection.
	template <class T>
	void releaseVec(T &v) {
		for (T::iterator i = v.begin(); i != v.end(); ++i) {
			(*i)->release();
		}
		v.clear();
	}

	// Release map.
	template <class T>
	void releaseMap(T &v) {
		for (T::iterator i = v.begin(); i != v.end(); ++i)
			i->second->release();
		v.clear();
	}

}

// Custom as<> implementation.
template <class To>
To *customAs(storm::Object *from) {
	if (from == null)
		return null;
	if (from->isA(To::type(from->engine())))
		return static_cast<To*>(from);
	return null;
}

template <class To>
To *customAs(const storm::Object *from) {
	if (from == null)
		return null;
	if (from->isA(To::type(from->engine())))
		return static_cast<To*>(from);
	return null;
}


