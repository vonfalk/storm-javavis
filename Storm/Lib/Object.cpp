#include "stdafx.h"
#include "Object.h"

namespace storm {

	Object::Object(Type *t) : type(t), refs(1) {}

	Object::~Object() {}

}
