#include "stdafx.h"
#include "Reflection.h"
#include "Type.h"

namespace storm {

	Type *typeOf(Par<Object> object) {
		object->myType->addRef();
		return object->myType;
	}

	Type *typeOf(Par<TObject> object) {
		object->myType->addRef();
		return object->myType;
	}

}
