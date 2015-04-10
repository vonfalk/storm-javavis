#include "stdafx.h"
#include "Wrap.h"
#include "Type.h"

namespace storm {

	Bool objectAs(Object *obj, Type *as) {
		if (obj == null)
			return false;
		return obj->myType->isA(as);
	}

	void fnParamsCtor(void *memory, void *ptr) {
		new (memory) code::FnParams(ptr);
	}

	void fnParamsDtor(code::FnParams *obj) {
		obj->~FnParams();
	}

	void fnParamsAdd(code::FnParams *obj, code::FnParams::CopyFn copy, code::FnParams::DestroyFn destroy,
					nat size, const void *value) {
		obj->add(copy, destroy, size, value);
	}

}
