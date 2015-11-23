#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Function.h"
#include "Shared/Str.h"
#include "OS/Sync.h"
#include "Code/VTable.h"
#include "Code/Memory.h"
#include "Shared/Future.h"
#include "Shared/Map.h"
#include "Engine.h"

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
	set<void *> allocd;
	Lock liveLock;

	void Object::dumpLeaks() {
		Lock::L z(liveLock);

		if (live.size() > 0 || allocd.size() > 0)
			PLN(L"Leaks detected!");
		for (map<Object *, String>::iterator i = live.begin(); i != live.end(); i++) {
			if (code::readable(i->first)) {
				PLN(L"Object " << i->first << L": " << i->second << L" (" << i->first->refs << L")");
			} else {
				PLN(L"Object " << i->first << L": " << i->second << L" (dead)");
			}
		}
		for (set<void *>::iterator i = allocd.begin(); i != allocd.end(); i++) {
			PLN(L"Uninitialized object at " << *i);
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
		if (allocd.count(o) == 0) {
			WARNING(L"Creating object at " << o << L" non mallocd memory!");
		} else {
			allocd.erase(o);
		}

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
		allocd.insert(o);
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
#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		allocd.insert(mem);
#endif
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
#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		set<void *>::iterator i = allocd.find(memory);
		if (i == allocd.end()) {
			WARNING(L"Trying to free non-allocd block at " << memory);
		} else {
			allocd.erase(i);
		}
#endif
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

	vector<ValueData> typeParams(const Type *t) {
		return vector<ValueData>(t->params.begin(), t->params.end());
	}

	/**
	 * Convenience functions.
	 */

	Object *createObj(Function *ctor, os::FnParams params) {
		assert(ctor->name == Type::CTOR, "Don't use create() with other functions than constructors.");
		assert(ctor->params.size() == params.count() + 1,
			"Wrong number of parameters to constructor! The first one is filled in automatically.");
		Type *type = ctor->params[0].type;
		return createObj(type, ctor->pointer(), params);
	}

	Object *createObj(Type *type, const void *ctor, os::FnParams params) {
		assert(type->typeFlags & typeClass);

		void *mem = Object::operator new(type->size().current(), type);

		try {
			params.addFirst(mem);
			os::call<void>(ctor, false, params);
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

	// Keep track of all known toS implementations.
	static map<void *, nat> toSFunctions;

	bool toSOverridden(const Object *o) {
		static nat slotId = code::findSlot(address(&Object::toS), Object::cppVTable());
		if (slotId == code::VTable::invalid) {
			WARNING(L"Can not find toS in Object's vtable. Output from C++ will be broken or crash.");
		}

		void *vtable = code::vtableOf(o);
		void *active = code::getSlot(vtable, slotId);
		return !o->engine().rootToS(active);
	}

	Size objectBaseSize() {
		// TODO: Maybe automate this?
		Size s;
		s += Size::sPtr; // vtable
		s += Size::sPtr; // myType
		s += Size::sNat; // refs
		assert(s.current() == sizeof(Object), L"Forgot to update baseSize!");
		return s;
	}

	Size tObjectBaseSize() {
		Size s = objectBaseSize();
		s += Size::sPtr; // thread
		assert(s.current() == sizeof(Object), L"Forgot to update baseSize!");
		return s;
	}

	Offset tObjectThreadOffset() {
		Offset r(objectBaseSize());
		assert(r.current() == OFFSET_OF(TObject, thread), L"Forgot to update threadOffset!");
		return r;
	}

	static Thread *getThread(Engine &e, uintptr_t id, DeclThread::CreateFn fn) {
		return e.thread(id, fn);
	}

	os::ThreadGroup &threadGroup(Engine &e) {
		return e.threadGroup;
	}

	DllInterface dllInterface() {
		DllInterface i = {
			os::osFns(),
			null, // Set later.
			null,
			null,
			null,
			&getThread,
			&objectCreated,
			&objectDestroyed,
			&allocObject,
			&freeObject,
			&storm::engine,
			&objectIsA,
			&typeIdentifier,
			&typeParams,
			&setVTable,
			&isClass,
			&cloneObjectEnv,
			&intType,
			&natType,
			&longType,
			&wordType,
			&byteType,
			&floatType,
			&boolType,
			&arrayType,
			&mapType,
			&futureType,
			&fnPtrType,
			&toSOverridden,
			&typeHandle,
			&threadGroup,
#ifdef DEBUG
			&checkLive,
#endif
		};
		return i;
	}
}
