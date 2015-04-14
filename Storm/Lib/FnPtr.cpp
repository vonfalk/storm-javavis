#include "stdafx.h"
#include "FnPtr.h"
#include "TObject.h"
#include "Code/Future.h"

namespace storm {

	FnPtrBase::FnPtrBase(const code::Ref &ref, Object *thisPtr, bool strongThis) :
		fnRef(ref),
		thisPtr(thisPtr),
		strongThisPtr(strongThis) {

		if (strongThis)
			thisPtr->addRef();

		// Note: We may not be able to use as<> here, since we treat Object and TObject as separate types!
		if (TObject *t = dynamic_cast<TObject *>(thisPtr))
			thread = t->thread;
	}

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
		clone(thread, env);

		if (thisPtr) {
			Auto<Object> v = capture(thisPtr);
			clone(v, env);

			if (strongThisPtr) {
				thisPtr->release();
				thisPtr = v.ret();
			} else {
				TODO(L"Is this always OK?");
				thisPtr = v.borrow();
			}
		}
	}

	void FnPtrBase::callRaw(void *output, const BasicTypeInfo &type, const code::FnParams &params) const {
		bool isMember = false;
		if (thisPtr) {
			isMember = true;
			// Note: We never need to copy the this-ptr here. Either the object is a TObject, and then
			// we have a thread or the this ptr is not a TObject and then we can not have a thread.

			// TODO: In the case of a thread call, we can avoid this allocation.
			code::FnParams p = params;
			p.addFirst(thisPtr);

			if (thread) {
				code::FutureSema<code::Sema> future(output);
				code::UThread::spawn(fnRef.address(), isMember, p, future, type, &thread->thread);
				future.result();
			} else {
				code::call(fnRef.address(), isMember, p, output, type);
			}
		} else {
			if (thread) {
				code::FutureSema<code::Sema> future(output);
				code::UThread::spawn(fnRef.address(), isMember, params, future, type, &thread->thread);
				future.result();
			} else {
				// No need to do anything special.
				code::call(fnRef.address(), isMember, params, output, type);
			}
		}
	}

}
