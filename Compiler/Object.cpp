#include "stdafx.h"
#include "Object.h"
#include "Gc.h"
#include "Type.h"
#include "Engine.h"
#include "Str.h"
#include "StrBuf.h"

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

	void Object::deepCopy(CloneEnv *env) {}

	Bool Object::equals(Object *o) const {
		TODO(L"Implement me!");
		return false;
	}

	Nat Object::hash() const {
		TODO(L"Implement me!");
		return 0;
	}

	Str *Object::toS() const {
		StrBuf *b = new (this) StrBuf();
		toS(b);
		return b->toS();
	}

	void Object::toS(StrBuf *buf) const {
		*buf << L"<TODO: Type> @" << (void *)this;
	}

	void *allocObject(size_t size, Type *type) {
		assert(size <= type->gcType->stride,
			L"Invalid type description found! " + ::toS(size) + L" vs " + ::toS(type->gcType->stride));
		return type->engine.gc.alloc(type->gcType);
	}

	wostream &operator <<(wostream &to, const Object *o) {
		return to << o->toS()->c_str();
	}

	wostream &operator <<(wostream &to, const Object &o) {
		return to << o.toS()->c_str();
	}

}
