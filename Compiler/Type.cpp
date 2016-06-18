#include "stdafx.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	Type::Type() : engine(Object::engine()), gcType(null) {}

	Type::Type(Engine &e) : engine(e), gcType(null) {}

	Type::~Type() {
		GcType *g = gcType;
		gcType = null;
		// Barrier here?
		engine.gc.freeType(g);
	}

	Type *Type::createType(Engine &e) {
		TODO(L"FIXME");
		return null;
	}

}
