#pragma once
#include "Shared/FnPtr.h"
#include "Value.h"

namespace storm {
	// Value version of fnPtrType.
	Type *fnPtrType(Engine &e, const vector<Value> &params);

	FnPtrBase *createRawFnPtr(Type *type, void *refData,
							Thread *t, Object *thisPtr,
							bool strongThis, bool member);
}
