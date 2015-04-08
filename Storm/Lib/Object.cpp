#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Function.h"
#include "Lib/Str.h"
#include "Code/Sync.h"

#ifdef DEBUG

// DEBUG_REFS print references, DEBUG_LEAKS tracks live
// objects to output a list of the remaining objects at
// program termination. It also tracks use (ie addRef/release)
// of free'd objects.
// #define DEBUG_REFS
// #define DEBUG_LEAKS

#endif

namespace storm {

#ifdef DEBUG_LEAKS
	map<Object *, String> live;
	Lock liveLock;

	void Object::dumpLeaks() {
		Lock::L z(liveLock);

		if (live.size() > 0)
			PLN(L"Leaks detected!");
		for (map<Object *, String>::iterator i = live.begin(); i != live.end(); i++) {
			PLN(L"Object " << i->first << L": " << i->second << L" (" << i->first->refs << L")");
		}
	}
#else
	void Object::dumpLeaks() {}
#endif

	static void created(Object *o) {
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

	// Note the hack: myType(myType). myType is initialized in operator new.
	Object::Object() : myType(myType), refs(1) {
		created(this);
	}

	// Nothing to copy, really. Added to avoid special cases in other parts of the compiler.
	Object::Object(Object *) : myType(myType), refs(1) {
		created(this);
	}

	Object::~Object() {
		// assert(refs == 0); // When an exception is thrown in a ctor, this would trigger!
#ifdef DEBUG_REFS
		if (myType == this) {
			PLN("Destroying " << this << ", Type");
		} else {
			PLN("Destroying " << this << ", " << myType->name);
		}
#endif

#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		if (live.count(this) == 0) {
			PLN("Found a double-free!");
		}
		live.erase(this);
#endif
	}

	void Object::setTypeLate(Par<Type> t) {
		assert(myType == null, L"You may not use SET_TYPE_LATE to change the type of an object!");
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(this, typeOffset, Type *) = t.borrow();

#ifdef DEBUG_LEAKS
		if (live.count(this)) {
			live[this] = t->name;
		}
#endif
	}

	void checkLive(void *o) {
#ifdef DEBUG_LEAKS
		Lock::L z(liveLock);
		if (live.count((Object *)o) == 0) {
			PLN("Access to dead object: " << o);
			PLN(format(stackTrace()));
			DebugBreak();
			assert(false);
		}
#endif
	}

	Size Object::baseSize() {
		// TODO: Maybe automate this?
		Size s;
		s += Size::sPtr; // vtable
		s += Size::sPtr; // myType
		s += Size::sNat; // refs
		assert(s.current() == sizeof(Object), L"Forgot to update baseSize!");
		return s;
	}

	Engine &Object::engine() const {
		return myType->engine;
	}

	bool Object::isA(Type *t) const {
		return myType->isA(t);
	}

	Str *Object::toS() {
		std::wostringstream out;
		out << *this;
		return CREATE(Str, this, out.str());
	}

	void Object::output(wostream &to) const {
		to << myType->name << L": " << toHex(this);
	}

	Bool Object::equals(Par<Object> o) {
		if (!o)
			return false;
		if (o->myType != myType)
			return false;
		return true;
	}

	void Object::deepCopy(Par<CloneEnv> env) {}

	/**
	 * Memory management.
	 */

	void *Object::allocDumb(Engine &e, size_t size) {
		void *mem = malloc(size);
		memset(mem, 0, size);
		return mem;
	}

	void *Object::operator new(size_t size, Type *type) {
		size_t s = type->size().current();

		assert(type->flags & typeClass);
		assert(size <= s || size == 0, L"Not enough memory for " + type->name
			+ L". From C++: " + ::toS(size) + L" from storm: " + ::toS(s));

		void *mem = allocDumb(type->engine, s);
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(mem, typeOffset, Type *) = type;

		return mem;
	}

	void *Object::operator new(size_t size, void *mem) {
		size_t typeOffset = OFFSET_OF(Object, myType);
		assert(OFFSET_IN(mem, typeOffset, Type *));
		return mem;
	}

	void *Object::operator new(size_t size, Engine &e) {
		return allocDumb(e, size);
	}

	void Object::operator delete(void *mem, Type *type) {
		Object::operator delete(mem);
	}

	void Object::operator delete(void *ptr, void *mem) {
		// No need...
	}

	void Object::operator delete(void *mem, Engine &e) {
		Object::operator delete(mem);
	}

	void Object::operator delete(void *mem) {
		free(mem);
	}

	void *Object::operator new[](size_t size, Type *type) { assert(false); return null; }
	void Object::operator delete[](void *ptr) { assert(false); }

	Object *createObj(Type *type, const void *ctor, code::FnParams params) {
		assert(type->flags & typeClass);

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

	Object *createObj(Function *ctor, code::FnParams params) {
		assert(ctor->name == Type::CTOR, "Don't use create() with other functions than constructors.");
		assert(ctor->params.size() == params.count() + 1,
			"Wrong number of parameters to constructor! The first one is filled in automatically.");
		Type *type = ctor->params[0].type;
		return createObj(type, ctor->pointer(), params);
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

}
