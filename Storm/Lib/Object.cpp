#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Function.h"
#include "Lib/Str.h"

#ifdef DEBUG

// DEBUG_REFS print references, DEBUG_LEAKS tracks live
// objects to output a list of the remaining objects at
// program termination. It also tracks use (ie addRef/release)
// of free'd objects.
// #define DEBUG_REFS
#define DEBUG_LEAKS

#endif

namespace storm {

#ifdef DEBUG_LEAKS
	map<Object *, String> live;

	void Object::dumpLeaks() {
		if (live.size() > 0)
			PLN(L"Leaks detected!");
		for (map<Object *, String>::iterator i = live.begin(); i != live.end(); i++) {
			PLN(L"Object " << i->first << L": " << i->second << L" (" << i->first->refs << L")");
		}
	}
#else
	void Object::dumpLeaks() {}
#endif

	// Note the hack: myType(myType). myType is initialized in operator new.
	Object::Object() : myType(myType), refs(1) {
#if defined(DEBUG) && defined(X86)
		if ((nat)myType == 0xCCCCCCCC) {
			PLN("Do not stack allocate objects, stupid!");
			DebugBreak();
			assert(false);
		}
#endif

#ifdef DEBUG_REFS
		if (myType == this) {
			// The Type-type is special.
			PLN("Created " << this << ", Type");
		} else {
			PLN("Created " << this << ", " << myType->name);
		}
#endif

#ifdef DEBUG_LEAKS
		if (myType == this) {
			live.insert(make_pair(this, L"Type"));
		} else {
			live.insert(make_pair(this, myType->name));
		}
#endif
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
		if (live.count(this) == 0) {
			PLN("Found a double-free!");
		}
		live.erase(this);
#endif
	}

	void checkLive(void *o) {
#ifdef DEBUG_LEAKS
		if (live.count((Object *)o) == 0) {
			PLN("Access to dead object: " << o);
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
		assert(("Forgot to update baseSize!", s.current() == sizeof(Object)));
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

	/**
	 * Memory management.
	 */

	void *Object::allocDumb(Engine &e, size_t size) {
		return malloc(size);
	}

	void *Object::operator new(size_t size, Type *type) {
		size_t s = type->size().current();

		assert(type->flags & typeClass);
		assert(("Not enough memory for the specified type!", size >= s || size == 0));

		void *mem = allocDumb(type->engine, s);
		memset(mem, 0, s);
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(mem, typeOffset, Type *) = type;

		return mem;
	}

	void *Object::operator new(size_t size, void *mem) {
		size_t typeOffset = OFFSET_OF(Object, myType);
		assert(OFFSET_IN(mem, typeOffset, Type *));
		return mem;
	}

	void Object::operator delete(void *mem, Type *type) {
		Object::operator delete(mem);
	}

	void Object::operator delete(void *ptr, void *mem) {
		// No need...
	}

	void Object::operator delete(void *mem) {
		free(mem);
	}

	void *Object::operator new[](size_t size, Type *type) { assert(false); return null; }
	void Object::operator delete[](void *ptr) { assert(false); }

	Object *createObj(Function *ctor, code::FnCall params) {
		assert(("Don't use create() with other functions than constructors.", ctor->name == Type::CTOR));
		assert(("Wrong number of parameters to constructor! The first one is filled in automatically.",
					ctor->params.size() == params.count() + 1));
		Type *type = ctor->params[0].type;
		assert(type->flags & typeClass);

		void *mem = Object::operator new(type->size().current(), type);

		try {
			if (ctor->params[0].ref)
				params.prependParam(&mem);
			else
				params.prependParam(mem);
			params.call<void>(ctor->pointer());
		} catch (...) {
			Object::operator delete(mem, type);
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
