#include "stdafx.h"
#include "Object.h"
#include "Type.h"
#include "Lib/Str.h"

// #define DEBUG_REFS

namespace storm {

	// Note the hack: myType(myType). myType is initialized in operator new.
	Object::Object() : myType(myType), refs(1) {
#ifdef DEBUG_REFS
		if (myType == this) {
			// The Type-type is special.
			PLN("Created " << this << ", Type");
		} else {
			PLN("Created " << this << ", " << myType->name);
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
	}

	Engine &Object::engine() const {
		return myType->engine;
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
