#include "stdafx.h"
#include "Wrap.h"
#include "Type.h"

namespace storm {

	Bool objectAs(Object *obj, Type *as) {
		// PLN(obj << " as? " << as->identifier());
		if (obj == null)
			return false;
		return obj->myType->isA(as);
	}

	void fnParamsCtor(void *memory, void *ptr) {
		new (memory) os::FnParams(ptr);
	}

	void fnParamsDtor(os::FnParams *obj) {
		obj->~FnParams();
	}

	void fnParamsAdd(os::FnParams *obj, os::FnParams::CopyFn copy, os::FnParams::DestroyFn destroy,
					nat size, const void *value) {
		obj->add(copy, destroy, size, value);
	}

}
