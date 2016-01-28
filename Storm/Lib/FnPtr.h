#pragma once
#include "Shared/FnPtr.h"
#include "Value.h"

namespace storm {
	// Value version of fnPtrType.
	Type *fnPtrType(Engine &e, const vector<Value> &params);

	FnPtrBase *createRawFnPtr(Type *type, void *refData,
							Thread *t, Object *thisPtr,
							bool strongThis, bool member);

	// Create a FnPtr from a reference. Parts that are not needed may be ignored.
	FnPtrBase *createFnPtr(Engine &e, const vector<Value> &params,
						const code::Ref &to, Thread *t, Object *thisPtr,
						bool strongThis, bool member);

}
