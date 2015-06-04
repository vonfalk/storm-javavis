#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Function.h"
#include "Shared/Str.h"
#include "Code/Sync.h"
#include "Code/VTable.h"
#include "Code/Memory.h"

#ifdef DEBUG

// DEBUG_REFS print references, DEBUG_LEAKS tracks live
// objects to output a list of the remaining objects at
// program termination. DEBUG_USE checks at each addRef
// and release call so that the pointer is valid. Requires
// DEBUG_LEAKS.
//#define DEBUG_REFS
//#define DEBUG_USE
#define DEBUG_LEAKS
#define DEBUG_PTRS

#endif

namespace storm {

	/**
	 * Object lifetime tracking.
	 */

#ifdef DEBUG_LEAKS
	map<Object *, String> live;
	Lock liveLock;

	void Object::dumpLeaks() {
		Lock::L z(liveLock);

		if (live.size() > 0)
			PLN(L"Leaks detected!");
		for (map<Object *, String>::iterator i = live.begin(); i != live.end(); i++) {
			if (code::readable(i->first)) {
				PLN(L"Object " << i->first << L": " << i->second << L" (" << i->first->refs << L")");
			} else {
				PLN(L"Object " << i->first << L": " << i->second << L" (dead)");
			}
		}
	}
#else
	void Object::dumpLeaks() {}
#endif

	void objectCreated(Object *o) {
#if defined(DEBUG) && defined(X86)
		if ((nat)o->myType == 0xCCCCCCCC) {
			PLN("Do not stack allocate Objects, stupid!");
			DebugBreak();
			assert(false);
		}
#endif

#ifdef DEBUG_REFS
		if (o->myType == o) {
			// The Type-type is special.
			PLN("Created " << o << ", Type");
		} else if (o->myType == 0) {
			// Type will come later.
			PLN("Created " << o << ", <unknown type>");
		} else {
			PLN("Created " << o << ", " << o->myType->name);
		}
#endif

#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		if (o->myType == o) {
			live.insert(make_pair(o, L"Type"));
		} else if (o->myType == null) {
			live.insert(make_pair(o, L"<unknown type>"));
		} else {
			live.insert(make_pair(o, o->myType->name));
		}
#endif
	}

	void objectDestroyed(Object *o) {
#ifdef DEBUG_REFS
		if (o->myType == o) {
			PLN("Destroying " << o << ", Type");
		} else {
			PLN("Destroying " << o << ", " << o->myType->name);
		}
#endif

#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		if (live.count(o) == 0) {
			PLN("Found a double-free!");
		}
		live.erase(o);
#endif
	}

	void checkLive(void *o) {
#ifdef DEBUG_USE
		Lock::L z(liveLock);
		if (live.count((Object *)o) == 0) {
			PLN("Access to dead object: " << o);
			PLN(format(stackTrace()));
			DebugBreak();
			assert(false);
		}
#endif
	}

	/**
	 * Memory allocation/deallocation.
	 */

	static void *allocMem(Engine &e, size_t size) {
		void *mem = malloc(size);
		memset(mem, 0, size);
		return mem;
	}

	Object::unsafe noTypeAlloc(Engine &e, size_t size) {
		return Object::unsafe(allocMem(e, size));
	}

	void *allocObject(Type *type, size_t reportedSize) {
		size_t s = type->size().current();

		assert(type->typeFlags & typeClass, L"Don't do 'new' on value types.");
		assert(reportedSize <= s || reportedSize == 0, L"Not enough memory for " + type->name +
			L". From C++: " + ::toS(reportedSize) + L" from Storm: " + ::toS(s) +
			L". Is the class correctly exposed to Storm?");

		void *mem = allocMem(type->engine, s);
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(mem, typeOffset, Type *) = type;

		return mem;
	}

	void freeObject(void *memory) {
		free(memory);
	}

	/**
	 * Late types.
	 */

	void setTypeLate(Par<Object> obj, Par<Type> t) {
		Object *o = obj.borrow();
		assert(o->myType == null, L"You may not use SET_TYPE_LATE to change the type of an object!");
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(o, typeOffset, Type *) = t.borrow();

#ifdef DEBUG_LEAKS
		if (live.count(o)) {
			live[o] = t->name;
		}
#endif
	}

	/**
	 * Type management.
	 */

	Engine &engine(const Object *o) {
		return o->myType->engine;
	}

	bool objectIsA(const Object *o, const Type *t) {
		return o->myType->isA(t);
	}

	String typeIdentifier(const Type *t) {
		return t->identifier();
	}

	/**
	 * Convenience functions.
	 */

	Object *createObj(Function *ctor, code::FnParams params) {
		assert(ctor->name == Type::CTOR, "Don't use create() with other functions than constructors.");
		assert(ctor->params.size() == params.count() + 1,
			"Wrong number of parameters to constructor! The first one is filled in automatically.");
		Type *type = ctor->params[0].type;
		return createObj(type, ctor->pointer(), params);
	}

	Object *createObj(Type *type, const void *ctor, code::FnParams params) {
		assert(type->typeFlags & typeClass);

		void *mem = Object::operator new(type->size().current(), type);

		try {
			params.addFirst(mem);
			code::call<void>(ctor, false, params);
		} catch (...) {
			Object::operator delete(mem, type);
			throw;
		}
		return (Object *)mem;
	}

	Object *createCopy(const void *copyCtor, Object *old) {
		typedef void (* Fn)(void *mem, Object *old);
		Fn fn = (Fn)copyCtor;
		Type *t = old->myType;
		void *mem = Object::operator new(t->size().current(), t);

		try {
			(*fn)(mem, old);
		} catch (...) {
			Object::operator delete(mem, t);
			throw;
		}
		return (Object *)mem;
	}

	void *CODECALL stormMalloc(Type *type) {
		return Object::operator new(0, type);
	}

	void CODECALL stormFree(void *mem) {
		// OK to pass null to mem.
		Object::operator delete(mem);
	}

	void setVTable(Object *o) {
		o->myType->vtable.update(o);
	}


	DllInterface dllInterface() {
		DllInterface i = {
			null, // Set later.
			null,
			&objectCreated,
			&objectDestroyed,
			&allocObject,
			&freeObject,
			&storm::engine,
			&objectIsA,
			&typeIdentifier,
#ifdef DEBUG
			&checkLive,
#endif
		};
		return i;
	}
}
