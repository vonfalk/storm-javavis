#include "stdafx.h"
#include "FnPtr.h"
#include "Code/Future.h"

namespace storm {

	FnPtrBase::FnPtrBase(Par<FnPtrBase> o) :
		fnRef(o->fnRef),
		thread(o->thread),
		thisPtr(o->thisPtr),
		strongThisPtr(o->strongThisPtr) {
		if (strongThisPtr)
			thisPtr->addRef();
	}

	FnPtrBase::~FnPtrBase() {
		if (strongThisPtr)
			thisPtr->release();
	}

	void FnPtrBase::deepCopy(Par<CloneEnv> env) {
		// Anything needed here?
	}

	void FnPtrBase::callRaw(void *output, BasicTypeInfo type, const code::FnParams &params) const {
		bool isMember = false;
		if (thisPtr) {
			isMember = true;
			// Note: We never need to copy the this-ptr here. Either the object is a TObject, and then
			// we have a thread or the this ptr is not a TObject and then we can not have a thread.
		}

		if (thread) {
			code::FutureSema<code::Sema> future(output);
			code::UThread::spawn(fnRef.address(), isMember, params, future, type, &thread->thread);
			future.result();
		} else {
			// No need to do anything.
			code::call(fnRef.address(), isMember, params, output, type);
		}
	}

}
