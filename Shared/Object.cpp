#include "stdafx.h"
#include "Object.h"
#include "Str.h"
#include "Code/VTable.h"
#include "Utils/Memory.h"

namespace storm {

	/**
	 * These functions are implemented in Storm/Lib/Object.cpp or Lib/DllObject.cpp depending on
	 * if we're built as a DLL or not.
	 */

	// For keeping track of object instances during debugging.
	void objectCreated(Object *object);
	void objectDestroyed(Object *object);

	// Memory management
	void *allocObject(Type *t, size_t reportedSize);
	void freeObject(void *memory);

	// Type management.
	Engine &engine(const Object *o);
	bool objectIsA(const Object *o, const Type *t);


	// Note the hack: myType(myType). myType is initialized in operator new.
	Object::Object() : myType(myType), refs(1) {
#ifdef DEBUG
		objectCreated(this);
#endif
	}

	// Nothing to copy, really. Added to avoid special cases in other parts of the compiler.
	Object::Object(Object *) : myType(myType), refs(1) {
#ifdef DEBUG
		objectCreated(this);
#endif
	}

	Object::~Object() {
#ifdef DEBUG
		objectDestroyed(this);
#endif
	}

	Engine &Object::engine() const {
		return storm::engine(this);
	}

	bool Object::isA(const Type *t) const {
		return objectIsA(this, t);
	}

	Str *Object::toS() {
		std::wostringstream out;
		out << *this;
		return CREATE(Str, this, out.str());
	}

	bool toSOverridden(const Object *o);

	wostream &operator <<(wostream &to, const Object &o) {
		if (toSOverridden(&o)) {
			// Sorry about the const-cast...
			Auto<Str> s = const_cast<Object &>(o).toS();
			to << s->v;
		} else {
			o.output(to);
		}
		return to;
	}

	void Object::output(wostream &to) const {
		to << typeIdentifier(myType) << L": " << toHex(this);
	}

	Bool Object::equals(Par<Object> o) {
		if (!o)
			return false;
		if (o->myType != myType)
			return false;
		return true;
	}

	Nat Object::hash() {
		return Nat(this);
	}

	void Object::deepCopy(Par<CloneEnv> env) {}

	/**
	 * Memory management.
	 */

	void *Object::operator new(size_t size, Type *type) {
		return allocObject(type, size);
	}

	void *Object::operator new(size_t size, void *mem) {
		size_t typeOffset = OFFSET_OF(Object, myType);
		assert(OFFSET_IN(mem, typeOffset, Type *));
		return mem;
	}

	void *Object::operator new(size_t size, unsafe mem) {
		return mem.mem;
	}

	void Object::operator delete(void *mem, Type *type) {
		Object::operator delete(mem);
	}

	void Object::operator delete(void *ptr, void *mem) {
		// No need...
	}

	void Object::operator delete(void *ptr, unsafe mem) {
		// No need...
	}

	void Object::operator delete(void *mem) {
		freeObject(mem);
	}

	void *Object::operator new[](size_t size, Type *type) { assert(false); return null; }
	void Object::operator delete[](void *ptr) { assert(false); }

}
