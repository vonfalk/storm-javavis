#include "stdafx.h"
#include "Object.h"
#include "Type.h"

//#define DEBUG_REFS

namespace storm {

	Object::Object(Type *t) : myType(t), refs(1) {
#ifdef DEBUG_REFS
		PLN("Created " << this << ", " << t->name);
#endif
	}

	Object::~Object() {
#ifdef DEBUG_REFS
		PLN("Destroying " << this << ", " << myType->name);
#endif
	}

}
