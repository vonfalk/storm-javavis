#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Lib/Str.h"

// #define DEBUG_REFS
// #define DEBUG_LEAKS

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

	Bool Object::equals(Auto<Object> o) {
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
		size_t s = type->size();

		assert(type->flags & typeClass);
		assert(("Triggers if a subtype is not properly declared.", size <= s));

		void *mem = allocDumb(type->engine, s);
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(mem, typeOffset, Type *) = type;

		return mem;
	}

	void Object::operator delete(void *mem, Type *type) {
		free(mem);
	}

	void Object::operator delete(void *mem) {
		size_t typeOffset = OFFSET_OF(Object, myType);
		Object::operator delete(mem, OFFSET_IN(mem, typeOffset, Type *));
	}

	void *Object::operator new[](size_t size, Type *type) { assert(false); return null; }
	void Object::operator delete[](void *ptr) { assert(false); }

}
