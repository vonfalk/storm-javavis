#include "stdafx.h"
#include "Object.h"
#include "Gc.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	Object::Object() {}

	Object::Object(const Object &o) {}

	Object::~Object() {}

	Engine &Object::engine() const {
		return Gc::allocType(this)->type->engine;
	}

	bool Object::isA(const Type *o) const {
		TODO(L"FIXME");
		return false;
	}

	void *Object::operator new(size_t size, Type *type) {
		assert(size <= type->gcType->stride, L"Invalid type description found!");
		return type->engine.gc.alloc(type->gcType);
	}

	void Object::operator delete(void *mem, Type *type) {}

	void *Object::operator new(size_t size, Engine &e, GcType *type) {
		assert(size <= type->stride, L"Invalid type description found!");
		return e.gc.alloc(type);
	}

	void Object::operator delete(void *mem, Engine &e, GcType *type) {}

}
