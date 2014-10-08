#include "stdafx.h"
#include "Object.h"

namespace storm {

	Object::Object(Type *t) : type(t), refs(0) {}

	Object::~Object() {}

	void Object::operator delete(void *ptr) {
		::operator delete(ptr);
	}

	void Object::operator delete[](void *ptr) {
		::operator delete[](ptr);
	}

}
