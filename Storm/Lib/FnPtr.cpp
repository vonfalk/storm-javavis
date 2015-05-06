#include "stdafx.h"
#include "FnPtr.h"
#include "TObject.h"
#include "Code/Future.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	FnPtrBase *FnPtrBase::createRaw(Type *type, void *refData, Thread *t, Object *thisPtr, bool strongThis) {
		Engine &e = type->engine;
		PLN("Creating a function pointer!");
		FnPtrBase *result = new (type) FnPtrBase(code::Ref::fromLea(e.arena, refData), thisPtr, strongThis);

		if (t != null && result->thread == null) {
			result->thread = capture(t);
		}
		type->vtable.update(result);
		return result;
	}

	FnPtrBase::FnPtrBase(const code::Ref &ref, Object *thisPtr, bool strongThis) :
		fnRef(ref),
		rawFn(null),
		thisPtr(thisPtr),
		strongThisPtr(strongThis) {
		init();
	}

	FnPtrBase::FnPtrBase(const void *fn, Object *thisPtr, bool strongThis) :
		fnRef(),
		rawFn(fn),
		thisPtr(thisPtr),
		strongThisPtr(strongThis) {
		init();
	}

	void FnPtrBase::init() {
		if (strongThisPtr)
			thisPtr->addRef();

		// Note: We may not be able to use as<> here, since we treat Object and TObject as separate types!
		if (TObject *t = dynamic_cast<TObject *>(thisPtr))
			thread = t->thread;
	}

	FnPtrBase::FnPtrBase(Par<FnPtrBase> o) :
		fnRef(o->fnRef),
		rawFn(o->rawFn),
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
		const void *toCall = rawFn;
		if (toCall == null)
			toCall = fnRef.address();

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
				code::UThread::spawn(toCall, isMember, p, future, type, &thread->thread);
				future.result();
			} else {
				code::call(toCall, isMember, p, output, type);
			}
		} else {
			if (thread) {
				code::FutureSema<code::Sema> future(output);
				code::UThread::spawn(fnRef.address(), isMember, params, future, type, &thread->thread);
				future.result();
			} else {
				// No need to do anything special.
				code::call(toCall, isMember, params, output, type);
			}
		}
	}

}
