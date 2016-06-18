#include "stdafx.h"
#include "Object.h"
#include "Gc.h"
#include "Type.h"

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

}
