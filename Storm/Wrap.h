#pragma once
#include "Code/FnParams.h"
#include "Code/UThread.h"
#include "Code/Listing.h"
#include "Utils/TypeInfo.h"

namespace storm {

	class Object;
	class Type;
	class FnPtrBase;

	/**
	 * Contains wrappers to call various C++ functions from machine code.
	 * All functions are defined in Wrap.cpp unless otherwise noted.
	 */

	// Does an object inherit from a specific type?
	bool objectAs(Object *obj, Type *as);

	// FnParams(void *ptr);
	void fnParamsCtor(void *memory, void *ptr);

	// ~FnParams();
	void fnParamsDtor(code::FnParams *obj);

	// fnParams->add(...);
	void fnParamsAdd(code::FnParams *obj, code::FnParams::CopyFn copy,
					code::FnParams::DestroyFn destroy, nat size, const void *value);

	// Implemented in FnPtrTemplate.cpp
	bool fnPtrNeedsCopy(FnPtrBase *b);


	class FutureBase;

	// Spawning a thread. Implemented in Function.cpp.
	void spawnThreadResult(const void *fn, bool member, const code::FnParams *params, void *result,
						BasicTypeInfo *resultType, Thread *on, code::UThreadData *data);
	void spawnThreadFuture(const void *fn, bool member, const code::FnParams *params, FutureBase *result,
						BasicTypeInfo *resultType, Thread *on, code::UThreadData *data);

}
